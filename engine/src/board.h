#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "piece.h"

enum CastleRights{
    CASTLE_NONE = 0,
    CASTLE_WK = 1,
    CASTLE_WQ = 2,
    CASTLE_BK = 4,
    CASTLE_BQ = 8,
    CASTLE_WHITE = CASTLE_WK | CASTLE_WQ,
    CASTLE_BLACK = CASTLE_BK | CASTLE_BQ,
    CASTLE_ALL = CASTLE_WHITE | CASTLE_BLACK
};



typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int king_square[2];
    
    u8 castling_rights;    /* bitmask CASTLE_* */
    int ep_square;         /* en passant: SQ_NONE se não houver */
    int halfmove_clock;   
    int fullmove_number;
} Board;

int  board_find_king(const Board *b, Color c);
bool board_check_invariants(const Board *b, const char **fail_msgs);

void board_clear(Board *b);
void board_print(const Board *board);

bool fill_sq(Board *b, const char *sq_str, Piece p);
bool is_empty(const Board *b, int sq);

#endif

