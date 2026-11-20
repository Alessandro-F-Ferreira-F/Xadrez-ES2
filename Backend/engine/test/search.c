#include "ia.h"
#include "niveis.h"
#include "../include/fen.h"
#include "../include/movegen.h"
#include "ctest/test_api.h"

TEST(search_real_draw_adapter) {
    const struct { const char *fen; int result; } cases[] = {
        {"7k/8/8/8/8/8/8/K7 w - - 0 1", RESULTADO_EMPATE_MATERIAL},
        {"7k/8/8/8/8/8/8/KR6 w - - 100 1", RESULTADO_EMPATE_50_LANCES},
        {"7k/6Q1/5K2/8/8/8/8/8 b - - 100 1", RESULTADO_RAIZ_MATE},
        {"7k/5Q2/6K1/8/8/8/8/8 b - - 100 1", RESULTADO_RAIZ_AFOGAMENTO}
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        Board b;
        ASSERT(fen_parse(cases[i].fen, &b));
        escolherJogada(&b, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO,
                       QUIESCENCIA_DESLIGADA, 2, 0);
        EXPECT_EQ_INT(JOGADA.resultado, cases[i].result);
        EXPECT_EQ_INT(JOGADA.lance, MOVE_NONE);
    }
}

TEST(search_returns_legal_move_and_restores_board) {
    Board b;
    char before[MAX_FEN_STRING], after[MAX_FEN_STRING];
    ASSERT(fen_parse(START_FEN, &b));
    fen_write(&b, before);
    escolherJogada(&b, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO,
                   QUIESCENCIA_DESLIGADA, 2, 0);
    ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
    EXPECT_EQ_INT(PROF_ALCANCADA, 2);
    EXPECT_EQ_INT(JOGADA.tipo, TIPO_CENTIPEAO);
    MoveList legal;
    generate_legal_moves(&b, &legal);
    char move[MOVE_STR_SIZE];
    move_to_str(JOGADA.lance, move);
    EXPECT(movelist_find(&legal, move) >= 0);
    fen_write(&b, after);
    EXPECT_EQ_STR(before, after);
}

TEST(search_checkmate_on_halfmove_100) {
    Board b;
    /* O mate em um e um lance de dama sem captura: o relogio chega a 100.
       Cobrir tambem o horizonte sem quiescencia e as duas variantes. */
    for (int variante = VARIANTE_MINIMAX; variante <= VARIANTE_ALFA_BETA; variante++) {
        for (int q = QUIESCENCIA_DESLIGADA; q <= QUIESCENCIA_LIGADA; q++) {
            ASSERT(fen_parse("7k/8/5KQ1/8/8/8/8/8 w - - 99 1", &b));
            escolherJogada(&b, variante, SORTEIO_DESLIGADO, q, 2, 0);
            ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
            EXPECT_EQ_INT(JOGADA.tipo, TIPO_MATE);
            EXPECT_EQ_INT(JOGADA.score, 1);
        }
    }
}
