#ifndef MAKEMOVE_H
#define MAKEMOVE_H

#include "types.h"
#include "piece.h"
#include "board.h"
#include "move.h"

typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move(Board *b, Move m, Undo *u);
void unmake_move(Board *b, Move move, const Undo *u);

#endif /* MAKEMOVE_H */