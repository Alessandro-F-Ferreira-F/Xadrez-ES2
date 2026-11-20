#ifndef IA_TEST_COMMON_H
#define IA_TEST_COMMON_H

#include "move.h"
#include <stdbool.h>

#define PROF_TESTE 4
#define FEN_KIWIPETE "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

static inline bool ia_mesmos_candidatos(const MoveList *a, const MoveList *b) {
    if (a->count != b->count) return false;
    for (int i = 0; i < a->count; i++) {
        if (a->moves[i] != b->moves[i]) return false;
    }
    return true;
}

static inline bool ia_contem_lance(const MoveList *lista, Move lance) {
    for (int i = 0; i < lista->count; i++) {
        if (lista->moves[i] == lance) return true;
    }
    return false;
}

static inline bool ia_mesmo_conjunto(const MoveList *a, const MoveList *b) {
    if (a->count != b->count) return false;
    for (int i = 0; i < a->count; i++) {
        if (!ia_contem_lance(b, a->moves[i])) return false;
    }
    return true;
}

#endif
