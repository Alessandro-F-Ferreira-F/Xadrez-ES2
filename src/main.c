#include "types.h"
#include "board.h"
#include "movegen.h"
#include "utils.h"

#define FEN_01 "r1bqkbnr/pp1pp1pp/2n2p2/2p3B1/3P4/1P3N2/P3PPPP/RNPQKB1R b KQkq - 0 1"

#define TEST_FEN "rnb1kb1r/2ppnppp/1p1Pp3/1p6/5P2/2N5/PPP1N2P/R1BK4 w kq - 0 11"

int main(void) {
    const char *fen = TEST_FEN;

    Board b;

    if (parse_fen(fen, &b)) {
        printf("FEN is valid.\n");
    } else {
        printf("FEN is invalid\n");
        b = (Board){0};
    }

    print_board(&b);
    
    char out_bksq[3];
    char out_wksq[3];

    coord_from_sq(b.king_square[WHITE], out_wksq);
    coord_from_sq(b.king_square[BLACK], out_bksq);


    char fen_out[MAX_FEN_STRING];
    board_to_fen(&b, fen_out);

    printf("White King Square: %s\n", out_wksq);
    printf("Black King Square: %s\n", out_bksq);
    printf("Out FEN: %s\n", fen_out);

    MoveList list = (MoveList){0};
    // MoveList list[GEN_MOVES_MAX];

    printf("list ptr: %p\n", list);
    
    generate_pawn_moves(&b, &list);

    print_moves(&list);
    print_piece_chart();
    return 0;
}
