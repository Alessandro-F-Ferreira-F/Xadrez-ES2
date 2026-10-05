#include "../include/bidhistory.h"

#include "../include/log.h"

static bool bidhistory_reserve(BidHistory *history, int needed) {
    int capacity = history->capacity == 0 ? 16 : history->capacity;
    Move *moves;
    Undo *undos;

    if (needed > MAX_GAME_PLY) {
        LOG_ERROR("historico atingiu o limite de %d lances", MAX_GAME_PLY);
        return false;
    }

    while (capacity < needed) {
        capacity *= 2;
        if (capacity > MAX_GAME_PLY) {
            capacity = MAX_GAME_PLY;
        }
    }

    if (capacity == history->capacity) {
        return true;
    }

    moves = malloc((size_t)capacity * sizeof(*moves));
    undos = malloc((size_t)capacity * sizeof(*undos));
    if (moves == NULL || undos == NULL) {
        free(moves);
        free(undos);
        LOG_ERROR("falha ao alocar memoria para o historico de lances");
        return false;
    }

    if (history->count > 0) {
        memcpy(moves, history->moves, (size_t)history->count * sizeof(*moves));
        memcpy(undos, history->undos, (size_t)history->count * sizeof(*undos));
    }

    free(history->moves);
    free(history->undos);
    history->moves = moves;
    history->undos = undos;
    history->capacity = capacity;
    return true;
}

void bidhistory_init(BidHistory *history) {
    assert(history != NULL);
    *history = (BidHistory){0};
}

bool bidhistory_add(BidHistory *history, Board *board, Move move) {
    Undo undo;

    assert(history != NULL);
    assert(board != NULL);

    if (!bidhistory_reserve(history, history->count + 1)) {
        return false;
    }

    make_move(board, move, &undo);
    history->moves[history->count] = move;
    history->undos[history->count] = undo;
    history->count++;
    return true;
}

bool bidhistory_last_move(const BidHistory *history, Move *move) {
    assert(history != NULL);

    if (history->count == 0 || move == NULL) {
        return false;
    }

    *move = history->moves[history->count - 1];
    return true;
}

bool bidhistory_undo_last(BidHistory *history, Board *board) {
    int last;

    assert(history != NULL);
    assert(board != NULL);

    if (history->count == 0) {
        return false;
    }

    last = history->count - 1;
    unmake_move(board, history->moves[last], &history->undos[last]);
    history->count--;
    return true;
}

void bidhistory_clear(BidHistory *history) {
    assert(history != NULL);
    history->count = 0;
}

void bidhistory_free(BidHistory *history) {
    assert(history != NULL);
    free(history->moves);
    free(history->undos);
    *history = (BidHistory){0};
}