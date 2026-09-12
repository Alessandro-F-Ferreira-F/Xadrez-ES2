#include "piece.h"



const char PIECE_CHAR[17] = ".pnbrqk..PNBRQK.";

char piece_to_char(Piece p) {
    return PIECE_CHAR[p & PIECE_MASK];
}

Piece piece_from_char(char c) {
    const char *hit;

    if (c == '\0') {
        return NO_PIECE;
    }

    /*
     * strchr(str, ch) retorna um ponteiro para o primeiro caractere na string que é igual ao char c passado como parametro
     * Basta subtrair hit de PIECE_CHAR para encontrar o indice em PIECE_CHAR -- que equivale ao ID da peça 
     */
    hit = strchr(PIECE_CHAR, c);
    if (hit == NULL) {
        return NO_PIECE;
    }

    return ((Piece)(hit - PIECE_CHAR)); // subtração de ponteiros
}