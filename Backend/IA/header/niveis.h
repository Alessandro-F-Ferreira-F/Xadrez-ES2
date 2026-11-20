#ifndef NIVEIS_H
#define NIVEIS_H

#include <stdbool.h>

// Níveis de dificuldade (RF03)
#define NIVEL_FACIL 1
#define NIVEL_MEDIO 2
#define NIVEL_DIFICIL 3

// Tetos do tempo de resposta da IA (RNF01), em ms: a resposta deve ser inferior a eles
#define RESPOSTA_MAX_BASICO_MS 200
#define RESPOSTA_MAX_DIFICIL_MS 1000

// Parâmetros de escolherJogada (ver ia.h) para um nível
typedef struct ParametrosNivel{
    int variante;
    int randomizador;
    int quiescencia;
    int profundidade;
    int tempoMs;
} ParametrosNivel;

bool parametrosNivel(int nivel, ParametrosNivel *p);
/*
Preenche *p com os parâmetros de escolherJogada do nível:
    NIVEL_FACIL:   alfa-beta, sorteio, sem quiescência, profundidade 2,        180 ms
    NIVEL_MEDIO:   alfa-beta, sorteio, com quiescência, profundidade 3,        180 ms
    NIVEL_DIFICIL: alfa-beta, sorteio, com quiescência, profundidade PROF_MAX, 950 ms
Os prazos ficam abaixo dos tetos do RNF01 (RESPOSTA_MAX_*_MS), com folga para o
relógio passar um pouco do prazo. Os níveis só pensam menos: não erram de propósito.

Pré-condição:
    p aponta para um objeto ParametrosNivel

Após a chamada:
    nivel entre NIVEL_FACIL e NIVEL_DIFICIL: devolve true e preenche *p
    senão: devolve false e não altera *p
*/

#endif
