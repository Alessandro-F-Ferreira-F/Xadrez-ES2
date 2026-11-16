// MinGW: printf ANSI, para PRIu64 sem aviso do -Wpedantic (sem efeito fora do MinGW). Precisa vir antes de qualquer include
#define __USE_MINGW_ANSI_STDIO 1

#include "fen.h"
#include "perft.h"
#include "avaliar.h"
#include "ordenar.h"
#include "ia.h"
#include "stubs.h"
#include "movegen.h"
#include "makemove.h"
#include "iaTeste.h"
#include "niveis.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

typedef struct CasoConversao{
    int32_t scoreBruto;
    Color lado;
    int8_t tipoEsperado;
    int32_t scoreEsperado;
} CasoConversao;

static const CasoConversao casos[] = {
    // Centipeão e perspectiva
    {150,                 WHITE, TIPO_CENTIPEAO,  150},
    {150,                 BLACK, TIPO_CENTIPEAO, -150},
    {-150,                BLACK, TIPO_CENTIPEAO,  150},
    {0,                   WHITE, TIPO_CENTIPEAO,  0},
    {0,                   BLACK, TIPO_CENTIPEAO,  0},

    // Mate, brancas na vez (p = 1..5)
    {MATE - 1,            WHITE, TIPO_MATE,  1},
    {-(MATE - 2),         WHITE, TIPO_MATE, -1},
    {MATE - 3,            WHITE, TIPO_MATE,  2},
    {-(MATE - 4),         WHITE, TIPO_MATE, -2},
    {MATE - 5,            WHITE, TIPO_MATE,  3},

    // Mate, pretas na vez (p = 1..4)
    {-(MATE - 1),         BLACK, TIPO_MATE,  1},
    {MATE - 2,            BLACK, TIPO_MATE, -1},
    {-(MATE - 3),         BLACK, TIPO_MATE,  2},
    {MATE - 4,            BLACK, TIPO_MATE, -2},

    // Fronteira do limiar
    {LIMIAR_MATE,         WHITE, TIPO_MATE,       (MATE - LIMIAR_MATE + 1) / 2},
    {-LIMIAR_MATE,        WHITE, TIPO_MATE,       -((MATE - LIMIAR_MATE + 1) / 2)},
    {LIMIAR_MATE - 1,     WHITE, TIPO_CENTIPEAO,  LIMIAR_MATE - 1},
    {-(LIMIAR_MATE - 1),  WHITE, TIPO_CENTIPEAO,  -(LIMIAR_MATE - 1)},
    {LIMIAR_MATE,         BLACK, TIPO_MATE,       -((MATE - LIMIAR_MATE + 1) / 2)},
    {LIMIAR_MATE - 1,     BLACK, TIPO_CENTIPEAO,  -(LIMIAR_MATE - 1)},
};

typedef struct CasoAvaliacao{
    const char *fen;
    int32_t esperado;
} CasoAvaliacao;

// Esperados calculados à mão a partir das tabelas de Michniewski (ver avaliar.txt). Peça branca: valor da
// tabela na própria casa; peça preta: na casa espelhada (fileira r vira 9 - r). Rei: (meio * fase +
// final * (24 - fase)) / 24 sobre o termo líquido, com truncamento em direção a zero. Reis em e1 e e8 se
// anulam nas duas tabelas
static const CasoAvaliacao casosAvaliacao[] = {
    // Posição inicial (simetria)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  0},

    // Extremos de material: 10400 de material; PST cavalos b3 5, h2 -40; bispos c3 10, d3 10; damas b2 0,
    // c2 5, d2 0, b1..g1 -10 -10 -5 -5 -10 -10; torres e2 0, f2 0: total -60. Fase 44, limitada a 24;
    // reis h1 e a8 (espelho a1) se anulam
    {"k7/8/8/8/8/1NBB4/1QQQRR1N/1QQQQQQK w - - 0 1",              10340},
    {"1qqqqqqk/1qqqrr1n/1nbb4/8/8/8/8/K7 b - - 0 1",             -10340},

    // Varredura inclui a1 e h8: torres h8 0 e a1 0, 1000. Fase 4; rei preto d5 (espelho d4): meio 0 - (-40)
    // = 40, final -30 - 40 = -70; (40 * 4 - 70 * 20) / 24 = -1240 / 24 = -51 (truncado). Espelho: -949
    {"7R/8/8/3k4/8/8/8/R3K3 w - - 0 1",                           949},
    {"r3k3/8/8/8/3K4/8/8/7r b - - 0 1",                          -949},

    // Só os reis, fase 0: só a tabela de final, -30 - 40 = -70. Espelho: 70
    {"8/8/8/3k4/8/8/8/4K3 w - - 0 1",                             -70},
    {"4k3/8/8/8/3K4/8/8/8 b - - 0 1",                              70},

    // Fase 24, branco rocado: em relação à inicial, as brancas trocam Bf1 (-10), Cg1 (-40) e Th1 (0) por
    // Bc4 (10), Cf3 (10) e Tf1 (0): +70. Rei g1 no meio-jogo 30, contra e8 (espelho e1) 0: +30. Espelho: -100
    {"rnbqkbnr/pppppppp/8/8/2B5/5N2/PPPPPPPP/RNBQ1RK1 w kq - 0 1", 100},
    {"rnbq1rk1/pppppppp/5n2/2b5/8/8/PPPPPPPP/RNBQKBNR b KQ - 0 1", -100},

    // Uma peça por vez, nos dois ramos de cor (reis e1 e e8)
    {"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",                           80},  // 100 + e2 -20
    {"4k3/4p3/8/8/8/8/8/4K3 w - - 0 1",                          -80},
    {"4k3/8/8/8/8/8/8/1N2K3 w - - 0 1",                           280}, // 320 + b1 -40
    {"1n2k3/8/8/8/8/8/8/4K3 w - - 0 1",                          -280},
    {"4k3/8/8/8/8/8/8/2B1K3 w - - 0 1",                           320}, // 330 + c1 -10
    {"2b1k3/8/8/8/8/8/8/4K3 w - - 0 1",                          -320},
    {"4k3/8/8/8/8/8/8/R3K3 w - - 0 1",                            500}, // 500 + a1 0
    {"r3k3/8/8/8/8/8/8/4K3 w - - 0 1",                           -500},
    {"4k3/8/8/8/8/8/8/3QK3 w - - 0 1",                            895}, // 900 + d1 -5
    {"3qk3/8/8/8/8/8/8/4K3 w - - 0 1",                           -895},
};

// Simetria do avaliar: espelhar as fileiras e trocar as cores inverte o sinal. Percorre as árvores como o
// perft (mesmas FENs do lanceGarantido)
#define PROF_SIMETRIA 3

// avaliar só lê board->array: a cópia espelhada só precisa dele
static void espelharCores(const Board *board, Board *espelho){
    *espelho = *board;
    for(int casa = 0; casa < 64; casa ++){
        Piece peca = board->array[casa];
        espelho->array[casa ^ 56] = (peca == EMPTY) ? peca
                                  : PIECE_MAKE(PIECE_COLOR(peca) == WHITE ? BLACK : WHITE, PIECE_TYPE(peca));
    }
}

static void percorrerSimetria(Board *board, int profundidade, bool *falha, uint64_t *nos){
    Board espelho;
    espelharCores(board, &espelho);
    (*nos) ++;
    if(avaliar(&espelho) != -avaliar(board)) *falha = true;
    if(profundidade == 0) return;

    MoveList lista;
    generate_legal_moves(board, &lista);
    for(int i = 0; i < lista.count; i ++){
        Undo undo;
        make_move(board, lista.moves[i], &undo);
        percorrerSimetria(board, profundidade - 1, falha, nos);
        unmake_move(board, lista.moves[i], &undo);
    }
}

// Profundidade dos testes de busca (go depth), sem limite de tempo
#define PROF_TESTE 4

// Kiwipete (meio-jogo carregado), usado em vários blocos. #define e não static const char*: este não
// serve de inicializador nas tabelas
#define FEN_KIWIPETE "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

typedef struct CasoEscolherJogada{
    const char *fen;
    int variante;
    int randomizador;
    int quiescencia;
    int profundidade;
    int tempoMs;
    int testarEmpate;
    int8_t resultadoEsperado;
} CasoEscolherJogada;

static const CasoEscolherJogada casosEscolherJogada[] = {
    // Parâmetros inválidos (o stub não chega a ser consultado)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      2,                  SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_VARIANTE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   2,                 QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RANDOMIZADOR_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      2,                  2,                 QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_VARIANTE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, 2,                     PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_QUIESCENCIA_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   2,                 2,                     PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RANDOMIZADOR_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, 2,                     0,            TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_QUIESCENCIA_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 0,            TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_PROFUNDIDADE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_MAX + 1, TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_PROFUNDIDADE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   -1,               SEM_EMPATE,        RESULTADO_TEMPO_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   2,                 QUIESCENCIA_DESLIGADA, 0,            -1,               SEM_EMPATE,        RESULTADO_RANDOMIZADOR_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 0,            -1,               SEM_EMPATE,        RESULTADO_PROFUNDIDADE_INVALIDA},

    // Empate por regra (valor devolvido pelo stub)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, EMPATE_PROVISORIO, RESULTADO_EMPATE_PROVISORIO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, 99,                RESULTADO_ERRO_INTERNO},

    // Raiz sem lances legais (FENs reais)
    {"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RAIZ_MATE},
    {"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",                                VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RAIZ_AFOGAMENTO},
};

// v1 = v2: minimax e alfa-beta devem devolver o mesmo score, tipo e conjunto de CANDIDATOS (SORTEIO_DESLIGADO),
// e o lance do alfa-beta deve estar entre os candidatos do minimax. Sem esperado fixo: o esperado é o próprio minimax.
// CANDIDATOS coincide porque os filhos da raiz são buscados com janela completa (valores exatos, não limites).
// Só o conjunto: o alfa-beta percorre a raiz em MVV-LVA e o minimax na ordem do gerador, então a ordem de
// CANDIDATOS e o lance (CANDIDATOS.moves[0]) podem diferir. Válido para PROF_TESTE sem limite de tempo.
// iteracoes: última iteração que deve rodar. Sem mate, PROF_TESTE; com mate em p meios-lances,
// a busca para na iteração p + 1, a primeira que o vê
typedef struct CasoV1V2{
    const char *fen;
    int iteracoes;
} CasoV1V2;

static const CasoV1V2 casosV1V2[] = {
    // Muitos lances empatados na raiz: a ordem dos candidatos importa
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",                PROF_TESTE},
    {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",              PROF_TESTE},

    // Mates em 1 (p = 1) e em 2 (p = 3), nos dois lados
    {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",                                       2},
    {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1",                                       2},
    {"6k1/8/8/8/8/8/8/RR4K1 w - - 0 1",                                         4},
    {"rr4k1/8/8/8/8/8/8/6K1 b - - 0 1",                                         4},

    // Ganho de material, nos dois lados
    {"4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",                                        PROF_TESTE},
    {"3qk3/8/8/8/3Q4/8/8/4K3 b - - 0 1",                                        PROF_TESTE},

    // Meio-jogo carregado (kiwipete)
    {FEN_KIWIPETE,                                                              PROF_TESTE},
};

// v1 = v2 com a quiescência ligada: a quiescência sem poda (minimax) é a referência da com poda.
// Profundidade por posição: sem poda, a árvore de capturas explode, mais ainda com as evasões em xeque.
// Fora, por custo (medido > 60 s): após 1.e4 já na profundidade 2 e o kiwipete já na 1; o kiwipete com
// quiescência fica coberto pelo teste de tempo.
// iteracoes: sem mate, a profundidade; com mate em p meios-lances, p (a quiescência reconhece o mate no
// próprio nó do horizonte, uma iteração antes da busca sem ela)
typedef struct CasoV1V2Quiescencia{
    const char *fen;
    int profundidade;
    int iteracoes;
} CasoV1V2Quiescencia;

static const CasoV1V2Quiescencia casosV1V2Quiescencia[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",                2, 2},

    // Mates em 1 (p = 1) e em 2 (p = 3), nos dois lados
    {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",                                       2, 1},
    {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1",                                       2, 1},
    {"6k1/8/8/8/8/8/8/RR4K1 w - - 0 1",                                         4, 3},
    {"rr4k1/8/8/8/8/8/8/6K1 b - - 0 1",                                         4, 3},

    // Ganho de material, nos dois lados
    {"4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",                                        3, 3},
    {"3qk3/8/8/8/3Q4/8/8/4K3 b - - 0 1",                                        3, 3},

    // Rica em capturas (torre solta, quatro peças a capturam; era a FEN do sorteio antes da PST)
    {"7k/8/8/3r4/5N2/1BN5/8/3QK3 w - - 0 1",                                    3, 3},
};

#define FOLGA_TEMPO_MS 50

typedef struct CasoTempo{
    int variante;
    int quiescencia;
    int tempoMs;
} CasoTempo;

// 1 ms: a iteração 1 (relógio desligado) sempre termina; no alfa-beta, a seguinte pode ser aproveitada
// parcialmente. Com a quiescência, confere também a saída por tempo nos laços dela
static const CasoTempo casosTempo[] = {
    {VARIANTE_ALFA_BETA, QUIESCENCIA_DESLIGADA, 1},
    {VARIANTE_ALFA_BETA, QUIESCENCIA_DESLIGADA, 100},
    {VARIANTE_MINIMAX,   QUIESCENCIA_DESLIGADA, 100},
    {VARIANTE_ALFA_BETA, QUIESCENCIA_LIGADA,    100},
    {VARIANTE_MINIMAX,   QUIESCENCIA_LIGADA,    100},
};

// Sementes 1..NUM_SEMENTES no teste do sorteio
#define NUM_SEMENTES 8

// Mesmos lances, na mesma ordem
static bool mesmosCandidatos(const MoveList *a, const MoveList *b){
    if(a->count != b->count) return false;
    for(int i = 0; i < a->count; i ++){
        if(a->moves[i] != b->moves[i]) return false;
    }
    return true;
}

static bool contemLance(const MoveList *lista, Move lance){
    for(int i = 0; i < lista->count; i ++){
        if(lista->moves[i] == lance) return true;
    }
    return false;
}

// Mesmos lances, em qualquer ordem (listas sem repetição, como as do gerador)
static bool mesmoConjunto(const MoveList *a, const MoveList *b){
    if(a->count != b->count) return false;
    for(int i = 0; i < a->count; i ++){
        if(!contemLance(b, a->moves[i])) return false;
    }
    return true;
}

// v1 = v2, determinismo e contagem de nós para uma FEN; acumula os totais e liga as flags em caso de falha
static void executarV1V2(const char *fen, int quiescencia, int profundidade, int iteracoes,
                         uint64_t *totalNosMinimax, uint64_t *totalNosAlfaBeta, int *retornoV1V2, int *retornoNos){
    Board board;

    if(!fen_parse(fen, &board)){
        *retornoV1V2 = 1;
        return;
    }
    // Esperado exato do minimax: sem empate por regra, a iteração de limite D visita todo nó
    // até D, e nós terminais não têm filhos, como no perft. Soma das iterações D = 1..iteracoes.
    // Com a quiescência, vale para a busca principal: NOS_VISITADOS - NOS_QUIESCENCIA.
    // O perft ignora a regra dos 50 lances; o minimax corta no empate. Para a soma valer
    // também com a regra real, o halfmove_clock não pode chegar a 100 dentro da árvore
    if(board.halfmove_clock + profundidade >= 100){
        printf("FEN v1 = v2 com halfmove alto demais para a profundidade: %s\n", fen);
        *retornoNos = 1;
        return;
    }
    uint64_t nosEsperados = 0;
    for(int limite = 1; limite <= iteracoes; limite ++){
        for(int d = 0; d <= limite; d ++) nosEsperados += perft(&board, d);
    }

    fen_parse(fen, &board);
    escolherJogada(&board, VARIANTE_MINIMAX, SORTEIO_DESLIGADO, quiescencia, profundidade, TEMPO_SEM_LIMITE);
    Play jogadaMinimax = JOGADA;
    MoveList candidatosMinimax = CANDIDATOS;
    uint64_t nosMinimax = NOS_VISITADOS;
    uint64_t nosQuiescenciaMinimax = NOS_QUIESCENCIA;
    // Sem limite de tempo: a última iteração rodada é completa
    if(PROF_ALCANCADA != iteracoes || PARCIAL_USADA) *retornoV1V2 = 1;

    fen_parse(fen, &board);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, quiescencia, profundidade, TEMPO_SEM_LIMITE);
    Play jogadaAlfaBeta = JOGADA;
    MoveList candidatosAlfaBeta = CANDIDATOS;
    uint64_t nosAlfaBeta = NOS_VISITADOS;
    uint64_t nosQuiescenciaAlfaBeta = NOS_QUIESCENCIA;
    if(PROF_ALCANCADA != iteracoes || PARCIAL_USADA) *retornoV1V2 = 1;

    fen_parse(fen, &board);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, quiescencia, profundidade, TEMPO_SEM_LIMITE);
    Play jogadaRepetida = JOGADA;
    MoveList candidatosRepetida = CANDIDATOS;
    uint64_t nosRepetida = NOS_VISITADOS;

    *totalNosMinimax += nosMinimax;
    *totalNosAlfaBeta += nosAlfaBeta;

    printf(" %u %u % " PRId32 " % " PRId32 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
           (unsigned) jogadaMinimax.lance, (unsigned) jogadaAlfaBeta.lance, jogadaMinimax.score, jogadaAlfaBeta.score,
           nosEsperados, nosMinimax, nosQuiescenciaMinimax, nosAlfaBeta, nosQuiescenciaAlfaBeta);

    // Contagem de nós: minimax exato na busca principal; alfa-beta nunca visita mais que o minimax (a poda
    // percorre um subconjunto da árvore, em qualquer ordem de lances, também na quiescência)
    if(nosMinimax - nosQuiescenciaMinimax != nosEsperados || nosAlfaBeta > nosMinimax || nosRepetida != nosAlfaBeta) *retornoNos = 1;

    // Desligada, nenhum nó de quiescência; ligada, todo horizonte entra nela
    if(quiescencia == QUIESCENCIA_DESLIGADA && (nosQuiescenciaMinimax != 0 || nosQuiescenciaAlfaBeta != 0)) *retornoNos = 1;
    if(quiescencia == QUIESCENCIA_LIGADA && (nosQuiescenciaMinimax == 0 || nosQuiescenciaAlfaBeta == 0)) *retornoNos = 1;

    // resultado == SUCESSO nas duas: sem isso, duas falhas iguais (MOVE_NONE) passariam no teste
    if(jogadaMinimax.resultado != RESULTADO_SUCESSO || jogadaAlfaBeta.resultado != RESULTADO_SUCESSO
    || jogadaMinimax.score != jogadaAlfaBeta.score || jogadaMinimax.tipo != jogadaAlfaBeta.tipo
    || !mesmoConjunto(&candidatosMinimax, &candidatosAlfaBeta)
    || !contemLance(&candidatosMinimax, jogadaAlfaBeta.lance)) *retornoV1V2 = 1;

    if(jogadaRepetida.resultado != jogadaAlfaBeta.resultado || jogadaRepetida.lance != jogadaAlfaBeta.lance
    || jogadaRepetida.score != jogadaAlfaBeta.score || jogadaRepetida.tipo != jogadaAlfaBeta.tipo
    || !mesmosCandidatos(&candidatosRepetida, &candidatosAlfaBeta)) *retornoV1V2 = 1;
}

// Quiescência no horizonte: dama captura um peão defendido por peão. Profundidade 1: sem quiescência, a
// captura parece ganhar um peão; com ela, a recaptura aparece e a captura deixa de ser candidata
typedef struct CasoHorizonte{
    const char *fen;
    const char *captura;
} CasoHorizonte;

static const CasoHorizonte casosHorizonte[] = {
    {"4k3/8/4p3/3p4/8/8/8/3QK3 w - - 0 1", "d1d5"},
    {"3qk3/8/8/8/3P4/4P3/8/4K3 b - - 0 1", "d8d4"},
};

// Mate em 1 visto pela quiescência já na profundidade 1 (o mate está no nó do horizonte); sem ela, não
typedef struct CasoMateQuiescencia{
    const char *fen;
    const char *lance;
} CasoMateQuiescencia;

static const CasoMateQuiescencia casosMateQuiescencia[] = {
    {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", "a1a8"},
    {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1", "a8a1"},
};

// lanceGarantido em afogamentos (fora de xeque, sem lances): tem de dar false. O terceiro tem um peão
// bloqueado fora das linhas do rei; o quarto, peão e rei brancos sem lance
static const char *fensAfogamento[] = {
    "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
    "k7/8/1Q6/8/8/8/8/7K b - - 0 1",
    "7k/5Q2/6K1/8/8/p7/P7/8 b - - 0 1",
    "8/8/8/8/8/5k2/5p2/5K2 w - - 0 1",
};

// lanceGarantido nas árvores percorridas como no perft: inicial, kiwipete, perft 3 e perft 4 (cravadas e xeques)
static const char *fensLanceGarantido[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    FEN_KIWIPETE,
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
};
#define PROF_LANCE_GARANTIDO 3

// Delta pruning no kiwipete (alfa-beta com quiescência): prof. 2 custa ~130 mil nós sem o delta
#define PROF_DELTA_KIWIPETE 2

// Percorre a árvore até profundidade (como o perft) e confere lanceGarantido em todo nó fora de xeque:
// true exige lista gerada não vazia. Conta os nós fora de xeque e aqueles em que a prova falhou havendo
// lance (só informativo: a quiescência gera a lista nesses casos)
static void percorrerLanceGarantido(Board *board, int profundidade, bool *falha, uint64_t *nosFora, uint64_t *provaFalhou){
    MoveList lista;
    generate_legal_moves(board, &lista);
    Color lado = board->side_to_move;

    // is_square_attacked(board, casa, lado): lado é o defensor
    if(!is_square_attacked(board, board->king_square[lado], lado)){
        bool garantido = lanceGarantido(board);
        (*nosFora) ++;
        if(garantido && lista.count == 0) *falha = true;
        if(!garantido && lista.count > 0) (*provaFalhou) ++;
    }
    if(profundidade == 0) return;

    for(int i = 0; i < lista.count; i ++){
        Undo undo;
        make_move(board, lista.moves[i], &undo);
        percorrerLanceGarantido(board, profundidade - 1, falha, nosFora, provaFalhou);
        unmake_move(board, lista.moves[i], &undo);
    }
}

// Ordenação MVV-LVA: uma captura de cada nota, todas distintas, incluindo en passant (d6)
static const char *fenOrdenacao = "7k/8/8/q2pP1r1/1P6/1n3N2/2B5/3QK3 w - d6 0 1";
// Capturas na ordem esperada: PxD (39), CxT (30), BxC (13), PxP e.p. (7), DxP (3)
static const char *capturasOrdenacao[] = {"b4a5", "f3g5", "c2b3", "e5d6", "d1d5"};

// Níveis de dificuldade: parâmetros esperados (exatos) e teto do RNF01 de cada nível
typedef struct CasoNivel{
    int nivel;
    ParametrosNivel esperado;
    int respostaMaxMs;
} CasoNivel;

static const CasoNivel casosNiveis[] = {
    {NIVEL_FACIL,   {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, 2,        180}, RESPOSTA_MAX_BASICO_MS},
    {NIVEL_MEDIO,   {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_LIGADA,    3,        180}, RESPOSTA_MAX_BASICO_MS},
    {NIVEL_DIFICIL, {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_LIGADA,    PROF_MAX, 950}, RESPOSTA_MAX_DIFICIL_MS},
};

static const int niveisInvalidos[] = {NIVEL_FACIL - 1, NIVEL_DIFICIL + 1, -1};

static bool mesmosParametros(const ParametrosNivel *a, const ParametrosNivel *b){
    return a->variante == b->variante && a->randomizador == b->randomizador && a->quiescencia == b->quiescencia
        && a->profundidade == b->profundidade && a->tempoMs == b->tempoMs;
}

int main(void){
    init_square_tables();

    Board board;
    int retornoTesteEscolherJogada = 0;
    bool parsed;

    for(size_t i = 0; i < sizeof(casosEscolherJogada)/sizeof(casosEscolherJogada[0]); i ++){
        parsed = fen_parse(casosEscolherJogada[i].fen, &board);

        if(parsed){
            varTesteEmpate = casosEscolherJogada[i].testarEmpate;
            NOS_VISITADOS = 1; // sentinela: só volta a 0 se escolherJogada zerar
            CANDIDATOS.count = 1; // sentinela: só volta a 0 se escolherJogada esvaziar
            PROF_ALCANCADA = 1; // sentinelas: só voltam a 0 e falso se escolherJogada zerar
            PARCIAL_USADA = true;
            escolherJogada(&board, casosEscolherJogada[i].variante, casosEscolherJogada[i].randomizador,
                           casosEscolherJogada[i].quiescencia, casosEscolherJogada[i].profundidade, casosEscolherJogada[i].tempoMs);
            printf(" % " PRId8 " % " PRId8 "\n", JOGADA.resultado, casosEscolherJogada[i].resultadoEsperado);        

            if(JOGADA.resultado != casosEscolherJogada[i].resultadoEsperado || JOGADA.lance != MOVE_NONE
            || JOGADA.score != SCORE_INVALIDO || JOGADA.tipo != TIPO_INVALIDO
            || NOS_VISITADOS != 0 || CANDIDATOS.count != 0
            || PROF_ALCANCADA != 0 || PARCIAL_USADA) retornoTesteEscolherJogada = 1; // sem sucesso na verificação, a busca não roda
        }
        else retornoTesteEscolherJogada = 1;
    }

    // Teste v1 = v2 (e determinismo: mesma variante duas vezes), com a quiescência desligada e ligada

    int retornoV1V2 = 0;
    int retornoNos = 0;
    uint64_t totalNosMinimax = 0, totalNosAlfaBeta = 0;

    varTesteEmpate = SEM_EMPATE;

    for(size_t i = 0; i < sizeof(casosV1V2)/sizeof(casosV1V2[0]); i ++){
        executarV1V2(casosV1V2[i].fen, QUIESCENCIA_DESLIGADA, PROF_TESTE, casosV1V2[i].iteracoes,
                     &totalNosMinimax, &totalNosAlfaBeta, &retornoV1V2, &retornoNos);
    }

    // A poda precisa podar: no total, alfa-beta visita estritamente menos nós
    if(totalNosAlfaBeta >= totalNosMinimax) retornoNos = 1;

    printf(" total: %" PRIu64 " %" PRIu64 "\n\n", totalNosMinimax, totalNosAlfaBeta);

    totalNosMinimax = 0;
    totalNosAlfaBeta = 0;

    // Delta pruning desligado: com ele, quiescenciaAlfaBeta deixa de ter o valor de quiescenciaMinimax
    ligarDeltaPruning(false);

    for(size_t i = 0; i < sizeof(casosV1V2Quiescencia)/sizeof(casosV1V2Quiescencia[0]); i ++){
        executarV1V2(casosV1V2Quiescencia[i].fen, QUIESCENCIA_LIGADA, casosV1V2Quiescencia[i].profundidade,
                     casosV1V2Quiescencia[i].iteracoes, &totalNosMinimax, &totalNosAlfaBeta, &retornoV1V2, &retornoNos);
    }

    ligarDeltaPruning(true);

    if(totalNosAlfaBeta >= totalNosMinimax) retornoNos = 1;

    printf(" total: %" PRIu64 " %" PRIu64 "\n\n", totalNosMinimax, totalNosAlfaBeta);

    // Teste do limite de tempo: profundidade PROF_MAX, que não termina no prazo.
    // O tempo gasto depende da máquina: só se exige que não passe muito do prazo (FOLGA_TEMPO_MS)

    int retornoTempo = 0;
    char fenAntes[MAX_FEN_STRING], fenDepois[MAX_FEN_STRING];
    MoveList lancesRaiz;

    for(size_t i = 0; i < sizeof(casosTempo)/sizeof(casosTempo[0]); i ++){
        if(!fen_parse(FEN_KIWIPETE, &board)){
            retornoTempo = 1;
            continue;
        }
        generate_legal_moves(&board, &lancesRaiz);
        // Nós da iteração 1 completa: raiz + um nó por lance
        uint64_t nosIteracao1 = 1 + (uint64_t) lancesRaiz.count;
        fen_write(&board, fenAntes);

        clock_t inicio = clock();
        escolherJogada(&board, casosTempo[i].variante, SORTEIO_DESLIGADO, casosTempo[i].quiescencia, PROF_MAX, casosTempo[i].tempoMs);
        long gastoMs = (long)((clock() - inicio) * 1000 / CLOCKS_PER_SEC);

        fen_write(&board, fenDepois);

        bool lanceLegal = false;
        for(int j = 0; j < lancesRaiz.count; j ++){
            if(lancesRaiz.moves[j] == JOGADA.lance) lanceLegal = true;
        }

        // A iteração aproveitada pode ser parcial (alfa-beta): os candidatos dela ainda têm de ser legais
        bool candidatosLegais = CANDIDATOS.count > 0;
        for(int j = 0; j < CANDIDATOS.count; j ++){
            if(!contemLance(&lancesRaiz, CANDIDATOS.moves[j])) candidatosLegais = false;
        }

        // PROF_ALCANCADA e PARCIAL_USADA são medição: dependem do relógio, só se confere o que vale sempre.
        // Com a quiescência, PROF_ALCANCADA = 0 é válido: só a passada de segurança valeu
        printf(" %d %d %d %ld %u %" PRIu64 " %" PRIu64 " %d %d\n", casosTempo[i].variante, casosTempo[i].quiescencia, casosTempo[i].tempoMs,
               gastoMs, (unsigned) JOGADA.lance, NOS_VISITADOS, NOS_QUIESCENCIA, PROF_ALCANCADA, (int) PARCIAL_USADA);

        if(JOGADA.resultado != RESULTADO_SUCESSO || !lanceLegal || strcmp(fenAntes, fenDepois) != 0
        || !candidatosLegais || !contemLance(&CANDIDATOS, JOGADA.lance)
        || (PROF_ALCANCADA < 1 && casosTempo[i].quiescencia == QUIESCENCIA_DESLIGADA)
        || (PROF_ALCANCADA == 0 && PARCIAL_USADA) || (casosTempo[i].variante == VARIANTE_MINIMAX && PARCIAL_USADA)
        || NOS_VISITADOS < nosIteracao1 || gastoMs > casosTempo[i].tempoMs + FOLGA_TEMPO_MS) retornoTempo = 1;
    }
    printf("\n");

    // Teste da quiescência em posições escolhidas (alfa-beta; o minimax é coberto pelo v1 = v2)

    int retornoQuiescencia = 0;

    // Horizonte: sem quiescência, a captura do peão defendido é o lance; com ela, nem é candidata
    for(size_t i = 0; i < sizeof(casosHorizonte)/sizeof(casosHorizonte[0]); i ++){
        if(!fen_parse(casosHorizonte[i].fen, &board)){
            retornoQuiescencia = 1;
            continue;
        }
        generate_legal_moves(&board, &lancesRaiz);
        int indice = movelist_find(&lancesRaiz, casosHorizonte[i].captura);
        if(indice < 0){
            retornoQuiescencia = 1;
            continue;
        }
        Move captura = lancesRaiz.moves[indice];

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 1, TEMPO_SEM_LIMITE);
        Play jogadaSem = JOGADA;

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, 1, TEMPO_SEM_LIMITE);

        printf(" %s %u % " PRId32 " %u % " PRId32 "\n", casosHorizonte[i].captura, (unsigned) jogadaSem.lance, jogadaSem.score,
               (unsigned) JOGADA.lance, JOGADA.score);

        if(jogadaSem.resultado != RESULTADO_SUCESSO || jogadaSem.lance != captura
        || JOGADA.resultado != RESULTADO_SUCESSO || contemLance(&CANDIDATOS, captura)) retornoQuiescencia = 1;
    }

    // Stand-pat: na posição inicial com profundidade 1, nenhuma resposta tem captura. Cada nó do horizonte
    // entra na quiescência uma vez e para nela: resultado igual ao sem quiescência
    const char *fenInicial = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    if(!fen_parse(fenInicial, &board)) retornoQuiescencia = 1;
    generate_legal_moves(&board, &lancesRaiz);

    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 1, TEMPO_SEM_LIMITE);
    Play jogadaSemQuiescencia = JOGADA;
    MoveList candidatosSemQuiescencia = CANDIDATOS;

    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, 1, TEMPO_SEM_LIMITE);

    printf(" stand-pat %" PRIu64 " %d\n", NOS_QUIESCENCIA, lancesRaiz.count);

    if(JOGADA.resultado != RESULTADO_SUCESSO || NOS_QUIESCENCIA != (uint64_t) lancesRaiz.count
    || JOGADA.score != jogadaSemQuiescencia.score || JOGADA.tipo != jogadaSemQuiescencia.tipo
    || !mesmosCandidatos(&CANDIDATOS, &candidatosSemQuiescencia)) retornoQuiescencia = 1;

    // Mate na quiescência: profundidade 1 basta com ela (mate em 1 lance); sem ela, centipeão
    for(size_t i = 0; i < sizeof(casosMateQuiescencia)/sizeof(casosMateQuiescencia[0]); i ++){
        if(!fen_parse(casosMateQuiescencia[i].fen, &board)){
            retornoQuiescencia = 1;
            continue;
        }
        generate_legal_moves(&board, &lancesRaiz);
        int indice = movelist_find(&lancesRaiz, casosMateQuiescencia[i].lance);
        if(indice < 0){
            retornoQuiescencia = 1;
            continue;
        }
        Move mate = lancesRaiz.moves[indice];

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 1, TEMPO_SEM_LIMITE);
        Play jogadaSem = JOGADA;

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, 1, TEMPO_SEM_LIMITE);

        printf(" %s % " PRId8 " %u % " PRId32 " % " PRId8 "\n", casosMateQuiescencia[i].lance, jogadaSem.tipo,
               (unsigned) JOGADA.lance, JOGADA.score, JOGADA.tipo);

        if(jogadaSem.resultado != RESULTADO_SUCESSO || jogadaSem.tipo != TIPO_CENTIPEAO
        || JOGADA.resultado != RESULTADO_SUCESSO || JOGADA.lance != mate || JOGADA.tipo != TIPO_MATE
        || JOGADA.score != 1) retornoQuiescencia = 1;
    }

    // lanceGarantido: false nos afogamentos; nas árvores, true só com lista não vazia
    for(size_t i = 0; i < sizeof(fensAfogamento)/sizeof(fensAfogamento[0]); i ++){
        if(!fen_parse(fensAfogamento[i], &board)){
            retornoQuiescencia = 1;
            continue;
        }
        generate_legal_moves(&board, &lancesRaiz);
        Color lado = board.side_to_move;
        bool xeque = is_square_attacked(&board, board.king_square[lado], lado);
        bool garantido = lanceGarantido(&board);

        printf(" afogamento %d %d %d\n", lancesRaiz.count, (int) xeque, (int) garantido);

        // Sem lances e fora de xeque confere a FEN; garantido tem de ser false
        if(lancesRaiz.count != 0 || xeque || garantido) retornoQuiescencia = 1;
    }

    for(size_t i = 0; i < sizeof(fensLanceGarantido)/sizeof(fensLanceGarantido[0]); i ++){
        if(!fen_parse(fensLanceGarantido[i], &board)){
            retornoQuiescencia = 1;
            continue;
        }
        bool falha = false;
        uint64_t nosFora = 0, provaFalhou = 0;

        percorrerLanceGarantido(&board, PROF_LANCE_GARANTIDO, &falha, &nosFora, &provaFalhou);

        printf(" lanceGarantido %" PRIu64 " %" PRIu64 " %d\n", nosFora, provaFalhou, (int) falha);

        if(falha) retornoQuiescencia = 1;
    }

    // Delta pruning: só tira nós. Com ele, cada posição visita no máximo os nós de sem ele, e no total
    // estritamente menos (senão o delta não está podando). Sem igualdade de lance ou score: o delta pode
    // mudá-los por construção. Posições do v1 = v2 com a quiescência, mais o kiwipete
    uint64_t totalSemDelta = 0, totalComDelta = 0;

    for(size_t i = 0; i <= sizeof(casosV1V2Quiescencia)/sizeof(casosV1V2Quiescencia[0]); i ++){
        bool kiwipete = (i == sizeof(casosV1V2Quiescencia)/sizeof(casosV1V2Quiescencia[0]));
        const char *fen = kiwipete ? FEN_KIWIPETE : casosV1V2Quiescencia[i].fen;
        int profundidade = kiwipete ? PROF_DELTA_KIWIPETE : casosV1V2Quiescencia[i].profundidade;

        if(!fen_parse(fen, &board)){
            retornoQuiescencia = 1;
            continue;
        }
        ligarDeltaPruning(false);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, profundidade, TEMPO_SEM_LIMITE);
        ligarDeltaPruning(true);
        int8_t resultadoSem = JOGADA.resultado;
        uint64_t nosSem = NOS_VISITADOS;

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, profundidade, TEMPO_SEM_LIMITE);

        printf(" delta %" PRIu64 " %" PRIu64 "\n", nosSem, NOS_VISITADOS);

        if(resultadoSem != RESULTADO_SUCESSO || JOGADA.resultado != RESULTADO_SUCESSO || NOS_VISITADOS > nosSem) retornoQuiescencia = 1;
        totalSemDelta += nosSem;
        totalComDelta += NOS_VISITADOS;
    }

    printf(" delta total: %" PRIu64 " %" PRIu64 "\n", totalSemDelta, totalComDelta);
    if(totalComDelta >= totalSemDelta) retornoQuiescencia = 1;
    printf("\n");

    // Teste do sorteio: três mates em 1 distintos (Ta8, Tb8, Tc8), empatados em MATE - 1 independentemente da
    // avaliação (com a PST, as capturas de uma mesma peça deixam de empatar). Para na iteração 1 (mate achado).
    // Desligado: o lance é o primeiro candidato. Ligado: o lance está entre os candidatos, o score
    // não muda, a mesma semente repete o lance e sementes diferentes não dão sempre o mesmo lance

    int retornoSorteio = 0;
    const char *fenSorteio = "6k1/5ppp/8/8/8/8/8/RRR3K1 w - - 0 1";

    if(!fen_parse(fenSorteio, &board)) retornoSorteio = 1;
    generate_legal_moves(&board, &lancesRaiz);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE, TEMPO_SEM_LIMITE);
    Play jogadaDesligado = JOGADA;
    MoveList candidatosDesligado = CANDIDATOS;

    // Com menos de 2 candidatos, o sorteio não teria o que sortear; com todos os lances legais como
    // candidatos, "lance ∈ CANDIDATOS" não distinguiria candidato de lance legal qualquer
    if(jogadaDesligado.resultado != RESULTADO_SUCESSO || candidatosDesligado.count < 2
    || candidatosDesligado.count >= lancesRaiz.count
    || jogadaDesligado.lance != candidatosDesligado.moves[0]) retornoSorteio = 1;

    bool lancesDistintos = false;

    for(unsigned int semente = 1; semente <= NUM_SEMENTES; semente ++){
        iniciarSorteio(semente);
        fen_parse(fenSorteio, &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE, TEMPO_SEM_LIMITE);
        Play jogadaSorteada = JOGADA;

        bool candidato = false;
        for(int j = 0; j < CANDIDATOS.count; j ++){
            if(CANDIDATOS.moves[j] == jogadaSorteada.lance) candidato = true;
        }

        iniciarSorteio(semente);
        fen_parse(fenSorteio, &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE, TEMPO_SEM_LIMITE);

        printf(" %u %d %u %u\n", semente, CANDIDATOS.count, (unsigned) jogadaSorteada.lance, (unsigned) JOGADA.lance);

        if(jogadaSorteada.lance != jogadaDesligado.lance) lancesDistintos = true;

        if(jogadaSorteada.resultado != RESULTADO_SUCESSO || !candidato || CANDIDATOS.count != candidatosDesligado.count
        || jogadaSorteada.score != jogadaDesligado.score || jogadaSorteada.tipo != jogadaDesligado.tipo
        || JOGADA.lance != jogadaSorteada.lance) retornoSorteio = 1;
    }

    if(!lancesDistintos) retornoSorteio = 1;
    printf("\n");

    // Teste da ordenação MVV-LVA: o lance primeiro (se houver) à frente, com NOTA_PRIMEIRO; depois as capturas,
    // na ordem esperada; depois os demais lances, na ordem do gerador (ordenação estável).
    // Casos de primeiro: nenhum (MOVE_NONE), o último lance sem captura do gerador (o que mais anda até a
    // frente) e a captura de menor nota (sai do meio das capturas)

    int retornoOrdenacao = 0;
    int numCapturas = (int)(sizeof(capturasOrdenacao)/sizeof(capturasOrdenacao[0]));
    LanceOrdenado ordenados[MAX_MOVES];
    char textoLance[MOVE_STR_SIZE];

    if(!fen_parse(fenOrdenacao, &board)) retornoOrdenacao = 1;
    else{
        generate_legal_moves(&board, &lancesRaiz);

        Move ultimoSemCaptura = MOVE_NONE;
        for(int j = 0; j < lancesRaiz.count; j ++){
            if(!move_is_capture(lancesRaiz.moves[j])) ultimoSemCaptura = lancesRaiz.moves[j];
        }
        int indiceCaptura = movelist_find(&lancesRaiz, capturasOrdenacao[numCapturas - 1]);
        Move menorCaptura = (indiceCaptura >= 0) ? lancesRaiz.moves[indiceCaptura] : MOVE_NONE;

        // Sem esses lances na FEN, os casos 2 e 3 repetiriam o caso 1 sem avisar
        if(ultimoSemCaptura == MOVE_NONE || menorCaptura == MOVE_NONE) retornoOrdenacao = 1;

        const Move primeiros[] = {MOVE_NONE, ultimoSemCaptura, menorCaptura};

        for(size_t c = 0; c < sizeof(primeiros)/sizeof(primeiros[0]); c ++){
            Move primeiro = primeiros[c];
            char textoPrimeiro[MOVE_STR_SIZE];
            move_to_str(primeiro, textoPrimeiro); // MOVE_NONE: "a1a1", que não casa com nenhuma captura

            ordenarLances(&board, &lancesRaiz, primeiro, ordenados);
            int k = 0;

            if(primeiro != MOVE_NONE){
                move_to_str(ordenados[0].lance, textoLance);
                printf(" primeiro %s %" PRId16 "\n", textoLance, ordenados[0].nota);

                if(ordenados[0].lance != primeiro || ordenados[0].nota != NOTA_PRIMEIRO) retornoOrdenacao = 1;
                k = 1;
            }

            for(int i = 0; i < numCapturas; i ++){
                if(strcmp(capturasOrdenacao[i], textoPrimeiro) == 0) continue;
                if(k >= lancesRaiz.count){
                    retornoOrdenacao = 1;
                    break;
                }
                move_to_str(ordenados[k].lance, textoLance);
                printf(" %s %s %" PRId16 "\n", capturasOrdenacao[i], textoLance, ordenados[k].nota);

                if(strcmp(textoLance, capturasOrdenacao[i]) != 0) retornoOrdenacao = 1;
                k ++;
            }

            for(int j = 0; j < lancesRaiz.count; j ++){
                if(move_is_capture(lancesRaiz.moves[j]) || lancesRaiz.moves[j] == primeiro) continue;
                if(k >= lancesRaiz.count || ordenados[k].lance != lancesRaiz.moves[j] || ordenados[k].nota != 0) retornoOrdenacao = 1;
                k ++;
            }
            // k == count: as capturas esperadas são todas as capturas, e nenhum lance some ou repete
            if(k != lancesRaiz.count) retornoOrdenacao = 1;
            printf("\n");
        }
    }

    // Teste dos níveis de dificuldade (RF03): nível inválido não altera os parâmetros; nível válido devolve os
    // parâmetros esperados e responde no kiwipete abaixo do teto do RNF01 (médio e difícil esgotam o prazo nele)

    int retornoNiveis = 0;
    ParametrosNivel parametros;
    const ParametrosNivel sentinelaNivel = {-7, -7, -7, -7, -7};

    for(size_t i = 0; i < sizeof(niveisInvalidos)/sizeof(niveisInvalidos[0]); i ++){
        parametros = sentinelaNivel;
        bool aceito = parametrosNivel(niveisInvalidos[i], &parametros);
        printf(" nivel %d %d\n", niveisInvalidos[i], (int) aceito);
        if(aceito || !mesmosParametros(&parametros, &sentinelaNivel)) retornoNiveis = 1;
    }

    for(size_t i = 0; i < sizeof(casosNiveis)/sizeof(casosNiveis[0]); i ++){
        parametros = sentinelaNivel;
        if(!parametrosNivel(casosNiveis[i].nivel, &parametros) || !mesmosParametros(&parametros, &casosNiveis[i].esperado)
        || !fen_parse(FEN_KIWIPETE, &board)){
            retornoNiveis = 1;
            continue;
        }

        clock_t inicio = clock();
        escolherJogada(&board, parametros.variante, parametros.randomizador, parametros.quiescencia, parametros.profundidade,
                       parametros.tempoMs);
        long gastoMs = (long)((clock() - inicio) * 1000 / CLOCKS_PER_SEC);

        printf(" nivel %d %ld %d %d %u\n", casosNiveis[i].nivel, gastoMs, PROF_ALCANCADA, (int) PARCIAL_USADA, (unsigned) JOGADA.lance);

        if(JOGADA.resultado != RESULTADO_SUCESSO || !contemLance(&CANDIDATOS, JOGADA.lance)
        || PROF_ALCANCADA > parametros.profundidade || gastoMs >= casosNiveis[i].respostaMaxMs) retornoNiveis = 1;
    }
    printf("\n");

    // Teste dos casos da avaliação do score do tabuleiro

    int retornoEvaluation = 0;
    int retornoParser = 0;
    int32_t evaluation;
    bool test;
    
    for(size_t i = 0; i < sizeof(casosAvaliacao)/sizeof(casosAvaliacao[0]); i ++){
        test = fen_parse(casosAvaliacao[i].fen, &board);

        if(test){
            evaluation = avaliar(&board);

            if(evaluation != casosAvaliacao[i].esperado){
                retornoEvaluation = 1;
            }
            else{
                printf(" % " PRId32 " % " PRId32 "\n", casosAvaliacao[i].esperado, evaluation);
            }
        }
        else{
            printf("Erro de parser\n");
            retornoParser = 1;
        }

    }

    for(size_t i = 0; i < sizeof(fensLanceGarantido)/sizeof(fensLanceGarantido[0]); i ++){
        if(!fen_parse(fensLanceGarantido[i], &board)){
            retornoParser = 1;
            continue;
        }
        bool falha = false;
        uint64_t nos = 0;

        percorrerSimetria(&board, PROF_SIMETRIA, &falha, &nos);

        printf(" simetria %" PRIu64 " %d\n", nos, (int) falha);

        if(falha) retornoEvaluation = 1;
    }

    printf("\n");

    // Teste dos casos de conversão do score

    int retornoConversao = 0;
    Move lanceAntes = -1;
    int8_t resultadoAntes = -1;

    JOGADA.lance = lanceAntes;
    JOGADA.resultado = resultadoAntes;

    int casosPulados = 0;

    for(size_t i = 0; i < sizeof(casos)/sizeof(casos[0]); i ++){
        // Mate a p meios-lances (p = MATE - |scoreBruto|) só é classificado como mate se p <= PROF_LIMITE
        if(casos[i].tipoEsperado == TIPO_MATE && MATE - abs(casos[i].scoreBruto) > PROF_LIMITE){
            casosPulados ++;
            continue;
        }
        converterScore(casos[i].scoreBruto, casos[i].lado, &JOGADA);
        
        printf(" % " PRId32 " % " PRId8 " % " PRId32 " % " PRId8 "\n", casos[i].scoreEsperado, casos[i].tipoEsperado, JOGADA.score, JOGADA.tipo);

        if(!retornoConversao && (casos[i].scoreEsperado != JOGADA.score || casos[i].tipoEsperado != JOGADA.tipo || JOGADA.lance != lanceAntes || JOGADA.resultado != resultadoAntes)){
            retornoConversao = 1;            
        }

    }
    
    if(casosPulados) printf("Conversao: %d caso(s) de mate pulado(s) (p > PROF_LIMITE = %d)\n", casosPulados, PROF_LIMITE);
    if(retornoParser) printf("Erro de parser\n");
    if(retornoEvaluation) printf("Erro de avaliaçao\n");
    if(retornoConversao) printf("erro de conversao\n");
    if(retornoTesteEscolherJogada) printf("Erro na escolha de jogada\n");
    if(retornoV1V2) printf("Erro em v1 = v2\n");
    if(retornoNos) printf("Erro na contagem de nos\n");
    if(retornoTempo) printf("Erro no limite de tempo\n");
    if(retornoQuiescencia) printf("Erro na quiescencia\n");
    if(retornoSorteio) printf("Erro no sorteio\n");
    if(retornoOrdenacao) printf("Erro na ordenacao\n");
    if(retornoNiveis) printf("Erro nos niveis\n");

    if(retornoConversao || retornoEvaluation || retornoParser || retornoTesteEscolherJogada || retornoV1V2 || retornoNos || retornoTempo
    || retornoQuiescencia || retornoSorteio || retornoOrdenacao || retornoNiveis) return 1;
    return 0;
}
