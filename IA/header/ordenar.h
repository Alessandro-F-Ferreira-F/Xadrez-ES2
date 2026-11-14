#ifndef ORDENAR_H
#define ORDENAR_H

#include "board.h"
#include "move.h"
#include <stdint.h>

// Lance com a nota de ordenação (MVV-LVA). Local ao nó: nota em int16_t deixa o
// elemento com 4 bytes (vetor de MAX_MOVES com 1 KB por nível de pilha)
typedef struct LanceOrdenado{
    Move lance;
    int16_t nota;
} LanceOrdenado;

// Nota do lance que deve vir primeiro: acima da maior nota MVV-LVA (39, dama capturada por peão)
#define NOTA_PRIMEIRO 64

void ordenarLances(const Board *board, const MoveList *lista, Move primeiro, LanceOrdenado ordenados[]);
/*
Copia os lista->count lances de lista para ordenados[0..lista->count - 1], em ordem
decrescente de nota MVV-LVA (vítima mais valiosa, atacante menos valioso):
    primeiro: nota = NOTA_PRIMEIRO (vem antes de todos)
    captura: nota = 8 * tipo(vítima) - tipo(atacante), sempre > 0
        en passant: vítima é peão; promoção com captura: atacante é peão
    demais lances (inclusive promoção sem captura): nota = 0
Ordenação estável: lances de mesma nota ficam na ordem de lista (a do gerador).
primeiro == MOVE_NONE (a1->a1, nunca legal): nenhum lance recebe NOTA_PRIMEIRO.

Pré-condição:
    lista gerada para board (os lances são de board, ainda não feitos)
    ordenados tem espaço para lista->count elementos (MAX_MOVES basta)
*/

#endif
