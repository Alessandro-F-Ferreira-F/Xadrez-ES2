#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "log.h"
#include "board.h"

void print_piece_chart();
void print_board(const Board *board);
void get_fen(char fen[MAX_FEN_STRING]);
int get_int(char msg[INPUT_STR_SIZE]);
void print_square_directions(int rank, int file);



#endif