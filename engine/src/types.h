#ifndef TYPES_H
#define TYPES_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int16_t i16;

#define INPUT_STR_SIZE 128
#define MAX_FEN_STRING 256

#define BOARD_SIZE 64
#define BOARD_WIDTH 8
#define MAX_MOVES 256
#define MAX_SEARCH_PLY   64     /* profundidade de busca; 64 é folgado */
#define MAX_GAME_PLY   1024     /* plies de uma partida */
#define NUM_COLORS 2


#define MemoryZero(addr, size) memset((addr), 0x0, (size))
#define MemoryZeroStruct(addr, st) MemoryZero((addr), sizeof(st))
#define PrintSize(type) printf("Size of '%s': %zu bytes", #type, sizeof(type))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))



/*
 * Limites dos campos da FEN.
 * FEN_MIN_FIELDS = 4: pecas, lado, roque e en passant sao obrigatorios.
 * Os relogios sao opcionais porque strings EPD e bancos de posicoes os omitem.
 * Os maximos nao vem da especificacao (que nao poe teto); sao generosos o
 * bastante para qualquer partida real e apertados o bastante para pegar
 * digitacao errada e estouro de int.
 */





/* direitos de roque como bitmask */






#endif