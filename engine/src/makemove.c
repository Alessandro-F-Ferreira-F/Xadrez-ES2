#include "makemove.h"
#include "move.h"
#include "square.h"
#include "board.h"

#include <assert.h>

static void check_rook_squares(int sq, u8 *castling_rights) {
    switch (sq)
    {
    case SQ_A1:
        *castling_rights &= ~(CASTLE_WQ);
        break;
    case SQ_E1:
        *castling_rights &= ~(CASTLE_WHITE);
        break;
    case SQ_H1:
        *castling_rights &= ~(CASTLE_WK);
        break;
    case SQ_A8:
        *castling_rights &= ~(CASTLE_BQ);
        break;
    case SQ_E8:
        *castling_rights &= ~(CASTLE_BLACK);
        break;
    case SQ_H8:
        *castling_rights &= ~(CASTLE_BK);
        break;
    default:
        break;
    }
}

static void update_castling_rights(Board *b, Move m) {
    int from = move_from(m);
    int to = move_to(m);

    check_rook_squares(from, &b->castling_rights);
    check_rook_squares(to, &b->castling_rights);
}

void make_move(Board *b, Move m, Undo *u) {
    int from = move_from(m);
    int to = move_to(m);

    assert(!SQ_OFFBOARD(from) && !SQ_OFFBOARD(to) && from != to && "move invalid");

    Color side = b->side_to_move; // lado que se moveu (move side)


    Piece p = b->array[from];
    Piece captured = b->array[to];

    // Cria Undo
    u->captured = captured;
    u->ep_square = b->ep_square;
    u->castling_rights = b->castling_rights;
    u->halfmove_clock = b->halfmove_clock;

    // atualiza tabuleiro
    b->array[to] = p;
    b->array[from] = EMPTY;
    b->side_to_move = (b->side_to_move == WHITE) ? BLACK : WHITE;

    // atualiza ep_square
    b->ep_square = SQ_NONE;
    if (move_type(m) == MV_DOUBLE_PUSH) {
        b->ep_square = move_from(m) + PAWN_PUSH[side];
    } 

    // atualiza king_square
    b->king_square[WHITE] = board_find_king(b, WHITE);
    b->king_square[BLACK] = board_find_king(b, BLACK);


    // atualiza direitos de roque
    update_castling_rights(b, m);

    // roque
    if (move_is_castle(m)) {
        assert((PIECE_TYPE(p) == KING) && "in castling, the moved piece must be a king");

        switch (to)
        {
        case SQ_C1:
            b->array[SQ_A1] = NO_PIECE;
            fill_sq(b, "d1", PIECE_MAKE(WHITE, ROOK));
            break;
        case SQ_G1:
            b->array[SQ_H1] = NO_PIECE;
            fill_sq(b, "f1", PIECE_MAKE(WHITE, ROOK));
            break;
        case SQ_C8:
            b->array[SQ_A8] = NO_PIECE;
            fill_sq(b, "d8", PIECE_MAKE(BLACK, ROOK));
            break;
        case SQ_G8:
            b->array[SQ_H8] = NO_PIECE;
            fill_sq(b, "f8", PIECE_MAKE(BLACK, ROOK));
            break;
        default:
            break;
        }
    }

    // capture en passant
    if (move_is_ep_capture(m)) {
        int ep_capture = to - PAWN_PUSH[side];
        b->array[ep_capture] = NO_PIECE;
    }

    // promoção
    if (move_is_promotion(m)) {
        PieceType promo_type = move_promo_type(m);
        Piece promoted = PIECE_MAKE(side, promo_type);

        b->array[to] = promoted;
    }

    if ((PIECE_TYPE(p) == PAWN) || (captured != NO_PIECE)) {
        b->halfmove_clock = 0;
    } else {
        b->halfmove_clock++;
    }
    if (side == BLACK) {
        b->fullmove_number++;
    }
}

void unmake_move(Board *b, Move move, const Undo *u);