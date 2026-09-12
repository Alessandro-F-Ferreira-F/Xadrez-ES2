#include "utils.h"

/* 
strslc() realiza a operação de fatiar uma string:
    começando no índice src[start] até src[end]
*/

const char *COLOR_CHAR[2] = {"BLACK", "WHITE"};

void strslc(const char *src, char *dest, int start, int end) {
    int length = end - start;
    strncpy(dest, src + start, length);
    dest[length] = '\0';
}


void print_piece_chart(void) {
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

