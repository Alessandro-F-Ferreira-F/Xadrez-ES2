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

#define INPUT_STR_SIZE 128
#define BOARD_SIZE 64
#define BOARD_WIDTH 8
#define GEN_MOVES_MAX 1024

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define MemoryZero(addr, size) memset((addr), 0x0, (size))
#define MemoryZeroStruct(addr, st) MemoryZero((addr), sizeof(st))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define SQ_NONE (-1)
#define RANK_OF(sq) ((sq) / BOARD_WIDTH)
#define FILE_OF(sq) ((sq) % BOARD_WIDTH)
#define FILE_DIST(dest, orig) (abs(FILE_OF(dest) - FILE_OF(orig)))
#define SQ_FROM_RF(rank, file) ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq) (((sq < 0) || (sq >= 64)) ? 1 : 0)

#define MOVE_MASK 0xFFUL
#define MAX_FEN_STRING 256

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

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int king_square[2];
} Board;

typedef enum {
    DIR_N,
    DIR_S,
    DIR_E,
    DIR_W,
    DIR_NE,
    DIR_SW,
    DIR_SE,
    DIR_NW,
    DIR_COUNT
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
    Move moves[GEN_MOVES_MAX];
    int count;
} MoveList;

#endif

