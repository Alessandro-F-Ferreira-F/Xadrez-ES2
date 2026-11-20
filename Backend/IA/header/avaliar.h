#ifndef AVALIAR_H
#define AVALIAR_H

#include "board.h"
#include <stdint.h>

// Maior valor, em módulo, que avaliar pode retornar: material + PSTs de Michniewski, pior caso
// (um lado só com o rei, o outro com nove damas, duas torres, dois cavalos e dois bispos, todos nas
// melhores casas, e os reis nos extremos das tabelas). A interpolação do rei nunca passa do maior
// valor das duas tabelas
#define AVALIACAO_MAX 10615

// Valor material de cada peça, indexado por PIECE_TYPE (EMPTY e KING valem 0). Usado por avaliar
// e pelo delta pruning da quiescência
extern const int32_t VALOR_PECA[7];

int32_t avaliar(const Board *b);

#endif
