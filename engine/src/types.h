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
#define BOARD_SIZE 64
#define BOARD_WIDTH 8
#define GEN_MOVES_MAX 256
#define MAX_SEARCH_PLY   64     /* profundidade de busca; 64 é folgado */
#define MAX_GAME_PLY   1024     /* plies de uma partida */

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define MemoryZero(addr, size) memset((addr), 0x0, (size))
#define MemoryZeroStruct(addr, st) MemoryZero((addr), sizeof(st))
#define PrintSize(type) printf("Size of '%s': %zu bytes", #type, sizeof(type))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define SQ_NONE (-1)
#define RANK_OF(sq) ((sq) / BOARD_WIDTH)
#define FILE_OF(sq) ((sq) % BOARD_WIDTH)
#define FILE_DIST(dest, orig) (abs(FILE_OF(dest) - FILE_OF(orig)))
#define SQ_FROM_RF(rank, file) ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq) (((sq) < 0) || ((sq) >= BOARD_SIZE))

#define MOVE_MASK 0xFFUL
#define MAX_FEN_STRING 256

/*
 * Limites dos campos da FEN.
 * FEN_MIN_FIELDS = 4: pecas, lado, roque e en passant sao obrigatorios.
 * Os relogios sao opcionais porque strings EPD e bancos de posicoes os omitem.
 * Os maximos nao vem da especificacao (que nao poe teto); sao generosos o
 * bastante para qualquer partida real e apertados o bastante para pegar
 * digitacao errada e estouro de int.
 */
#define FEN_MIN_FIELDS    4
#define FEN_MAX_FIELDS    6
#define FEN_HALFMOVE_MAX  1000
#define FEN_FULLMOVE_MAX  9999

typedef enum {
    WHITE = 1,
    BLACK = 0
} Color;

typedef enum {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
} PieceType;

typedef uint8_t Piece;

#define PIECE_TYPE_MASK 0x7
#define COLOR_MASK 0x1
#define MAKE_PIECE(color, type) \
    ((Piece)((((color) & COLOR_MASK) << 3) | ((type) & PIECE_TYPE_MASK)))
#define TYPE_OF(piece) ((PieceType)((piece) & PIECE_TYPE_MASK))
#define COLOR_OF(piece) ((Color)(((piece) >> 3) & COLOR_MASK))


/*
 * Casas de origem do roque padrao (nao Chess960), na indexacao a1 = 0.
 * Nomear evita repetir 0/4/7/56/60/63 espalhados pelo codigo.
 */
enum {
    SQ_A1 =  0, SQ_E1 =  4, SQ_H1 =  7,
    SQ_A8 = 56, SQ_E8 = 60, SQ_H8 = 63
};

/* direitos de roque como bitmask */
enum ClastleRights{
    CASTLE_WK = 1,   /* branco, lado do rei   (O-O)   */
    CASTLE_WQ = 2,   /* branco, lado da dama  (O-O-O) */
    CASTLE_BK = 4,
    CASTLE_BQ = 8
};

/* flags de lances */
enum MoveFlag {
    QUIET_MOVE,
    DOUBLE_PAWN_PUSH,
    CAPTURE,
    EP_CAPTURE
};

enum Promotion{
    KNIGHT_PROMOTION,
    BISHOP_PROMOTION,
    ROOK_PROMOTION,
    QUEEN_PROMOTION
};

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int king_square[2];
    
    u8 castling_rights;    /* bitmask CASTLE_* */
    int ep_square;         /* en passant: SQ_NONE se não houver */
    int halfmove_clock;   
    int fullmove_number;
} Board;

typedef enum {
    DIR_N,  // NORTE
    DIR_S,  // SUL
    DIR_E,  // LESTE
    DIR_W,  // OESTE
    DIR_NE, // NORDESTE
    DIR_SW, // SUDOESTE
    DIR_SE, // SUDESTE
    DIR_NW, // NOROESTE
    DIR_COUNT // NADA
} Direction;

typedef struct {
    u8 origin_sq;
    u8 target_sq;
    u8 promotion;
    u8 flags;
} MoveDescription;

typedef struct {
    u32 move;
    int score;
} Move;

typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;


typedef struct {
    Move moves[GEN_MOVES_MAX]; // [CLAUDE?]: MoveList pode virar: {u32 moves[GEN_MOVES_MAX], int scores[GEN_MOVE_MAX], int count}, o que acha?
    int count;
} MoveList;


#endif