#ifndef FEN_H
#define FEN_H

#include "types.h"
#include "board.h"

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


bool fen_parse(const char *fen_string, Board *out);
void fen_write(const Board *board, char fen_out[MAX_FEN_STRING]);

#endif /* FEN_H */