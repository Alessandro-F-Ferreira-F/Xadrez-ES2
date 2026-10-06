#ifndef PENDENCIASREGRAS_H
#define PENDENCIASREGRAS_H

/*
Arquivo provisório que lista as pendências da máquina de regras com a IA.

Declara as funções das regras que a busca usa e que ainda não existem no
repositório. Nomes, assinaturas e valores devem ser acordados com o
responsável pela máquina de regras; quando as funções reais existirem,
este arquivo é substituído pelos cabeçalhos das regras.

Até lá, as definições vêm de stubs (apenas para compilar e testar a IA).
*/

#include "board.h"

// Motivos de empate por regra, retornados por testeEmpateRegra.
// SEM_EMPATE deve ser 0: minimax e podaAlfaBeta testam o retorno como condição.
// Valores provisórios, a definir com a máquina de regras.
// EMPATE_PROVISORIO é temporário, apenas para testar se houve empate, sem capturar qual regra foi acionada
#define SEM_EMPATE 0
#define EMPATE_PROVISORIO 4
#define EMPATE_50_LANCES 1
#define EMPATE_REPETICAO 2
#define EMPATE_MATERIAL 3

int testeEmpateRegra(const Board *b);
/*
Retorna SEM_EMPATE ou EMPATE_PROVISORIO  (futuramente, irá retornar o motivo do empate por regra (50 lances, tripla
repetição, material insuficiente)).
Tripla repetição exige histórico de posições, que não está no Board:
a forma de acesso a esse histórico ainda precisa ser acordada.
*/

#endif
