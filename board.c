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

#include "types.h"
#include "fen_parser.h"

/* 
VARIÁVEIS GLOBAIS
*/



const char dir_charmap[8][16] = {"NORTE", "SUL", "LESTE", "OESTE", "NORDESTE", "SUDOESTE", "SUDESTE", "NOROESTE"};

static const int DIR_OFFSET[DIR_COUNT] = {-8, 8, 1, -1, -7, 7, 9, -9}; // TODO: inverter offsets

static int squares_to_edge[BOARD_SIZE][8]; // guarda para o numero de casas até o fim do tabuleiro para cada direção -- para CADA casa

static MoveList gen_moves;                   






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

void init_move_list() {
    MemoryZeroStruct(&gen_moves, MoveList);
}

void gen_reset();

void gen_push(Move move) {
    int i = gen_moves.index;
    Move *m = &gen_moves.moves[i];

    m->origin = move.origin;
    m->target = move.target;
    m->promo = move.promo;
    m->flags = move.flags;

    (gen_moves.index)++;
}

void generate_sliding_moves() {

}


void generate_moves(Board *board);




/* 
AUXILIARES
*/

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

