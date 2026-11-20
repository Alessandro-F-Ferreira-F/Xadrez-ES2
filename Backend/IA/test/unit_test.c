#define __USE_MINGW_ANSI_STDIO 1

#include "fen.h"
#include "square.h"
#include "avaliar.h"
#include "ordenar.h"
#include "ia.h"
#include "stubs.h"
#include "movegen.h"
#include "makemove.h"
#include "ia_test_common.h"
#include "../../../tools/ctest/test_api.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

typedef struct CasoConversao {
    int32_t scoreBruto;
    Color lado;
    int8_t tipoEsperado;
    int32_t scoreEsperado;
} CasoConversao;

static const CasoConversao casosConversao[] = {
    {150,                 WHITE, TIPO_CENTIPEAO,  150},
    {150,                 BLACK, TIPO_CENTIPEAO, -150},
    {-150,                BLACK, TIPO_CENTIPEAO,  150},
    {0,                   WHITE, TIPO_CENTIPEAO,  0},
    {0,                   BLACK, TIPO_CENTIPEAO,  0},
    {MATE - 1,            WHITE, TIPO_MATE,  1},
    {-(MATE - 2),         WHITE, TIPO_MATE, -1},
    {MATE - 3,            WHITE, TIPO_MATE,  2},
    {-(MATE - 4),         WHITE, TIPO_MATE, -2},
    {MATE - 5,            WHITE, TIPO_MATE,  3},
    {-(MATE - 1),         BLACK, TIPO_MATE,  1},
    {MATE - 2,            BLACK, TIPO_MATE, -1},
    {-(MATE - 3),         BLACK, TIPO_MATE,  2},
    {MATE - 4,            BLACK, TIPO_MATE, -2},
    {LIMIAR_MATE,         WHITE, TIPO_MATE,       (MATE - LIMIAR_MATE + 1) / 2},
    {-LIMIAR_MATE,        WHITE, TIPO_MATE,       -((MATE - LIMIAR_MATE + 1) / 2)},
    {LIMIAR_MATE - 1,     WHITE, TIPO_CENTIPEAO,  LIMIAR_MATE - 1},
    {-(LIMIAR_MATE - 1),  WHITE, TIPO_CENTIPEAO,  -(LIMIAR_MATE - 1)},
    {LIMIAR_MATE,         BLACK, TIPO_MATE,       -((MATE - LIMIAR_MATE + 1) / 2)},
    {LIMIAR_MATE - 1,     BLACK, TIPO_CENTIPEAO,  -(LIMIAR_MATE - 1)},
};

typedef struct CasoAvaliacao {
    const char *fen;
    int32_t esperado;
} CasoAvaliacao;

static const CasoAvaliacao casosAvaliacao[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  0},
    {"k7/8/8/8/8/1NBB4/1QQQRR1N/1QQQQQQK w - - 0 1",              10340},
    {"1qqqqqqk/1qqqrr1n/1nbb4/8/8/8/8/K7 b - - 0 1",             -10340},
    {"7R/8/8/3k4/8/8/8/R3K3 w - - 0 1",                           949},
    {"r3k3/8/8/8/3K4/8/8/7r b - - 0 1",                          -949},
    {"8/8/8/3k4/8/8/8/4K3 w - - 0 1",                             -70},
    {"4k3/8/8/8/3K4/8/8/8 b - - 0 1",                              70},
    {"rnbqkbnr/pppppppp/8/8/2B5/5N2/PPPPPPPP/RNBQ1RK1 w kq - 0 1", 100},
    {"rnbq1rk1/pppppppp/5n2/2b5/8/8/PPPPPPPP/RNBQKBNR b KQ - 0 1", -100},
    {"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",                           80},
    {"4k3/4p3/8/8/8/8/8/4K3 w - - 0 1",                          -80},
    {"4k3/8/8/8/8/8/8/1N2K3 w - - 0 1",                           280},
    {"1n2k3/8/8/8/8/8/8/4K3 w - - 0 1",                          -280},
    {"4k3/8/8/8/8/8/8/2B1K3 w - - 0 1",                           320},
    {"2b1k3/8/8/8/8/8/8/4K3 w - - 0 1",                          -320},
    {"4k3/8/8/8/8/8/8/R3K3 w - - 0 1",                            500},
    {"r3k3/8/8/8/8/8/8/4K3 w - - 0 1",                           -500},
    {"4k3/8/8/8/8/8/8/3QK3 w - - 0 1",                            895},
    {"3qk3/8/8/8/8/8/8/4K3 w - - 0 1",                           -895},
};

#define PROF_SIMETRIA 3
static const char *fensSimetria[] = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    FEN_KIWIPETE,
    "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
};

static void espelharCores(const Board *board, Board *espelho) {
    *espelho = *board;
    for (int casa = 0; casa < 64; casa++) {
        Piece peca = board->array[casa];
        espelho->array[casa ^ 56] = (peca == EMPTY) ? peca
                                  : PIECE_MAKE(PIECE_COLOR(peca) == WHITE ? BLACK : WHITE, PIECE_TYPE(peca));
    }
}

static void percorrerSimetria(Board *board, int profundidade, bool *falha) {
    Board espelho;
    espelharCores(board, &espelho);
    if (avaliar(&espelho) != -avaliar(board)) *falha = true;
    if (profundidade == 0) return;

    MoveList lista;
    generate_legal_moves(board, &lista);
    for (int i = 0; i < lista.count; i++) {
        Undo undo;
        make_move(board, lista.moves[i], &undo);
        percorrerSimetria(board, profundidade - 1, falha);
        unmake_move(board, lista.moves[i], &undo);
    }
}

typedef struct CasoEscolherJogada {
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
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, EMPATE_PROVISORIO, RESULTADO_EMPATE_PROVISORIO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, 99,                RESULTADO_ERRO_INTERNO},
    {"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RAIZ_MATE},
    {"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",                                VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RAIZ_AFOGAMENTO},
};

TEST(ia_verificar_chamada) {
    Board board;
    for (size_t i = 0; i < sizeof(casosEscolherJogada) / sizeof(casosEscolherJogada[0]); i++) {
        ASSERT(fen_parse(casosEscolherJogada[i].fen, &board));
        varTesteEmpate = casosEscolherJogada[i].testarEmpate;
        NOS_VISITADOS = 1;
        CANDIDATOS.count = 1;
        PROF_ALCANCADA = 1;
        PARCIAL_USADA = true;
        escolherJogada(&board, casosEscolherJogada[i].variante, casosEscolherJogada[i].randomizador,
                       casosEscolherJogada[i].quiescencia, casosEscolherJogada[i].profundidade,
                       casosEscolherJogada[i].tempoMs);
        ASSERT_EQ_INT(JOGADA.resultado, casosEscolherJogada[i].resultadoEsperado);
        ASSERT_EQ(JOGADA.lance, MOVE_NONE);
        ASSERT_EQ_INT(JOGADA.score, SCORE_INVALIDO);
        ASSERT_EQ_INT(JOGADA.tipo, TIPO_INVALIDO);
        ASSERT_EQ_U64(NOS_VISITADOS, 0);
        ASSERT_EQ_INT(CANDIDATOS.count, 0);
        ASSERT_EQ_INT(PROF_ALCANCADA, 0);
        ASSERT(!PARCIAL_USADA);
    }
    varTesteEmpate = SEM_EMPATE;
}

TEST(ia_avaliacao) {
    Board board;
    for (size_t i = 0; i < sizeof(casosAvaliacao) / sizeof(casosAvaliacao[0]); i++) {
        ASSERT(fen_parse(casosAvaliacao[i].fen, &board));
        ASSERT_EQ_INT(avaliar(&board), casosAvaliacao[i].esperado);
    }
    for (size_t i = 0; i < sizeof(fensSimetria) / sizeof(fensSimetria[0]); i++) {
        ASSERT(fen_parse(fensSimetria[i], &board));
        bool falha = false;
        percorrerSimetria(&board, PROF_SIMETRIA, &falha);
        ASSERT(!falha);
    }
}

TEST(ia_conversao_score) {
    Move lanceAntes = (Move)-1;
    int8_t resultadoAntes = -1;
    JOGADA.lance = lanceAntes;
    JOGADA.resultado = resultadoAntes;

    for (size_t i = 0; i < sizeof(casosConversao) / sizeof(casosConversao[0]); i++) {
        if (casosConversao[i].tipoEsperado == TIPO_MATE &&
            MATE - abs(casosConversao[i].scoreBruto) > PROF_LIMITE) {
            continue;
        }
        converterScore(casosConversao[i].scoreBruto, casosConversao[i].lado, &JOGADA);
        ASSERT_EQ_INT(JOGADA.score, casosConversao[i].scoreEsperado);
        ASSERT_EQ_INT(JOGADA.tipo, casosConversao[i].tipoEsperado);
        ASSERT_EQ(JOGADA.lance, lanceAntes);
        ASSERT_EQ_INT(JOGADA.resultado, resultadoAntes);
    }
}

TEST(ia_ordenacao_mvv_lva) {
    const char *fenOrdenacao = "7k/8/8/q2pP1r1/1P6/1n3N2/2B5/3QK3 w - d6 0 1";
    const char *capturasOrdenacao[] = {"b4a5", "f3g5", "c2b3", "e5d6", "d1d5"};
    int numCapturas = (int)(sizeof(capturasOrdenacao) / sizeof(capturasOrdenacao[0]));

    Board board;
    MoveList lancesRaiz;
    LanceOrdenado ordenados[MAX_MOVES];
    char textoLance[MOVE_STR_SIZE];

    ASSERT(fen_parse(fenOrdenacao, &board));
    generate_legal_moves(&board, &lancesRaiz);

    Move ultimoSemCaptura = MOVE_NONE;
    for (int j = 0; j < lancesRaiz.count; j++) {
        if (!move_is_capture(lancesRaiz.moves[j])) ultimoSemCaptura = lancesRaiz.moves[j];
    }
    int indiceCaptura = movelist_find(&lancesRaiz, capturasOrdenacao[numCapturas - 1]);
    ASSERT(indiceCaptura >= 0);
    Move menorCaptura = lancesRaiz.moves[indiceCaptura];
    ASSERT(ultimoSemCaptura != MOVE_NONE);

    const Move primeiros[] = {MOVE_NONE, ultimoSemCaptura, menorCaptura};
    for (size_t c = 0; c < sizeof(primeiros) / sizeof(primeiros[0]); c++) {
        Move primeiro = primeiros[c];
        char textoPrimeiro[MOVE_STR_SIZE];
        move_to_str(primeiro, textoPrimeiro);

        ordenarLances(&board, &lancesRaiz, primeiro, ordenados);
        int k = 0;
        if (primeiro != MOVE_NONE) {
            ASSERT_EQ(ordenados[0].lance, primeiro);
            ASSERT_EQ_INT(ordenados[0].nota, NOTA_PRIMEIRO);
            k = 1;
        }
        for (int i = 0; i < numCapturas; i++) {
            if (strcmp(capturasOrdenacao[i], textoPrimeiro) == 0) continue;
            ASSERT(k < lancesRaiz.count);
            move_to_str(ordenados[k].lance, textoLance);
            ASSERT_EQ_STR(textoLance, capturasOrdenacao[i]);
            k++;
        }
        for (int j = 0; j < lancesRaiz.count; j++) {
            if (move_is_capture(lancesRaiz.moves[j]) || lancesRaiz.moves[j] == primeiro) continue;
            ASSERT(k < lancesRaiz.count);
            ASSERT_EQ(ordenados[k].lance, lancesRaiz.moves[j]);
            ASSERT_EQ_INT(ordenados[k].nota, 0);
            k++;
        }
        ASSERT_EQ_INT(k, lancesRaiz.count);
    }
}

#define NUM_SEMENTES 8
TEST(ia_sorteio) {
    Board board;
    MoveList lancesRaiz;
    const char *fenSorteio = "6k1/5ppp/8/8/8/8/8/RRR3K1 w - - 0 1";

    ASSERT(fen_parse(fenSorteio, &board));
    generate_legal_moves(&board, &lancesRaiz);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE, TEMPO_SEM_LIMITE);
    Play jogadaDesligado = JOGADA;
    MoveList candidatosDesligado = CANDIDATOS;

    ASSERT_EQ_INT(jogadaDesligado.resultado, RESULTADO_SUCESSO);
    ASSERT(candidatosDesligado.count >= 2 && candidatosDesligado.count < lancesRaiz.count);
    ASSERT_EQ(jogadaDesligado.lance, candidatosDesligado.moves[0]);

    bool lancesDistintos = false;
    for (unsigned int semente = 1; semente <= NUM_SEMENTES; semente++) {
        iniciarSorteio(semente);
        ASSERT(fen_parse(fenSorteio, &board));
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE, TEMPO_SEM_LIMITE);
        Play jogadaSorteada = JOGADA;

        ASSERT_EQ_INT(jogadaSorteada.resultado, RESULTADO_SUCESSO);
        ASSERT(ia_contem_lance(&CANDIDATOS, jogadaSorteada.lance));

        iniciarSorteio(semente);
        ASSERT(fen_parse(fenSorteio, &board));
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, PROF_TESTE, TEMPO_SEM_LIMITE);

        if (jogadaSorteada.lance != jogadaDesligado.lance) lancesDistintos = true;
        ASSERT_EQ_INT(CANDIDATOS.count, candidatosDesligado.count);
        ASSERT_EQ_INT(jogadaSorteada.score, jogadaDesligado.score);
        ASSERT_EQ_INT(jogadaSorteada.tipo, jogadaDesligado.tipo);
        ASSERT_EQ(JOGADA.lance, jogadaSorteada.lance);
    }
    ASSERT(lancesDistintos);
}

int main(void) {
    init_square_tables();
    return test_run_all();
}
