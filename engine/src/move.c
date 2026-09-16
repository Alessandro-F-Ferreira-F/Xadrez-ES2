#include "move.h"

#include <assert.h>

#include "log.h"
#include "square.h"
#include "utils.h"

#define MOVE_SQ_MASK 0x3Fu //63 
#define MOVE_TO_SHIFT 6
#define MOVE_TYPE_SHIFT 12
#define MOVE_TYPE_MASK 0xFu
#define CASTLE_TYPE_MASK 0x2

Move encode_move(int from, int to, MoveType type) {
    Move encoded = (Move)((from) | (to << MOVE_TO_SHIFT) | (type << MOVE_TYPE_SHIFT));
    return encoded;
}

int move_from(Move m) {
    return (int)(m & MOVE_SQ_MASK);
}

int move_to(Move m) {
    return (int)((m >> MOVE_TO_SHIFT) & MOVE_SQ_MASK);
}

MoveType move_type(Move m) {
    return (int)((m >> MOVE_TYPE_SHIFT) & MOVE_TYPE_MASK);
}

bool move_is_castle(Move m) {
    MoveType t = move_type(m);
    return ((t == MV_CASTLE_KING) || (t == MV_CASTLE_QUEEN));
}

bool move_is_capture(Move m) {
    return (move_type(m) & 4) != 0;
}

bool move_is_ep_capture(Move m) {
    MoveType t = move_type(m);
    return (t == MV_EP_CAPTURE);
}

bool move_is_promotion(Move m) {
    return (move_type(m) & 8) != 0;
}

PieceType move_promo_type(Move m) {
    if (!move_is_promotion(m)) {
        return EMPTY;
    }

    return ((PieceType)(KNIGHT + (move_type(m) & 3)));
}

void move_to_str(Move m, char out[6]) {
    int n = 0;

    sq_to_coord(move_from(m), &out[0]);
    sq_to_coord(move_to(m), &out[2]);
    n = 4;

    if (move_is_promotion(m)) {
        /* Minuscula sempre, em UCI: 'e7e8q' vale para os dois lados. */
        out[n++] = piece_to_char(PIECE_MAKE(BLACK, move_promo_type(m)));
    }

    out[n] = '\0';
}

Move move_from_str(const char *in) {
    char from_str[3];
    char to_str[3];
    char promoted;

    size_t len = strlen(in);
    if ((len < 4) || (len > 5)) {
        return MOVE_NONE;
    }

    strslc(in, from_str, 0, 2);
    strslc(in, to_str, 2, 4);
    promoted = in[4];

    int from, to;
    MoveType promotion_type;

    from = sq_from_coord(from_str);
    to = sq_from_coord(to_str);
    if ((from == SQ_NONE) || (to == SQ_NONE)) {
        return MOVE_NONE;
    }

    switch (promoted)
    {
    case 'n':
        promotion_type = MV_PROMO_N;
        break;
    case 'b':
        promotion_type = MV_PROMO_B;
        break;
    case 'r':
        promotion_type = MV_PROMO_R;
        break;
    case 'q':
        promotion_type = MV_PROMO_Q;
        break;
        
    default:
        promotion_type = EMPTY;
        break;
    }

    Move out = encode_move(from, to, promotion_type);
    return out;
}


void movelist_clear(MoveList *l) {
    l->count = 0;
}

void movelist_add(MoveList *l, Move m) {
    if (l->count >= MAX_MOVES) {
        LOG_ERROR("MoveList cheia (%d lances) -- lance descartado", l->count);
        return;
    }

    l->moves[l->count] = m;
    l->count++;
}



int movelist_find(MoveList *l, const char *uci) {
    Move m = move_from_str(uci);
    if (m == MOVE_NONE) return -1;

    int from = move_from(m);
    int to = move_to(m);

    int t_from, t_to;

    for (int i = 0; i < l->count; i++) {
        Move t = l->moves[i];
        t_from = move_from(t);
        t_to = move_to(t);

        if ((to == t_to) && (from == t_from) && (move_promo_type(m) == move_promo_type(t))) {
            return i;
        }
        
    }
    return -1;
}




void print_move(Move m) {
    char out_move[6];
    move_to_str(m, out_move);

    printf("Move: (%s)\n", out_move);
}


void print_moves(MoveList *list) {
    printf("\n<<< Valid Moves >>>\n");
    for (int i = 0; i < list->count; i++) {
        Move m = list->moves[i];
        print_move(m);
    }
    printf("\n");
}