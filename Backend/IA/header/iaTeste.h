#ifndef IA_TESTE_H
#define IA_TESTE_H

#include "board.h"
#include <stdbool.h>

// Funções internas da IA expostas só para os testes; o motor usa apenas ia.h

bool lanceGarantido(const Board *board);
/*
Prova barata de que o lado da vez tem ao menos um lance legal, sem gerar a lista.
Pré-condição: o lado da vez não está em xeque.
Devolve true só se há lance legal (condição suficiente); false não quer dizer
afogamento, só que a prova falhou e é preciso gerar os lances.
*/

void ligarDeltaPruning(bool ligado);
/*
Liga ou desliga o delta pruning da quiescência com poda (ligado por padrão).
O teste v1 = v2 com a quiescência desliga antes e religa depois: com o delta,
quiescenciaAlfaBeta deixa de ter o mesmo valor que quiescenciaMinimax.
*/

#endif
