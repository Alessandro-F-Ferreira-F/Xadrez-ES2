/* nanosleep nao e C11: precisa deste macro antes do primeiro include. */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <time.h>

#include "../include/types.h"
#include "../include/board.h"
#include "../include/move.h"
#include "../include/movegen.h"
#include "../include/utils.h"
#include "../include/fen.h"
#include "../include/square.h"
#include "../include/makemove.h"
#include "../include/io.h"
#include "../include/uci.h"
#include "../test/test.h"

#define TEST_FEN_POSITION_5 "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"

/* Definido aqui, antes dos prototipos que o usam: ISO C nao permite declarar
   um enum antecipadamente. */
typedef enum {
    AI_OK,
    AI_NO_MOVES,       /* lista de lances vazia */
    AI_KING_CAPTURE    /* o sorteado captura um rei */
} AiResult;

static void ai_game(int ply, int delay_ms);
static void draw_frame(const Board *b, int played, int ply, int delay_ms);
static void filter_moves_from_sq(MoveList *all, MoveList *filtered, int sq);
static void sleep_ms(int ms);
static AiResult make_random_move(Board *b, Undo *u);
static Move get_random_move(const MoveList *l);


typedef struct {
    char move[MOVE_STR_SIZE];
    int nodes;
} PerftResult;



u64 perft(Board *board, int depth) {
    if (depth == 0) {
        return 1;
    }
    Undo u;

    MoveList list;
    int n_moves, i;
    u64 nodes = 0;

    generate_legal_moves(board, &list);
    n_moves = list.count;

    for (int i = 0; i < n_moves; i++) {
        make_move(board, list.moves[i], &u);
        nodes += perft(board, depth-1);
        unmake_move(board, list.moves[i], &u);
    }

    return nodes;
}


u64 perft_divide(Board *b, int depth, FILE *out) {
    if (depth < 1) depth = 1;

    MoveList list;
    Undo u;
    int n_moves;
    u64 nodes = 0;
    u64 total_nodes = 0;
    char move_str[MOVE_STR_SIZE];

    PerftResult result_list[MAX_MOVES];

    generate_legal_moves(b, &list);
    n_moves = list.count;

    for (int i = 0; i < n_moves; i++) {
        Move m = list.moves[i];
        move_to_str(m, move_str);

        make_move(b, m, &u);
        nodes = perft(b, depth-1);
        total_nodes += nodes;
        unmake_move(b, m, &u);

        fprintf(out, "%s: %zu\n", move_str, nodes);
    }

    fprintf(out, "\nNodes searched: %zu\n", total_nodes);
    return total_nodes;
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
        printf("10 - Perft\n");
        printf("11 - Check square attacked\n");
        printf("12 - Run make/unmake tests\n");
        printf("13 - Perft divide\n");

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
            generate_legal_moves(b, &l);
            // generate_pseudo_legal_moves(b, &l);
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
            generate_legal_moves(b, &l);
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
            generate_legal_moves(b, &l);
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
        case 10:
            int d = get_int("Insert depth: ");
            Board temp = copy_board(b);
            for (int depth = 0; depth <= d; depth++) {
                u64 perft_result = perft(&temp, depth);
                printf("Perft result [depth: %d]: %lu\n", depth, perft_result);
            }
            wait_enter();
            break;
        case 11:
            printf("Insert square coord: ");
            fflush(stdout);
            sq = read_coord();
            if (sq == SQ_NONE) {
                printf("Invalid coord\n");
                wait_enter();
                break;
            }
            Color side = get_int("Insert side (0 for BLACK, 1 for WHITE): ");
            if (side != 0 && side != 1) {
                printf("Invalid side\n");
                wait_enter();
                break;
            }
            bool is_attacked = is_square_attacked(b, sq, side);

            if (is_attacked) {
                printf("SQUARE ATTACKED!\n");
            }
            else {
                printf("SQUARE IS NOT ATTACKED!\n");
            }
            wait_enter();
            break;
        case 12:
            if (run_make_unmake_tests()) {
                printf("All make/unmake tests passed.\n");
            } else {
                printf("Make/unmake tests failed.\n");
            }
            wait_enter();
            break;
        case 13: {
            int divide_depth = get_int("Insert depth: ");
            if (divide_depth < 1) {
                printf("Invalid depth\n");
                wait_enter();
                break;
            }

            /* Em copia: se unmake_move tiver bug, o tabuleiro do menu nao e corrompido. */
            Board divide_board = copy_board(b);
            perft_divide(&divide_board, divide_depth, stdout);
            wait_enter();
            break;
        }
        default:
            break;
        }

    } while ((ch != 'n'));
}
/*
 * Sem argumento: o laco do protocolo. E' o default porque e' o que a interface
 * executa -- ela roda este binario como subprocesso e fala por stdin/stdout
 * (protocolo.md secao 3). O menu interativo continua existindo, atras do
 * argumento 'repl', porque depurar a mao continua valendo.
 */
int main(int argc, char **argv) {
    init_square_tables();

    if (argc > 1 && strcmp(argv[1], "repl") == 0) {
        Board b;
        board_new(&b);
        if(!fen_parse(TEST_FEN_POSITION_5, &b)) {
            printf("Invalid FEN\n");
            return -1;
        }

        ui(&b);

        return 0;
    }

    return uci_main(argc, argv);
}

















// * IA ALEATÓRIA (apenas teste)

static void filter_moves_from_sq(MoveList *all, MoveList *filtered, int sq) {
    movelist_clear(filtered);
    for (int i = 0; i < all->count; i++) {
        Move curr = all->moves[i];
        if (move_from(curr) == sq) {
            movelist_add(filtered, curr);
        }
    }
}

/* Pre-condicao: l->count > 0. rand() % count ja cai em [0, count). */
static Move get_random_move(const MoveList *l) {
    assert(l != NULL && l->count != 0 && "lista de lances vazia");
    return l->moves[rand() % l->count];
}

/* Sem filtro de legalidade o gerador devolve capturas de rei. Aplicar uma deixa
   o tabuleiro sem rei (king_square == -1) e a proxima geracao indexa fora dos
   limites; entao o lance NAO e aplicado e quem chama encerra a partida. */
static AiResult make_random_move(Board *b, Undo *u) {
    MoveList l;
    generate_pseudo_legal_moves(b, &l);
    if (l.count == 0) return AI_NO_MOVES;

    Move selected = get_random_move(&l);
    if (PIECE_TYPE(b->array[move_to(selected)]) == KING) return AI_KING_CAPTURE;

    make_move(b, selected, u);
    return AI_OK;
}

static void sleep_ms(int ms) {
    if (ms <= 0) return;

    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (long)(ms % 1000) * 1000000L
    };
    nanosleep(&ts, NULL);
}

/* Redesenha por cima do quadro anterior em vez de empilhar tabuleiros.
   O fflush garante que o quadro apareca mesmo quando a saida e um pipe
   (buffer de bloco) e a pausa seguinte nao o segure na memoria. */
static void draw_frame(const Board *b, int played, int ply, int delay_ms) {
    clear_screen();
    board_print(b);
    printf("\nPly %d/%d  |  %d ms per move\n", played, ply, delay_ms);
    fflush(stdout);
}

static void ai_game(int ply, int delay_ms) {
    Board b;
    Undo u;
    board_new(&b);
    draw_frame(&b, 0, ply, delay_ms);

    for (int i = 0; i < ply; i++) {
        sleep_ms(delay_ms);

        AiResult r = make_random_move(&b, &u);

        /* A mensagem de fim fica logo abaixo do ultimo quadro, sem limpar a tela. */
        if (r == AI_NO_MOVES) {
            printf("\n%s has no moves -- game over after %d plies.\n",
                   COLOR_CHAR[b.side_to_move], i);
            return;
        }
        if (r == AI_KING_CAPTURE) {
            printf("\n%s would capture a king (no legality filter yet) -- game over after %d plies.\n",
                   COLOR_CHAR[b.side_to_move], i);
            return;
        }

        draw_frame(&b, i + 1, ply, delay_ms);
    }
}
