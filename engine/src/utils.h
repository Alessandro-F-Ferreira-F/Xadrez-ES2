#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "log.h"
#include "board.h"

void print_piece_chart();
void print_board(const Board *board);
void get_fen(char fen[MAX_FEN_STRING]);
int get_int(char msg[INPUT_STR_SIZE]);
void strslc(const char *src, char *dest, int start, int end);

static void clear_screen() {
    printf("\e[1;1H\e[2J");
    fflush(stdout); 
}

#endif