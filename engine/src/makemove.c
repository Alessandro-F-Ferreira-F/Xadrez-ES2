#include "makemove.h"
#include "move.h"
#include "board.h"


void make_move(Board *b, Move m, Undo *u) {
    int from = move_from(m);
    int to = move_to(m);



    Piece p = b->array[from];
    Piece prev_on_target = b->array[to];

    b->array[to] = p;
    b->array[from] = EMPTY;
    b->side_to_move = (b->side_to_move == WHITE) ? BLACK : WHITE;
}

void unmake_move(Board *b, Move move, const Undo *u) {
    return;
}