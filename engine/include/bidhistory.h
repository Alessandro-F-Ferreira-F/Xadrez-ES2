#ifndef BIDHISTORY_H
#define BIDHISTORY_H

#include "makemove.h"

typedef struct {
    Move *moves;
    Undo *undos;
    int count;
    int capacity;
} BidHistory;

void bidhistory_init(BidHistory *history);
bool bidhistory_add(BidHistory *history, Board *board, Move move);
bool bidhistory_last_move(const BidHistory *history, Move *move);
bool bidhistory_undo_last(BidHistory *history, Board *board);
void bidhistory_clear(BidHistory *history);
void bidhistory_free(BidHistory *history);

#endif /* BIDHISTORY_H */
