#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "log.h"



bool parse_fen(const char *fen_string, Board *out);
void board_to_fen(Board *board, char fen_out[MAX_FEN_STRING]);

int sq_from_coord(const char *coord);
void coord_from_sq(int sq, char out[3]);

extern const char PIECE_CHAR[17];

#endif

