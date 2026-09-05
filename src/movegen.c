#include "board.h"




/* 
GLOBAIS
*/
const char DIR_CHARMAP[8][16] = {"NORTE", "SUL", "LESTE", "OESTE", "NORDESTE", "SUDOESTE", "SUDESTE", "NOROESTE"};
static const int DIR_OFFSET[DIR_COUNT] = {8, -8, 1, -1, 9, -9, -7, 7};
int SQ_TO_EDGE[BOARD_SIZE][8]; // guarda para o numero de casas até o fim do tabuleiro para cada direção -- para CADA casa


/* 
* Codificação de lances
                        origem   destino   promotion   flags      
unsigned 32 bits  ->  [00000000][00000000][00000000][00000000]

*/

static u32 encode_move(int origin_sq, int target_sq, int promo, int flags) {
    u32 encoded = ((u32)origin_sq << 24) | ((u32)target_sq << 16) | ((u32)promo << 8) | (u32)flags;
    return encoded;
}


static MoveDescription decode_move(u32 move) {
    MoveDescription decoded = {0};
    decoded.origin_sq = ((move >> 24) & MOVE_MASK);
    decoded.target_sq= ((move >> 16) & MOVE_MASK);
    decoded.promotion = ((move >> 8) & MOVE_MASK);
    decoded.flags = ((move) & MOVE_MASK);

    return decoded;
}


void precompute_move_data() {
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            int num_north = rank;
            int num_south = 7 - rank;
            int num_west = file;
            int num_east = 7 - file;

            int square_index = rank * 8 + file;

            SQ_TO_EDGE[square_index][DIR_N] = num_north;
            SQ_TO_EDGE[square_index][DIR_S] = num_south;
            SQ_TO_EDGE[square_index][DIR_E] = num_east;
            SQ_TO_EDGE[square_index][DIR_W] = num_west;
            SQ_TO_EDGE[square_index][DIR_NE] = MIN(num_north, num_east);
            SQ_TO_EDGE[square_index][DIR_SW] = MIN(num_south, num_west);
            SQ_TO_EDGE[square_index][DIR_SE] = MIN(num_south, num_east);
            SQ_TO_EDGE[square_index][DIR_NW] = MIN(num_north, num_west);
        }
    }
}



void add_move(u32 move, MoveList *list) {
    list->moves[list->count].move = move;
    list->moves[list->count].score = 0;
    list->count++;
}

void generate_pawn_moves(Board *board, MoveList *list) {
    Piece piece;
    int target_sq;
    u32 move;

    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        piece = board->array[sq];
        if (TYPE_OF(piece) != PAWN) continue;

        if (COLOR_OF(piece) == WHITE) {
            target_sq = sq + DIR_OFFSET[DIR_N];
            if ((SQ_OFFBOARD(target_sq) == 0) && (board->array[target_sq] == EMPTY)) {
                move = encode_move(sq, target_sq, 0, 0);
                add_move(move, list);
            } else
                continue;
            // implementar caso de captura
        }

        if (COLOR_OF(piece) == BLACK) {
            target_sq = sq + DIR_OFFSET[DIR_S];
            if ((SQ_OFFBOARD(sq) == 0) && (board->array[target_sq] == EMPTY)) {
                target_sq = sq + DIR_OFFSET[DIR_S];
                move = encode_move(sq, target_sq, 0, 0);
                add_move(move, list);
            } else
                continue;
            // implementar caso de captura
        }
    }
}

void generate_sliding_moves(Board *board, MoveList *list) {

}


void generate_all_moves(Board *board, MoveList *list);

void print_move(int origin_sq, int target_sq) {
    char out_origin[3];
    char out_target[3];
    coord_from_sq(origin_sq, out_origin);
    coord_from_sq(target_sq, out_target);

    printf("Move: (%s, %s)\n", out_origin, out_target);
}

void print_moves(MoveList *list) {
    MoveDescription movedesc;

    printf("\n<<< Valid Moves >>>\n");
    for (int i = 0; i < list->count; i++) {
        u32 move = list->moves[i].move;
        movedesc = decode_move(move);
        print_move(movedesc.origin_sq, movedesc.target_sq);
    }
    printf("\n");
}



/* 
* AUXILIARES
*/


void print_square_directions(int rank, int file) {
    if ((rank < 0 || rank > 7) || (file < 0 || file > 7)) {
        printf("invalid rank or file\n");
        return;
    }

    int sq_check = SQ_FROM_RF(rank, file);
    int *sq_data = SQ_TO_EDGE[sq_check];

    printf("Directions for square (%d, %d)\n", RANK_OF(sq_check), FILE_OF(sq_check));
    for (int i = 0; i < 8; i++) {
        printf("%c: %d\n", PIECE_CHAR[i], sq_data[i]);
    }
}