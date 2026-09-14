#ifndef MOVE_H
#define MOVE_H

#include "types.h"
#include "piece.h"


typedef u16 Move;

#define MOVE_NONE ((Move)0)

typedef enum {
    MV_QUIET         =  0,
    MV_DOUBLE_PUSH   =  1,
    MV_CASTLE_KING   =  2,
    MV_CASTLE_QUEEN  =  3,
    MV_CAPTURE       =  4,
    MV_EP_CAPTURE    =  5,
    /* 6 e 7 nao existem */
    MV_PROMO_N       =  8,
    MV_PROMO_B       =  9,
    MV_PROMO_R       = 10,
    MV_PROMO_Q       = 11,
    MV_PROMO_CAP_N   = 12,
    MV_PROMO_CAP_B   = 13,
    MV_PROMO_CAP_R   = 14,
    MV_PROMO_CAP_Q   = 15
} MoveType;


typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

Move encode_move(int from, int to, MoveType type);

int move_from(Move m);
int move_to(Move m);
MoveType move_type(Move m);

bool move_is_capture(Move m);
bool move_is_promotion(Move m);

PieceType move_promo_type(Move m);

void movelist_clear(MoveList *l);
void movelist_add(MoveList *l, Move m);
int movelist_find(MoveList *l, const char *uci);

void move_to_str(Move m, char out[6]);
Move move_from_str(const char *in);

#endif /* MOVE_H */