#ifndef IA_H
#define IA_H

// includes da máquina de regras
#include "board.h"
#include "move.h"

// include para a estrutura
#include <stdint.h>

// Constantes de compilação
#define VARIANTE_MINIMAX 0
#define VARIANTE_ALFA_BETA 1

#define SORTEIO_LIGADO 1
#define SORTEIO_DESLIGADO 0

#define TIPO_INVALIDO 0
#define TIPO_CENTIPEAO 1
#define TIPO_MATE -1

#define LANCE_NULO 0

typedef struct Play{
    int32_t score;
    Move lance;
    int8_t tipo;
} Play;

void escolherJogada(Board *b, int variante, int randomizador); 
/*
Pré-condições, garantidas pelo chamador:
    variante: VARIANTE_MINIMAX ou VARIANTE_ALFA_BETA
    randomizador: SORTEIO_LIGADO ou SORTEIO_DESLIGADO
    a raiz (b) não é posição terminal

Após a chamada:
    os valores de JOGADA são válidos apenas após o término da chamada
    o chamador deve conferir se JOGADA.lance != LANCE_NULO antes de usar
    o lance; LANCE_NULO indica que não houve jogada válida (só ocorre se
    a pré-condição da raiz for violada)
*/

extern Play JOGADA;

#endif
