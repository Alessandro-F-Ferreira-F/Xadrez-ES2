#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;


#define MAX_FEN_STRING 256 
#define BOARD_SIZE 64


typedef enum {WHITE = 1, BLACK = 0} Color;

typedef enum {
    EMPTY = 0, PAWN = 1, KNIGHT = 2,
    BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6
} PieceType;

static const char PIECE_CHAR[16] = ".pnbrqk..PNBRQK.";

typedef uint8_t Piece;
typedef int Square;

#define SQ_NONE (-1)

typedef struct {
    Square from, to;
    u8 flags; // indicar captura
} Move;

typedef struct {
    Piece board[64];
    Color side_to_move;

    Square king_sq[2]; // cache da posição do rei
} Board;

/* 
   unused    color  type
    [0 0 0 0] [X] [Y Y Y]
    
    0 0 0 0 0 1 1 1
*/

#define PIECE_TYPE_MASK 0x7
#define COLOR_MASK 0x1

#define MAKE_PIECE(c, t) ((Piece)(((c) << 3) | (t)))
#define TYPE_OF(p) ((PieceType)(p & PIECE_TYPE_MASK))
#define COLOR_OF(p) ((Color)(((p) >> 3) & COLOR_MASK)) // 00001XXX (white) >> 3 00000001


int decode_piece(char ch) {
    PieceType piece_type;
    Color color;
    Piece piece;

    switch (toupper(ch))
    {
        case 'P':
            piece_type = PAWN;
            break;
        case 'R':
            piece_type = ROOK;
            break;
        case 'N':
            piece_type = KNIGHT;
            break;
        case 'B':
            piece_type = BISHOP;  
            break;
        case 'Q':
            piece_type = QUEEN;
            break;
        case 'K':
            piece_type = KING;
            break;
        default:
            return -1;
            break;
    }

    if (isupper(ch)) {
        color = WHITE;
    } else color = BLACK;

    piece = MAKE_PIECE(color, piece_type);
    return piece;
}

void fill_blanck(Piece *board, int *index, int n) {
    for (int i = *index; i < (*index + n); i++) {
        board[i] = EMPTY;
    }
    *index += n;
}


void parse_fen(Piece *board, char fen_string[MAX_FEN_STRING]) {
    char *ptr = fen_string;
    char ch;
    int index = 0;
    Piece piece;

    
    while(*ptr != '\0') {
        if (index >= 64) {
            fprintf(stderr, "board indexing out of bounds");
            break;
        }
        ch = *ptr;
        
        if (isalpha(ch)) {
            piece = decode_piece(ch);
            board[index++] = piece;
        } else if (isdigit(ch)) {
            int n = ch - '0';
            fill_blanck(board, &index, n);
        }
        ptr++;
    }
}

char* board_to_fen(Piece *board) { // CLAUDE? Aqui não seria uma boa ideia usar a struct String? Para saber o tamanho da string retornada...
    char *fen = (char *)malloc(MAX_FEN_STRING);
    int pos = 0;
    int sq;
    for (int rank = 0; rank < 8; rank++) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            sq = (rank * 8) + file;
            Piece piece = board[sq];

            if (piece == EMPTY) {
                empty++;
                continue;
            }

            if (empty > 0) {
                fen[pos++] = '0' + empty;
                empty = 0;
            }

            fen[pos++] = PIECE_CHAR[piece];

        }

        if (empty > 0) {
            fen[pos++] = '0' + empty;
        }

        if (rank < 7) {
            fen[pos++] = '/';
        }
    }

    fen[pos] = '\0';
    return fen;
}



void print_board(Piece *board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%-2u ", board[i]);
        if (((i + 1) % 8 == 0) && (i != 0)) printf("\n");
    }
    printf("\n");
}


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

int main() {
    //tabuleiro FEN: rnbqk1nr/pppp1ppp/8/2b5/3pP3/5N2/PPP2PPP/RNBQKB1R
    Piece board[BOARD_SIZE] = {
    // Rank 8
     4,  2,  3,  5,  6,  0,  2,  4,
    // Rank 7
     1,  1,  1,  1,  0,  1,  1,  1,
    // Rank 6
     0,  0,  0,  0,  0,  0,  0,  0,
    // Rank 5
     0,  0,  3,  0,  0,  0,  0,  0,
    // Rank 4
     0,  0,  0,  1,  9,  0,  0,  0,
    // Rank 3
     0,  0,  0,  0,  0, 10,  0,  0,
    // Rank 2
     9,  9,  9,  0,  0,  9,  9,  9,
    // Rank 1
    12, 10, 11, 13, 14, 11,  0, 12
    };

    
    print_board(board);
    printf("\n");

    char *output_fen = board_to_fen(board);
    printf("Output FEN: %s\n", output_fen);

    printf("\n");
    // print_piece_chart();
}