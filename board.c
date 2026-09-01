#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_FEN_STRING 256 
#define IS_UPPERCASE(c) ((c >= 'A') && (c <= 'Z')) 
#define IS_LOWERCASE(c) ((c >= 'a') && (c <= 'z'))
#define BOARD_SIZE 64

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct String {
    char *buffer;
    u64 length;
} String;

String* create_string(char *buffer) {
    u64 len = strlen(buffer);
    String *str = (String*)malloc(sizeof(String));
    str->buffer = buffer;
    str->length;
    return str;
}

void print_string(String *str) {
    printf("%s", str->buffer);
}

typedef enum Color {
    WHITE = 1,
    BLACK = 2
} Color;

typedef enum Piece {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,  // 011
    ROOK = 4, 
    QUEEN = 5, 
    KING = 6
} Piece;

// typedef struct Piece {
//     int type;
//     int color;
// }

// 64 bits 2 inteiros de 32 bits

u8 encode_piece(Piece piece, Color color) {
    uint8_t piece_encoded = ((color << 6) | piece);
    return piece_encoded;
}

int decode_fen(char ch) {
    int piece_type;
    int color;
    int piece;

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

    if (IS_UPPERCASE(ch)) {
        color = WHITE;
    } else color = BLACK;

    piece = (int) encode_piece(piece_type, color);

    return piece;
}

void fill_blanck(u8 *board, int *index, int n) {
    for (int i = *index; i < (*index + n); i++) {
        board[i] = EMPTY;
    }
    *index += n;
}

/* 
"PP2nk2"
[P, P, 0, 0, n, k, 0, 0]
*/

u8* fen_to_board(char fen_string[MAX_FEN_STRING]) {
    char *ptr = fen_string;
    char ch;
    int piece;
    int index = 0;

    u8 *board = (u8*)malloc(sizeof(u8) * BOARD_SIZE);
    
    while(*ptr != '\0') {
        ch = *ptr;
        
        if (isalpha(ch)) {
            piece = decode_fen(ch);
            board[index++] = piece;
            // if (piece == -1) printf("<ERROR>");
            // printf("Piece: %c\n", ch);
            // printf("Index: %d\n", index);

        } else if (isdigit(ch)) {
            // printf("Piece: %c\n", ch);
            int n = atoi(&ch);
            // printf("N: %d\n", n);
            // printf("Index (before fill): %d\n", index);
            fill_blanck(board, &index, n);
            // printf("Index (after fill): %d\n", index);
        }
        ptr++;
    }

    return board;
}

void print_board(u8 *board) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf("%u ", board[i]);
        if (((i + 1) % 8 == 0) && (i != 0)) printf("\n");
    }
    printf("\n");
}

char* board_to_fen(uint8_t *board);

int main() {
    // u8 board[64];
    /* 
    [r, n, b, q, k, b, n, r],
    [p, p, p, p, p, p, p, p]
    ]
    */
    
    // String *my_str = create_string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    // print_string(my_str);

    char fen_string[MAX_FEN_STRING] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

    u8 *board = fen_to_board(fen_string);

    printf("\n");
    print_board(board);

    u8 r = encode_piece(ROOK, BLACK);
    u8 R = encode_piece(ROOK, WHITE);
    u8 b = encode_piece(BISHOP, BLACK);
    u8 B = encode_piece(BISHOP, WHITE);
    u8 p = encode_piece(PAWN, BLACK);
    u8 P = encode_piece(PAWN, WHITE);
    u8 n = encode_piece(KNIGHT, BLACK);
    u8 N = encode_piece(KNIGHT, WHITE);
    u8 k = encode_piece(KING, BLACK);
    u8 K = encode_piece(KING, WHITE);
    u8 q = encode_piece(QUEEN, BLACK);
    u8 Q = encode_piece(QUEEN, WHITE);

    printf("\nTABELA DE PEÇAS\n");

    printf("Black rook: %u\n", r);
    printf("White rook: %u\n", R);
    printf("Black bishop: %u\n", b);
    printf("White bishop: %u\n", B);
    printf("Black pawn: %u\n", p);
    printf("White pawn: %u\n", P);
    printf("Black knight: %u\n", n);
    printf("White knight: %u\n", N);
    printf("Black king: %u\n", k);
    printf("White king: %u\n", K);
    printf("Black queen: %u\n", q);
    printf("White queen: %u\n", Q);

    // char c;

    // printf("Type a char: ");
    // scanf("%c", &c);

    // if (IS_UPPERCASE(c) == false) printf("false\n");
    // else printf("true\n");

}