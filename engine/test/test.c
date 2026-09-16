#include "../src/square.h"
#include "../src/piece.h"
#include "../src/board.h"
#include "../src/move.h"
#include "../src/movegen.h"
#include "../src/fen.h"
#include "../src/makemove.c"

#include "../src/types.h"
#include <stdio.h>

#define TEST_CASTLE_WQ_FEN "rnb1kbnr/pppp1ppp/8/4p1q1/3P1B2/2N5/PPPQPPPP/R3KBNR w KQkq - 0 1"
#define TEST_CASTLE_WK_FEN "rnb1kbnr/pppp1ppp/8/4p1q1/3PP3/3B1N2/PPP2PPP/RNBQK2R w KQkq - 0 1"

int main(void) {
    init_square_tables();
    Board board = {0};

    fen_parse(TEST_CASTLE_WK_FEN, &board);

    MoveList ml;
    generate_all_moves(&board, &ml);
    board_print(&board);
    print_moves(&ml);

    char move_str[6];
    Move move;
    char fen_out[MAX_FEN_STRING];

    Undo u;
    printf("Insert move: ");

    fgets(move_str, sizeof move_str, stdin);
    int move_i = movelist_find(&ml, move_str);
    
    if (move_i == -1) {
        printf("invalid move");
        return -1;
    } else {
        move = ml.moves[move_i];
        make_move(&board, move, &u);
        clear_screen();
        board_print(&board);
    }



    return 0;
}