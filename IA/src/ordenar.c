#include "ordenar.h"

// O lance primeiro precisa vir antes de qualquer captura (maior nota MVV-LVA: dama capturada por peão)
_Static_assert(NOTA_PRIMEIRO > 8 * QUEEN - PAWN, "NOTA_PRIMEIRO nao supera a maior nota MVV-LVA");

// Nota MVV-LVA: a vítima domina (8 * vítima >= 8 > 6 >= atacante), o atacante desempata
static int16_t notaMVVLVA(const Board *board, Move lance){
    if(!move_is_capture(lance)) return 0;

    // En passant: a casa de destino está vazia, a vítima é o peão capturado
    int vitima = move_is_ep_capture(lance) ? PAWN : PIECE_TYPE(board->array[move_to(lance)]);
    int atacante = PIECE_TYPE(board->array[move_from(lance)]);

    return (int16_t)(8 * vitima - atacante);
}

// Inserção estável: só passa à frente de nota estritamente menor
void ordenarLances(const Board *board, const MoveList *lista, Move primeiro, LanceOrdenado ordenados[]){
    for(int i = 0; i < lista->count; i ++){
        LanceOrdenado atual;
        atual.lance = lista->moves[i];
        if(atual.lance == primeiro) atual.nota = NOTA_PRIMEIRO;
        else atual.nota = notaMVVLVA(board, atual.lance);

        int j = i;
        while(j > 0 && ordenados[j - 1].nota < atual.nota){
            ordenados[j] = ordenados[j - 1];
            j --;
        }
        ordenados[j] = atual;
    }
}
