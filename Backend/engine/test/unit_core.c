#include "../include/board.h"
#include "../include/fen.h"
#include "../include/io.h"
#include "../include/move.h"
#include "../include/piece.h"
#include "../include/square.h"
#include "../../../tools/ctest/test_api.h"

#include <stdlib.h>
#include <string.h>

TEST(board_invariants_and_lifecycle) {
    Board b;
    board_new(&b);

    const int sq_e4 = SQ_AT(3, 4);
    const int sq_e5 = SQ_AT(4, 4);
    const int sq_d1 = SQ_AT(0, 3);
    const int sq_d8 = SQ_AT(7, 3);

    ASSERT(board_check_invariants(&b));
    EXPECT(is_empty(&b, sq_e4));
    EXPECT(!is_empty(&b, SQ_E1));
    EXPECT(!fill_sq(&b, "z9", NO_PIECE));
    EXPECT(fill_sq(&b, "e4", PIECE_MAKE(WHITE, PAWN)));
    EXPECT(!is_empty(&b, sq_e4));
    EXPECT(fill_sq(&b, "e4", EMPTY));

    /* Imprime o tabuleiro (exercita board_print) */
    board_print(&b);

    /* 1. Codigo de peca >= 16 */
    Board bad = b;
    bad.array[sq_e4] = (Piece)16u;
    EXPECT(!board_check_invariants(&bad));

    /* 2. Tipo de peca invalido (ex.: 7) */
    bad = b;
    bad.array[sq_e4] = (Piece)7u;
    EXPECT(!board_check_invariants(&bad));

    /* 3. Numero invalido de reis brancos (0 e 2) */
    bad = b;
    bad.array[SQ_E1] = EMPTY;
    EXPECT(!board_check_invariants(&bad));

    bad = b;
    bad.array[sq_e4] = PIECE_MAKE(WHITE, KING);
    EXPECT(!board_check_invariants(&bad));

    /* 4. Numero invalido de reis pretos (0 e 2) */
    bad = b;
    bad.array[SQ_E8] = EMPTY;
    EXPECT(!board_check_invariants(&bad));

    bad = b;
    bad.array[sq_e5] = PIECE_MAKE(BLACK, KING);
    EXPECT(!board_check_invariants(&bad));

    /* 5. Cache king_square dessincronizado (brancas e pretas) */
    bad = b;
    bad.king_square[WHITE] = sq_d1;
    EXPECT(!board_check_invariants(&bad));

    bad = b;
    bad.king_square[BLACK] = sq_d8;
    EXPECT(!board_check_invariants(&bad));

    /* 6. Peao na primeira ou ultima fileira */
    bad = b;
    bad.array[SQ_A1] = PIECE_MAKE(WHITE, PAWN);
    EXPECT(!board_check_invariants(&bad));

    bad = b;
    bad.array[SQ_H8] = PIECE_MAKE(BLACK, PAWN);
    EXPECT(!board_check_invariants(&bad));

    /* 7. board_clear zera estado e remove reis */
    board_clear(&b);
    EXPECT_EQ_INT(board_find_king(&b, WHITE), -1);
    EXPECT_EQ_INT(board_find_king(&b, BLACK), -1);
    EXPECT_EQ_INT(b.ep_square, SQ_NONE);
    EXPECT_EQ_INT(b.fullmove_number, 1);
}

TEST(io_strings_and_join_args) {
    String empty = string_make("", 0);
    String s = string_from_cstr("  \t xadrez-es2 \r\n");

    EXPECT_EQ_U64(string_len(empty), 0);
    EXPECT(string_to_cstr(empty) == NULL);

    /* Trim e chop */
    string_trim(&s);
    EXPECT(string_eq(s, string_from_cstr("xadrez-es2")));
    EXPECT(!string_eq(s, string_from_cstr("outro")));
    EXPECT(string_starts_with(s, string_from_cstr("xadrez")));
    EXPECT(!string_starts_with(s, string_from_cstr("xadrez-es2-extra")));
    EXPECT(!string_starts_with(s, string_from_cstr("chess")));

    char *cstr = string_to_cstr(s);
    ASSERT(cstr != NULL);
    EXPECT_EQ_STR(cstr, "xadrez-es2");
    free(cstr);

    string_print(s);

    string_chop_left(&s);
    string_chop_right(&s);
    EXPECT(string_eq(s, string_from_cstr("adrez-es")));
    string_chop_left(&empty);
    string_chop_right(&empty);
    EXPECT_EQ_U64(string_len(empty), 0);

    /* string_parse_int */
    int val = 0;
    EXPECT(string_parse_int(string_from_cstr("123"), &val));
    EXPECT_EQ_INT(val, 123);
    EXPECT(string_parse_int(string_from_cstr("-45"), &val));
    EXPECT_EQ_INT(val, -45);
    EXPECT(!string_parse_int(string_from_cstr("12x"), &val));
    EXPECT(!string_parse_int(empty, &val));

    /* string_split */
    String toks[2];
    EXPECT_EQ_INT(string_split("a b c", toks, 0), -1);
    EXPECT_EQ_INT(string_split("  um   dois   tres ", toks, 2), 2);
    EXPECT(string_eq(toks[0], string_from_cstr("um")));
    EXPECT(string_eq(toks[1], string_from_cstr("dois")));

    /* join_args */
    char buf[16];
    char arg0[] = "prog";
    char arg1[] = "abc";
    char arg2[] = "def";
    char *argv_ok[] = { arg0, arg1, arg2 };
    char *argv_null[] = { arg0, NULL };

    EXPECT(!join_args(3, argv_ok, 1, buf, 0));
    EXPECT(!join_args(2, argv_null, 1, buf, sizeof(buf)));
    EXPECT(!join_args(3, argv_ok, 1, buf, 4));
    EXPECT(join_args(3, argv_ok, 1, buf, sizeof(buf)));
    EXPECT_EQ_STR(buf, "abcdef");
}

TEST(fen_validation_error_paths) {
    Board b;
    char out[MAX_FEN_STRING];

    /* Argumentos nulos e string longa demais */
    EXPECT(!fen_parse(NULL, &b));
    EXPECT(!fen_parse(START_FEN, NULL));

    char long_fen[MAX_FEN_STRING + 16];
    memset(long_fen, 'a', sizeof(long_fen) - 1);
    long_fen[sizeof(long_fen) - 1] = '\0';
    EXPECT(!fen_parse(long_fen, &b));

    /* Campos demais e campos de menos */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 w - - 0 1 extra", &b));
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 w -", &b));

    /* Erros de disposicao de pecas (campo 1) */
    EXPECT(!fen_parse("7/8/8/8/8/8/8/4k2K w - - 0 1", &b));                 /* rank < 8 */
    EXPECT(!fen_parse("8/8/8/8/8/8/8/4k2K/8 w - - 0 1", &b));               /* 9 ranks */
    EXPECT(!fen_parse("9/8/8/8/8/8/8/4k2K w - - 0 1", &b));                 /* digito > 8 */
    EXPECT(!fen_parse("0/8/8/8/8/8/8/4k2K w - - 0 1", &b));                 /* digito 0 */
    EXPECT(!fen_parse("4k4/8/8/8/8/8/8/4K3 w - - 0 1", &b));                /* run overflow */
    EXPECT(!fen_parse("rrrrrrrrr/8/8/8/8/8/8/4k2K w - - 0 1", &b));         /* > 8 pecas na rank */
    EXPECT(!fen_parse("P3k3/8/8/8/8/8/8/4K3 w - - 0 1", &b));               /* peao na rank 8 */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K2p w - - 0 1", &b));               /* peao na rank 1 */
    EXPECT(!fen_parse("4k2X/8/8/8/8/8/8/4K3 w - - 0 1", &b));               /* char invalido */
    EXPECT(!fen_parse("4k3/8/8/4K3 w - - 0 1", &b));                        /* < 64 casas */
    EXPECT(!fen_parse("8/8/8/8/8/8/8/4K3 w - - 0 1", &b));                  /* falta rei preto */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/3KK3 w - - 0 1", &b));               /* 2 reis brancos */
    EXPECT(!fen_parse("4k3/8/8/8/8/PPPPPPPP/P7/4K3 w - - 0 1", &b));        /* 9 peoes */
    EXPECT(!fen_parse("4k3/8/8/8/QQQQQQQQ/QQQQQQQQ/Q7/4K3 w - - 0 1", &b)); /* > 16 pecas */

    /* Campo 2 (lado invalido) */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 x - - 0 1", &b));

    /* Campo 3 (roque invalido ou incoerente com tabuleiro) */
    EXPECT(!fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkqK - 0 1", &b));        /* > 4 chars */
    EXPECT(!fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KQx - 0 1", &b));          /* char invalido */
    EXPECT(!fen_parse("r3k2r/8/8/8/8/8/8/R3K2R w KK - 0 1", &b));           /* duplicado */
    EXPECT(!fen_parse("r3k2r/8/8/8/8/8/8/R2K3R w KQ - 0 1", &b));           /* rei fora de e1 */
    EXPECT(!fen_parse("r3k2r/8/8/8/8/8/8/4K3 w K - 0 1", &b));              /* torre fora de h1 */

    /* Campo 4 (en passant invalido) */
    EXPECT(!fen_parse("4k3/8/8/8/4P3/8/8/4K3 b - e34 0 1", &b));            /* len != 2 */
    EXPECT(!fen_parse("4k3/8/8/8/4P3/8/8/4K3 b - z3 0 1", &b));             /* coord invalida */
    EXPECT(!fen_parse("4k3/8/8/8/4P3/8/8/4K3 w - e3 0 1", &b));             /* rank errada para w */
    EXPECT(!fen_parse("4k3/8/8/8/4P3/4N3/8/4K3 b - e3 0 1", &b));           /* casa ep ocupada */
    EXPECT(!fen_parse("4k3/8/8/8/4P3/8/4N3/4K3 b - e3 0 1", &b));           /* origem ocupada */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 b - e3 0 1", &b));               /* sem peao inimigo */

    /* Campos 5 e 6 (relogios invalidos) */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 w - - abc 1", &b));              /* halfmove nao digito */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 w - - 9999999 1", &b));          /* halfmove fora faixa */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 w - - 0 abc", &b));              /* fullmove nao digito */
    EXPECT(!fen_parse("4k3/8/8/8/8/8/8/4K3 w - - 0 0", &b));                /* fullmove < 1 */

    /* FEN de 4 campos (halfmove/fullmove opcionais) + fen_write round-trip */
    ASSERT(fen_parse("4k3/8/8/8/4P3/8/8/4K3 b - e3", &b));
    EXPECT_EQ_INT(b.halfmove_clock, 0);
    EXPECT_EQ_INT(b.fullmove_number, 1);
    EXPECT_EQ_INT(b.ep_square, SQ_AT(2, 4));
    fen_write(&b, out);
    EXPECT_EQ_STR(out, "4k3/8/8/8/4P3/8/8/4K3 b - e3 0 1");
}

TEST(move_and_square_utilities) {
    /* square.c: bordas de sq_from_coord e sq_to_coord(SQ_NONE) */
    EXPECT_EQ_INT(sq_from_coord(NULL), SQ_NONE);
    EXPECT_EQ_INT(sq_from_coord("z1"), SQ_NONE);
    EXPECT_EQ_INT(sq_from_coord("a9"), SQ_NONE);

    char coord[3];
    sq_to_coord(SQ_NONE, coord);
    EXPECT_EQ_STR(coord, "--");

    /* move.c: move_from_str para promocoes n/b/r/q e entradas invalidas */
    EXPECT_EQ(move_from_str(""), MOVE_NONE);
    EXPECT_EQ(move_from_str("e2"), MOVE_NONE);
    EXPECT_EQ(move_from_str("e2e4qq"), MOVE_NONE);
    EXPECT_EQ(move_from_str("z1e4"), MOVE_NONE);
    EXPECT_EQ(move_from_str("e2z4"), MOVE_NONE);

    Move mn = move_from_str("a7a8n");
    Move mb = move_from_str("a7a8b");
    Move mr = move_from_str("a7a8r");
    Move mq = move_from_str("a7a8q");
    Move mquiet = move_from_str("e2e4");

    EXPECT_EQ_INT((int)move_type(mn), (int)MV_PROMO_N);
    EXPECT_EQ_INT((int)move_type(mb), (int)MV_PROMO_B);
    EXPECT_EQ_INT((int)move_type(mr), (int)MV_PROMO_R);
    EXPECT_EQ_INT((int)move_type(mq), (int)MV_PROMO_Q);
    EXPECT_EQ_INT((int)move_type(mquiet), (int)MV_QUIET);

    /* movelist_add (inclusive overflow em MAX_MOVES), movelist_find e print_moves */
    MoveList list;
    movelist_clear(&list);
    movelist_add(&list, mquiet);
    movelist_add(&list, mb);
    EXPECT_EQ_INT(movelist_find(&list, "z9z9"), -1);
    EXPECT_EQ_INT(movelist_find(&list, "e2e4"), 0);
    EXPECT_EQ_INT(movelist_find(&list, "a7a8b"), 1);
    EXPECT_EQ_INT(movelist_find(&list, "a7a8q"), -1);

    print_moves(&list);

    while (list.count < MAX_MOVES) {
        movelist_add(&list, mquiet);
    }
    movelist_add(&list, mq); /* deve descartar sem estourar o vetor */
    EXPECT_EQ_INT(list.count, MAX_MOVES);
}
