#include "movegen.h"
#include "utils.h"



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

static inline bool is_own(Piece p, Color c)   { return p != EMPTY && COLOR_OF(p) == c; }
static inline bool is_enemy(Piece p, Color c) { return p != EMPTY && COLOR_OF(p) != c; }


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


void add_move(u32 move, MoveList *list) {
    if (list->count > GEN_MOVES_MAX) {
        LOG_ERROR("move list overflow");
        return;
    }

    list->moves[list->count].move = move;
    list->moves[list->count].score = 0;
    list->count++;
}


void precompute_move_data(void) {
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


/* 
cor como argumento opcional para gerar argumentos de uma cor especifica de maneira manual.
TODO: pensar em solução melhor depois
*/

bool check_pawn_promotion(int sq) {
    if ((RANK_OF(sq) == 0) || (RANK_OF(sq) == 7)) return true;
    return false;
}

void generate_pawn_moves(const Board *board, MoveList *list, Color side) {
    Piece piece;
    int target_sq;
    u32 move;

    /*
    filtra a direção de avanço e captura do peão;
    push_dir é a direção do avanço
    cap_dir são as direções de captura [2 posições]
     */
    int push_dir;
    Direction cap_dir[2]; // direção de captura

    if (side == WHITE) {
        push_dir = DIR_OFFSET[DIR_N];
        cap_dir[0] = DIR_NE;
        cap_dir[1] = DIR_NW;
    } else {
        push_dir = DIR_OFFSET[DIR_S];
        cap_dir[0] = DIR_SE;
        cap_dir[1] = DIR_SW;
    }

    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        piece = board->array[sq];
        if (TYPE_OF(piece) != PAWN) continue;

        if (COLOR_OF(piece) != side) continue;



        // double pawn push
        if (((RANK_OF(sq) == 1) && side == WHITE) || ((RANK_OF(sq) == 6) && side == BLACK)) {
            target_sq = sq + (2 * push_dir);
            move = encode_move(sq, target_sq, 0, DOUBLE_PAWN_PUSH);
            add_move(move, list);
        }

        // push padrão do peão
        target_sq = sq + push_dir;
        if ((SQ_OFFBOARD(target_sq) == 0) && (board->array[target_sq] == EMPTY)) {
            move = encode_move(sq, target_sq, 0, QUIET_MOVE);
            if (check_pawn_promotion(target_sq)) {
                // [CLAUDE?]: como guardar o resultado da promoção sem saber a escolha do user?
                move = encode_move(sq, target_sq, QUEEN_PROMOTION, QUIET_MOVE); 
            }
            add_move(move, list);
        } 
        

        // captura
        for (int i = 0; i < 2; i++) {
            if (SQ_TO_EDGE[sq][cap_dir[i]] == 0) continue; // verifica se a casa de captura está fora do array

            target_sq = sq + DIR_OFFSET[cap_dir[i]];
            piece = board->array[target_sq];

            if ((board->array[target_sq] != EMPTY) && (COLOR_OF(piece) != side)) {
                move = encode_move(sq, target_sq, 0, (CAPTURE));
                if (check_pawn_promotion(target_sq)) {
                    move = encode_move(sq, target_sq, QUEEN_PROMOTION, (CAPTURE));
                }
                add_move(move, list);
            }
        }
    }
}


void genenare_moves_from_direction(const Board *board, MoveList *list, int sq, int dir) {
    int dist_to_edge = SQ_TO_EDGE[sq][dir];
    int target_sq = sq;
    u32 move;

    for (;dist_to_edge > 0; dist_to_edge--) {
        target_sq = target_sq + DIR_OFFSET[dir];

        if (board->array[target_sq] == EMPTY) {
            move = encode_move(sq, target_sq, 0, QUIET_MOVE);
            add_move(move, list);
        } else {
            Piece p = board->array[target_sq];
            if (COLOR_OF(p) != board->side_to_move) {
                move = encode_move(sq, target_sq, 0, CAPTURE);
                add_move(move, list);
            }
            break;
        }
    }
}

// ATAQUES_CAVALO[64][8]
// ATAQUES_CAVALO[21] = {4, 6, 11, 15, 27, 36, 31, 38}

void generate_sliding_moves(const Board *board, MoveList *list) {
    Piece piece;

    for (int sq = 0; sq < BOARD_SIZE; sq++) {
        piece = board->array[sq];

        if (TYPE_OF(piece) == ROOK) {
            for (int dir = 0; dir < 4; dir++) {
                genenare_moves_from_direction(board, list, sq, dir);
            }
        }
        else if (TYPE_OF(piece) == BISHOP) {
            for (int dir = 4; dir < 8; dir++) {
                genenare_moves_from_direction(board, list, sq, dir);
            }
        }
        else if (TYPE_OF(piece) == QUEEN) {
            for (int dir = 0; dir < 8; dir++) {
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
bool find_move(const Board *board, int origin_sq, int target_sq, int promo, u32 *out) {
    MoveList temp = (MoveList){0};
    Piece p = board->array[origin_sq];
    if (COLOR_OF(p) != board->side_to_move) {
        LOG_ERROR("error: wrong side tried to make move");
    }
    /* TODO: trocar por generate_all_moves quando as outras peças existirem */
    generate_pawn_moves(board, &temp, WHITE);
    generate_pawn_moves(board, &temp, BLACK);
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


void make_move(Board *b, u32 move, Undo *u) {
    MoveDescription movedesc = decode_move(move);
    u8 origin_sq = movedesc.origin_sq;
    u8 target_sq = movedesc.target_sq;


    u32 *out_move;
    if (!find_move(b, origin_sq, target_sq, 0, out_move)) {
        return;
    }

    Piece p = b->array[origin_sq];
    Piece prev_on_target = b->array[target_sq];

    b->array[target_sq] = p;
    b->array[origin_sq] = EMPTY;
    b->side_to_move = (b->side_to_move == WHITE) ? BLACK : WHITE;
}

void unmake_move(Board *b, u32 move, const Undo *u);

/* 
* AUXILIARES
*/

u32 str_to_move(char move_in[6]) {
    int origin_sq, target_sq;
    char origin_str[3], target_str[3];

    strslc(move_in, origin_str, 0, 2);
    strslc(move_in, target_str, 2, 4);

    origin_sq = sq_from_coord(origin_str);
    target_sq = sq_from_coord(target_str);

    u32 move_out = encode_move(origin_sq, target_sq, 0, 0);
    return move_out;
}


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