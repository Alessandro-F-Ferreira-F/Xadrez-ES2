#include "board.h"
#include "piece.h"
#include "square.h"
#include "log.h"


bool is_empty(const Board *b, int sq) {return b->array[sq] == NO_PIECE; }

bool fill_sq(Board *b, const char *sq_str, Piece p) {
    int sq = sq_from_coord(sq_str);
    if (sq == SQ_NONE) return false;

    b->array[sq] = p;
    return true;
}

int  board_find_king(const Board *b, Color c) {
    int sq;
    Piece p;
    for (sq = 0; sq < BOARD_SIZE; sq++) {
        p = b->array[sq];

        if (is_own(p,c) && PIECE_TYPE(p) == KING) {
            return sq;
        }
    }
    return -1;
}

static bool piece_code_is_valid(Piece p) {
    PieceType type = PIECE_TYPE(p);

    if (p >= 16u) return false;
    if (p == NO_PIECE) return true;

    return (type >= PAWN || type <= KING);

}


bool board_check_invariants(const Board *b, const char **fail_msgs) {
    int kings[NUM_COLORS] = {0, 0};
    Piece p;
    int sq;

    for (sq = 0; sq < BOARD_SIZE; sq++) {
        p = b->array[sq];

        if (!piece_code_is_valid(p)) {
            LOG_ERROR("invalid piece code");
            return false;
        }
        if (!is_empty(b, sq) && PIECE_TYPE(p) == KING) {
            kings[PIECE_COLOR(p)]++;
        }
    }

    if (kings[WHITE] != 1) {
        LOG_ERROR("number of white kings not equal to 1");
        return false;
    }
    if (kings[BLACK] != 1) {
        LOG_ERROR("number of black kings not equal to 1");
        return false;
    }

    int wk_sq = board_find_king(b, WHITE);
    int bk_sq = board_find_king(b, BLACK);

    if (wk_sq != b->king_square[WHITE]) {
        LOG_ERROR("white king square cache does not match actual king square");
        return false;
    }
    if (bk_sq != b->king_square[BLACK]) {
        LOG_ERROR("black king square cache does not match actual king square");
        return false;
    }

    // verifica peões
    for (sq = 0; sq < BOARD_SIZE; sq++) {
        p = b->array[sq];

        if (!is_empty(b, sq) && PIECE_TYPE(p) == PAWN) {
            int rank = RANK_OF(sq);

            if (rank == 0 || rank == 7) {
                LOG_ERROR("pawn at first or last rank");
                return false;
            }
        }
    }

    
}


void board_clear(Board *b)
{
    memset(b, 0, sizeof(Board));

    b->ep_square           = SQ_NONE;
    b->king_square[WHITE]  = SQ_NONE;
    b->king_square[BLACK]  = SQ_NONE;
    b->fullmove_number     = 1;
}

void board_print(const Board *board) {
    printf("\n");

    for (int rank = 7; rank >= 0; rank--) {
        printf("%d  ", rank + 1);
        for (int file = 0; file < 8; file++) {
            Piece p = board->array[SQ_AT(rank, file)];
            printf("[%c]", piece_to_char(p));
        }
        printf("\n");
    }

    printf("\n   ");
    for (char file = 'a'; file <= 'h'; file++) {
        printf(" %c ", file);
    }
    printf("\n");

    char ep[3];
    sq_to_coord(board->ep_square, ep);

    printf("Side to move:      %s\n", (board->side_to_move == WHITE) ? "WHITE" : "BLACK");
    printf("Castling rights:   %c%c%c%c\n",
           (board->castling_rights & CASTLE_WK) ? 'K' : '-',
           (board->castling_rights & CASTLE_WQ) ? 'Q' : '-',
           (board->castling_rights & CASTLE_BK) ? 'k' : '-',
           (board->castling_rights & CASTLE_BQ) ? 'q' : '-');
    printf("En passant square: %s\n", ep);
    printf("Halfmove clock:    %d\n", board->halfmove_clock);
    printf("Fullmove number:   %d\n", board->fullmove_number);
}
