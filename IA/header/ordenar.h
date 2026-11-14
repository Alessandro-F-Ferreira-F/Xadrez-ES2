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

void ordenarLances(const Board *board, const MoveList *lista, LanceOrdenado ordenados[]);
/*
Copia os lista->count lances de lista para ordenados[0..lista->count - 1], em ordem
decrescente de nota MVV-LVA (vítima mais valiosa, atacante menos valioso):
    captura: nota = 8 * tipo(vítima) - tipo(atacante), sempre > 0
        en passant: vítima é peão; promoção com captura: atacante é peão
    demais lances (inclusive promoção sem captura): nota = 0
Ordenação estável: lances de mesma nota ficam na ordem de lista (a do gerador).

Pré-condição:
    lista gerada para board (os lances são de board, ainda não feitos)
    ordenados tem espaço para lista->count elementos (MAX_MOVES basta)
*/

#endif
