#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "types.h"

u32 encode_move(int origin_sq, int target_sq, int promo, int flags);
MoveDescription decode_move(u32 move);

void precompute_move_data(void);
// void generate_pawn_moves(Board *board, MoveList *list);
void generate_pawn_moves(const Board *board, MoveList *list, Color side);
void generate_sliding_moves(const Board *board, MoveList *list);

void add_move(u32 move, MoveList *list);
bool find_move(const Board *board, int origin_sq, int target_sq, int promo, u32 *out);
void make_move(Board *b, u32 move);
void unmake_move(Board *b);

void print_moves(MoveList *list);
void print_square_directions(int sq);
u32 str_to_move(char move_in[6]);


#endif