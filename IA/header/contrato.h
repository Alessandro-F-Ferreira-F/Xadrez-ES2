#ifndef CONTRATO_H
#define CONTRATO_H

/*
Contrato provisório entre a IA e a máquina de regras.

Declara as funções das regras que a busca usa e que ainda não existem no
repositório. Nomes, assinaturas e valores devem ser acordados com o
responsável pela máquina de regras; quando as funções reais existirem,
este arquivo é substituído pelos cabeçalhos das regras.

Até lá, as definições vêm de stubs (apenas para compilar e testar a IA).
*/

#include <stdbool.h>

#include "board.h"
#include "move.h"

// Motivos de empate por regra, retornados por testeEmpateRegra.
// SEM_EMPATE deve ser 0: minimax e podaAlfaBeta testam o retorno como condição.
// Valores provisórios, a definir com a máquina de regras.
#define SEM_EMPATE 0
#define EMPATE_50_LANCES 1
#define EMPATE_REPETICAO 2
#define EMPATE_MATERIAL 3

void gerarLancesLegais(Board *b, MoveList *lista);
/*
Preenche lista com todos os lances legais do lado da vez em b
(lista->count == 0 se não houver nenhum). A lista é alocada por quem chama.
b não é const: o filtro de legalidade pode fazer e desfazer lances, mas
b volta ao estado original ao final.
*/

bool emXeque(const Board *b);
/*
Retorna true se o rei do lado da vez em b está em xeque.
*/

int testeEmpateRegra(const Board *b);
/*
Retorna SEM_EMPATE ou o motivo do empate por regra (50 lances, tripla
repetição, material insuficiente).
Tripla repetição exige histórico de posições, que não está no Board:
a forma de acesso a esse histórico ainda precisa ser acordada.
*/

#endif
