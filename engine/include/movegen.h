#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "types.h"
#include "move.h"

void generate_pawn_moves(const Board *board, MoveList *list);
void generate_sliding_moves(const Board *board, MoveList *list);
void generate_king_moves(const Board *b, MoveList *list);
void generate_all_moves(Board *b, MoveList *list);

#endif