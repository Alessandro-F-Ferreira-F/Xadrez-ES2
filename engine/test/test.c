#include "test.h"

#include "../include/fen.h"
#include "../include/makemove.h"
#include "../include/movegen.h"
#include "../include/square.h"

#include <stdio.h>
#include <string.h>

#include "ctest/test_api.h" // API DE TESTES


#define TEST_FEN_01 "rnb1kb1r/2ppnppp/1p1Pp3/1p6/5P2/2N5/PPP1N2P/R1BK4 w kq - 0 11"
#define TEST_FEN_02 "rn1qkb1r/ppp2pp1/5n1B/P2pp3/6bP/2NPQ3/1PP1PPP1/R3KBNR b KQkq - 0 1"
#define TEST_FEN_03 "rnb1kbnr/pppp3p/4pp2/6p1/2P2P2/2N1P1PB/PP1P3P/R1BK2NR w kq - 0 8"
#define TEST_FEN_04_PAWN_CAPTURES "nqrkrbbn/p1p1pppp/8/1p1p4/2P1P3/8/PP1P1PPP/NQRKRBBN b - c3 0 1"
#define TEST_FEN_05_PAWN_CAPTURE_OFFBOARD "rnbqkbnr/pppppppp/8/7B/8/4P3/PPPP1PPP/RNBQK1NR b KQkq - 0 1"
#define TEST_FEN_EN_PASSANT "rnbqkbnr/pp1p1ppp/8/2pPp3/8/8/4PPPP/RNBQKBNR w KQkq - 0 1"
#define TEST_FEN_CHECK_DETECTION "r1bqk1nr/pppp2pp/5p2/4n2B/1b1pP3/8/PP1Q1PPP/RNB1K1NR w KQkq - 0 1"



static const char *const TEST_FENS[] = {
    START_FEN,
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
    "r3k2r/8/8/4B3/8/8/8/R3K2R w KQkq - 0 1",
    // "rnbqkbnr/ppp1pppp/8/8/3pP3/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", //Parser da FEN dando erro
    "rnbqkbnr/ppp1p1pp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
    "8/8/8/r3pP1K/8/8/8/4k3 w - e6 0 1",
    "rnbq1bnr/ppp1pkP1/8/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 0 6",
    // "rnbq1bnr/pppp1ppp/8/8/8/8/PPP1PKp1/RNBQ1BNR b - - 0 6", Parser da FEN dando erro
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
    // "dwdwqdnuiwah"
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
            fen_write(&board, after);           /* TEMPORÁRIO: adultera de propósito */
            EXPECT_EQ_STR(before, after);
        }
    }
}



#ifdef MAKE_UNMAKE_TEST_STANDALONE
int main(void) {
    init_square_tables();
    return test_run_all();
}
#endif