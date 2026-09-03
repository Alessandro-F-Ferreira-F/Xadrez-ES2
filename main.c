#include "types.h"
#include "fen_parser.h"

int main(void) {
    const char *fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board b;

    if (parse_fen(fen, &b)) {
        printf("FEN is valid.\n");
    } else {
        printf("FEN is invalid\n");
    }

    return 0;
}
