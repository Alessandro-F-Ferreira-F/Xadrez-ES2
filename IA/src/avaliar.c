#include "avaliar.h"
#include "piece.h"

static const int32_t tabelaValores[7] = {0, 100, 320, 330, 500, 900, 0};

int32_t avaliar(const Board *b){
    int32_t totBrancas = 0, totPretas = 0;
    Piece aux;
    
    for(int i = 0; i <= 63; i ++){
        aux = b->array[i];
        if(aux != EMPTY){
            int32_t tabValue = tabelaValores[PIECE_TYPE(aux)];
            if(PIECE_COLOR(aux) == WHITE){
                totBrancas += tabValue;
            }
            else{
                totPretas += tabValue;
            }
        }
    }

    return totBrancas - totPretas;
}
