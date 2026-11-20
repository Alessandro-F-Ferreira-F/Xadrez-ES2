#include "pendenciasRegras.h"
#include "draw.h"

/* Adaptador para preservar o contrato da IA e os testes isolados com stubs.
   O build do motor liga este arquivo, nunca stubs.c. */
int testeEmpateRegra(const Board *board) {
    switch (draw_reason(board)) {
        case DRAW_50: return EMPATE_50_LANCES;
        case DRAW_MATERIAL: return EMPATE_MATERIAL;
        case DRAW_REPETITION: return EMPATE_REPETICAO;
        case DRAW_NONE: return SEM_EMPATE;
    }
    return SEM_EMPATE;
}
