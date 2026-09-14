#include "square.h"
#include "piece.h"





static const char DIR_CHARMAP[8][16] = {"NORTE", "SUL", "LESTE", "OESTE", "NORDESTE", "SUDOESTE", "SUDESTE", "NOROESTE"};
const int DIR_OFFSET[NUM_DIRS] = {+8, -8, +1, -1, +9, -9, -7, +7};
const int PAWN_PUSH[NUM_COLORS] = {-8, +8};



int SQ_TO_EDGE[BOARD_SIZE][NUM_DIRS]; // * guarda para o numero de casas até o fim do tabuleiro para cada direção -- para CADA casa
int KNIGHT_TARGETS[BOARD_SIZE][8];
int KING_TARGETS[BOARD_SIZE][8];
int PAWN_ATTACKS[NUM_COLORS][BOARD_SIZE][2]; // * para cada casa das 64 do tabuleiro, guarda as casas de ataque do peão, para cada cor

/*
 * Verifica se a casa alvo está fora do tabuleiro
 * drank -> direção vertical; dfile -> direção horizontal
 */
static int offset_square(int rank, int file, int drank, int dfile)
{
    int r = rank + drank;
    int f = file + dfile;

    if (r < 0 || r >= BOARD_WIDTH || f < 0 || f >= BOARD_WIDTH) {
        return SQ_NONE;
    }
    return SQ_AT(r, f);
}

static void init_sq_to_edge(void) {
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

static void init_pawn_attacks() {
    int rank;
    for (rank = 0; rank < BOARD_WIDTH; rank++) {
        int file;
        for (file = 0; file < BOARD_WIDTH; file++) {
            int sq = SQ_AT(rank, file);
            PAWN_ATTACKS[WHITE][sq][0] = offset_square(rank, file,  1, -1);
            PAWN_ATTACKS[WHITE][sq][1] = offset_square(rank, file,  1,  1);
            PAWN_ATTACKS[BLACK][sq][0] = offset_square(rank, file, -1, -1);
            PAWN_ATTACKS[BLACK][sq][1] = offset_square(rank, file, -1,  1);
        }
    }
}


void init_square_tables() {
    init_sq_to_edge();
    init_pawn_attacks();
}

int sq_from_coord(const char *coord) {
    if (coord == NULL) {
        return SQ_NONE;
    }

    char file_ch = coord[0];
    if ((file_ch < 'a') || (file_ch > 'h')) return SQ_NONE;

    char rank_ch = coord[1];
    if ((rank_ch < '1') || (rank_ch > '8')) return SQ_NONE;

    return SQ_AT(rank_ch - '1', file_ch - 'a');
}


void sq_to_coord(int sq, char out[3]) {
    if (SQ_OFFBOARD(sq)) {
        out[0] = '-';
        out[1] = '-';
        out[2] = '\0';
        return;
    }

    out[0] = (char)('a' + FILE_OF(sq));
    out[1] = (char)('1' + RANK_OF(sq));
    out[2] = '\0';
}