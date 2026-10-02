#include "avaliar.h"
#include "piece.h"

static const int32_t tabelaValores[7] = {0, 100, 320, 330, 500, 900, 0};

int32_t avaliar(const Board *b){
    int32_t totBrancas = 0, totPretas = 0, aux;
    
    for(int i = 0; i <= 63; i ++){
        aux = b->array[i];
        if(aux != EMPTY){
            int32_t tabValue = tabelaValores[PIECE_TYPE(aux)];
            if(PIECE_COLOR(aux == WHITE)){
                totBrancas += tabValue;
            }
            else{
                totPretas += tabValue;
            }
        }
    }

    return totBrancas - totPretas
}

/*
função avaliar(estado):
    totBrancas = 0
    totPretas = 0
    para toda casa = 0...63:
        se estado.array[casa] != EMPTY então
            se PIECE_COLOR(estado.array[casa]) == WHITE então
                totBrancas += tabelaValores[PIECE_TYPE(estado.array[casa])]
            senão totPretas += tabelaValores[PIECE_TYPE(estado.array[casa])]

    retorna totBrancas - totPretas
*/
