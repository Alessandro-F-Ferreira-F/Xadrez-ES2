#include "board.h"
#include "piece.h"
#include "square.h"




int  board_find_king(const Board *b, Color c);
bool board_check_invariants(const Board *b, const char **fail_msgs);


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

    char fen[MAX_FEN_STRING];
    fen_write(board, fen);
    printf("FEN: %s\n\n", fen);
}
