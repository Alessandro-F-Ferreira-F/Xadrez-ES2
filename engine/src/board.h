#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "log.h"



bool parse_fen(const char *fen_string, Board *out);
void board_to_fen(const Board *board, char fen_out[MAX_FEN_STRING]);

int  sq_from_coord(const char *coord);
void coord_from_sq(int sq, char out[3]);

/* Declarada aqui, e nao em utils.h, porque e uma operacao sobre Board e e
   definida em board.c. Manter declaracao e definicao no mesmo modulo e o que
   transforma o header em contrato verificado pelo compilador. */
void print_board(const Board *board);

extern const char PIECE_CHAR[17];

#endif

