#ifndef AVALIAR_H
#define AVALIAR_H

#include "board.h"
#include <stdint.h>

// Maior valor, em módulo, que avaliar pode retornar. Margem adotada para a
// v6+ (material + PSTs de Michniewski, pior caso); a v1 só material chega a 10400
#define AVALIACAO_MAX 10615

// Valor material de cada peça, indexado por PIECE_TYPE (EMPTY e KING valem 0). Usado por avaliar
// e pelo delta pruning da quiescência
extern const int32_t VALOR_PECA[7];

int32_t avaliar(const Board *b);

#endif
