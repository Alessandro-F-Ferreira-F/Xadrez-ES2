#include "types.h"
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "utils.h"
#include "fen.h"
#include "square.h"
#include "makemove.h"


#define TEST_FEN_01 "rnb1kb1r/2ppnppp/1p1Pp3/1p6/5P2/2N5/PPP1N2P/R1BK4 w kq - 0 11"
#define TEST_FEN_02 "rn1qkb1r/ppp2pp1/5n1B/P2pp3/6bP/2NPQ3/1PP1PPP1/R3KBNR b KQkq - 0 1"
#define TEST_FEN_03 "rnb1kbnr/pppp3p/4pp2/6p1/2P2P2/2N1P1PB/PP1P3P/R1BK2NR w kq - 0 8"
#define TEST_FEN_04_PAWN_CAPTURES "nqrkrbbn/p1p1pppp/8/1p1p4/2P1P3/8/PP1P1PPP/NQRKRBBN b - c3 0 1"
#define TEST_FEN_05_PAWN_CAPTURE_OFFBOARD "rnbqkbnr/pppppppp/8/7B/8/4P3/PPPP1PPP/RNBQK1NR b KQkq - 0 1"

void ui(Board *b) {
    char ch = 'y';
    int opt;
    char move_str[INPUT_STR_SIZE];
    Move move;
    char fen_out[MAX_FEN_STRING];
    MoveList l = {0};
    Undo u;
    do
    {
        clear_screen();
        board_print(b);
        printf("1 - Insert FEN\n");
        printf("2 - Make move\n");
        printf("3 - Print moves\n");
        printf("4 - Clear screen\n");
        printf("5 - Quit\n");

        opt = get_int("Insert option: ");

        switch (opt)
        {
        case 1:
            get_fen(fen_out);
            /* fen_parse so escreve em '*b' se a FEN inteira validar, entao o
               tabuleiro atual sobrevive a uma entrada invalida. A mensagem vai
               por printf e nao por LOG_ERROR porque LOG_ERROR some fora do
               build de debug, e "sua FEN esta errada" e coisa que o usuario
               precisa ver sempre. */
            if (!fen_parse(fen_out, b)) {
                printf("FEN invalida -- tabuleiro nao alterado.\n");
                fgetc(stdin);
            }
            break;
        case 2:
            printf("Insert move: ");
            if (fgets(move_str, sizeof move_str, stdin) == NULL) {
                ch = 'n';
                break;
            }
            move_str[strcspn(move_str, "\r\n")] = '\0';
            generate_all_moves(b, &l);
            int move_i = movelist_find(&l, move_str);

            if (move_i == -1) {
                LOG_ERROR("invalid move");
                break;
            } else {
                move = l.moves[move_i];
                make_move(b, move, &u);
                clear_screen();
                board_print(b);
            }
            break;
        case 3:
            generate_all_moves(b, &l);
            print_moves(&l);
            MemoryZeroStruct(&l, MoveList); //reset
            fgetc(stdin);
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

        // scanf("%c", &ch);
    } while ((ch != 'n'));
}

int main(void) {
    init_square_tables();

    Board b;
    const char *fen = TEST_FEN_05_PAWN_CAPTURE_OFFBOARD;


    if (fen_parse(fen, &b)) {
        printf("FEN is valid.\n");
    } else {
        printf("FEN is invalid\n");
        b = (Board){0};
    }

    board_print(&b);
    printf("Side to move: %s\n", COLOR_CHAR[b.side_to_move]);
    


    ui(&b);

    return 0;
}
