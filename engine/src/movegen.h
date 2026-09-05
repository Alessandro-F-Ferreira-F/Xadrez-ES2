#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "types.h"
#include "utils.h"


void precompute_move_data();
void add_move(u32 move, MoveList *list);
void generate_pawn_moves(Board *board, MoveList *list);
void print_moves(MoveList *list);
void print_square_directions(char sq_str[3]);
bool find_move(Board *board, int origin_sq, int target_sq, int promo, u32 *out);
u32 encode_move(int origin_sq, int target_sq, int promo, int flags);
MoveDescription decode_move(u32 move);
u32 str_to_move(char move_in[5]);
void make_move(Board *b, u32 move);

#endif