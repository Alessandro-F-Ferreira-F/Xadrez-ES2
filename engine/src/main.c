#include "types.h"
#include "board.h"
#include "movegen.h"
#include "utils.h"

#define FEN_01 "r1bqkbnr/pp1pp1pp/2n2p2/2p3B1/3P4/1P3N2/P3PPPP/RNPQKB1R b KQkq - 0 1"

#define TEST_FEN_01 "rnb1kb1r/2ppnppp/1p1Pp3/1p6/5P2/2N5/PPP1N2P/R1BK4 w kq - 0 11"
#define TEST_FEN_02 "rn1qkb1r/ppp2pp1/5n1B/P2pp3/6bP/2NPQ3/1PP1PPP1/R3KBNR b KQkq - 0 1"



int main(void) {
    const char *fen = TEST_FEN_02;

    Board b;

    if (parse_fen(fen, &b)) {
        printf("FEN is valid.\n");
    } else {
        printf("FEN is invalid\n");
        b = (Board){0};
    }

    char fen_out[MAX_FEN_STRING];
    board_to_fen(&b, fen_out);
    printf("Out FEN: %s\n", fen_out);

    print_board(&b);
    
    char out_bksq[3];
    char out_wksq[3];

    coord_from_sq(b.king_square[WHITE], out_wksq);
    coord_from_sq(b.king_square[BLACK], out_bksq);



    printf("White King Square: %s\n", out_wksq);
    printf("Black King Square: %s\n", out_bksq);

    MoveList list = (MoveList){0};

    precompute_move_data();
    generate_pawn_moves(&b, &list);
    generate_sliding_moves(&b, &list);

    print_moves(&list);
    

    // char move_out[5];
    // printf("Insert move (ex: e2e3): ");
    // fgets(move_out, 5, stdin);

    // u32 move = str_to_move(move_out);
    // MoveDescription movedesc = decode_move(move);
    // if (!find_move(&b, movedesc.origin_sq, movedesc.target_sq, movedesc.promotion, movedesc.flags)) {
    //     printf("INVALID MOVE!");
    // } else {
    //     make_move(&b, move);
    // }
    

    print_board(&b);

    return 0;
}
