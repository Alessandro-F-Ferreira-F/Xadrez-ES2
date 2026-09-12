#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "log.h"
#include "board.h"

extern const char *COLOR_CHAR[2];

/*
 * '(void)' e nao '()': em C, '()' significa "argumentos nao especificados",
 * o que desliga a checagem de tipos na chamada e dispara -Wstrict-prototypes.
 */
void print_piece_chart(void);
void get_fen(char fen[MAX_FEN_STRING]);
void strslc(const char *src, char *dest, int start, int end);
void clear_screen(void);

/*
 * 'const char *msg' e nao 'char msg[INPUT_STR_SIZE]': a forma com colchetes
 * promete ao compilador um vetor de 128 bytes, entao passar um literal menor
 * ("Insert option: ", 16 bytes) fazia o GCC acusar -Wstringop-overflow.
 * A funcao so le a string, logo o tipo honesto e ponteiro para const.
 */
int get_int(const char *msg);

#endif