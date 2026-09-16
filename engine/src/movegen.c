#include "movegen.h"
#include "piece.h"
#include "square.h"

#include "assert.h"

bool check_pawn_promotion(int from) {
    if ((RANK_OF(from) == 0) || (RANK_OF(from) == 7)) return true;
    return false;
}

void generate_pawn_moves(const Board *b, MoveList *list) {
    int from, to;
    Piece piece;
    Move move;

    Color side = b->side_to_move;


    for (from = 0; from < BOARD_SIZE; from++) {
        piece = b->array[from];

        if (PIECE_TYPE(piece) != PAWN) continue;
        if (PIECE_COLOR(piece) != side) continue;

        // push padrão do peão
        to = from + PAWN_PUSH[side];
        if ((SQ_OFFBOARD(to) == 0) && is_empty(b, to)) {
            // promoção do peão
            if (check_pawn_promotion(to)) {
                for (int t = MV_PROMO_N; t <= MV_PROMO_Q; t++) {
                    move = encode_move(from, to, (MoveType)t);
                    movelist_add(list, move);
                }
            } else {
                move = encode_move(from, to, MV_QUIET);
                movelist_add(list, move);
            }

            // double pawn push
            if (((RANK_OF(from) == 1) && side == WHITE) || ((RANK_OF(from) == 6) && side == BLACK)) {
                to = from + (2 * PAWN_PUSH[side]);
                if (is_empty(b, to)) {
                    move = encode_move(from, to, MV_DOUBLE_PUSH);
                    movelist_add(list, move);
                }
            }
        }
        

        // captura
        for (int i = 0; i < 2; i++) {
            to = PAWN_ATTACKS[side][from][i];
            if (to == SQ_NONE) continue;

            piece = b->array[to];
            if (is_enemy(piece, side)) {
                // promoção do peão com captura
                if (check_pawn_promotion(to)) {
                    for (int t = MV_PROMO_CAP_N; t <= MV_PROMO_CAP_Q; t++) {
                        move = encode_move(from, to, (MoveType)t);
                        movelist_add(list, move);
                    }
                } else {
                    move = encode_move(from, to, MV_CAPTURE);
                    movelist_add(list, move);
                }
            }
            // captura en passant
            if (b->ep_square == to) {
                move = encode_move(from, to, MV_EP_CAPTURE);
                movelist_add(list, move);
            }
        }
    }
}


void genenare_moves_from_direction(const Board *b, MoveList *list, int from, int dir) {
    int dist_to_edge = SQ_TO_EDGE[from][dir];
    int to = from;
    Move move;

    for (;dist_to_edge > 0; dist_to_edge--) {
        to += DIR_OFFSET[dir];

        if (b->array[to] == EMPTY) {
            move = encode_move(from, to, MV_QUIET);
            movelist_add(list, move);
        } else {
            Piece p = b->array[to];
            if (PIECE_COLOR(p) != b->side_to_move) {
                move = encode_move(from, to, MV_CAPTURE);
                movelist_add(list, move);
            }
            break;
        }
    }
}



void generate_sliding_moves(const Board *b, MoveList *list) {
    Piece piece;

    for (int from = 0; from < BOARD_SIZE; from++) {
        piece = b->array[from];
        if (!is_own(piece, b->side_to_move)) continue;

        if (PIECE_TYPE(piece) == ROOK) {
            for (int dir = 0; dir < 4; dir++) {
                genenare_moves_from_direction(b, list, from, dir);
            }
        }
        else if (PIECE_TYPE(piece) == BISHOP) {
            for (int dir = 4; dir < 8; dir++) {
                genenare_moves_from_direction(b, list, from, dir);
            }
        }
        else if (PIECE_TYPE(piece) == QUEEN) {
            for (int dir = 0; dir < 8; dir++) {
                genenare_moves_from_direction(b, list, from, dir);
            }
        }
    }
}

/* 
 * Verifica se as casas entre o rei e as torres estão vazias, e retorna os roques permitidos
 */
static bool check_castle_side(const Board *b, Direction dir, int king_sq) {
    int distance = SQ_TO_EDGE[king_sq][dir];
    if (distance == 0) {
        return false;
    }

    Piece p;
    int sq;
    int i;
    for (i = 1; i < distance; i++) {
        sq = king_sq + (i * DIR_OFFSET[dir]);
        p = b->array[sq];
        if (!is_empty(b, sq)) return false;
    }
    
    sq = king_sq + (i * DIR_OFFSET[dir]);
    p = b->array[sq];
    if (is_own(p, b->side_to_move) && PIECE_TYPE(p) == ROOK) return true;

    return false;
}

u8 check_allowed_castles(const Board *b) {
    Color side = b->side_to_move;

    int king_sq = board_find_king(b, side);
    assert(king_sq != SQ_NONE);

    static const enum {LEFT, RIGHT};

    u8 castle[2][2];
    castle[WHITE][LEFT] = CASTLE_WQ;
    castle[WHITE][RIGHT] = CASTLE_WK;
    castle[BLACK][LEFT] = CASTLE_BQ;
    castle[BLACK][RIGHT] = CASTLE_BK;

    u8 free_castle = CASTLE_NONE;

    if (check_castle_side(b, DIR_W, king_sq)) {
        free_castle |= castle[side][LEFT];
    }
    if (check_castle_side(b, DIR_E, king_sq)) {
        free_castle |= castle[side][RIGHT];
    }

    return free_castle;
}


void generate_king_moves(const Board *b, MoveList *list) {
    Color side = b->side_to_move;
    int from = b->king_square[b->side_to_move];
    int to;
    Move move;

    // loop das direções do rei
    for (int dir = 0; dir < NUM_DIRS; dir++) {
        to = from + DIR_OFFSET[dir];

        if (SQ_OFFBOARD(to)) continue;
        if (SQ_TO_EDGE[from][dir] == 0) continue;


        if (is_empty(b, to)) {
            move = encode_move(from, to, MV_QUIET);
            movelist_add(list, move);
            continue;
        }
        if (is_enemy(b->array[to], side)) {
            move = encode_move(from, to, MV_CAPTURE);
            movelist_add(list, move);
        }
    }

    // gera lances de roque
    u8 castles = check_allowed_castles(b);
    if (side == WHITE) {
        if ((b->castling_rights & CASTLE_WK) && (castles & CASTLE_WK)) {
            move = encode_move(from, SQ_G1, MV_CASTLE_KING);
            movelist_add(list, move);
        }
        if ((b->castling_rights & CASTLE_WQ) && (castles & CASTLE_WQ)) {
            move = encode_move(from, SQ_C1, MV_CASTLE_QUEEN);
            movelist_add(list, move);
        }
    }
    else if (side == BLACK) {
        if ((b->castling_rights & CASTLE_BK) && (castles & CASTLE_BK)) {
            move = encode_move(from, SQ_G8, MV_CASTLE_KING);
            movelist_add(list, move);
        }
        if ((b->castling_rights & CASTLE_BQ) && (castles & CASTLE_BQ)) {
            move = encode_move(from, SQ_C8, MV_CASTLE_QUEEN);
            movelist_add(list, move);
        }
    }
}


void generate_all_moves(Board *b, MoveList *list) {
    movelist_clear(list);
    generate_pawn_moves(b, list);
    generate_sliding_moves(b, list);
    generate_king_moves(b, list);
}
