#include "../include/perft.h"

#include "../include/move.h"
#include "../include/movegen.h"
#include "../include/makemove.h"

u64 perft(Board *board, int depth) {
    if (depth == 0) {
        return 1;
    }
    Undo u;

    MoveList list;
    int n_moves;
    u64 nodes = 0;

    generate_legal_moves(board, &list);
    n_moves = list.count;

    for (int i = 0; i < n_moves; i++) {
        make_move(board, list.moves[i], &u);
        nodes += perft(board, depth-1);
        unmake_move(board, list.moves[i], &u);
    }

    return nodes;
}
