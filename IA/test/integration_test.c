#define __USE_MINGW_ANSI_STDIO 1

#include "fen.h"
#include "perft.h"
#include "square.h"
#include "ia.h"
#include "stubs.h"
#include "movegen.h"
#include "makemove.h"
#include "iaTeste.h"
#include "ia_test_common.h"
#include "../../tools/ctest/test_api.h"

#include <stdbool.h>
#include <inttypes.h>

typedef struct CasoV1V2 {
    const char *fen;
    int iteracoes;
} CasoV1V2;

static const CasoV1V2 casosV1V2[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",   PROF_TESTE},
    {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", PROF_TESTE},
    {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",                          2},
    {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1",                          2},
    {"6k1/8/8/8/8/8/8/RR4K1 w - - 0 1",                            4},
    {"rr4k1/8/8/8/8/8/8/6K1 b - - 0 1",                            4},
    {"4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",                           PROF_TESTE},
    {"3qk3/8/8/8/3Q4/8/8/4K3 b - - 0 1",                           PROF_TESTE},
    {FEN_KIWIPETE,                                                 PROF_TESTE},
};

typedef struct CasoV1V2Quiescencia {
    const char *fen;
    int profundidade;
    int iteracoes;
} CasoV1V2Quiescencia;

static const CasoV1V2Quiescencia casosV1V2Quiescencia[] = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 2},
    {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",                        2, 1},
    {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1",                        2, 1},
    {"6k1/8/8/8/8/8/8/RR4K1 w - - 0 1",                          4, 3},
    {"rr4k1/8/8/8/8/8/8/6K1 b - - 0 1",                          4, 3},
    {"4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",                         3, 3},
    {"3qk3/8/8/8/3Q4/8/8/4K3 b - - 0 1",                         3, 3},
    {"7k/8/8/3r4/5N2/1BN5/8/3QK3 w - - 0 1",                     3, 3},
};

static bool executarV1V2(const char *fen, int quiescencia, int profundidade, int iteracoes,
                         uint64_t *totalNosMinimax, uint64_t *totalNosAlfaBeta) {
    Board board;
    if (!fen_parse(fen, &board)) return false;
    if (board.halfmove_clock + profundidade >= 100) return false;

    uint64_t nosEsperados = 0;
    for (int limite = 1; limite <= iteracoes; limite++) {
        for (int d = 0; d <= limite; d++) nosEsperados += perft(&board, d);
    }

    fen_parse(fen, &board);
    escolherJogada(&board, VARIANTE_MINIMAX, SORTEIO_DESLIGADO, quiescencia, profundidade, TEMPO_SEM_LIMITE);
    Play jogadaMinimax = JOGADA;
    MoveList candidatosMinimax = CANDIDATOS;
    uint64_t nosMinimax = NOS_VISITADOS;
    uint64_t nosQuiescenciaMinimax = NOS_QUIESCENCIA;
    if (PROF_ALCANCADA != iteracoes || PARCIAL_USADA) return false;

    fen_parse(fen, &board);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, quiescencia, profundidade, TEMPO_SEM_LIMITE);
    Play jogadaAlfaBeta = JOGADA;
    MoveList candidatosAlfaBeta = CANDIDATOS;
    uint64_t nosAlfaBeta = NOS_VISITADOS;
    uint64_t nosQuiescenciaAlfaBeta = NOS_QUIESCENCIA;
    if (PROF_ALCANCADA != iteracoes || PARCIAL_USADA) return false;

    fen_parse(fen, &board);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, quiescencia, profundidade, TEMPO_SEM_LIMITE);
    Play jogadaRepetida = JOGADA;
    MoveList candidatosRepetida = CANDIDATOS;
    uint64_t nosRepetida = NOS_VISITADOS;

    *totalNosMinimax += nosMinimax;
    *totalNosAlfaBeta += nosAlfaBeta;

    if (nosMinimax - nosQuiescenciaMinimax != nosEsperados || nosAlfaBeta > nosMinimax || nosRepetida != nosAlfaBeta)
        return false;
    if (quiescencia == QUIESCENCIA_DESLIGADA && (nosQuiescenciaMinimax != 0 || nosQuiescenciaAlfaBeta != 0))
        return false;
    if (quiescencia == QUIESCENCIA_LIGADA && (nosQuiescenciaMinimax == 0 || nosQuiescenciaAlfaBeta == 0))
        return false;
    if (jogadaMinimax.resultado != RESULTADO_SUCESSO || jogadaAlfaBeta.resultado != RESULTADO_SUCESSO ||
        jogadaMinimax.score != jogadaAlfaBeta.score || jogadaMinimax.tipo != jogadaAlfaBeta.tipo ||
        !ia_mesmo_conjunto(&candidatosMinimax, &candidatosAlfaBeta) ||
        !ia_contem_lance(&candidatosMinimax, jogadaAlfaBeta.lance))
        return false;
    if (jogadaRepetida.resultado != jogadaAlfaBeta.resultado || jogadaRepetida.lance != jogadaAlfaBeta.lance ||
        jogadaRepetida.score != jogadaAlfaBeta.score || jogadaRepetida.tipo != jogadaAlfaBeta.tipo ||
        !ia_mesmos_candidatos(&candidatosRepetida, &candidatosAlfaBeta))
        return false;

    return true;
}

static void percorrerLanceGarantido(Board *board, int profundidade, bool *falha) {
    MoveList lista;
    generate_legal_moves(board, &lista);
    Color lado = board->side_to_move;

    if (!is_square_attacked(board, board->king_square[lado], lado)) {
        bool garantido = lanceGarantido(board);
        if (garantido && lista.count == 0) *falha = true;
    }
    if (profundidade == 0) return;

    for (int i = 0; i < lista.count; i++) {
        Undo undo;
        make_move(board, lista.moves[i], &undo);
        percorrerLanceGarantido(board, profundidade - 1, falha);
        unmake_move(board, lista.moves[i], &undo);
    }
}

TEST(ia_v1_v2_e_nos) {
    uint64_t totalNosMinimax = 0, totalNosAlfaBeta = 0;
    varTesteEmpate = SEM_EMPATE;

    for (size_t i = 0; i < sizeof(casosV1V2) / sizeof(casosV1V2[0]); i++) {
        ASSERT_MSG(executarV1V2(casosV1V2[i].fen, QUIESCENCIA_DESLIGADA, PROF_TESTE, casosV1V2[i].iteracoes,
                                &totalNosMinimax, &totalNosAlfaBeta),
                   "Falha v1 = v2 (sem quiescencia) na FEN: %s", casosV1V2[i].fen);
    }
    ASSERT(totalNosAlfaBeta < totalNosMinimax);

    totalNosMinimax = 0;
    totalNosAlfaBeta = 0;
    ligarDeltaPruning(false);
    for (size_t i = 0; i < sizeof(casosV1V2Quiescencia) / sizeof(casosV1V2Quiescencia[0]); i++) {
        ASSERT_MSG(executarV1V2(casosV1V2Quiescencia[i].fen, QUIESCENCIA_LIGADA,
                                casosV1V2Quiescencia[i].profundidade, casosV1V2Quiescencia[i].iteracoes,
                                &totalNosMinimax, &totalNosAlfaBeta),
                   "Falha v1 = v2 (com quiescencia) na FEN: %s", casosV1V2Quiescencia[i].fen);
    }
    ligarDeltaPruning(true);
    ASSERT(totalNosAlfaBeta < totalNosMinimax);
}

TEST(ia_quiescencia) {
    Board board;
    MoveList lancesRaiz;
    varTesteEmpate = SEM_EMPATE;

    const struct { const char *fen; const char *captura; } casosHorizonte[] = {
        {"4k3/8/4p3/3p4/8/8/8/3QK3 w - - 0 1", "d1d5"},
        {"3qk3/8/8/8/3P4/4P3/8/4K3 b - - 0 1", "d8d4"},
    };
    for (size_t i = 0; i < 2; i++) {
        ASSERT(fen_parse(casosHorizonte[i].fen, &board));
        generate_legal_moves(&board, &lancesRaiz);
        int idx = movelist_find(&lancesRaiz, casosHorizonte[i].captura);
        ASSERT(idx >= 0);
        Move captura = lancesRaiz.moves[idx];

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 1, TEMPO_SEM_LIMITE);
        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT_EQ(JOGADA.lance, captura);

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, 1, TEMPO_SEM_LIMITE);
        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT(!ia_contem_lance(&CANDIDATOS, captura));
    }

    // Stand-pat inicial
    ASSERT(fen_parse("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", &board));
    generate_legal_moves(&board, &lancesRaiz);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 1, TEMPO_SEM_LIMITE);
    Play jogadaSemQ = JOGADA;
    MoveList candSemQ = CANDIDATOS;
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, 1, TEMPO_SEM_LIMITE);
    ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
    ASSERT_EQ_U64(NOS_QUIESCENCIA, (uint64_t)lancesRaiz.count);
    ASSERT_EQ_INT(JOGADA.score, jogadaSemQ.score);
    ASSERT_EQ_INT(JOGADA.tipo, jogadaSemQ.tipo);
    ASSERT(ia_mesmos_candidatos(&CANDIDATOS, &candSemQ));

    // Mate na quiescencia
    const struct { const char *fen; const char *lance; } casosMateQ[] = {
        {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", "a1a8"},
        {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1", "a8a1"},
    };
    for (size_t i = 0; i < 2; i++) {
        ASSERT(fen_parse(casosMateQ[i].fen, &board));
        generate_legal_moves(&board, &lancesRaiz);
        int idx = movelist_find(&lancesRaiz, casosMateQ[i].lance);
        ASSERT(idx >= 0);
        Move mate = lancesRaiz.moves[idx];

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_DESLIGADA, 1, TEMPO_SEM_LIMITE);
        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT_EQ_INT(JOGADA.tipo, TIPO_CENTIPEAO);

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, 1, TEMPO_SEM_LIMITE);
        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT_EQ(JOGADA.lance, mate);
        ASSERT_EQ_INT(JOGADA.tipo, TIPO_MATE);
        ASSERT_EQ_INT(JOGADA.score, 1);
    }

    // Afogamentos e lanceGarantido
    const char *fensAfogamento[] = {
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
        "k7/8/1Q6/8/8/8/8/7K b - - 0 1",
        "7k/5Q2/6K1/8/8/p7/P7/8 b - - 0 1",
        "8/8/8/8/8/5k2/5p2/5K2 w - - 0 1",
    };
    for (size_t i = 0; i < 4; i++) {
        ASSERT(fen_parse(fensAfogamento[i], &board));
        generate_legal_moves(&board, &lancesRaiz);
        ASSERT_EQ_INT(lancesRaiz.count, 0);
        ASSERT(!is_square_attacked(&board, board.king_square[board.side_to_move], board.side_to_move));
        ASSERT(!lanceGarantido(&board));
    }

    const char *fensLG[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        FEN_KIWIPETE,
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    };
    for (size_t i = 0; i < 4; i++) {
        ASSERT(fen_parse(fensLG[i], &board));
        bool falha = false;
        percorrerLanceGarantido(&board, 3, &falha);
        ASSERT(!falha);
    }

    // Delta pruning
    uint64_t totalSemDelta = 0, totalComDelta = 0;
    size_t nQ = sizeof(casosV1V2Quiescencia) / sizeof(casosV1V2Quiescencia[0]);
    for (size_t i = 0; i <= nQ; i++) {
        bool kiwipete = (i == nQ);
        const char *fen = kiwipete ? FEN_KIWIPETE : casosV1V2Quiescencia[i].fen;
        int prof = kiwipete ? 2 : casosV1V2Quiescencia[i].profundidade;

        ASSERT(fen_parse(fen, &board));
        ligarDeltaPruning(false);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, prof, TEMPO_SEM_LIMITE);
        ligarDeltaPruning(true);
        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        uint64_t nosSem = NOS_VISITADOS;

        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, QUIESCENCIA_LIGADA, prof, TEMPO_SEM_LIMITE);
        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT(NOS_VISITADOS <= nosSem);
        totalSemDelta += nosSem;
        totalComDelta += NOS_VISITADOS;
    }
    ASSERT(totalComDelta < totalSemDelta);
}

int main(void) {
    init_square_tables();
    return test_run_all();
}
