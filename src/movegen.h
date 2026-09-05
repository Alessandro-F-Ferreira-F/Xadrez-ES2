#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"


void precompute_move_data();
void add_move(u32 move, MoveList *list);
void generate_pawn_moves(Board *board, MoveList *list);
void print_moves(MoveList *list);


#endif