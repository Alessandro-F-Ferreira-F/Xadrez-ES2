#include "movegen.h"
#include "utils.h"

#include "move.h"
#include "piece.h"
#include "square.h"

static inline bool is_empty(int sq, const Board *b) {return b->array[sq] == EMPTY; }

bool check_pawn_promotion(int sq) {
    if ((RANK_OF(sq) == 0) || (RANK_OF(sq) == 7)) return true;
    return false;
}

void generate_pawn_moves(const Board *board, MoveList *list, Color side) {
    Piece piece;
    int target_sq;
    Move move;


    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        piece = board->array[sq];

        if (PIECE_TYPE(piece) != PAWN) continue;
        if (PIECE_COLOR(piece) != side) continue;

        // push padrão do peão
        target_sq = sq + PAWN_PUSH[side];
        if ((SQ_OFFBOARD(target_sq) == 0) && is_empty(target_sq, board)) {
            move = encode_move(sq, target_sq, MV_QUIET);
            if (check_pawn_promotion(target_sq)) {
                move = encode_move(sq, target_sq, MV_PROMO_Q); 
            }
            movelist_add(list, move);

            // double pawn push
            if (((RANK_OF(sq) == 1) && side == WHITE) || ((RANK_OF(sq) == 6) && side == BLACK)) {
                target_sq = sq + (2 * PAWN_PUSH[side]);
                move = encode_move(sq, target_sq, MV_DOUBLE_PUSH);
                movelist_add(list, move);
            }
        }
        

        // captura
        for (int i = 0; i < 2; i++) {
            target_sq = PAWN_ATTACKS[side][sq][i];
            piece = board->array[target_sq];
            
            if ((target_sq != SQ_NONE) && (board->array[target_sq] != EMPTY) && (PIECE_COLOR(piece) != side)) {
                move = encode_move(sq, target_sq, MV_CAPTURE);
                if (check_pawn_promotion(target_sq)) {
                    move = encode_move(sq, target_sq, MV_PROMO_CAP_Q);
                }
                movelist_add(list, move);
            }
        }
    }
}


void genenare_moves_from_direction(const Board *board, MoveList *list, int sq, int dir) {
    int dist_to_edge = SQ_TO_EDGE[sq][dir];
    int target_sq = sq;
    Move move;

    for (;dist_to_edge > 0; dist_to_edge--) {
        target_sq += DIR_OFFSET[dir];

        if (board->array[target_sq] == EMPTY) {
            move = encode_move(sq, target_sq, MV_QUIET);
            movelist_add(list, move);
        } else {
            Piece p = board->array[target_sq];
            if (PIECE_COLOR(p) != board->side_to_move) {
                move = encode_move(sq, target_sq, MV_CAPTURE);
                movelist_add(list, move);
            }
            break;
        }
    }
}



void generate_sliding_moves(const Board *board, MoveList *list) {
    Piece piece;

    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        piece = board->array[sq];
        if (!is_own(piece, board->side_to_move)) continue;

        if (PIECE_TYPE(piece) == ROOK) {
            for (int dir = 0; dir < 4; dir++) {
                genenare_moves_from_direction(board, list, sq, dir);
            }
        }
        else if (PIECE_TYPE(piece) == BISHOP) {
            for (int dir = 4; dir < 8; dir++) {
                genenare_moves_from_direction(board, list, sq, dir);
            }
        }
        else if (PIECE_TYPE(piece) == QUEEN) {
            for (int dir = 0; dir < 8; dir++) {
                genenare_moves_from_direction(board, list, sq, dir);
            }
        }
    }
}


void generate_all_moves(Board *board, MoveList *list);

