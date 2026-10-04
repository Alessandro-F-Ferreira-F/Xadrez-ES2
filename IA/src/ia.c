#include "ia.h"
#include "avaliar.h"
#include <stdlib.h>

// Separação entre avaliação e mate: nenhum valor de avaliar pode ser
// classificado como mate. Falha se PROF_MAX >= MATE - AVALIACAO_MAX (9384)
_Static_assert(LIMIAR_MATE > AVALIACAO_MAX, "PROF_MAX grande demais: LIMIAR_MATE invade a faixa da avaliacao");

Play JOGADA;

void converterScore(int32_t scoreBruto, Color lado, Play *p){
    int32_t score = scoreBruto;

    if(lado == BLACK){
        score *= -1;
    }
    if(LIMIAR_MATE <= abs(score)){
        p->tipo = TIPO_MATE;
        int32_t sinalMate;

        if(score > 0){
            sinalMate = 1;
        }
        else{
            sinalMate = -1;
        }

        p->score = sinalMate * ((MATE - abs(score) + 1)/2);
    }
    else{
        p->tipo = TIPO_CENTIPEAO;
        p->score = score;
    }
}
