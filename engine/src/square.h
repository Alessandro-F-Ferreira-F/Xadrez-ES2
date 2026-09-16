#ifndef SQUARE_H
#define SQUARE_H

#include "types.h"


#define SQ_NONE (-1)
#define RANK_OF(sq) ((sq) / BOARD_WIDTH)
#define FILE_OF(sq) ((sq) % BOARD_WIDTH)
#define FILE_DIST(dest, orig) (abs(FILE_OF(dest) - FILE_OF(orig)))
#define SQ_AT(rank, file) ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq) (((sq) < 0) || ((sq) >= BOARD_SIZE))


enum {
    SQ_A1 =  0, SQ_E1 =  4, SQ_H1 =  7,
    SQ_A8 = 56, SQ_E8 = 60, SQ_H8 = 63,
    SQ_C1 = 2, SQ_G1 = 6, SQ_C8 = 58, SQ_G8 = 62
};

typedef enum {
    DIR_N,  // NORTE
    DIR_S,  // SUL
    DIR_E,  // LESTE
    DIR_W,  // OESTE
    DIR_NE, // NORDESTE
    DIR_SW, // SUDOESTE
    DIR_SE, // SUDESTE
    DIR_NW, // NOROESTE
    NUM_DIRS // NUMERO DE DIREÇÕES
} Direction;

extern const int DIR_OFFSET[NUM_DIRS];
extern const int PAWN_PUSH[NUM_COLORS];

extern int SQ_TO_EDGE[BOARD_SIZE][NUM_DIRS];
extern int KNIGHT_TARGETS[BOARD_SIZE][8];
extern int KING_TARGETS[BOARD_SIZE][8];
extern int PAWN_ATTACKS[NUM_COLORS][BOARD_SIZE][2];


void init_square_tables(void);
int sq_from_coord(const char *coord);
void sq_to_coord(int sq, char out[3]);



#endif /* SQUARE_H */