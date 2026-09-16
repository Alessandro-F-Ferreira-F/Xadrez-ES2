#include "utils.h"
// #include <stdlib.h>


#define FAIL_MSGS_MAX 128

struct FailLog {
    char *msgs[FAIL_MSGS_MAX];
    int count;
};

static struct FailLog msg_log = {.count = 0};

void fail_msg(char *msg) {
    if (msg_log.count >= 128) {
        LOG_ERROR("too many fail messages");
        return;
    }
    msg_log.msgs[msg_log.count++] = msg;
}

void print_fail_log(void) {
    printf("[ == FAIL LOG == ]\n");
    for (int i = 0; i < msg_log.count; i++) {
        printf("Fail [%d]: %s\n", i, msg_log.msgs[i]);
    }
    printf("\n");
}

const char *COLOR_CHAR[2] = {"BLACK", "WHITE"};

/* 
strslc() realiza a operação de fatiar uma string:
    começando no índice src[start] até src[end]
*/
void strslc(const char *src, char *dest, int start, int end) {
    int length = end - start;
    strncpy(dest, src + start, length);
    dest[length] = '\0';
}


void print_piece_chart(void) {
    printf("\nTABELA DE PEÇAS\n");
    printf("====== ** ======\n");
    printf("Black pawn: %u\n", PIECE_MAKE(BLACK, PAWN));
    printf("Black knight: %u\n", PIECE_MAKE(BLACK, KNIGHT));
    printf("Black bishop: %u\n", PIECE_MAKE(BLACK, BISHOP));
    printf("Black rook: %u\n", PIECE_MAKE(BLACK, ROOK));
    printf("Black queen: %u\n", PIECE_MAKE(BLACK, QUEEN));
    printf("Black king: %u\n", PIECE_MAKE(BLACK, KING));
    printf("----------\n");
    printf("White pawn: %u\n", PIECE_MAKE(WHITE, PAWN));
    printf("White knight: %u\n", PIECE_MAKE(WHITE, KNIGHT));
    printf("White bishop: %u\n", PIECE_MAKE(WHITE, BISHOP));
    printf("White rook: %u\n", PIECE_MAKE(WHITE, ROOK));
    printf("White queen: %u\n", PIECE_MAKE(WHITE, QUEEN));
    printf("White king: %u\n", PIECE_MAKE(WHITE,KING));
}




/*
 * clear_screen() mora aqui e nao mais dentro de utils.h.
 *
 * Como 'static' no header, ela era recompilada em cada .c que incluia utils.h
 * e o GCC acusava "defined but not used" em todos eles. Warning que aparece em
 * todo build vira ruido que a equipe aprende a ignorar -- e o proximo, o de
 * verdade, passa junto.
 *
 * '\033' e nao '\e': '\e' e extensao do GCC e -Wpedantic reclama dela.
 */
void clear_screen(void) {
    printf("\033[1;1H\033[2J");
    fflush(stdout);
}

/*
 * fgets() devolve NULL em fim de arquivo ou erro. Ignorar o retorno deixava
 * 'fen' com lixo (ou com o conteudo da chamada anterior) quando a entrada
 * acabava -- um Ctrl-D no menu bastava. Agora o buffer vira string vazia,
 * que parse_fen rejeita normalmente.
 */
void get_fen(char fen[MAX_FEN_STRING]) {
    printf("Insert FEN: ");

    if (fgets(fen, MAX_FEN_STRING, stdin) == NULL) {
        fen[0] = '\0';
        return;
    }

    fen[strcspn(fen, "\r\n")] = '\0';   /* tira o '\n' que o fgets guarda */
}

int get_int(const char *msg) {
    if (msg == NULL) printf("Type int: ");
    else printf("%s", msg);

    char buffer[16];
    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        return 0;
    }

    return (int)strtol(buffer, NULL, 10);
}

