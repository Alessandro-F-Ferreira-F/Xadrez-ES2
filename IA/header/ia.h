#ifndef IA_H
#define IA_H

// includes da máquina de regras
#include "board.h"
#include "move.h"

// include para a estrutura
#include <stdint.h>
#include <stdbool.h>

// Constantes de compilação
// PROF_MAX: teto do parâmetro profundidade de escolherJogada (go depth); não é
// a profundidade usada na busca
#define PROF_MAX 64
// PROF_LIMITE: teto absoluto de profundidade de qualquer nó, inclusive além do
// limite da iteração (quiescência); define a classificação de mate (LIMIAR_MATE)
#define PROF_LIMITE (2 * PROF_MAX)
#define TEMPO_SEM_LIMITE 0
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
#define LIMIAR_MATE (MATE - PROF_LIMITE)

// Resultado da chamada de escolherJogada (JOGADA.resultado)
// 0: sucesso; negativos: bug; positivos: partida terminada na raiz
// Valores de empate provisórios, a definir com a máquina de regras
// RESULTADO_EMPATE_PROVISORIO é temporário para testes com stub, até todas as regras de empate serem implementadas

#define RESULTADO_ERRO_INTERNO (-1)
#define RESULTADO_VARIANTE_INVALIDA (-2)
#define RESULTADO_RANDOMIZADOR_INVALIDO (-3)
#define RESULTADO_PROFUNDIDADE_INVALIDA (-4)
#define RESULTADO_TEMPO_INVALIDO (-5)
#define RESULTADO_SUCESSO 0
#define RESULTADO_RAIZ_MATE 1
#define RESULTADO_RAIZ_AFOGAMENTO 2
#define RESULTADO_EMPATE_PROVISORIO 6
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

void escolherJogada(Board *board, int variante, int randomizador, int profundidade, int tempoMs);
/*
Escolhe um lance para o lado da vez em board e grava o resultado em JOGADA.
Variante, randomizador, profundidade, tempoMs e raiz são verificados
internamente; não há pré-condição a cargo do chamador.

Aprofundamento iterativo: busca com limite 1, 2, ..., profundidade
(go depth), parando antes se tempoMs (go movetime) se esgotar ou se
uma iteração completa achar mate (para qualquer lado).
    profundidade: 1 <= profundidade <= PROF_MAX
    tempoMs: em milissegundos, >= 0; TEMPO_SEM_LIMITE (0) desliga o relógio
    se clock() falhar, o prazo conta como passado: só a iteração 1 roda
Iteração interrompida pelo tempo: no alfa-beta, se ao menos o primeiro filho
da raiz (o melhor lance da iteração anterior) completou, lance e score vêm dos
filhos que completaram (parcial; empates entre os filhos não buscados ficam de
fora); senão, e sempre no minimax, ela é descartada e lance e score vêm da
última iteração completa. A iteração 1 sempre termina, mesmo com o tempo
esgotado, para que haja lance.

Após a chamada:
    os valores de JOGADA são válidos apenas após o término da chamada
    o chamador deve conferir JOGADA.resultado antes de usar os demais campos:
        RESULTADO_SUCESSO: lance, score e tipo válidos
        negativo: bug (parâmetro inválido ou erro interno)
        positivo: partida já terminada na raiz (mate, afogamento ou empate
        por regra); não há lance a jogar
    se JOGADA.resultado != RESULTADO_SUCESSO, os demais campos ficam nos
    valores padrão (MOVE_NONE, SCORE_INVALIDO, TIPO_INVALIDO)
*/

void iniciarSorteio(unsigned int semente);
/*
Define a semente do sorteio entre lances empatados (SORTEIO_LIGADO).
Deve ser chamada uma única vez, antes da primeira escolherJogada:
    no motor: iniciarSorteio((unsigned int) time(NULL))
    nos testes: semente fixa, para reproduzir o sorteio
Sem esta chamada, o sorteio repete a mesma sequência a cada execução.
*/

extern Play JOGADA;

extern MoveList CANDIDATOS;
/*
Lances da raiz empatados no melhor valor na última iteração aproveitada (completa
ou, no alfa-beta, parcial; ver escolherJogada) da última chamada de escolherJogada,
na ordem em que a busca percorreu a raiz (minimax: a do
gerador; alfa-beta: o melhor lance da iteração anterior primeiro, depois MVV-LVA,
ver ordenar.h). JOGADA.lance é CANDIDATOS.moves[0]
com SORTEIO_DESLIGADO, ou um deles sorteado com SORTEIO_LIGADO.
Esvaziada no início de escolherJogada; fica vazia se JOGADA.resultado != RESULTADO_SUCESSO.
*/

extern uint64_t NOS_VISITADOS;
/*
Número de nós visitados pela última chamada de escolherJogada: cada entrada
em minimax ou podaAlfaBeta conta um nó, inclusive a raiz, somando todas as
iterações (também a interrompida). Zerado no início de escolherJogada; fica em 0 se verificarChamada não devolver sucesso.
Métrica de comparação entre variantes (mostra se a poda poda).
*/

extern int PROF_ALCANCADA;
extern bool PARCIAL_USADA;
/*
Instrumentação da última chamada de escolherJogada:
    PROF_ALCANCADA: limite da iteração aproveitada (a que gerou JOGADA)
    PARCIAL_USADA: verdadeiro se essa iteração foi interrompida pelo tempo e
    aproveitada parcialmente (só alfa-beta)
Zeradas (0 e falso) no início de escolherJogada; ficam assim se
JOGADA.resultado != RESULTADO_SUCESSO.
*/

#endif
