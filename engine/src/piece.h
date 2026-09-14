#ifndef PIECE_H
#define PIECE_H

#include "types.h"

typedef enum {
    WHITE = 1,
    BLACK = 0
} Color;

typedef enum {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
} PieceType;

typedef u8 Piece;

/* 
    0 0 0 0 [1] [0 0 0]
*/

#define PIECE_MASK 0xFu
#define PIECE_TYPE_MASK 0x7u
#define PIECE_COLOR_MASK 0x1u

#define PIECE_MAKE(color, type) ((Piece)((((color) & 1u) << 3) | ((type) & 7u)))
#define PIECE_TYPE(p) ((PieceType)((p) & PIECE_TYPE_MASK))
#define PIECE_COLOR(p) ((Color)(((unsigned)(p) >> 3) & PIECE_COLOR_MASK))

#define NO_PIECE ((Piece)0)

static inline bool is_own(Piece p, Color c)   { return p != EMPTY && PIECE_COLOR(p) == c; }
static inline bool is_enemy(Piece p, Color c) { return p != EMPTY && PIECE_COLOR(p) != c; }



char piece_to_char(Piece p);
Piece piece_from_char(char c);



#endif /* PIECE_H */