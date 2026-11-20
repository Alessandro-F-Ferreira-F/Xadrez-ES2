#ifndef PENDENCIASREGRAS_H
#define PENDENCIASREGRAS_H

/*
Arquivo provisório que lista as pendências da máquina de regras com a IA.

Contrato de compatibilidade da busca. No motor, regras.c adapta draw_reason
(engine/include/draw.h) para os valores abaixo. stubs.c continua exclusivo
dos testes isolados da IA; os dois arquivos nunca devem ser ligados juntos.
*/

#include "board.h"

// Motivos de empate por regra, retornados por testeEmpateRegra.
// SEM_EMPATE deve ser 0: minimax e podaAlfaBeta testam o retorno como condição.
// EMPATE_PROVISORIO é temporário, apenas para testar se houve empate, sem capturar qual regra foi acionada
#define SEM_EMPATE 0
#define EMPATE_PROVISORIO 4
#define EMPATE_50_LANCES 1
#define EMPATE_REPETICAO 2
#define EMPATE_MATERIAL 3

int testeEmpateRegra(const Board *b);
/*
Retorna SEM_EMPATE, EMPATE_50_LANCES ou EMPATE_MATERIAL no motor.
EMPATE_PROVISORIO e reservado aos testes com stub. A repeticao e detectada
somente no protocolo; nao e consultada dentro da arvore nesta versao.
*/

#endif
