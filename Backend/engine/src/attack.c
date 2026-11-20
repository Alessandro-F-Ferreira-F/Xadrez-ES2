#include "../include/attack.h"

bool is_square_attacked(const Board *b, const int sq, const Color defender) {
    assert(!SQ_OFFBOARD(sq) && "Square offboard.\n");
    
    Piece piece;
    Color opposite = defender == WHITE ? BLACK : WHITE;

    //peões
    int *pawn_atk = PAWN_ATTACKS[defender][sq];
    Piece enemy = PIECE_MAKE(opposite, PAWN);

    if (pawn_atk[0] != SQ_NONE && b->array[pawn_atk[0]] == enemy) return true;
    if (pawn_atk[1] != SQ_NONE && b->array[pawn_atk[1]] == enemy) return true;
    
    // cavalos
    int *knight_atk = KNIGHT_ATTACKS[sq];
    int to;
    for (int i = 0; i < 8; i++) {
        to = knight_atk[i];
        if (to == SQ_NONE) continue;

        piece = b->array[to];
        if (piece == PIECE_MAKE(opposite, KNIGHT)) {
            return true;
        }
    }

    // diagonais (bispos e rainhas)
    int dist_to_edge;

    for (int dir = 4; dir < 8; dir++) {
        dist_to_edge = SQ_TO_EDGE[sq][dir];
        to = sq;
        for (;dist_to_edge > 0; dist_to_edge--) {
            to += DIR_OFFSET[dir];
            piece = b->array[to];
            if (piece == PIECE_MAKE(opposite, BISHOP) || piece == PIECE_MAKE(opposite, QUEEN)) return true;
            else if (piece != NO_PIECE) break;
        }
    }

    // ortogonais (torres e rainhas)
    for (int dir = 0; dir < 4; dir++) {
        to = sq;
        dist_to_edge = SQ_TO_EDGE[sq][dir];
        for (;dist_to_edge > 0; dist_to_edge--) {
            to += DIR_OFFSET[dir];
            piece = b->array[to];
            if (piece == PIECE_MAKE(opposite, ROOK)) return true;
            else if (piece == PIECE_MAKE(opposite, QUEEN)) return true;
            else if (piece != NO_PIECE) break;
        }
    }

    //king
    for (int dir = 0; dir < 8; dir++) {
        if (SQ_TO_EDGE[sq][dir] == 0) continue;

        to = sq + DIR_OFFSET[dir];
        if (SQ_OFFBOARD(to)) continue;
        piece = b->array[to];

        if (piece == PIECE_MAKE(opposite, KING)) return true;
    }

    return false;
}

bool in_check(const Board *b) {
    int sq = b->king_square[b->side_to_move];
    return !SQ_OFFBOARD(sq) && is_square_attacked(b, sq, b->side_to_move);
}
