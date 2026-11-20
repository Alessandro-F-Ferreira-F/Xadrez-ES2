#include "../include/draw.h"

DrawReason draw_reason(const Board *b) {
    if (b->halfmove_clock >= 100) return DRAW_50;

    int bishops = 0, knights = 0;
    int bishop_color = -1;
    bool same_color = true;

    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        PieceType type = PIECE_TYPE(b->array[sq]);
        if (type == EMPTY || type == KING) continue;
        if (type == KNIGHT) {
            knights++;
        } else if (type == BISHOP) {
            int color = (RANK_OF(sq) + FILE_OF(sq)) % 2;
            if (bishop_color >= 0 && color != bishop_color) same_color = false;
            bishop_color = color;
            bishops++;
        } else {
            return DRAW_NONE;
        }
    }

    /* Reis sozinhos, uma unica peca menor, ou apenas bispos todos na mesma
       cor de casa (inclui bispos promovidos). K+NN x K nao e empate automatico. */
    if (bishops + knights <= 1 || (knights == 0 && same_color)) {
        return DRAW_MATERIAL;
    }
    return DRAW_NONE;
}
