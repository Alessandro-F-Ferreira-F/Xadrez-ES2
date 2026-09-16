#include "../src/square.h"
#include "../src/piece.h"
#include "../src/board.h"
#include "../src/move.h"
#include "../src/movegen.h"
#include "../src/fen.h"
#include "../src/makemove.h"
#include "../src/utils.h"
#include "../src/types.h"


#include <stdio.h>

#define TEST_CASTLE_WQ_FEN "rnb1kbnr/pppp1ppp/8/4p1q1/3P1B2/2N5/PPPQPPPP/R3KBNR w KQkq - 0 1"
#define TEST_CASTLE_WK_FEN "rnb1kbnr/pppp1ppp/8/4p1q1/3PP3/3B1N2/PPP2PPP/RNBQK2R w KQkq - 0 1"
#define TEST_FEN_PROMOTION_CASES "rn5k/1P2P3/8/8/8/8/8/7K w - - 0 1"

#define TEST_FEN_EDGE_CASES "r3k2r/8/2q5/8/8/8/1p6/R3K2R b KQkq - 0 1"

bool process_move(Board *board, MoveList *ml) {
    char move_str[INPUT_STR_SIZE];
    Move move;
    Undo u;

    printf("Insert move: ");

    if (fgets(move_str, sizeof move_str, stdin) == NULL) {
        return false;
    }
    move_str[strcspn(move_str, "\r\n")] = '\0';

    int move_i = movelist_find(ml, move_str);

    if (move_i == -1) {
        printf("invalid move\n");
        return false;
    } else {
        move = ml->moves[move_i];
        make_move(board, move, &u);
    }

    clear_screen();
    board_print(board);
    return true;
}

int main(void) {
    init_square_tables();
    Board board = {0};

    if (!fen_parse(TEST_FEN_EDGE_CASES, &board)) {
        return 1;
    }

    MoveList ml;

    generate_all_moves(&board, &ml);
    board_print(&board);
    print_moves(&ml);

    char move_str[6];
    Move move;
    char fen_out[MAX_FEN_STRING];

    Undo u;

    if (!process_move(&board, &ml)) return 1;
    generate_all_moves(&board, &ml);
    print_moves(&ml);

    if (!process_move(&board, &ml)) return 1;
    generate_all_moves(&board, &ml);
    print_moves(&ml);

    return 0;
}