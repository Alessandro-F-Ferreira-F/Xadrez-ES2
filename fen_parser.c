#include "fen_parser.h"

#define FEN_PARSE_FIELDS 6

const char PIECE_CHAR[17] = ".pnbrqk..PNBRQK.";

/*
* DEFINIÇÕES DE FUNÇÕES  
*/ 
static int square_from_coord(const char *coord);
static int piece_from_char(char ch);
static bool is_fen_piece(char ch);
static bool valid_board_placement(const char *fen, Board *board);



char* board_to_fen(Piece *board) {
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

static int piece_from_char(char ch) {
    PieceType piece_type;
    Color color;
    Piece piece;

    switch (toupper((unsigned char)ch))
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

    if (isupper((unsigned char)ch)) {
        color = WHITE;
    } else color = BLACK;

    piece = MAKE_PIECE(color, piece_type);
    return piece;
}


/* validate_fen() verifica se a string FEN possui algum destes invariantes:
Tabuleiro:
 - exatamente 8 ranks;
 - cada rank contém exatamente 8 casas;
 - apenas peças válidas pnbrqkPNBRQK;
 - números apenas de 1 a 8;
 - exatamente um rei branco e um preto;
 - no máximo 8 peões de cada cor;
 - nenhum peão na primeira ou oitava rank.
Campos da FEN
 - side to move = w ou b;
 - castling = - ou combinação válida de KQkq, sem repetição;
 - en passant = - ou casa válida;
 - halfmove clock = inteiro >= 0;
 - fullmove number = inteiro >= 1.
Consistência básica
 - se existe K, deve existir rei branco em e1 e torre branca em h1;
 - Q → rei branco em e1 e torre em a1;
 - k → rei preto em e8 e torre em h8;
 - q → rei preto em e8 e torre em a8;
 - os dois reis não podem estar adjacentes.
*/


bool parse_fen(const char *fen_string, Board *out) {
    if (fen_string == NULL || out == NULL) {
        LOG_ERROR("null argument");
        return false;
    }

    if (strlen(fen_string) >= MAX_FEN_STRING) {
        LOG_ERROR("fen string too long");
        return false;
    }

    Board b = {0};

    char copy[MAX_FEN_STRING];
    strncpy(copy, fen_string, MAX_FEN_STRING - 1);
    copy[MAX_FEN_STRING - 1] = '\0';

    char *fields[FEN_PARSE_FIELDS];
    char *token = strtok(copy, " \t\r\n");

    for (int i = 0; i < FEN_PARSE_FIELDS; i++) {
        if (token == NULL) {
            LOG_ERROR("invalid number of fields in FEN string");
            return false;
        }

        fields[i] = token;

        token = strtok(NULL, " \t\r\n");
    }

    //* Nao pode existir um setimo campo.

    if (token != NULL) {
        LOG_ERROR("invalid number of fields in FEN string");
        return false;
    }

    if (!valid_board_placement(fields[0], &b)) {
        LOG_ERROR("invalid board placement");
        return false;
    }

    if (strcmp(fields[1], "w") != 0 && strcmp(fields[1], "b") != 0) {
        LOG_ERROR("invalid side to move");
        return false;
    }

    b.side_to_move = (fields[1][0] == 'w') ? WHITE : BLACK;

    *out = b;

    return true;
    /*
    * Falta implementar validacoes de:
        - ROQUE
        - EN PASSANT
        - HALFMOVE CLOCK
        - NUMERO TOTAL DE JOGADAS
    */
}



/* FUNÇÕES AUXILIARES */

static int square_from_coord(const char *coord) {
    char file_str = coord[0];
    char rank_str = coord[1];

    if ((file_str < 'a') || (file_str > 'h')) return -1;
    if ((rank_str < '1') || (rank_str > '8')) return -1;

    int file = file_str - 'a';
    int rank = rank_str - '1';
    int square = rank * 8 + file;

    return square;
}

static bool is_fen_piece(char ch) {
    return (ch != '\0') && (strchr("pnbrqkPNBRQK", ch) != NULL); 
}

static bool valid_board_placement(const char *fen, Board *board) {
    /* indexados por Color: BLACK == 0, WHITE == 1 */
    int kings[2]  = {0};
    int pawns[2]  = {0};
    int pieces[2] = {0};

    int rank = 0;
    int file = 0;
    int sq = 0;

    for (const char *p = fen; *p != '\0'; p++) {
        char ch = *p;

        if (ch == '/') {
            if (file != 8) {
                LOG_ERROR("rank does not add up to 8 files");
                return false;
            }
            rank++;
            file = 0;
            if (rank >= 8) {
                LOG_ERROR("too many ranks in board placement");
                return false;
            }
            continue;
        }

        if (isdigit((unsigned char)ch)) {
            if ((ch < '1') || (ch > '8')) {
                LOG_ERROR("fen digit out of limits");
                return false;
            }

            int n = ch - '0';
            if ((file + n > 8) || (sq + n > BOARD_SIZE)) {
                LOG_ERROR("empty run overflows the rank");
                return false;
            }

            sq += n;
            file += n;
            continue;
        }

        if (is_fen_piece(ch)) {
            if ((file >= 8) || (sq >= BOARD_SIZE)) {
                LOG_ERROR("too many squares in rank");
                return false;
            }

            int decoded = piece_from_char(ch);
            if (decoded < 0) {
                LOG_ERROR("invalid piece character");
                return false;
            }

            Piece piece = (Piece)decoded;
            PieceType type = TYPE_OF(piece);
            Color color = COLOR_OF(piece);

            board->board[sq++] = piece;
            file++;
            pieces[color]++;

            if (type == KING) {
                kings[color]++;
            }

            if (type == PAWN) {
                pawns[color]++;

                if (rank == 0 || rank == 7) {
                    LOG_ERROR("pawn on a back rank");
                    return false;
                }
            }
            continue;
        }

        LOG_ERROR("invalid character in board placement");
        return false;
    }

    if ((rank != 7) || (file != 8)) {
        LOG_ERROR("board placement does not cover 64 squares");
        return false;
    }

    if ((kings[WHITE] != 1) || (kings[BLACK] != 1)) {
        LOG_ERROR("board must have exactly one king of each color");
        return false;
    }

    if ((pawns[WHITE] > 8) || (pawns[BLACK] > 8)) {
        LOG_ERROR("too many pawns of one color");
        return false;
    }

    if ((pieces[WHITE] > 16) || (pieces[BLACK] > 16)) {
        LOG_ERROR("too many pieces of one color");
        return false;
    }

    return true;
}
