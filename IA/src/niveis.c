#include "niveis.h"
#include "ia.h"

// Prazos abaixo dos tetos do RNF01: o relógio passou do prazo em até 6 ms nas medições
#define TEMPO_BASICO_MS 180
#define TEMPO_DIFICIL_MS 950

#define PROF_FACIL 2
#define PROF_MEDIO 3

// Prazo 0 seria TEMPO_SEM_LIMITE: o relógio ficaria desligado
_Static_assert(TEMPO_BASICO_MS > TEMPO_SEM_LIMITE && TEMPO_BASICO_MS < RESPOSTA_MAX_BASICO_MS, "prazo dos níveis básicos fora do RNF01");
_Static_assert(TEMPO_DIFICIL_MS > TEMPO_SEM_LIMITE && TEMPO_DIFICIL_MS < RESPOSTA_MAX_DIFICIL_MS, "prazo do nível difícil fora do RNF01");
_Static_assert(PROF_FACIL >= 1 && PROF_FACIL <= PROF_MAX && PROF_MEDIO >= 1 && PROF_MEDIO <= PROF_MAX, "profundidade de nível fora de [1, PROF_MAX]");

// Indexada por nivel - NIVEL_FACIL
static const ParametrosNivel tabelaNiveis[] = {
    {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, PROF_FACIL, TEMPO_BASICO_MS},   // NIVEL_FACIL
    {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_LIGADA,    PROF_MEDIO, TEMPO_BASICO_MS},   // NIVEL_MEDIO
    {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_LIGADA,    PROF_MAX,   TEMPO_DIFICIL_MS},  // NIVEL_DIFICIL
};

_Static_assert(sizeof(tabelaNiveis)/sizeof(tabelaNiveis[0]) == NIVEL_DIFICIL - NIVEL_FACIL + 1, "tabelaNiveis sem um nível por linha");

bool parametrosNivel(int nivel, ParametrosNivel *p){
    if(nivel < NIVEL_FACIL || nivel > NIVEL_DIFICIL) return false;
    *p = tabelaNiveis[nivel - NIVEL_FACIL];
    return true;
}
