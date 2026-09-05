#include "utils.h"


void print_piece_chart() {
    printf("\nTABELA DE PEÇAS\n");
    printf("====== ** ======\n");
    printf("Black pawn: %u\n", MAKE_PIECE(BLACK, PAWN));
    printf("Black knight: %u\n", MAKE_PIECE(BLACK, KNIGHT));
    printf("Black bishop: %u\n", MAKE_PIECE(BLACK, BISHOP));
    printf("Black rook: %u\n", MAKE_PIECE(BLACK, ROOK));
    printf("Black queen: %u\n", MAKE_PIECE(BLACK, QUEEN));
    printf("Black king: %u\n", MAKE_PIECE(BLACK, KING));
    printf("----------\n");
    printf("White pawn: %u\n", MAKE_PIECE(WHITE, PAWN));
    printf("White knight: %u\n", MAKE_PIECE(WHITE, KNIGHT));
    printf("White bishop: %u\n", MAKE_PIECE(WHITE, BISHOP));
    printf("White rook: %u\n", MAKE_PIECE(WHITE, ROOK));
    printf("White queen: %u\n", MAKE_PIECE(WHITE, QUEEN));
    printf("White king: %u\n", MAKE_PIECE(WHITE,KING));
}


void print_board(const Board *board) {
    printf("\n");

    for (int rank = 7; rank >= 0; rank--) {
        printf("%d  ", rank + 1);

        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            printf("[%c]", PIECE_CHAR[board->array[sq]]);
        }
        printf("\n");
    }
    printf("\n   ");   

    for (char file = 'a'; file <= 'h'; file++) {
        printf(" %c ", file);
    }
    printf("\n\n");
}

void get_fen(char fen[MAX_FEN_STRING]) {
    printf("Insert FEN: ");
    fgets(fen, MAX_FEN_STRING, stdin);
}

int get_int(char msg[INPUT_STR_SIZE]) {
    if (msg == NULL) printf("Type int: ");
    else printf("%s", msg);

    char buffer[16];
    fgets(buffer, 16, stdin);

    int input_int = strtol(buffer, NULL, 10);
    return input_int;
}




