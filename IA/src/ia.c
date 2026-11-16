#include "ia.h"
#include "avaliar.h"
#include "ordenar.h"
#include "pendenciasRegras.h"
#include "makemove.h"
#include "movegen.h"
#include "square.h"
#include "iaTeste.h"

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// Separação entre avaliação e mate: nenhum valor de avaliar pode ser
// classificado como mate. Falha se PROF_LIMITE >= MATE - AVALIACAO_MAX (9384)
_Static_assert(LIMIAR_MATE > AVALIACAO_MAX, "PROF_LIMITE grande demais: LIMIAR_MATE invade a faixa da avaliacao");

// O relógio é consultado a cada INTERVALO_RELOGIO nós (potência de 2)
#define INTERVALO_RELOGIO 1024

Play JOGADA;
uint64_t NOS_VISITADOS;
uint64_t NOS_QUIESCENCIA;
MoveList CANDIDATOS;
int PROF_ALCANCADA;
bool PARCIAL_USADA;
static MoveList listaCandidatos;
// Melhor valor entre os filhos da raiz já completos na iteração corrente (só alfa-beta)
static int32_t scoreParcial;

// Estado da iteração corrente, definido por escolherJogada antes de cada busca
static int limiteIteracao;
static bool quiescenciaAtiva;
static bool relogioAtivo;
// Em int64_t: no Windows clock_t é long de 32 bits, e inicio + tempoMs pode estourar
static int64_t prazo;
static bool tempoEsgotado;

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

// Prova barata de que o lado da vez tem um lance legal, sem gerar a lista. Pré-condição: fora de xeque.
// Condição suficiente, não necessária: false não quer dizer afogamento, só que a prova falhou.
// Uma peça fora da fileira, da coluna e das diagonais do próprio rei não pode estar cravada; fora de
// xeque, qualquer lance pseudolegal dela é legal (en passant fica de fora). Se nenhuma servir, o rei:
// fora de xeque ele não bloqueia raio nenhum, então is_square_attacked com o rei no lugar é exato
bool lanceGarantido(const Board *board){
    Color lado = board->side_to_move;
    int rei = board->king_square[lado];
    int filaRei = RANK_OF(rei), colunaRei = FILE_OF(rei);

    for(int casa = 0; casa < BOARD_SIZE; casa ++){
        Piece peca = board->array[casa];
        if(peca == EMPTY || PIECE_COLOR(peca) != lado || PIECE_TYPE(peca) == KING) continue;

        int fila = RANK_OF(casa), coluna = FILE_OF(casa);
        if(fila == filaRei || coluna == colunaRei || fila - coluna == filaRei - colunaRei
           || fila + coluna == filaRei + colunaRei) continue;

        PieceType tipo = PIECE_TYPE(peca);

        if(tipo == PAWN){
            // Avanço simples (com ou sem promoção) ou captura normal
            int destino = casa + PAWN_PUSH[lado];
            if(!SQ_OFFBOARD(destino) && board->array[destino] == EMPTY) return true;
            for(int i = 0; i < 2; i ++){
                destino = PAWN_ATTACKS[lado][casa][i];
                if(destino == SQ_NONE) continue;
                Piece alvo = board->array[destino];
                if(alvo != EMPTY && PIECE_COLOR(alvo) != lado) return true;
            }
        }
        else if(tipo == KNIGHT){
            for(int i = 0; i < 8; i ++){
                int destino = KNIGHT_ATTACKS[casa][i];
                if(destino == SQ_NONE) continue;
                Piece alvo = board->array[destino];
                if(alvo == EMPTY || PIECE_COLOR(alvo) != lado) return true;
            }
        }
        else{
            // Um passo numa direção da peça: ortogonais (DIR_N..DIR_W) para torre, diagonais (DIR_NE..DIR_NW)
            // para bispo, todas para a dama
            int primeira = (tipo == BISHOP) ? DIR_NE : DIR_N;
            int ultima = (tipo == ROOK) ? DIR_W : DIR_NW;
            for(int dir = primeira; dir <= ultima; dir ++){
                if(SQ_TO_EDGE[casa][dir] == 0) continue;
                Piece alvo = board->array[casa + DIR_OFFSET[dir]];
                if(alvo == EMPTY || PIECE_COLOR(alvo) != lado) return true;
            }
        }
    }

    // KING_TARGETS (square.h) não é inicializada pelo motor: as casas vizinhas saem de SQ_TO_EDGE e DIR_OFFSET.
    // is_square_attacked(board, casa, lado): lado é o defensor
    for(int dir = DIR_N; dir <= DIR_NW; dir ++){
        if(SQ_TO_EDGE[rei][dir] == 0) continue;
        int destino = rei + DIR_OFFSET[dir];
        Piece alvo = board->array[destino];
        if(alvo != EMPTY && PIECE_COLOR(alvo) == lado) continue;
        if(!is_square_attacked(board, destino, lado)) return true;
    }

    return false;
}

static int8_t verificarChamada(Board *board, int variante, int randomizador, int quiescencia, int profundidade, int tempoMs){
    if(variante != VARIANTE_MINIMAX && variante != VARIANTE_ALFA_BETA) return RESULTADO_VARIANTE_INVALIDA;
    if(randomizador != SORTEIO_LIGADO && randomizador != SORTEIO_DESLIGADO) return RESULTADO_RANDOMIZADOR_INVALIDO;
    if(quiescencia != QUIESCENCIA_LIGADA && quiescencia != QUIESCENCIA_DESLIGADA) return RESULTADO_QUIESCENCIA_INVALIDA;
    if(profundidade < 1 || profundidade > PROF_MAX) return RESULTADO_PROFUNDIDADE_INVALIDA;
    if(tempoMs < 0) return RESULTADO_TEMPO_INVALIDO;

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

// clock() falhando ((clock_t)-1): sem como medir o tempo, o prazo conta como passado
static bool prazoPassou(void){
    clock_t agora = clock();

    return agora == (clock_t)-1 || (int64_t)agora >= prazo;
}

// Liga tempoEsgotado quando o prazo passa; depois de ligado, fica ligado até a próxima
// escolherJogada. Com o relógio desligado (iteração 1 ou TEMPO_SEM_LIMITE), nunca liga
static bool estourouTempo(void){
    if(!tempoEsgotado && relogioAtivo && NOS_VISITADOS % INTERVALO_RELOGIO == 0 && prazoPassou()){
        tempoEsgotado = true;
    }
    return tempoEsgotado;
}

// Quiescência: continua a busca além de limiteIteracao só com capturas, até a posição ficar quieta.
// Fora de xeque, o lado da vez pode parar (stand-pat: vale avaliar) ou capturar; em xeque, parar não
// é opção, e todas as evasões são buscadas. Sem lances legais, mate ou afogamento, como na busca
// principal. Em PROF_LIMITE, devolve avaliar: mantém todo mate >= LIMIAR_MATE e a pilha limitada.
// Versão sem poda, na ordem do gerador: referência da quiescência com poda (v1 = v2)
static int32_t quiescenciaMinimax(Board *board, int profundidade){
    NOS_VISITADOS ++;
    NOS_QUIESCENCIA ++;

    if(estourouTempo()) return 0;

    if(testeEmpateRegra(board)) return 0;
    else if(PROF_LIMITE <= profundidade) return avaliar(board);
    else{
        MoveList listaGerada;
        generate_legal_moves(board, &listaGerada);
        bool xeque = emXeque(board);

        if(listaGerada.count == 0){
            if(xeque){
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
                if(!xeque) melhor = avaliar(board);

                for(int i = 0; i < listaGerada.count; i ++){
                    Move lance = listaGerada.moves[i];
                    if(!xeque && !move_is_capture(lance)) continue;
                    Undo undo;

                    make_move(board, lance, &undo);
                    int32_t atual = quiescenciaMinimax(board, profundidade + 1);
                    unmake_move(board, lance, &undo);

                    if(tempoEsgotado) return 0;

                    if(atual > melhor){
                        melhor = atual;
                    }
                }

                return melhor;
            }
            else{
                int32_t melhor = INFINITO;
                if(!xeque) melhor = avaliar(board);

                for(int i = 0; i < listaGerada.count; i ++){
                    Move lance = listaGerada.moves[i];
                    if(!xeque && !move_is_capture(lance)) continue;
                    Undo undo;

                    make_move(board, lance, &undo);
                    int32_t atual = quiescenciaMinimax(board, profundidade + 1);
                    unmake_move(board, lance, &undo);

                    if(tempoEsgotado) return 0;

                    if(atual < melhor){
                        melhor = atual;
                    }
                }

                return melhor;
            }
        }
    }
}

// Quiescência com poda, em ordem MVV-LVA. O stand-pat entra como piso (brancas) ou teto (pretas) antes
// dos lances, e pode cortar sozinho. Nunca é a raiz (profundidade >= limiteIteracao >= 1): alfa e beta
// sempre podem ser atualizados
static int32_t quiescenciaAlfaBeta(Board *board, int profundidade, int32_t alfa, int32_t beta){
    NOS_VISITADOS ++;
    NOS_QUIESCENCIA ++;

    if(estourouTempo()) return 0;

    if(testeEmpateRegra(board)) return 0;
    else if(PROF_LIMITE <= profundidade) return avaliar(board);
    else{
        bool xeque = emXeque(board);
        int32_t standPat = 0;

        // Corte do stand-pat antes de gerar os lances, se houver lance legal garantido: com a lista
        // gerada, ela não seria vazia e o corte seria o mesmo. Sem a garantia, pode ser afogamento (0)
        if(!xeque){
            standPat = avaliar(board);
            bool corta = (board->side_to_move == WHITE) ? (standPat >= beta) : (standPat <= alfa);
            if(corta && lanceGarantido(board)) return standPat;
        }

        MoveList listaGerada;
        generate_legal_moves(board, &listaGerada);

        if(listaGerada.count == 0){
            if(xeque){
                if(board->side_to_move == WHITE){
                    return -(MATE - profundidade);
                }
                else return MATE - profundidade;
            }
            else return 0;
        }
        else{
            // Fora de xeque, só as capturas são buscadas: filtradas antes de ordenar, na ordem do gerador
            MoveList listaCapturas;
            const MoveList *listaBusca = &listaGerada;

            if(!xeque){
                movelist_clear(&listaCapturas);
                for(int i = 0; i < listaGerada.count; i ++){
                    if(move_is_capture(listaGerada.moves[i])) movelist_add(&listaCapturas, listaGerada.moves[i]);
                }
                listaBusca = &listaCapturas;
            }

            if(board->side_to_move == WHITE){
                int32_t melhor = -INFINITO;

                if(!xeque){
                    melhor = standPat;
                    if(melhor > alfa) alfa = melhor;
                    if(alfa >= beta) return melhor;
                }

                LanceOrdenado listaOrdenada[MAX_MOVES];
                ordenarLances(board, listaBusca, MOVE_NONE, listaOrdenada);

                for(int i = 0; i < listaBusca->count; i ++){
                    Move lance = listaOrdenada[i].lance;
                    Undo undo;

                    make_move(board, lance, &undo);
                    int32_t atual = quiescenciaAlfaBeta(board, profundidade + 1, alfa, beta);
                    unmake_move(board, lance, &undo);

                    if(tempoEsgotado) return 0;

                    if(atual > melhor){
                        melhor = atual;

                        if(melhor > alfa){
                            alfa = melhor;
                        }
                    }

                    if(alfa >= beta){
                        break;
                    }
                }

                return melhor;
            }
            else{
                int32_t melhor = INFINITO;

                if(!xeque){
                    melhor = standPat;
                    if(melhor < beta) beta = melhor;
                    if(alfa >= beta) return melhor;
                }

                LanceOrdenado listaOrdenada[MAX_MOVES];
                ordenarLances(board, listaBusca, MOVE_NONE, listaOrdenada);

                for(int i = 0; i < listaBusca->count; i ++){
                    Move lance = listaOrdenada[i].lance;
                    Undo undo;

                    make_move(board, lance, &undo);
                    int32_t atual = quiescenciaAlfaBeta(board, profundidade + 1, alfa, beta);
                    unmake_move(board, lance, &undo);

                    if(tempoEsgotado) return 0;

                    if(atual < melhor){
                        melhor = atual;

                        if(melhor < beta){
                            beta = melhor;
                        }
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

static int32_t minimax(Board *board, int profundidade){
    NOS_VISITADOS ++;

    if(estourouTempo()) return 0;

    if(testeEmpateRegra(board)) return 0;
    else if(limiteIteracao <= profundidade){
        if(quiescenciaAtiva) return quiescenciaMinimax(board, profundidade);
        else return avaliar(board);
    }
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

                    if(tempoEsgotado) return 0;

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

                    if(tempoEsgotado) return 0;

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
    NOS_VISITADOS ++;

    if(estourouTempo()) return 0;

    if(testeEmpateRegra(board)) return 0;
    else if(limiteIteracao <= profundidade){
        if(quiescenciaAtiva) return quiescenciaAlfaBeta(board, profundidade, alfa, beta);
        else return avaliar(board);
    }
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
            // MVV-LVA: capturas primeiro, para cortar mais cedo. Na raiz, antes de tudo, o melhor lance da
            // iteração anterior (CANDIDATOS guarda a última completa; vazia na iteração 1)
            Move primeiro = MOVE_NONE;
            if(profundidade == 0 && CANDIDATOS.count > 0) primeiro = CANDIDATOS.moves[0];

            LanceOrdenado listaOrdenada[MAX_MOVES];
            ordenarLances(board, &listaGerada, primeiro, listaOrdenada);

            if(board->side_to_move == WHITE){
                int32_t melhor = -INFINITO;
            
                for(int i = 0; i < listaGerada.count; i ++){
                    Move lance = listaOrdenada[i].lance;
                    Undo undo;

                    make_move(board, lance, &undo);
                    int32_t atual = podaAlfaBeta(board, profundidade + 1, alfa, beta);
                    unmake_move(board, lance, &undo);

                    if(tempoEsgotado) return 0;

                    if(atual > melhor){
                        melhor = atual;

                        if(melhor > alfa && profundidade >0){
                            alfa = melhor;
                        }

                        // scoreParcial acompanha melhor: se o tempo acabar no meio da raiz,
                        // escolherJogada aproveita os filhos que completaram
                        if(profundidade == 0){
                            movelist_clear(&listaCandidatos);
                            movelist_add(&listaCandidatos, lance);
                            scoreParcial = melhor;
                        }
                    }
                    else if(atual == melhor && profundidade == 0){
                        movelist_add(&listaCandidatos, lance);
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
                    Move lance = listaOrdenada[i].lance;
                    Undo undo;

                    make_move(board, lance, &undo);
                    int32_t atual = podaAlfaBeta(board, profundidade + 1, alfa, beta);
                    unmake_move(board, lance, &undo);

                    if(tempoEsgotado) return 0;

                    if(atual < melhor){
                        melhor = atual;

                        if(melhor < beta && profundidade > 0){
                            beta = melhor;
                        }

                        if(profundidade == 0){
                            movelist_clear(&listaCandidatos);
                            movelist_add(&listaCandidatos, lance);
                            scoreParcial = melhor;
                        }
                    }
                    else if(atual == melhor && profundidade == 0){
                        movelist_add(&listaCandidatos, lance);
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
// Pré-condição: quantidade > 0 (escolherJogada só chama com CANDIDATOS não vazia).
// Não semeia o gerador: a semente é definida uma vez, fora daqui.
static int sorteioLance(int quantidade){
    return rand() % quantidade;
}

void iniciarSorteio(unsigned int semente){
    srand(semente);
}

void escolherJogada(Board *board, int variante, int randomizador, int quiescencia, int profundidade, int tempoMs){
    // O prazo conta a partir da chamada, incluindo a verificação
    clock_t inicio = clock();

    movelist_clear(&listaCandidatos);
    movelist_clear(&CANDIDATOS);
    NOS_VISITADOS = 0;
    NOS_QUIESCENCIA = 0;
    PROF_ALCANCADA = 0;
    PARCIAL_USADA = false;
    relogioAtivo = false;
    tempoEsgotado = false;

    JOGADA.lance = MOVE_NONE;
    JOGADA.score = SCORE_INVALIDO;
    JOGADA.tipo = TIPO_INVALIDO;
    JOGADA.resultado = verificarChamada(board, variante, randomizador, quiescencia, profundidade, tempoMs);

    if(JOGADA.resultado != RESULTADO_SUCESSO) return;

    if(tempoMs != TEMPO_SEM_LIMITE){
        // clock() falhou: sem início, o prazo conta como já passado (só a iteração 1, ou a passada
        // de segurança, roda)
        if(inicio == (clock_t)-1) prazo = INT64_MIN;
        else prazo = (int64_t)inicio + (int64_t)tempoMs * CLOCKS_PER_SEC / 1000;
    }

    // Score da última iteração aproveitada (os candidatos dela ficam em CANDIDATOS)
    int32_t scoreBruto = SCORE_INVALIDO;

    // Com a quiescência, nem a iteração 1 tem custo limitado. Com relógio, uma passada de segurança
    // (limite 1, sem quiescência, relógio desligado) sempre termina e garante lance; depois, todas as
    // iterações rodam com a quiescência e sob o relógio. Se só ela valer, PROF_ALCANCADA fica em 0
    bool passadaSeguranca = (tempoMs != TEMPO_SEM_LIMITE && quiescencia == QUIESCENCIA_LIGADA);

    if(passadaSeguranca){
        limiteIteracao = 1;
        quiescenciaAtiva = false;
        relogioAtivo = false;
        movelist_clear(&listaCandidatos);

        if(variante == VARIANTE_MINIMAX){
            scoreBruto = minimax(board, 0);
        }
        else if(variante == VARIANTE_ALFA_BETA){
            scoreBruto = podaAlfaBeta(board, 0, -INFINITO, INFINITO);
        }
        CANDIDATOS = listaCandidatos;
    }

    quiescenciaAtiva = (quiescencia == QUIESCENCIA_LIGADA);

    for(limiteIteracao = 1; limiteIteracao <= profundidade; limiteIteracao ++){
        // Sem passada de segurança, a iteração 1 roda com o relógio desligado: sempre termina, para que haja lance
        relogioAtivo = (tempoMs != TEMPO_SEM_LIMITE && (limiteIteracao > 1 || passadaSeguranca));

        if(relogioAtivo && prazoPassou()) break;

        movelist_clear(&listaCandidatos);
        scoreParcial = SCORE_INVALIDO;
        int32_t scoreIteracao = SCORE_INVALIDO;

        if(variante == VARIANTE_MINIMAX){
            scoreIteracao = minimax(board, 0);
        }
        else if(variante == VARIANTE_ALFA_BETA){
            scoreIteracao = podaAlfaBeta(board, 0, -INFINITO, INFINITO);
        }

        // Iteração interrompida. No alfa-beta, o primeiro filho da raiz é o melhor da iteração
        // anterior; se ao menos ele completou (só filho completo entra em listaCandidatos), os
        // filhos completos têm valor exato nesta profundidade e substituem a iteração anterior.
        // No minimax, ou sem filho completo, a iteração é descartada
        if(tempoEsgotado){
            if(variante == VARIANTE_ALFA_BETA && listaCandidatos.count > 0){
                CANDIDATOS = listaCandidatos;
                scoreBruto = scoreParcial;
                PROF_ALCANCADA = limiteIteracao;
                PARCIAL_USADA = true;
            }
            break;
        }

        CANDIDATOS = listaCandidatos;
        scoreBruto = scoreIteracao;
        PROF_ALCANCADA = limiteIteracao;

        // Mate achado dentro de limiteIteracao (largura completa): uma iteração mais funda não acha
        // mate mais curto. Mate achado só na quiescência (mais fundo) não para: lá a busca não é
        // completa, e o mate pode não ser o mais curto
        if(abs(scoreBruto) >= MATE - limiteIteracao) break;
    }

    if(CANDIDATOS.count > 0){
        converterScore(scoreBruto, board->side_to_move, &JOGADA);

        if(randomizador == SORTEIO_LIGADO){
            JOGADA.lance = CANDIDATOS.moves[sorteioLance(CANDIDATOS.count)];
        }
        else{
            JOGADA.lance = CANDIDATOS.moves[0];
        }
    }

    if(JOGADA.lance == MOVE_NONE){
        movelist_clear(&CANDIDATOS);
        PROF_ALCANCADA = 0;
        PARCIAL_USADA = false;
        JOGADA.tipo = TIPO_INVALIDO;
        JOGADA.score = SCORE_INVALIDO;
        JOGADA.resultado = RESULTADO_ERRO_INTERNO;
    }
}
