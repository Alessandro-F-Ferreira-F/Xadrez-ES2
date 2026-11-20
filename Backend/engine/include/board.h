#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "piece.h"
#include "square.h"

#include <assert.h>

/*
 * Posicao inicial em ORDEM VISUAL: a primeira linha e a fileira 8, da
 * coluna a para h. O indice 0 desta tabela e a8, NAO a1 -- o Board usa
 * a1 = 0, entao quem copia daqui precisa inverter a fileira.
 */
static const char BOARD_START_POS[BOARD_SIZE] = {
    'r','n','b','q','k','b','n','r',   /* 8 */
    'p','p','p','p','p','p','p','p',   /* 7 */
    '.','.','.','.','.','.','.','.',   /* 6 */
    '.','.','.','.','.','.','.','.',   /* 5 */
    '.','.','.','.','.','.','.','.',   /* 4 */
    '.','.','.','.','.','.','.','.',   /* 3 */
    'P','P','P','P','P','P','P','P',   /* 2 */
    'R','N','B','Q','K','B','N','R'    /* 1 */
};

enum CastleRights{
    CASTLE_NONE = 0,
    CASTLE_WK = 1,
    CASTLE_WQ = 2,
    CASTLE_BK = 4,
    CASTLE_BQ = 8,
    CASTLE_WHITE = CASTLE_WK | CASTLE_WQ,
    CASTLE_BLACK = CASTLE_BK | CASTLE_BQ,
    CASTLE_ALL = CASTLE_WHITE | CASTLE_BLACK
};

// int CASTLE_UNDO_POSITIONS[4][2] = {{3, 0}, {5, 7}, {59, 56}, {61, 63}};
// Tabela para reverter o castle em unmake move
static const int CASTLE_POSITIONS[NUM_COLORS][2][2] = {{{59, 56}, {61, 63}}, {{3, 0}, {5, 7}}};

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int king_square[2];
    
    u8 castling_rights;    /* bitmask CASTLE_* */
    int ep_square;         /* en passant: SQ_NONE se não houver */
    int halfmove_clock;
    int fullmove_number;
} Board;

int  board_find_king(const Board *b, Color c);
bool board_check_invariants(const Board *b);

void board_clear(Board *b);
void board_new(Board *new);
void board_print(const Board *board);

bool fill_sq(Board *b, const char *sq_str, Piece p);

static inline bool is_empty(const Board *b, int sq) {
    assert(!SQ_OFFBOARD(sq) && "sq offboard");
    return b->array[sq] == NO_PIECE; 
}

static inline bool is_own(const Board *b, int sq) {
    assert(!SQ_OFFBOARD(sq) && "sq offboard");    
    Piece p = b->array[sq];
    return p != EMPTY && PIECE_COLOR(p) == b->side_to_move; 
}

static inline bool is_enemy(const Board *b, int sq) { 
    assert(!SQ_OFFBOARD(sq) && "sq offboard");    
    Piece p = b->array[sq];
    return p != EMPTY && PIECE_COLOR(p) != b->side_to_move;
}


#endif

