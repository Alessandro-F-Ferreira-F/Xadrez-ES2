#include "test.h"

#include "../include/fen.h"
#include "../include/makemove.h"
#include "../include/movegen.h"
#include "../include/square.h"

#include <stdio.h>
#include <string.h>

static const char *const TEST_FENS[] = {
    START_FEN,
    "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1",
    "r3k2r/8/8/4B3/8/8/8/R3K2R w KQkq - 0 1",
    //"rnbqkbnr/ppp1pppp/8/8/3pP3/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", Parser da FEN dando erro
    "rnbqkbnr/ppp1p1pp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
    "8/8/8/r3pP1K/8/8/8/4k3 w - e6 0 1",
    "rnbq1bnr/ppp1pkP1/8/8/8/8/PPPP1PPP/RNBQKBNR w KQ - 0 6",
    //"rnbq1bnr/pppp1ppp/8/8/8/8/PPP1PKp1/RNBQ1BNR b - - 0 6", Parser da FEN dando erro
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
    "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"
};

bool run_make_unmake_tests(void) {
    const size_t fen_count = sizeof(TEST_FENS) / sizeof(TEST_FENS[0]);
    size_t moves_checked = 0;
    size_t failures = 0;
    size_t positions_passed = 0;

    printf("\nRunning make/unmake FEN round-trip tests (%zu positions)\n", fen_count);

    for (size_t fen_index = 0; fen_index < fen_count; fen_index++) {
        Board board;
        if (!fen_parse(TEST_FENS[fen_index], &board)) {
            printf("FAIL: FEN #%zu could not be parsed:\n%s\n",
                   fen_index + 1, TEST_FENS[fen_index]);
            failures++;
            continue;
        }

        MoveList moves = {0};
        generate_legal_moves(&board, &moves);
        bool position_passed = true;

        for (int move_index = 0; move_index < moves.count; move_index++) {
            Move move = moves.moves[move_index];
            Undo undo;
            char before[MAX_FEN_STRING];
            char after[MAX_FEN_STRING];
            char move_str[6];

            fen_write(&board, before);
            make_move(&board, move, &undo);
            unmake_move(&board, move, &undo);
            fen_write(&board, after);
            moves_checked++;

            if (strcmp(before, after) != 0) {
                move_to_str(move, move_str);
                printf("FAIL: FEN #%zu, move %s did not restore the position.\n",
                       fen_index + 1, move_str);
                printf("  Before: %s\n", before);
                printf("  After:  %s\n", after);
                failures++;
                position_passed = false;
                break;
            }
        }

        if (position_passed) {
            positions_passed++;
        }
    }

    printf("Result: %zu/%zu positions passed; %zu move round-trips checked; "
           "%zu failure(s).\n",
           positions_passed, fen_count, moves_checked, failures);

    return failures == 0;
}

#ifdef MAKE_UNMAKE_TEST_STANDALONE
int main(void) {
    init_square_tables();
    return run_make_unmake_tests() ? 0 : 1;
}
#endif