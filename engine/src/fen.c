/*
 * board.c — representação do tabuleiro, leitura e escrita de FEN.
 *
 * Responsabilidades deste módulo:
 *   - parse_fen()    : texto FEN  -> struct Board  (valida tudo antes de escrever)
 *   - board_to_fen() : struct Board -> texto FEN   (os 6 campos)
 *   - conversão de coordenada ("e4") <-> índice de casa (28)
 *   - impressão do tabuleiro para depuração
 *
 * Invariante central de parse_fen: ou a FEN inteira é válida e '*out' é
 * totalmente sobrescrito, ou '*out' não é tocado. Nunca existe um Board
 * meio-preenchido. Por isso a montagem acontece num Board local e só é
 * copiada para '*out' na última linha.
 */

#include "fen.h"

#include <errno.h>
#include "piece.h"
#include "square.h"
#include "log.h"

#define FEN_MIN_FIELDS    4
#define FEN_MAX_FIELDS    6
#define FEN_HALFMOVE_MAX  1000
#define FEN_FULLMOVE_MAX  9999

/*
 * Índice de PIECE_CHAR é o valor cru da Piece: (color << 3) | type.
 * Peão preto  = (0 << 3) | 1 =  1 -> 'p'
 * Peão branco = (1 << 3) | 1 =  9 -> 'P'
 * As posições 7, 8 e 15 são buracos na codificação e nunca são indexadas.
 */



/* Posição de cada campo dentro do vetor devolvido por split_fen_fields(). */
enum {
    FEN_PLACEMENT = 0,
    FEN_SIDE      = 1,
    FEN_CASTLING  = 2,
    FEN_EP        = 3,
    FEN_HALFMOVE  = 4,
    FEN_FULLMOVE  = 5
};


static int  split_fen_fields(char *fen_mut, char *fields[], int max_fields);
static bool is_fen_piece(char ch);

static bool parse_placement(const char *field, Board *b);
static bool parse_side_to_move(const char *field, Board *b);
static bool parse_castling(const char *field, Board *b);
static bool castling_matches_board(const Board *b);
static bool parse_ep_square(const char *field, Board *b);
static bool parse_uint_field(const char *field, int min, int max, int *out);


/* 
 * ========================================================================
 * FEN -> Board
 * ======================================================================== */

/*
 * Quebra a FEN em campos separados por espaço em branco.
 *
 * 'fen_mut' é modificado no lugar (strtok escreve '\0' sobre os separadores),
 * então tem que ser uma cópia mutável — nunca o literal do chamador.
 * 'fields' é fornecido pelo chamador; a função não aloca nada, seguindo a
 * convenção do projeto de preencher struct/vetor do chamador em vez de
 * devolver ponteiro. Os ponteiros gravados em 'fields' apontam PARA DENTRO
 * de 'fen_mut' e só são válidos enquanto ele existir.
 *
 * Devolve a quantidade de campos encontrados, ou -1 se houver campos demais.
 *
 * Nota: strtok guarda estado em variável estática interna — não é reentrante
 * e não pode ser chamada de dois lugares intercalados. A engine é single
 * thread e ninguém mais usa strtok, então está seguro; se isso mudar, trocar
 * por strtok_r (POSIX) ou por uma varredura manual.
 */

static bool is_fen_piece(char ch) {
    return (ch != '\0') && (strchr("pnbrqkPNBRQK", ch) != NULL);
}

static int split_fen_fields(char *fen_mut, char *fields[], int max_fields) {
    int count = 0;
    char *token = strtok(fen_mut, " \t\r\n");

    while ((token != NULL) && (count < max_fields)) {
        fields[count++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    /* Sobrou token depois de encher o vetor: a FEN tem campos demais. */
    if (token != NULL) {
        return -1;
    }

    return count;
}

/*
 * Lê uma FEN completa para '*out'.
 *
 * Aceita de 4 a 6 campos. Os campos 5 (halfmove clock) e 6 (fullmove number)
 * são opcionais porque bancos de posições e strings EPD costumam omiti-los;
 * quando faltam, assumem 0 e 1 respectivamente. Os quatro primeiros são
 * obrigatórios: sem eles não dá para saber de quem é a vez nem se o roque
 * ainda é possível.
 *
 * A ORDEM das etapas importa e não é arbitrária:
 *   1. peças    — as validações de roque e en passant consultam o tabuleiro;
 *   2. lado     — a fileira válida do en passant depende de quem joga;
 *   3. roque    — precisa do tabuleiro montado (etapa 1);
 *   4. en passant — precisa do tabuleiro (1) e do lado (2);
 *   5/6. relógios — independentes, ficam por último.
 *
 * Devolve true se a FEN é válida. Em qualquer falha devolve false, registra
 * o motivo por LOG_ERROR e deixa '*out' intacto.
 */
bool fen_parse(const char *fen_string, Board *out) {
    if ((fen_string == NULL) || (out == NULL)) {
        LOG_ERROR("null argument");
        return false;
    }

    if (strlen(fen_string) >= MAX_FEN_STRING) {
        LOG_ERROR("fen string too long");
        return false;
    }

    /* Cópia mutável: split_fen_fields escreve '\0' sobre os espaços, e o
       parâmetro de entrada é const. */
    char copy[MAX_FEN_STRING];
    strncpy(copy, fen_string, MAX_FEN_STRING - 1);
    copy[MAX_FEN_STRING - 1] = '\0';

    char *fields[FEN_MAX_FIELDS];
    int  field_count = split_fen_fields(copy, fields, FEN_MAX_FIELDS);

    if (field_count < 0) {
        LOG_ERROR("too many fields in FEN string");
        return false;
    }

    if (field_count < FEN_MIN_FIELDS) {
        LOG_ERROR("too few fields in FEN string");
        return false;
    }

    /* Board local: só vira o resultado se TUDO validar. */
    Board b = {0};
    b.ep_square = SQ_NONE;

    if (!parse_placement(fields[FEN_PLACEMENT], &b))   return false;
    if (!parse_side_to_move(fields[FEN_SIDE], &b))     return false;
    if (!parse_castling(fields[FEN_CASTLING], &b))     return false;
    if (!castling_matches_board(&b))                   return false;
    if (!parse_ep_square(fields[FEN_EP], &b))          return false;

    b.halfmove_clock  = 0;
    b.fullmove_number = 1;

    if (field_count > FEN_HALFMOVE) {
        if (!parse_uint_field(fields[FEN_HALFMOVE], 0, FEN_HALFMOVE_MAX,
                              &b.halfmove_clock)) {
            LOG_ERROR("invalid halfmove clock");
            return false;
        }
    }

    if (field_count > FEN_FULLMOVE) {
        if (!parse_uint_field(fields[FEN_FULLMOVE], 1, FEN_FULLMOVE_MAX,
                              &b.fullmove_number)) {
            LOG_ERROR("invalid fullmove number");
            return false;
        }
    }

    *out = b;
    return true;
}

/*
 * Campo 1 — disposição das peças.
 *
 * A FEN descreve o tabuleiro da fileira 8 para a 1, e cada fileira da coluna
 * a para a h. Com a indexação a1 = 0 do projeto, isso significa começar em
 * rank = 7 e decrementar: a primeira linha da FEN preenche os índices 56..63.
 *
 * Valida enquanto monta. Escreve direto em '*b' mesmo nos caminhos de erro —
 * é seguro porque parse_fen passa um Board local que é descartado se algo
 * falhar.
 */
static bool parse_placement(const char *field, Board *b) {
    /* Contadores indexados por Color; funciona porque BLACK == 0, WHITE == 1. */
    int kings[2]  = {0};
    int pawns[2]  = {0};
    int pieces[2] = {0};

    int rank = 7;
    int file = 0;

    for (const char *p = field; *p != '\0'; p++) {
        char ch = *p;

        if (ch == '/') {
            if (file != 8) {
                LOG_ERROR("rank does not add up to 8 files");
                return false;
            }
            rank--;
            file = 0;

            /* NECESSÁRIO: com 9 ou mais fileiras ("8/8/8/8/8/8/8/8/8"), rank
               chega a -1 e o cálculo sq = rank*8 + file fica negativo, o que
               seria uma escrita fora dos limites de board->array. O teste tem
               que vir ANTES de qualquer escrita.
               (rank >= 8 é impossível: rank só decrementa. Removido.) */
            if (rank < 0) {
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
            if (file + n > 8) {
                LOG_ERROR("empty run overflows the rank");
                return false;
            }

            file += n;
            continue;
        }

        if (is_fen_piece(ch)) {
            if (file >= 8) {
                LOG_ERROR("too many squares in rank");
                return false;
            }

            int decoded = piece_from_char(ch);
            if (decoded < 0) {
                LOG_ERROR("invalid piece character");
                return false;
            }

            Piece     piece = (Piece)decoded;
            PieceType type  = PIECE_TYPE(piece);
            Color     color = PIECE_COLOR(piece);

            int sq = SQ_AT(rank, file);

            b->array[sq] = piece;
            file++;
            pieces[color]++;

            if (type == KING) {
                kings[color]++;
                b->king_square[color] = sq;
            }

            if (type == PAWN) {
                pawns[color]++;
                if ((rank == 0) || (rank == 7)) {
                    LOG_ERROR("pawn on a back rank");
                    return false;
                }
            }
            continue;
        }

        LOG_ERROR("invalid character in board placement");
        return false;
    }

    if ((rank != 0) || (file != 8)) {
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

/* Campo 2 — lado a jogar. Exatamente "w" ou "b". */
static bool parse_side_to_move(const char *field, Board *b) {
    if (strcmp(field, "w") == 0) {
        b->side_to_move = WHITE;
        return true;
    }
    if (strcmp(field, "b") == 0) {
        b->side_to_move = BLACK;
        return true;
    }

    LOG_ERROR("invalid side to move");
    return false;
}

/*
 * Campo 3 — direitos de roque, como bitmask.
 *
 * "-" significa nenhum. Caso contrário, um subconjunto de "KQkq", cada letra
 * no máximo uma vez. Diferente da versão anterior, caractere desconhecido é
 * ERRO e não é ignorado em silêncio: "KQxq" antes virava KQq sem aviso, e uma
 * FEN corrompida passava despercebida até o perft acusar dezenas de lances a
 * mais lá na frente.
 */
static bool parse_castling(const char *field, Board *b) {
    b->castling_rights = 0;

    if (strcmp(field, "-") == 0) {
        return true;
    }

    if (strlen(field) > 4) {
        LOG_ERROR("castling field too long");
        return false;
    }

    for (const char *p = field; *p != '\0'; p++) {
        u8 bit;

        switch (*p) {
            case 'K': bit = CASTLE_WK; break;
            case 'Q': bit = CASTLE_WQ; break;
            case 'k': bit = CASTLE_BK; break;
            case 'q': bit = CASTLE_BQ; break;
            default:
                LOG_ERROR("invalid character in castling field");
                return false;
        }

        if (b->castling_rights & bit) {
            LOG_ERROR("duplicate castling right");
            return false;
        }

        b->castling_rights |= bit;
    }

    return true;
}

/*
 * Coerência entre o campo de roque e as peças no tabuleiro.
 *
 * Um direito de roque só faz sentido se o rei e a torre correspondentes ainda
 * estiverem nas casas de origem. "KQkq" num tabuleiro sem torre em h1 é uma
 * FEN inconsistente, e aceitá-la produz um lance de roque com uma torre que
 * não existe.
 *
 * A escolha aqui é REJEITAR, não corrigir em silêncio (algumas engines apenas
 * apagam o bit). O motivo é de depuração: quando board_to_fen() reescrever a
 * posição, um make_move que esqueceu de limpar o direito ao mover a torre
 * aparece na hora, em vez de virar uma divergência de perft na profundidade 5.
 * Se algum dia for preciso engolir FENs de bancos externos mal formados,
 * trocar os 'return false' por 'b->castling_rights &= ~need[i].bit'.
 *
 * Assume roque padrão (não Chess960): rei em e1/e8, torres em a1/h1/a8/h8.
 */
static bool castling_matches_board(const Board *b) {
    static const struct {
        u8  bit;
        int king_sq;
        int rook_sq;
    } need[4] = {
        { CASTLE_WK, SQ_E1, SQ_H1 },
        { CASTLE_WQ, SQ_E1, SQ_A1 },
        { CASTLE_BK, SQ_E8, SQ_H8 },
        { CASTLE_BQ, SQ_E8, SQ_A8 }
    };

    for (int i = 0; i < 4; i++) {
        if ((b->castling_rights & need[i].bit) == 0) {
            continue;
        }

        Color us = (need[i].bit & (CASTLE_WK | CASTLE_WQ)) ? WHITE : BLACK;

        if (b->array[need[i].king_sq] != PIECE_MAKE(us, KING)) {
            LOG_ERROR("castling right without the king on its home square");
            return false;
        }
        if (b->array[need[i].rook_sq] != PIECE_MAKE(us, ROOK)) {
            LOG_ERROR("castling right without the rook on its home square");
            return false;
        }
    }

    return true;
}

/*
 * Campo 4 — casa de en passant.
 *
 * "-" quando não há. Caso contrário é a casa ATRÁS do peão que acabou de dar
 * o avanço duplo — a casa onde o capturador vai parar, não onde o peão está.
 *
 * Consequência: a fileira é determinada por quem joga.
 *   brancas a jogar -> o último lance foi das pretas, de 7 para 5,
 *                      logo a casa de en passant está na fileira 6 (índice 5);
 *   pretas a jogar  -> lance branco de 2 para 4, casa na fileira 3 (índice 2).
 *
 * Validar também as três casas envolvidas custa quase nada e evita gerar uma
 * captura en passant fantasma:
 *   - a própria casa de en passant tem que estar vazia;
 *   - a casa de onde o peão saiu tem que estar vazia;
 *   - o peão que avançou tem que estar de fato lá, e ser do adversário.
 *
 * Repare na ordem do último teste: PIECE_TYPE() vem antes de PIECE_COLOR(), porque
 * PIECE_COLOR(EMPTY) devolve BLACK — casa vazia se disfarçaria de peão preto.
 * Como PIECE_TYPE(EMPTY) == EMPTY != PAWN, testar o tipo primeiro já barra.
 */
static bool parse_ep_square(const char *field, Board *b) {
    b->ep_square = SQ_NONE;

    if (strcmp(field, "-") == 0) {
        return true;
    }

    if (strlen(field) != 2) {
        LOG_ERROR("en passant field must be '-' or a two-character square");
        return false;
    }

    int sq = sq_from_coord(field);
    if (sq == SQ_NONE) {
        LOG_ERROR("invalid en passant square");
        return false;
    }

    int expected_rank = (b->side_to_move == WHITE) ? 5 : 2;
    if (RANK_OF(sq) != expected_rank) {
        LOG_ERROR("en passant square on the wrong rank for the side to move");
        return false;
    }

    /* Fileira 5 ou 2 garante que ambos os deslocamentos caem no tabuleiro. */
    int   pawn_sq   = (b->side_to_move == WHITE) ? (sq - 8) : (sq + 8);
    int   origin_sq = (b->side_to_move == WHITE) ? (sq + 8) : (sq - 8);
    Color pusher    = (b->side_to_move == WHITE) ? BLACK : WHITE;

    if (b->array[sq] != EMPTY) {
        LOG_ERROR("en passant square is occupied");
        return false;
    }
    if (b->array[origin_sq] != EMPTY) {
        LOG_ERROR("square behind the en passant target is occupied");
        return false;
    }
    if ((PIECE_TYPE(b->array[pawn_sq]) != PAWN) ||
        (PIECE_COLOR(b->array[pawn_sq]) != pusher)) {
        LOG_ERROR("no enemy pawn to be captured en passant");
        return false;
    }

    b->ep_square = sq;
    return true;
}

/*
 * Campos 5 e 6 — inteiros sem sinal, dentro de [min, max].
 *
 * A versão anterior usava strtol(field, NULL, 10) e aceitava qualquer coisa:
 * "abc" virava 0 sem reclamar. Aqui o campo precisa ser não vazio e conter
 * só dígitos, e o resultado precisa caber na faixa. ERANGE cobre o caso de
 * um número absurdamente grande.
 */
static bool parse_uint_field(const char *field, int min, int max, int *out) {
    if (*field == '\0') {
        LOG_ERROR("empty numeric field");
        return false;
    }

    for (const char *p = field; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) {
            LOG_ERROR("non-digit in numeric field");
            return false;
        }
    }

    errno = 0;
    long value = strtol(field, NULL, 10);

    if ((errno == ERANGE) || (value < min) || (value > max)) {
        LOG_ERROR("numeric field out of range");
        return false;
    }

    *out = (int)value;
    return true;
}


/* ========================================================================
 * Board -> FEN
 * ======================================================================== */

/*
 * Escreve a posição como FEN completa, os 6 campos.
 *
 * A versão anterior emitia só o campo das peças e terminava com
 *     fen_out[pos++] = '\0';  fen_out[pos] = ' ';
 * ou seja, gravava um espaço DEPOIS do terminador, onde ninguém lê. O efeito
 * prático era um round-trip mentiroso: parse_fen -> board_to_fen -> parse_fen
 * perdia lado a jogar, roque, en passant e relógios, e a segunda leitura
 * falhava por falta de campos. Como o servidor JS vai receber exatamente esta
 * string, ela precisa carregar a posição inteira.
 *
 * 'fen_out' precisa ter MAX_FEN_STRING bytes. O pior caso real é ~90 bytes
 * (71 de peças + 19 do resto), bem abaixo dos 256, mas os limites são
 * checados mesmo assim: a função não pode estourar o buffer nem que a Board
 * chegue com lixo.
 */
void fen_write(const Board *board, char fen_out[MAX_FEN_STRING]) {
    int pos = 0;

    /* Campo 1: peças, da fileira 8 para a 1. */
    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;

        for (int file = 0; file < 8; file++) {
            Piece p = board->array[SQ_AT(rank, file)];

            if (p == EMPTY) {
                empty++;
                continue;
            }

            if (empty > 0) {
                fen_out[pos++] = (char)('0' + empty);
                empty = 0;
            }

            fen_out[pos++] = piece_to_char(p);
        }

        if (empty > 0) {
            fen_out[pos++] = (char)('0' + empty);
        }

        if (rank > 0) {
            fen_out[pos++] = '/';
        }
    }

    /* Campo 2: lado a jogar. */
    fen_out[pos++] = ' ';
    fen_out[pos++] = (board->side_to_move == WHITE) ? 'w' : 'b';

    /* Campo 3: roque. A ordem KQkq é obrigatória pela especificação da FEN —
       "qkQK" descreve a mesma posição mas não é uma FEN válida. */
    fen_out[pos++] = ' ';
    if (board->castling_rights == 0) {
        fen_out[pos++] = '-';
    } else {
        if (board->castling_rights & CASTLE_WK) fen_out[pos++] = 'K';
        if (board->castling_rights & CASTLE_WQ) fen_out[pos++] = 'Q';
        if (board->castling_rights & CASTLE_BK) fen_out[pos++] = 'k';
        if (board->castling_rights & CASTLE_BQ) fen_out[pos++] = 'q';
    }

    /* Campo 4: casa de en passant. */
    fen_out[pos++] = ' ';
    if (SQ_OFFBOARD(board->ep_square)) {
        fen_out[pos++] = '-';
    } else {
        char coord[3];
        sq_to_coord(board->ep_square, coord);
        fen_out[pos++] = coord[0];
        fen_out[pos++] = coord[1];
    }

    /* Campos 5 e 6: relógios. snprintf devolve quantos bytes ESCREVERIA, que
       pode passar do espaço disponível; por isso o resultado é limitado antes
       de virar índice. */
    int left    = MAX_FEN_STRING - pos;
    int written = snprintf(fen_out + pos, (size_t)left, " %d %d",
                           board->halfmove_clock, board->fullmove_number);

    if ((written < 0) || (written >= left)) {
        /* Truncou: termina onde der e sinaliza. Não deveria acontecer. */
        fen_out[MAX_FEN_STRING - 1] = '\0';
        LOG_ERROR("fen output truncated");
        return;
    }

    fen_out[pos + written] = '\0';
}