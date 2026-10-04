#ifndef IA_H
#define IA_H

// includes da máquina de regras
#include "board.h"
#include "move.h"

// include para a estrutura
#include <stdint.h>

// Constantes de compilação
#define PROF_MAX 9000
#define VARIANTE_MINIMAX 0
#define VARIANTE_ALFA_BETA 1

#define SORTEIO_LIGADO 1
#define SORTEIO_DESLIGADO 0

#define TIPO_INVALIDO 0
#define TIPO_CENTIPEAO 1
#define TIPO_MATE (-1)

#define MATE 19999
#define INFINITO 20000
#define SCORE_INVALIDO 30000
#define LIMIAR_MATE (MATE - PROF_MAX)

#define LANCE_NULO 0

// Resultado da chamada de escolherJogada (JOGADA.resultado)
// 0: sucesso; negativos: bug; positivos: partida terminada na raiz
// Valores de empate provisórios, a definir com a máquina de regras

#define RESULTADO_ERRO_INTERNO (-1)
#define RESULTADO_VARIANTE_INVALIDA (-2)
#define RESULTADO_RANDOMIZADOR_INVALIDO (-3)
#define RESULTADO_SUCESSO 0
#define RESULTADO_RAIZ_MATE 1
#define RESULTADO_RAIZ_AFOGAMENTO 2
#define RESULTADO_EMPATE_50_LANCES 3
#define RESULTADO_EMPATE_REPETICAO 4
#define RESULTADO_EMPATE_MATERIAL 5

typedef struct Play{
    int32_t score;
    Move lance;
    int8_t tipo;
    int8_t resultado;
} Play;

void converterScore(int32_t scoreBruto, Color lado, Play *p);
/*
Converte o score devolvido pela busca (convenção interna: brancas positivas)
para o formato do UCI e preenche p->score e p->tipo. Não altera p->lance
nem p->resultado.

Pré-condição:
    scoreBruto é o valor devolvido pela busca para uma raiz válida
    (estritamente entre -INFINITO e +INFINITO); nunca SCORE_INVALIDO

Após a chamada:
    p->score está na perspectiva de quem está na vez (lado)
    p->tipo == TIPO_MATE: p->score é o número de lances completos até o
    mate (positivo: lado vence; negativo: lado leva mate)
    p->tipo == TIPO_CENTIPEAO: p->score em centipeões
*/

void escolherJogada(Board *b, int variante, int randomizador);
/*
Escolhe um lance para o lado da vez em b e grava o resultado em JOGADA.
Variante, randomizador e raiz são verificados internamente; não há
pré-condição a cargo do chamador.

Após a chamada:
    os valores de JOGADA são válidos apenas após o término da chamada
    o chamador deve conferir JOGADA.resultado antes de usar os demais campos:
        RESULTADO_SUCESSO: lance, score e tipo válidos
        negativo: bug (parâmetro inválido ou erro interno)
        positivo: partida já terminada na raiz (mate, afogamento ou empate
        por regra); não há lance a jogar
    se JOGADA.resultado != RESULTADO_SUCESSO, os demais campos ficam nos
    valores padrão (LANCE_NULO, SCORE_INVALIDO, TIPO_INVALIDO)
*/

extern Play JOGADA;

#endif
