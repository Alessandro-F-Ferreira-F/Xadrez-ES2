#ifndef DRAW_H
#define DRAW_H

#include "board.h"

typedef enum {
    DRAW_NONE = 0,
    DRAW_50,
    DRAW_MATERIAL,
    DRAW_REPETITION
} DrawReason;

/* Consulta barata, sem gerar lances ou alterar b. Nao determina mate/afogamento;
   o chamador verifica esses estados antes (mate prevalece sobre 50 lances).
   Repeticao exige historico e e consultada somente pela camada de protocolo. */
DrawReason draw_reason(const Board *b);

#endif /* DRAW_H */
