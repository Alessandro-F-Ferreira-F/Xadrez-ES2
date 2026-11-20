#include "../include/attack.h"
#include "../include/draw.h"
#include "../include/fen.h"
#include "../include/movegen.h"
#include "ctest/test_api.h"

TEST(king_targets) {
    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        int count = 0;
        for (int dir = 0; dir < NUM_DIRS; dir++) {
            int to = KING_TARGETS[sq][dir];
            if (to == SQ_NONE) continue;
            EXPECT(!SQ_OFFBOARD(to));
            EXPECT(abs(RANK_OF(to) - RANK_OF(sq)) <= 1);
            EXPECT(abs(FILE_OF(to) - FILE_OF(sq)) <= 1);
            EXPECT(to != sq);
            for (int prev = 0; prev < dir; prev++) EXPECT(to != KING_TARGETS[sq][prev]);
            count++;
        }
        bool edge_rank = RANK_OF(sq) == 0 || RANK_OF(sq) == 7;
        bool edge_file = FILE_OF(sq) == 0 || FILE_OF(sq) == 7;
        EXPECT_EQ_INT(count, edge_rank && edge_file ? 3 : edge_rank || edge_file ? 5 : 8);
    }
    EXPECT_EQ_INT(KING_TARGETS[SQ_A1][DIR_S], SQ_NONE);
    EXPECT_EQ_INT(KING_TARGETS[SQ_A1][DIR_N], 8);
}

TEST(fen_rejects_illegal_check_atomically) {
    Board b;
    char before[MAX_FEN_STRING], after[MAX_FEN_STRING];
    ASSERT(fen_parse(START_FEN, &b));
    fen_write(&b, before);
    const char *invalid[] = {
        "4k3/8/8/8/8/8/4R3/4K3 w - - 0 1",
        "4k3/4r3/8/8/8/8/8/4K3 b - - 0 1",
        "8/8/8/8/8/8/4k3/4K3 w - - 0 1"
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        EXPECT(!fen_parse(invalid[i], &b));
        fen_write(&b, after);
        EXPECT_EQ_STR(before, after);
    }
    /* O lado a jogar PODE estar em xeque. O terceiro argumento e o defensor. */
    ASSERT(fen_parse("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1", &b));
    EXPECT(in_check(&b));
    EXPECT(is_square_attacked(&b, SQ_E1, WHITE));
    EXPECT(!is_square_attacked(&b, SQ_E1, BLACK));
}

TEST(draw_rules) {
    const struct { const char *fen; DrawReason expected; } cases[] = {
        {START_FEN, DRAW_NONE},
        {"7k/8/8/8/8/8/8/K7 w - - 0 1", DRAW_MATERIAL},
        {"7k/8/8/8/8/8/8/KB6 w - - 0 1", DRAW_MATERIAL},
        {"7k/8/8/8/8/8/8/KN6 w - - 0 1", DRAW_MATERIAL},
        {"7k/8/8/8/8/8/8/KR6 w - - 0 1", DRAW_NONE},
        {"7k/8/8/8/8/8/P7/K7 w - - 0 1", DRAW_NONE},
        {"7k/8/8/8/8/8/8/KNN5 w - - 0 1", DRAW_NONE},
        {"7k/8/8/8/8/8/8/KNB5 w - - 0 1", DRAW_NONE},
        {"7k/8/8/8/8/8/8/KBB5 w - - 0 1", DRAW_NONE},
        {"7k/8/8/8/8/8/8/KB1B4 w - - 0 1", DRAW_MATERIAL},
        {"7k/8/8/8/8/8/2b5/KB6 w - - 0 1", DRAW_MATERIAL},
        {"7k/8/8/8/8/8/1b6/KB6 w - - 0 1", DRAW_NONE},
        {"7k/8/8/8/8/8/8/K7 w - - 100 1", DRAW_50},
        {"7k/8/8/8/8/8/8/KR6 w - - 99 1", DRAW_NONE},
        {"7k/8/8/8/8/8/8/KR6 w - - 100 1", DRAW_50}
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        Board b;
        char before[MAX_FEN_STRING], after[MAX_FEN_STRING];
        ASSERT_MSG(fen_parse(cases[i].fen, &b), "FEN invalida: %s", cases[i].fen);
        fen_write(&b, before);
        EXPECT_EQ_INT(draw_reason(&b), cases[i].expected);
        fen_write(&b, after);
        EXPECT_EQ_STR(before, after);
    }
}
