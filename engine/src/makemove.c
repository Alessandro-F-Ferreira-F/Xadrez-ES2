#include "../include/makemove.h"
#include "../include/move.h"
#include "../include/square.h"
#include "../include/board.h"

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
        u->captured = b->array[ep_capture];
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

void unmake_move(Board *b, Move move, const Undo *u) {
    // Inverter side to move
    Color moved_side = b->side_to_move == WHITE ? BLACK : WHITE;
    b->side_to_move = moved_side;
    
    int from = move_from(move);
    int to = move_to(move);

    assert(!SQ_OFFBOARD(from) && !SQ_OFFBOARD(to) && "invalid 'from' or 'to' square");

    // Tratar casos especiais
    MoveType mt = move_type(move);

    if (move_is_promotion(move)) {
        b->array[to] = PIECE_MAKE(moved_side, PAWN);
    }
    else if (move_is_castle(move)) {
        int rook_from, rook_to;
        switch (mt)
        {
        case MV_CASTLE_QUEEN:
            rook_from = CASTLE_POSITIONS[moved_side][0][0];
            rook_to   = CASTLE_POSITIONS[moved_side][0][1];

            b->array[rook_to]   = b->array[rook_from];
            b->array[rook_from] = NO_PIECE;
            break;
        case MV_CASTLE_KING:
            rook_from = CASTLE_POSITIONS[moved_side][1][0];
            rook_to   = CASTLE_POSITIONS[moved_side][1][1];

            b->array[rook_to]   = b->array[rook_from];
            b->array[rook_from] = NO_PIECE;
            break;    
        default:
            break;
        }
    }


    // Desfazer o lance
    b->array[from] = b->array[to];
    if (move_is_ep_capture(move)) {
        int capture_sq = u->ep_square - PAWN_PUSH[moved_side];
        b->array[capture_sq] = u->captured; 
        b->array[to] = NO_PIECE;
    } else {
        b->array[to] = u->captured;
    }

    b->ep_square = u->ep_square;
    b->castling_rights = u->castling_rights;
    if (moved_side == BLACK) b->fullmove_number--;
    b->halfmove_clock = u->halfmove_clock;
    b->king_square[moved_side] = board_find_king(b, moved_side);
}