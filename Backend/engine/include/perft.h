#ifndef PERFT_H
#define PERFT_H

#include "types.h"
#include "board.h"

/*
 * Conta os nos folha a 'depth' meios-lances da posicao de 'board', por lances
 * LEGAIS (generate_legal_moves). depth == 0 devolve 1.
 *
 * 'board' e modificado durante a contagem (make_move/unmake_move) e devolvido
 * igual ao que entrou -- desde que unmake_move seja o inverso exato de
 * make_move, que e justamente o que o perft das seis posicoes da CPW prova.
 * Quem nao quer correr esse risco passa uma copia.
 */
u64 perft(Board *board, int depth);

#endif /* PERFT_H */
