#include "fen.h"
#include "avaliar.h"
#include "ia.h"
#include "stubs.h"
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

typedef struct CasoConversao{
    int32_t scoreBruto;
    Color lado;
    int8_t tipoEsperado;
    int32_t scoreEsperado;
} CasoConversao;

static const CasoConversao casos[] = {
    // Centipeão e perspectiva
    {150,                 WHITE, TIPO_CENTIPEAO,  150},
    {150,                 BLACK, TIPO_CENTIPEAO, -150},
    {-150,                BLACK, TIPO_CENTIPEAO,  150},
    {0,                   WHITE, TIPO_CENTIPEAO,  0},
    {0,                   BLACK, TIPO_CENTIPEAO,  0},

    // Mate, brancas na vez (p = 1..5)
    {MATE - 1,            WHITE, TIPO_MATE,  1},
    {-(MATE - 2),         WHITE, TIPO_MATE, -1},
    {MATE - 3,            WHITE, TIPO_MATE,  2},
    {-(MATE - 4),         WHITE, TIPO_MATE, -2},
    {MATE - 5,            WHITE, TIPO_MATE,  3},

    // Mate, pretas na vez (p = 1..4)
    {-(MATE - 1),         BLACK, TIPO_MATE,  1},
    {MATE - 2,            BLACK, TIPO_MATE, -1},
    {-(MATE - 3),         BLACK, TIPO_MATE,  2},
    {MATE - 4,            BLACK, TIPO_MATE, -2},

    // Fronteira do limiar
    {LIMIAR_MATE,         WHITE, TIPO_MATE,       (MATE - LIMIAR_MATE + 1) / 2},
    {-LIMIAR_MATE,        WHITE, TIPO_MATE,       -((MATE - LIMIAR_MATE + 1) / 2)},
    {LIMIAR_MATE - 1,     WHITE, TIPO_CENTIPEAO,  LIMIAR_MATE - 1},
    {-(LIMIAR_MATE - 1),  WHITE, TIPO_CENTIPEAO,  -(LIMIAR_MATE - 1)},
    {LIMIAR_MATE,         BLACK, TIPO_MATE,       -((MATE - LIMIAR_MATE + 1) / 2)},
    {LIMIAR_MATE - 1,     BLACK, TIPO_CENTIPEAO,  -(LIMIAR_MATE - 1)},
};

typedef struct CasoAvaliacao{
    const char *fen;
    int32_t esperado;
} CasoAvaliacao;

static const CasoAvaliacao casosAvaliacao[] = {
    // Posição inicial (simetria)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  0},

    // Extremos de material (v1)
    {"k7/8/8/8/8/1NBB4/1QQQRR1N/1QQQQQQK w - - 0 1",              10400},
    {"1qqqqqqk/1qqqrr1n/1nbb4/8/8/8/8/K7 b - - 0 1",             -10400},

    // Varredura inclui a1 e h8
    {"7R/8/8/3k4/8/8/8/R3K3 w - - 0 1",                           1000},

    // Uma peça por vez, nos dois ramos de cor
    {"4k3/8/8/8/8/8/4P3/4K3 w - - 0 1",                           100},
    {"4k3/4p3/8/8/8/8/8/4K3 w - - 0 1",                          -100},
    {"4k3/8/8/8/8/8/8/1N2K3 w - - 0 1",                           320},
    {"1n2k3/8/8/8/8/8/8/4K3 w - - 0 1",                          -320},
    {"4k3/8/8/8/8/8/8/2B1K3 w - - 0 1",                           330},
    {"2b1k3/8/8/8/8/8/8/4K3 w - - 0 1",                          -330},
    {"4k3/8/8/8/8/8/8/R3K3 w - - 0 1",                            500},
    {"r3k3/8/8/8/8/8/8/4K3 w - - 0 1",                           -500},
    {"4k3/8/8/8/8/8/8/3QK3 w - - 0 1",                            900},
    {"3qk3/8/8/8/8/8/8/4K3 w - - 0 1",                           -900},
};

typedef struct CasoEscolherJogada{
    const char *fen;
    int variante;
    int randomizador;
    int testarEmpate;
    int8_t resultadoEsperado;
} CasoEscolherJogada;

static const CasoEscolherJogada casosEscolherJogada[] = {
    // Parâmetros inválidos (o stub não chega a ser consultado)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      2,                  SORTEIO_DESLIGADO, SEM_EMPATE,        RESULTADO_VARIANTE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   2,                 SEM_EMPATE,        RESULTADO_RANDOMIZADOR_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      2,                  2,                 SEM_EMPATE,        RESULTADO_VARIANTE_INVALIDA},

    // Empate por regra (valor devolvido pelo stub)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, EMPATE_PROVISORIO, RESULTADO_EMPATE_PROVISORIO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, 99,                RESULTADO_ERRO_INTERNO},

    // Raiz sem lances legais (FENs reais)
    {"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, SEM_EMPATE,        RESULTADO_RAIZ_MATE},
    {"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",                                VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, SEM_EMPATE,        RESULTADO_RAIZ_AFOGAMENTO},
};


int main(void){
    init_square_tables();

    Board board;
    int retornoTesteEscolherJogada = 0;
    bool parsed;

    for(size_t i = 0; i < sizeof(casosEscolherJogada)/sizeof(casosEscolherJogada[0]); i ++){
        parsed = fen_parse(casosEscolherJogada[i].fen, &board);

        if(parsed){
            varTesteEmpate = casosEscolherJogada[i].testarEmpate;
            escolherJogada(&board, casosEscolherJogada[i].variante, casosEscolherJogada[i].randomizador);
            printf(" % " PRId8 " % " PRId8 "\n", JOGADA.resultado, casosEscolherJogada[i].resultadoEsperado);        

            if(JOGADA.resultado != casosEscolherJogada[i].resultadoEsperado || JOGADA.lance != LANCE_NULO
            || JOGADA.score != SCORE_INVALIDO || JOGADA.tipo != TIPO_INVALIDO) retornoTesteEscolherJogada = 1;
        }
        else retornoTesteEscolherJogada = 1;
    }

    // Teste dos casos da avaliação do score do tabuleiro 

    int retornoEvaluation = 0;
    int retornoParser = 0;
    int32_t evaluation;
    bool test;
    
    for(size_t i = 0; i < sizeof(casosAvaliacao)/sizeof(casosAvaliacao[0]); i ++){
        test = fen_parse(casosAvaliacao[i].fen, &board);

        if(test){
            evaluation = avaliar(&board);

            if(evaluation != casosAvaliacao[i].esperado){
                retornoEvaluation = 1;
            }
            else{
                printf(" % " PRId32 " % " PRId32 "\n", casosAvaliacao[i].esperado, evaluation);
            }
        }
        else{
            printf("Erro de parser\n");
            retornoParser = 1;
        }

    }

    printf("\n");

    // Teste dos casos de conversão do score

    int retornoConversao = 0;
    Move lanceAntes = -1;
    int8_t resultadoAntes = -1;

    JOGADA.lance = lanceAntes;
    JOGADA.resultado = resultadoAntes;

    for(size_t i = 0; i < sizeof(casos)/sizeof(casos[0]); i ++){
        converterScore(casos[i].scoreBruto, casos[i].lado, &JOGADA);
        
        printf(" % " PRId32 " % " PRId8 " % " PRId32 " % " PRId8 "\n", casos[i].scoreEsperado, casos[i].tipoEsperado, JOGADA.score, JOGADA.tipo);

        if(!retornoConversao && (casos[i].scoreEsperado != JOGADA.score || casos[i].tipoEsperado != JOGADA.tipo || JOGADA.lance != lanceAntes || JOGADA.resultado != resultadoAntes)){
            retornoConversao = 1;            
        }

    }
    
    if(retornoParser) printf("Erro de parser\n");
    if(retornoEvaluation) printf("Erro de avaliaçao\n");
    if(retornoConversao) printf("erro de conversao\n");
    if(retornoTesteEscolherJogada) printf("Erro na escolha de jogada\n");

    if(retornoConversao || retornoEvaluation || retornoParser || retornoTesteEscolherJogada) return 1;
    return 0;
}
