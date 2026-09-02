/* 
// TODO: Criar função para precomputar direções para cada casa
TODO: Criar função para calcular casas possíveis para peças deslizantes (torre, bispo e rainha)
TODO: Implementar lances pseudo-legais para: torre, bispo e rainha

Ordem de implementação:

1. Corrigir os três bugs acima ([8] no array, bound no fill_blanck, parar no espaço).
2. Separar Direction (índice) de DIR_OFFSET (offset), e escrever precompute_move_data.
3. Definir Move/flags e a assinatura de generate_moves antes de gerar qualquer lance.
4. Deslizantes com a função única parametrizada por intervalo de direção.
5. Cavalo e rei por tabela pré-computada; peão por último (é o mais cheio de casos).
6. is_square_attacked → make/unmake → filtro de legalidade → perft.

*/




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


#define INPUT_STR_SIZE 128
#define MAX_FEN_STRING 256 
#define BOARD_SIZE 64


typedef enum {WHITE = 1, BLACK = 0} Color;

typedef enum {
    EMPTY = 0, PAWN = 1, KNIGHT = 2,
    BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6
} PieceType;

static const char PIECE_CHAR[16] = ".pnbrqk..PNBRQK.";

typedef uint8_t Piece;

#define SQ_NONE (-1)

typedef struct {
    int start_square;
    int target_square;
    u8 flags; // indicar captura
} Move;

typedef struct {
    Piece board[64];
    Color side_to_move;

    int king_square[2]; // cache da posição do rei
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

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define BOARD_WIDTH 8;

#define RANK_OF(sq) ((sq) / 8)
#define FILE_OF(sq) ((sq) % 8)

#define SQ_FROM_RF(rank, file) ((rank) * 8 + (file))

// BOARD DIRECTIONS

typedef enum Direction {DIR_N, DIR_S, DIR_E, DIR_W, DIR_NE, DIR_SW, DIR_SE, DIR_NW, DIR_COUNT} Direction;
const char dir_charmap[8][32] = {"NORTE", "SUL", "LESTE", "OESTE", "NORDESTE", "SUDOESTE", "SUDESTE", "NOROESTE"};
// -> NORTE, SUL, LESTE, OESTE, NORDESTE, SUDOESTE, SUDESTE, NOROESTE

static const int DIR_OFFSET[DIR_COUNT] = {-8, 8, 1, -1, -7, 7, 9, -9};

static int squares_to_edge[BOARD_SIZE][8]; // guarda para o numero de casas até o fim do tabuleiro para cada direção -- para CADA casa

void precompute_move_data() {
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            int num_north = rank;
            int num_south = 7 - rank;
            int num_west = file;
            int num_east = 7 - file;

            int square_index = rank * 8 + file;

            squares_to_edge[square_index][DIR_N] = num_north;
            squares_to_edge[square_index][DIR_S] = num_south;
            squares_to_edge[square_index][DIR_E] = num_east;
            squares_to_edge[square_index][DIR_W] = num_west;
            squares_to_edge[square_index][DIR_NE] = MIN(num_north, num_east);
            squares_to_edge[square_index][DIR_SW] = MIN(num_south, num_west);
            squares_to_edge[square_index][DIR_SE] = MIN(num_south, num_east);
            squares_to_edge[square_index][DIR_NW] = MIN(num_north, num_west);
        }
    }
}


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
            if ((index + n) >= 64) {
                fprintf(stderr, "error: invalid empty spaces in board");
                break;
            }     
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



void print_board(const Piece *board)
{
    printf("\n");

    for (int rank = 0; rank < 8; rank++) {
        printf("%d  ", 8 - rank);

        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            printf("[%c]", PIECE_CHAR[board[sq]]);
        }
        printf("\n");
    }
    printf("\n   ");   

    for (char file = 'a'; file <= 'h'; file++) {
        printf(" %c ", file);
    }
    printf("\n");
}

int get_fen(char fen[MAX_FEN_STRING]) {
    printf("Insert FEN: ");
    return(fgets(fen, MAX_FEN_STRING, stdin));
}

int get_int(char msg[INPUT_STR_SIZE]) {
    if (msg == NULL) printf("Type int: ");
    else printf("%s", msg);

    char buffer[16];
    fgets(buffer, 16, stdin);

    int input_int = strtol(buffer, NULL, 10);
    return input_int;
}

void print_square_directions(int rank, int file) {
    if ((rank < 0 || rank > 7) || (file < 0 || file > 7)) {
        printf("invalid rank or file\n");
        return;
    }

    int sq_check = SQ_FROM_RF(rank, file);
    int *sq_data = squares_to_edge[sq_check];

    printf("Directions for square (%d, %d)\n", RANK_OF(sq_check), FILE_OF(sq_check));
    for (int i = 0; i < 8; i++) {
        printf("%s: %d\n", dir_charmap[i], sq_data[i]);
    }
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
    char fen[MAX_FEN_STRING] = {0};
    Piece board[BOARD_SIZE] = {0};

    precompute_move_data();

    char ch;

    do {
        printf("\n==== Menu ====\n");
        printf("1 - Insert FEN\n");
        printf("2 - Get piece direction\n");
        printf("3 - Print piece chart\n");


        int opt = get_int("Select option:");
        if (opt < 1 || opt > 3) {
            printf("Invalid option\n");
            continue;
        }

        if (opt == 1) {
            get_fen(fen);
            parse_fen(board, fen);
            print_board(board);
        }

        if (opt == 2) {
            int rank, file, sq;
            rank = get_int("RANK: ");
            file = get_int("FILE: ");
            print_square_directions(rank, file);
        }

        if (opt == 3) {
            print_piece_chart();
        }

        printf("\n");
        printf("Type q for quit, y for continue\n");
        ch = fgetc(stdin);
    } while(ch != 'q' && ch != 'Q');

    print_board(board);
    printf("\n");

    char *output_fen = board_to_fen(board);
    printf("Output FEN: %s\n", output_fen);
    printf("\n");
}