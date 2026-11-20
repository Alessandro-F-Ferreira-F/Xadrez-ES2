#ifndef ATTACK_H
#define ATTACK_H

#include "board.h"

/* defender e o DONO da casa, nao o atacante. Requer init_square_tables(). */
bool is_square_attacked(const Board *b, const int sq, const Color defender);
/* Consulta o rei do lado a jogar. Nao gera lances nem modifica o tabuleiro. */
bool in_check(const Board *b);

#endif /* ATTACK_H */
