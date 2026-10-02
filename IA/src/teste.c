#include "fen.h"
#include "avaliar.h"
#include <stdio.h>
#include <stdbool.h>

int main(void){

    Board board;
    char FEN [] = "3qk3/8/8/8/8/8/8/4K3 w - - 0 1";

    bool test = fen_parse(FEN, &board);

    if(test){
        int32_t evaluate = avaliar(&board);
        printf("%d\n", evaluate);
        if(evaluate != -900){
            printf("Erro de evaluaiton\n");
            return 1;
        }
    }
    else{
        printf("Erro de parser\n");
        return 1;
    }

    return 0;
}
