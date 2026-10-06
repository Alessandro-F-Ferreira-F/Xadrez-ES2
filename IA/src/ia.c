#include "ia.h"
#include "avaliar.h"
#include "pendenciasRegras.h"
#include "makemove.h"
#include "movegen.h"

#include <stdlib.h>
#include <stdbool.h>

// Separação entre avaliação e mate: nenhum valor de avaliar pode ser
// classificado como mate. Falha se PROF_MAX >= MATE - AVALIACAO_MAX (9384)
_Static_assert(LIMIAR_MATE > AVALIACAO_MAX, "PROF_MAX grande demais: LIMIAR_MATE invade a faixa da avaliacao");

Play JOGADA;
static MoveList listaCandidatos;

void converterScore(int32_t scoreBruto, Color lado, Play *p){
    int32_t score = scoreBruto;

    if(lado == BLACK){
        score *= -1;
    }
    if(LIMIAR_MATE <= abs(score)){
        p->tipo = TIPO_MATE;
        int32_t sinalMate;

        if(score > 0){
            sinalMate = 1;
        }
        else{
            sinalMate = -1;
        }

        p->score = sinalMate * ((MATE - abs(score) + 1)/2);
    }
    else{
        p->tipo = TIPO_CENTIPEAO;
        p->score = score;
    }
}

static bool emXeque(const Board *board){
    return is_square_attacked(board, board->king_square[board->side_to_move], board->side_to_move);
}

static int8_t verificarChamada(Board *board, int variante, int randomizador){
    if(variante != VARIANTE_MINIMAX && variante != VARIANTE_ALFA_BETA) return RESULTADO_VARIANTE_INVALIDA;
    if(randomizador != SORTEIO_LIGADO && randomizador != SORTEIO_DESLIGADO) return RESULTADO_RANDOMIZADOR_INVALIDO;

    int motivoEmpate = testeEmpateRegra(board);

    if(motivoEmpate != SEM_EMPATE){
        if(motivoEmpate == EMPATE_PROVISORIO) return RESULTADO_EMPATE_PROVISORIO;
        else return RESULTADO_ERRO_INTERNO;
    }
    MoveList listaRaiz;

    generate_legal_moves(board, &listaRaiz);

    if(listaRaiz.count == 0){
        if(emXeque(board)) return RESULTADO_RAIZ_MATE;
        else return RESULTADO_RAIZ_AFOGAMENTO;
    }
    return RESULTADO_SUCESSO;
}

static int32_t minimax(Board *board, int profundidade){
    if(testeEmpateRegra(board)) return 0;
    else if(PROF_MAX <= profundidade) return avaliar(board);
    else{
        MoveList listaGerada;
        generate_legal_moves(board, &listaGerada);

        if(listaGerada.count == 0){
            if(emXeque(board)){
                if(board->side_to_move == WHITE){
                    return -(MATE - profundidade);
                }
                else return MATE - profundidade;
            }
            else return 0;
        }
        else{
            if(board->side_to_move == WHITE){
                int32_t melhor = -INFINITO;
            
                for(int i = 0; i < listaGerada.count; i ++){
                    Undo undo;

                    make_move(board, listaGerada.moves[i] ,&undo);
                    int32_t atual = minimax(board, profundidade + 1);
                    unmake_move(board, listaGerada.moves[i], &undo);

                    if(atual > melhor){
                        melhor = atual;

                        if(profundidade == 0){
                            movelist_clear(&listaCandidatos);
                            movelist_add(&listaCandidatos, listaGerada.moves[i]);
                        }
                    }
                    else if(atual == melhor && profundidade == 0){
                        movelist_add(&listaCandidatos, listaGerada.moves[i]);
                    }
                }

                return melhor;
            }
            else{
                int32_t melhor = INFINITO;

                for(int i = 0; i < listaGerada.count; i ++){
                    Undo undo;

                    make_move(board, listaGerada.moves[i], &undo);
                    int32_t atual = minimax(board, profundidade + 1);
                    unmake_move(board, listaGerada.moves[i], &undo);

                    if(atual < melhor){
                        melhor = atual;

                        if(profundidade == 0){
                            movelist_clear(&listaCandidatos);
                            movelist_add(&listaCandidatos, listaGerada.moves[i]);
                        }
                    }
                    else if(atual == melhor && profundidade == 0){
                        movelist_add(&listaCandidatos, listaGerada.moves[i]);
                    }
                }

                return melhor;
            }  
        }
    }
}

static int32_t podaAlfaBeta(Board *board, int profundidade, int32_t alfa, int32_t beta){
    if(testeEmpateRegra(board)) return 0;
    else if(PROF_MAX <= profundidade) return avaliar(board);
    else{
        MoveList listaGerada;
        generate_legal_moves(board, &listaGerada);

        if(listaGerada.count == 0){
            if(emXeque(board)){
                if(board->side_to_move == WHITE){
                    return -(MATE - profundidade);
                }
                else return MATE - profundidade;
            }
            else return 0;
        }
        else{
            if(board->side_to_move == WHITE){
                int32_t melhor = -INFINITO;
            
                for(int i = 0; i < listaGerada.count; i ++){
                    Undo undo;

                    make_move(board, listaGerada.moves[i] ,&undo);
                    int32_t atual = podaAlfaBeta(board, profundidade + 1, alfa, beta);
                    unmake_move(board, listaGerada.moves[i], &undo);

                    if(atual > melhor){
                        melhor = atual;

                        if(melhor > alfa && profundidade >0){
                            alfa = melhor;
                        }

                        if(profundidade == 0){
                            movelist_clear(&listaCandidatos);
                            movelist_add(&listaCandidatos, listaGerada.moves[i]);
                        }
                    }
                    else if(atual == melhor && profundidade == 0){
                        movelist_add(&listaCandidatos, listaGerada.moves[i]);
                    }

                    if(alfa >= beta){
                        break;
                    }
                }

                return melhor;
            }
            else{
                int32_t melhor = INFINITO;

                for(int i = 0; i < listaGerada.count; i ++){
                    Undo undo;

                    make_move(board, listaGerada.moves[i], &undo);
                    int32_t atual = podaAlfaBeta(board, profundidade + 1, alfa, beta);
                    unmake_move(board, listaGerada.moves[i], &undo);

                    if(atual < melhor){
                        melhor = atual;

                        if(melhor < beta && profundidade > 0){
                            beta = melhor;
                        }

                        if(profundidade == 0){
                            movelist_clear(&listaCandidatos);
                            movelist_add(&listaCandidatos, listaGerada.moves[i]);
                        }
                    }
                    else if(atual == melhor && profundidade == 0){
                        movelist_add(&listaCandidatos, listaGerada.moves[i]);
                    }

                    if(alfa >= beta){
                        break;
                    }
                }

                return melhor;
            }  
        }
    }
}

// Sorteia um índice no intervalo fechado [0, quantidade - 1].
// Pré-condição: quantidade > 0 (escolherJogada só chama com listaCandidatos não vazia).
// Não semeia o gerador: a semente é definida uma vez, fora daqui.
static int sorteioLance(int quantidade){
    return rand() % quantidade;
}

void iniciarSorteio(unsigned int semente){
    srand(semente);
}

void escolherJogada(Board *board, int variante, int randomizador){
    movelist_clear(&listaCandidatos);

    JOGADA.lance = LANCE_NULO;
    JOGADA.score = SCORE_INVALIDO;
    JOGADA.tipo = TIPO_INVALIDO;
    JOGADA.resultado = verificarChamada(board, variante, randomizador);

    if(JOGADA.resultado != RESULTADO_SUCESSO) return;

    int32_t scoreBruto = SCORE_INVALIDO;

    if(variante == VARIANTE_MINIMAX){
        scoreBruto = minimax(board, 0);
    }
    else if(variante == VARIANTE_ALFA_BETA){
        scoreBruto = podaAlfaBeta(board, 0, -INFINITO, INFINITO);
    }
    
    if(listaCandidatos.count > 0){
        converterScore(scoreBruto, board->side_to_move, &JOGADA);

        if(randomizador == SORTEIO_LIGADO){
            JOGADA.lance = listaCandidatos.moves[sorteioLance(listaCandidatos.count)];
        }
        else{
            JOGADA.lance = listaCandidatos.moves[0];
        }
    }

    if(JOGADA.lance == LANCE_NULO){
        JOGADA.tipo = TIPO_INVALIDO;
        JOGADA.score = SCORE_INVALIDO;
        JOGADA.resultado = RESULTADO_ERRO_INTERNO;
    }
}
