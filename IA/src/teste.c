// MinGW: printf ANSI, para PRIu64 sem aviso do -Wpedantic (sem efeito fora do MinGW). Precisa vir antes de qualquer include
#define __USE_MINGW_ANSI_STDIO 1

#include "fen.h"
#include "perft.h"
#include "avaliar.h"
#include "ia.h"
#include "stubs.h"
#include <stdio.h>
#include <stdlib.h>
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

// v1 = v2: minimax e alfa-beta devem devolver o mesmo lance, score e tipo (SORTEIO_DESLIGADO).
// Sem esperado fixo: o esperado é o próprio minimax.
// Válido para o PROF_MAX compilado e enquanto as duas variantes percorrerem a raiz na mesma ordem
// (com ordenação de lances, o lance pode diferir entre candidatos empatados).
static const char *casosV1V2[] = {
    // Muitos lances empatados na raiz: a ordem dos candidatos importa
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",

    // Mates em 1 e em 2, nos dois lados
    "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",
    "r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1",
    "6k1/8/8/8/8/8/8/RR4K1 w - - 0 1",
    "rr4k1/8/8/8/8/8/8/6K1 b - - 0 1",

    // Ganho de material, nos dois lados
    "4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",
    "3qk3/8/8/8/3Q4/8/8/4K3 b - - 0 1",

    // Meio-jogo carregado (kiwipete)
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
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
            NOS_VISITADOS = 1; // sentinela: só volta a 0 se escolherJogada zerar
            escolherJogada(&board, casosEscolherJogada[i].variante, casosEscolherJogada[i].randomizador);
            printf(" % " PRId8 " % " PRId8 "\n", JOGADA.resultado, casosEscolherJogada[i].resultadoEsperado);        

            if(JOGADA.resultado != casosEscolherJogada[i].resultadoEsperado || JOGADA.lance != LANCE_NULO
            || JOGADA.score != SCORE_INVALIDO || JOGADA.tipo != TIPO_INVALIDO
            || NOS_VISITADOS != 0) retornoTesteEscolherJogada = 1; // sem sucesso na verificação, a busca não roda
        }
        else retornoTesteEscolherJogada = 1;
    }

    // Teste v1 = v2 (e determinismo: mesma variante duas vezes)

    int retornoV1V2 = 0;
    Play jogadaMinimax, jogadaAlfaBeta, jogadaRepetida;
    int retornoNos = 0;
    uint64_t totalNosMinimax = 0, totalNosAlfaBeta = 0;

    varTesteEmpate = SEM_EMPATE;

    for(size_t i = 0; i < sizeof(casosV1V2)/sizeof(casosV1V2[0]); i ++){
        if(!fen_parse(casosV1V2[i], &board)){
            retornoV1V2 = 1;
            continue;
        }
        // Esperado exato do minimax: sem empate por regra, ele visita todo nó até PROF_MAX,
        // e nós terminais não têm filhos, como no perft.
        // O perft ignora a regra dos 50 lances; o minimax corta no empate. Para a soma valer
        // também com a regra real, o halfmove_clock não pode chegar a 100 dentro da árvore
        if(board.halfmove_clock + PROF_MAX >= 100){
            printf("FEN v1 = v2 com halfmove alto demais para PROF_MAX: %s\n", casosV1V2[i]);
            retornoNos = 1;
            continue;
        }
        uint64_t nosEsperados = 0;
        for(int d = 0; d <= PROF_MAX; d ++) nosEsperados += perft(&board, d);

        fen_parse(casosV1V2[i], &board);
        escolherJogada(&board, VARIANTE_MINIMAX, SORTEIO_DESLIGADO);
        jogadaMinimax = JOGADA;
        uint64_t nosMinimax = NOS_VISITADOS;

        fen_parse(casosV1V2[i], &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO);
        jogadaAlfaBeta = JOGADA;
        uint64_t nosAlfaBeta = NOS_VISITADOS;

        fen_parse(casosV1V2[i], &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO);
        jogadaRepetida = JOGADA;
        uint64_t nosRepetida = NOS_VISITADOS;

        totalNosMinimax += nosMinimax;
        totalNosAlfaBeta += nosAlfaBeta;

        printf(" %u %u % " PRId32 " % " PRId32 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", (unsigned) jogadaMinimax.lance, (unsigned) jogadaAlfaBeta.lance,
               jogadaMinimax.score, jogadaAlfaBeta.score, nosEsperados, nosMinimax, nosAlfaBeta);

        // Contagem de nós: minimax exato; alfa-beta nunca visita mais que o minimax (mesma ordem de lances)
        if(nosMinimax != nosEsperados || nosAlfaBeta > nosMinimax || nosRepetida != nosAlfaBeta) retornoNos = 1;

        // resultado == SUCESSO nas duas: sem isso, duas falhas iguais (LANCE_NULO) passariam no teste
        if(jogadaMinimax.resultado != RESULTADO_SUCESSO || jogadaAlfaBeta.resultado != RESULTADO_SUCESSO
        || jogadaMinimax.lance != jogadaAlfaBeta.lance || jogadaMinimax.score != jogadaAlfaBeta.score
        || jogadaMinimax.tipo != jogadaAlfaBeta.tipo) retornoV1V2 = 1;

        if(jogadaRepetida.resultado != jogadaAlfaBeta.resultado || jogadaRepetida.lance != jogadaAlfaBeta.lance
        || jogadaRepetida.score != jogadaAlfaBeta.score || jogadaRepetida.tipo != jogadaAlfaBeta.tipo) retornoV1V2 = 1;
    }

    // A poda precisa podar: no total, alfa-beta visita estritamente menos nós
    if(totalNosAlfaBeta >= totalNosMinimax) retornoNos = 1;

    printf(" total: %" PRIu64 " %" PRIu64 "\n\n", totalNosMinimax, totalNosAlfaBeta);

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

    int casosPulados = 0;

    for(size_t i = 0; i < sizeof(casos)/sizeof(casos[0]); i ++){
        // Mate a p meios-lances (p = MATE - |scoreBruto|) só é classificado como mate se p <= PROF_MAX
        if(casos[i].tipoEsperado == TIPO_MATE && MATE - abs(casos[i].scoreBruto) > PROF_MAX){
            casosPulados ++;
            continue;
        }
        converterScore(casos[i].scoreBruto, casos[i].lado, &JOGADA);
        
        printf(" % " PRId32 " % " PRId8 " % " PRId32 " % " PRId8 "\n", casos[i].scoreEsperado, casos[i].tipoEsperado, JOGADA.score, JOGADA.tipo);

        if(!retornoConversao && (casos[i].scoreEsperado != JOGADA.score || casos[i].tipoEsperado != JOGADA.tipo || JOGADA.lance != lanceAntes || JOGADA.resultado != resultadoAntes)){
            retornoConversao = 1;            
        }

    }
    
    if(casosPulados) printf("Conversao: %d caso(s) de mate pulado(s) (p > PROF_MAX = %d)\n", casosPulados, PROF_MAX);
    if(retornoParser) printf("Erro de parser\n");
    if(retornoEvaluation) printf("Erro de avaliaçao\n");
    if(retornoConversao) printf("erro de conversao\n");
    if(retornoTesteEscolherJogada) printf("Erro na escolha de jogada\n");
    if(retornoV1V2) printf("Erro em v1 = v2\n");
    if(retornoNos) printf("Erro na contagem de nos\n");

    if(retornoConversao || retornoEvaluation || retornoParser || retornoTesteEscolherJogada || retornoV1V2 || retornoNos) return 1;
    return 0;
}
