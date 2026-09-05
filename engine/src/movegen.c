#include "movegen.h"



/* 
GLOBAIS
*/
static const char DIR_CHARMAP[8][16] = {"NORTE", "SUL", "LESTE", "OESTE", "NORDESTE", "SUDOESTE", "SUDESTE", "NOROESTE"};
static const int DIR_OFFSET[DIR_COUNT] = {8, -8, 1, -1, 9, -9, -7, 7};
static int SQ_TO_EDGE[BOARD_SIZE][8]; // guarda para o numero de casas até o fim do tabuleiro para cada direção -- para CADA casa


/* 
* Codificação de lances
                        origem   destino   promotion   flags      
unsigned 32 bits  ->  [00000000][00000000][00000000][00000000]

*/

u32 encode_move(int origin_sq, int target_sq, int promo, int flags) {
    u32 encoded = ((u32)origin_sq << 24) | ((u32)target_sq << 16) | ((u32)promo << 8) | (u32)flags;
    return encoded;
}


MoveDescription decode_move(u32 move) {
    MoveDescription decoded = {0};
    decoded.origin_sq = ((move >> 24) & MOVE_MASK);
    decoded.target_sq= ((move >> 16) & MOVE_MASK);
    decoded.promotion = ((move >> 8) & MOVE_MASK);
    decoded.flags = ((move) & MOVE_MASK);

    return decoded;
}

u32 str_to_move(char move_in[5]) {
    int origin_sq, target_sq;
    char origin_str[3], target_str[3];

    strslc(move_in, origin_str, 0, 2);
    strslc(move_in, target_str, 2, 4);

    origin_sq = sq_from_coord(origin_str);
    target_sq = sq_from_coord(target_str);

    u32 move_out = encode_move(origin_sq, target_sq, 0, 0);
    return move_out;
}

void precompute_move_data() {
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            int num_north = 7 - rank;
            int num_south = rank;
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
    if (list->count > GEN_MOVES_MAX) {
        LOG_ERROR("move list overflow");
        return;
    }

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
            if ((SQ_OFFBOARD(target_sq) == 0) && (board->array[target_sq] == EMPTY)) {
                move = encode_move(sq, target_sq, 0, 0);
                add_move(move, list);
            } else
                continue;
            // implementar caso de captura
        }
    }
}



void genenare_moves_from_direction(Board *board, MoveList *list, int sq, int dir) {
    int dist_to_edge = SQ_TO_EDGE[sq][dir];
    
    int target_sq = sq;

    for (;dist_to_edge > 0; dist_to_edge--) {
        target_sq = target_sq + DIR_OFFSET[dir];

        if (board->array[target_sq] == EMPTY) {
            u32 move = encode_move(sq, target_sq, 0, 0);
            add_move(move, list);
        } else {
            break;
        }
    }
}

void generate_sliding_moves(Board *board, MoveList *list) {
    Piece piece;
    int target_sq;
    u32 move;


    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        piece = board->array[sq];

        if (TYPE_OF(piece) == ROOK) {
            char out[3];
            coord_from_sq(sq, out);
            printf("-- Rook on %s [%d] --\n", out, sq);

            for (int dir = 0; dir < 4; dir++) {
                genenare_moves_from_direction(board, list, sq, dir);
            }
        }
    }
}


void generate_all_moves(Board *board, MoveList *list);



/*
Procura um lance na lista gerada a partir de origem, destino e promoção.
Devolve por 'out' o lance COMO O GERADOR O PRODUZIU -- com as flags corretas
(captura, en passant, roque), que quem digitou "e5d6" não tem como saber.
'out' pode ser NULL se o chamador só quer saber se o lance existe.
*/
bool find_move(Board *board, int origin_sq, int target_sq, int promo, u32 *out) {
    MoveList temp = (MoveList){0};

    /* TODO: trocar por generate_all_moves quando as outras peças existirem */
    generate_pawn_moves(board, &temp);
    generate_sliding_moves(board, &temp);

    for (int i = 0; i < temp.count; i++) {
        MoveDescription cand = decode_move(temp.moves[i].move);

        if ((cand.origin_sq == origin_sq) &&
            (cand.target_sq == target_sq) &&
            (cand.promotion == promo)) {
            if (out != NULL) *out = temp.moves[i].move;
            return true;
        }
    }
    return false;
}





void make_move(Board *b, u32 move) {
    MoveDescription movedesc = decode_move(move);
    u8 origin_sq = movedesc.origin_sq;
    u8 target_sq = movedesc.target_sq;

    Piece p = b->array[origin_sq];

    b->array[target_sq] = p;
    b->array[origin_sq] = EMPTY;
}

/* 
* AUXILIARES
*/

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


void print_square_directions(int sq) {
    if ((sq < 0) || (sq >= 64)) {
        LOG_ERROR("invalid square coordinate");
        return;
    }

    char out[3];
    int *sq_data = SQ_TO_EDGE[sq];

    coord_from_sq(sq, out);
    printf("Directions for square %s\n", out);
    for (int i = 0; i < 8; i++) {
        printf("%s: %d\n", DIR_CHARMAP[i], sq_data[i]);
    }
}