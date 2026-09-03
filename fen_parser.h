#ifndef CHESS_ENGINE_FEN_PARSER_H
#define CHESS_ENGINE_FEN_PARSER_H

#include "types.h"
#include "log.h"

#define MAX_FEN_STRING 256

bool parse_fen(const char *fen_string, Board *out);
char *board_to_fen(Piece *board);

extern const char PIECE_CHAR[17];

#endif /* CHESS_ENGINE_FEN_PARSER_H */
