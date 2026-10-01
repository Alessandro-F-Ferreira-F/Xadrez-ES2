#include "../include/types.h"
#include "../include/board.h"
#include "../include/move.h"
#include "../include/movegen.h"
#include "../include/utils.h"
#include "../include/fen.h"
#include "../include/square.h"
#include "../include/makemove.h"
#include "../include/io.h"



#define TEST_FEN_01 "rnb1kb1r/2ppnppp/1p1Pp3/1p6/5P2/2N5/PPP1N2P/R1BK4 w kq - 0 11"
#define TEST_FEN_02 "rn1qkb1r/ppp2pp1/5n1B/P2pp3/6bP/2NPQ3/1PP1PPP1/R3KBNR b KQkq - 0 1"
#define TEST_FEN_03 "rnb1kbnr/pppp3p/4pp2/6p1/2P2P2/2N1P1PB/PP1P3P/R1BK2NR w kq - 0 8"
#define TEST_FEN_04_PAWN_CAPTURES "nqrkrbbn/p1p1pppp/8/1p1p4/2P1P3/8/PP1P1PPP/NQRKRBBN b - c3 0 1"
#define TEST_FEN_05_PAWN_CAPTURE_OFFBOARD "rnbqkbnr/pppppppp/8/7B/8/4P3/PPPP1PPP/RNBQK1NR b KQkq - 0 1"

static Move read_move(void) {
    char move_str[WORD_CAP];
    if (!read_word(move_str, WORD_CAP)) return MOVE_NONE;


    Move temp = move_from_str(move_str);
    if (temp == MOVE_NONE) {
        LOG_ERROR("Invalid move");
        return MOVE_NONE;
    }

    return temp;
}

static int read_coord(void) {
    char coord[WORD_CAP];

    if (!read_word(coord, WORD_CAP)) return SQ_NONE;

    return sq_from_coord(coord);
}

/* O laco do menu comeca com clear_screen(): qualquer mensagem impressa no fim
   de uma opcao some antes de ser lida, a menos que o usuario tenha tempo. */
static void wait_enter(void) {
    char tmp[WORD_CAP];

    printf("\nPress Enter to continue...");
    fflush(stdout);
    read_line(tmp, sizeof(tmp));
}

static void filter_moves_from_sq(MoveList *all, MoveList *filtered, int sq) {
    movelist_clear(filtered);
    for (int i = 0; i < all->count; i++) {
        Move curr = all->moves[i];
        if (move_from(curr) == sq) {
            movelist_add(filtered, curr);
        }
    }
}

static void ui(Board *b) {
    char ch = 'y';
    int opt;
    char move_str[INPUT_STR_SIZE];
    Move move;
    Move last_move = MOVE_NONE;   /* ultimo lance jogado e ainda desfazivel; Undo so guarda um */
    char fen_out[MAX_FEN_STRING];
    MoveList l = {0};
    Undo u;
    do
    {
        clear_screen();
        board_print(b);
        printf("1 - Insert FEN\n");
        printf("2 - Make move\n");
        printf("3 - Unmake move\n");
        printf("4 - Reset board\n");
        printf("5 - Print moves from square\n");
        printf("6 - Print all moves\n");
        printf("7 - Edit square\n");
        printf("8 - Clear screen\n");
        printf("9 - Quit\n");

        opt = get_int("Insert option: ");
        /* get_int devolve 0 em EOF, e 0 cai no default: sem isto o laco nunca acaba com Ctrl-D */
        if (feof(stdin)) break;

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
            } else {
                last_move = MOVE_NONE;   /* o Undo guardado era de outra posicao */
            }
            break;
        case 2:
            printf("Insert move: ");
            if (fgets(move_str, sizeof(move_str), stdin) == NULL) {
                ch = 'n';
                break;
            }
            move_str[strcspn(move_str, "\r\n")] = '\0';
            generate_all_moves(b, &l);
            int move_i = movelist_find(&l, move_str);

            if (move_i == -1) {
                LOG_ERROR("invalid move");
                wait_enter();
                break;
            } else {
                move = l.moves[move_i];
                make_move(b, move, &u);
                clear_screen();
                board_print(b);
                last_move = move;
            }
            break;
        case 3:
            if (last_move == MOVE_NONE) {
                printf("Nothing to unmake.\n");
                wait_enter();
                break;
            }
            unmake_move(b, last_move, &u);
            last_move = MOVE_NONE;
            break;
        case 4:
            board_new(b);   /* ja comeca com board_clear */
            last_move = MOVE_NONE;
            break;
        case 5:
            printf("Insert square coord: ");
            fflush(stdout);
            int sq = read_coord();
            if (sq == SQ_NONE) {
                printf("Invalid coord\n");
                wait_enter();
                break;
            }
            generate_all_moves(b, &l);
            MoveList filtered;
            filter_moves_from_sq(&l, &filtered, sq);

            if (filtered.count == 0) {
                char coord[3];
                sq_to_coord(sq, coord);
                printf("No moves from %s (empty square, or a piece of the side not to move).\n", coord);
            } else {
                print_moves(&filtered);
            }
            wait_enter();
            break;
        case 6:
            generate_all_moves(b, &l);
            print_moves(&l);
            printf("%d moves\n", l.count);
            wait_enter();
            break;
        case 7:
            printf("Square to edit: ");
            int edit_sq = read_coord();
            printf("New piece ('.' for empty piecd): ");
            char pc = fgetc(stdin);
            int p = piece_from_char(pc);
            b->array[edit_sq] = p;
            break;   /* a tela e limpa no topo da proxima volta */
        case 8:
            break;
        case 9:
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