#include "test.h"

#include "../include/fen.h"
#include "../include/makemove.h"
#include "../include/movegen.h"
#include "../include/square.h"
#include "../include/perft.h"
#include "ctest/test_api.h" // API DE TESTES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>


/* Profundidade máxima testada por posição (as tabelas vão além). Pode ser trocada
   sem recompilar: PERFT_MAX_DEPTH=6 ./build/test.out */
#define PERFT_MAX_DEPTH 5

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static void check_perft(const char *name, const char *fen, const uint64_t *expected, int max_depth);

#define PERFT_CASES(X) \
    X(start, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", \
      20, 400, 8902, 197281, 4865609, 119060324, 3195901860) \
    X(kiwipete, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", \
      48, 2039, 97862, 4085603, 193690690) \
    X(pos3, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", \
      14, 191, 2812, 43238, 674624, 11030083, 178633661) \
    X(pos4, "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", \
      6, 264, 9467, 422333, 15833292) \
    X(pos5, "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", \
      44, 1486, 62379, 2103487, 89941194) \
    X(pos6, "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", \
      46, 2079, 89890, 3894594, 164075551)

#define PERFT_TEST(id, fen, ...) \
    TEST(perft_##id) { \
        static const uint64_t expected[] = {1, __VA_ARGS__}; \
        check_perft(#id, (fen), expected, (int)ARRAY_SIZE(expected) - 1); \
    }

static void check_perft(const char *name, const char *fen, const uint64_t *expected, int max_depth) {
    Board board;
    ASSERT_MSG(fen_parse(fen, &board), "%s: fen_parse rejeitou a FEN: %s", name, fen);

    int limit = PERFT_MAX_DEPTH;
    const char *env = getenv("PERFT_MAX_DEPTH");
    if (env && atoi(env) > 0) limit = atoi(env);

    int depth = limit < max_depth ? limit : max_depth;
    int reached = 0;

    for (int d = 1; d <= depth; d++) {
        char before[MAX_FEN_STRING], after[MAX_FEN_STRING];
        fen_write(&board, before);
        uint64_t got = perft(&board, d);
        fen_write(&board, after);

        ASSERT_MSG(got == expected[d],
                   "%s, profundidade %d: obtido %" PRIu64 ", esperado %" PRIu64 "\n\tfen: %s",
                   name, d, got, expected[d], fen);
        /* perft promete devolver o tabuleiro como entrou (unmake inverso de make). */
        ASSERT_EQ_STR(before, after);
        reached = d;
    }

    /* "Passou" sem ter testado nada é pior que falhar. */
    ASSERT_MSG(reached > 0, "%s: nenhuma profundidade testada (limite = %d)", name, limit);

    /* A linha [ RUNNING ] do runner ainda está aberta; isto completa ela. */
    if (reached < max_depth) {
        printf("d1..d%d (d%d+ pulado: limite de profundidade %d)  ", reached, reached + 1, limit);
    } else {
        printf("d1..d%d  ", reached);
    }
}

static const char *const TEST_FENS[] = {
    START_FEN,
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
    "r3k2r/8/8/4B3/8/8/8/R3K2R w KQkq - 0 1",
    "rnbqkbnr/ppp1p1pp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
    "8/8/8/r3pP1K/8/8/8/4k3 w - e6 0 1",
    "rnbq1bnr/ppp1pkP1/8/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 0 6",
    "3k3r/8/8/4N3/8/8/8/3RK2R w K - 0 1",
    "rnbqk2r/pppp1ppp/5n2/4p3/1b2P3/3P4/PPPNBPPP/R1BQK2R w KQkq - 0 5",
    "8/8/8/4N3/8/8/8/4K2k w - - 0 1",
    "8/8/8/3B4/8/8/8/3K2k1 w - - 0 1",
    "8/8/8/3R4/8/8/8/4K2k w - - 0 1",
    "8/8/8/3Q4/8/8/8/4K2k w - - 0 1",
    "8/8/3ppp2/3pKp2/3ppp2/8/8/7k w - - 0 1",
    "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
    "r1bqkbnr/pppp1ppp/2n5/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR w KQkq - 4 4",
    "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
    "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
};


TEST(make_unmake) {
    const size_t fen_count = sizeof(TEST_FENS) / sizeof(TEST_FENS[0]);

    for (size_t fen_index = 0; fen_index < fen_count; fen_index++) {
        Board board;
        ASSERT(fen_parse(TEST_FENS[fen_index], &board));

        MoveList moves = {0};
        generate_legal_moves(&board, &moves);

        for (int move_index = 0; move_index < moves.count; move_index++) {
            Move move = moves.moves[move_index];
            Undo undo;
            char before[MAX_FEN_STRING];
            char after[MAX_FEN_STRING];

            fen_write(&board, before);
            make_move(&board, move, &undo);
            unmake_move(&board, move, &undo);
            fen_write(&board, after);
            EXPECT_EQ_STR(before, after);   
        }
    }
}

PERFT_CASES(PERFT_TEST)

#ifdef MAKE_UNMAKE_TEST_STANDALONE
int main(void) {
    init_square_tables();
    return test_run_all();
}
#endif