#include "types.h"
#include "board.h"
#include "movegen.h"
#include "utils.h"


#define TEST_FEN_01 "rnb1kb1r/2ppnppp/1p1Pp3/1p6/5P2/2N5/PPP1N2P/R1BK4 w kq - 0 11"
#define TEST_FEN_02 "rn1qkb1r/ppp2pp1/5n1B/P2pp3/6bP/2NPQ3/1PP1PPP1/R3KBNR b KQkq - 0 1"
#define TEST_FEN_03 "rnb1kbnr/pppp3p/4pp2/6p1/2P2P2/2N1P1PB/PP1P3P/R1BK2NR w kq - 0 8"
#define TEST_FEN_04_PAWN_CAPTURES "nqrkrbbn/p1p1pppp/8/1p1p4/2P1P3/8/PP1P1PPP/NQRKRBBN b - c3 0 1"
#define TEST_FEN_05_PAWN_CAPTURE_OFFBOARD "rnbqkbnr/pppppppp/8/7B/8/4P3/PPPP1PPP/RNBQK1NR b KQkq - 0 1"

void ui(Board *b) {
    char ch = 'y';
    int opt;
    char move_out[6];
    u32 move;
    char fen_out[MAX_FEN_STRING];
    print_board(b);

    do
    {
        printf("1 - Insert FEN\n");
        printf("2 - Make move\n");
        printf("3 - Print board\n");
        printf("4 - Clear screen\n");
        printf("5 - Quit\n");

        opt = get_int("Insert option: ");

        switch (opt)
        {
        case 1:
            get_fen(fen_out);
            parse_fen(fen_out, b);
            break;
        case 2:
            printf("Insert move: ");
            fgets(move_out, 6, stdin);
            move = str_to_move(move_out);
            MoveDescription movedesc = decode_move(move);
            if (!find_move(b, movedesc.origin_sq, movedesc.target_sq, movedesc.promotion, NULL)) {
                LOG_ERROR("invalid move");
            } else {
                make_move(b, move);
                clear_screen();
                print_board(b);
            }
            break;
        case 3:
            print_board(b);
            break;
        case 4:
            clear_screen();
            break;     
        case 5:
            ch = 'n';
            break;      
        default:
            break;
        }

    } while ((ch != 'n'));
}

int main(void) {
    precompute_move_data();

    Board b;
    const char *fen = TEST_FEN_05_PAWN_CAPTURE_OFFBOARD;


    if (parse_fen(fen, &b)) {
        printf("FEN is valid.\n");
    } else {
        printf("FEN is invalid\n");
        b = (Board){0};
    }

    print_board(&b);
    printf("Side to move: %s\n", COLOR_CHAR[b.side_to_move]);
    
    ui(&b);

    return 0;
}
