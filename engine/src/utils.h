#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "log.h"
#include "board.h"

extern const char *COLOR_CHAR[2];

void print_piece_chart(void);
void get_fen(char fen[MAX_FEN_STRING]);
int get_int(const char *msg);

void clear_screen(void);
void strslc(const char *src, char *dest, int start, int end);


#endif