// MinGW: printf ANSI, para PRIu64 sem aviso do -Wpedantic (sem efeito fora do MinGW). Precisa vir antes de qualquer include
#define __USE_MINGW_ANSI_STDIO 1

#include "fen.h"
#include "perft.h"
#include "avaliar.h"
#include "ia.h"
#include "stubs.h"
#include "movegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
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

// Profundidade dos testes de busca (go depth), sem limite de tempo
#define PROF_TESTE 4

typedef struct CasoEscolherJogada{
    const char *fen;
    int variante;
    int randomizador;
    int profundidade;
    int tempoMs;
    int testarEmpate;
    int8_t resultadoEsperado;
} CasoEscolherJogada;

static const CasoEscolherJogada casosEscolherJogada[] = {
    // Parâmetros inválidos (o stub não chega a ser consultado)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      2,                  SORTEIO_DESLIGADO, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_VARIANTE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   2,                 PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RANDOMIZADOR_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      2,                  2,                 PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_VARIANTE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, 0,            TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_PROFUNDIDADE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, PROF_MAX + 1, TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_PROFUNDIDADE_INVALIDA},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, PROF_TESTE,   -1,               SEM_EMPATE,        RESULTADO_TEMPO_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   2,                 0,            -1,               SEM_EMPATE,        RESULTADO_RANDOMIZADOR_INVALIDO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, 0,            -1,               SEM_EMPATE,        RESULTADO_PROFUNDIDADE_INVALIDA},

    // Empate por regra (valor devolvido pelo stub)
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, PROF_TESTE,   TEMPO_SEM_LIMITE, EMPATE_PROVISORIO, RESULTADO_EMPATE_PROVISORIO},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, PROF_TESTE,   TEMPO_SEM_LIMITE, 99,                RESULTADO_ERRO_INTERNO},

    // Raiz sem lances legais (FENs reais)
    {"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RAIZ_MATE},
    {"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",                                VARIANTE_MINIMAX,   SORTEIO_DESLIGADO, PROF_TESTE,   TEMPO_SEM_LIMITE, SEM_EMPATE,        RESULTADO_RAIZ_AFOGAMENTO},
};

// v1 = v2: minimax e alfa-beta devem devolver o mesmo lance, score e tipo (SORTEIO_DESLIGADO).
// Sem esperado fixo: o esperado é o próprio minimax.
// Válido para PROF_TESTE sem limite de tempo e enquanto as duas variantes percorrerem a raiz na mesma ordem
// (com ordenação de lances, o lance pode diferir entre candidatos empatados).
// iteracoes: última iteração que deve rodar. Sem mate, PROF_TESTE; com mate em p meios-lances,
// a busca para na iteração p + 1, a primeira que o vê
typedef struct CasoV1V2{
    const char *fen;
    int iteracoes;
} CasoV1V2;

static const CasoV1V2 casosV1V2[] = {
    // Muitos lances empatados na raiz: a ordem dos candidatos importa
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",                PROF_TESTE},
    {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1",              PROF_TESTE},

    // Mates em 1 (p = 1) e em 2 (p = 3), nos dois lados
    {"6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1",                                       2},
    {"r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1",                                       2},
    {"6k1/8/8/8/8/8/8/RR4K1 w - - 0 1",                                         4},
    {"rr4k1/8/8/8/8/8/8/6K1 b - - 0 1",                                         4},

    // Ganho de material, nos dois lados
    {"4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1",                                        PROF_TESTE},
    {"3qk3/8/8/8/3Q4/8/8/4K3 b - - 0 1",                                        PROF_TESTE},

    // Meio-jogo carregado (kiwipete)
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",    PROF_TESTE},
};

#define FOLGA_TEMPO_MS 50

typedef struct CasoTempo{
    int variante;
    int tempoMs;
} CasoTempo;

// 1 ms: só a iteração 1 (relógio desligado) cabe; o prazo já passou ao fim dela
static const CasoTempo casosTempo[] = {
    {VARIANTE_ALFA_BETA, 1},
    {VARIANTE_ALFA_BETA, 100},
    {VARIANTE_MINIMAX,   100},
};

// Sementes 1..NUM_SEMENTES no teste do sorteio
#define NUM_SEMENTES 8


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
            CANDIDATOS.count = 1; // sentinela: só volta a 0 se escolherJogada esvaziar
            escolherJogada(&board, casosEscolherJogada[i].variante, casosEscolherJogada[i].randomizador,
                           casosEscolherJogada[i].profundidade, casosEscolherJogada[i].tempoMs);
            printf(" % " PRId8 " % " PRId8 "\n", JOGADA.resultado, casosEscolherJogada[i].resultadoEsperado);        

            if(JOGADA.resultado != casosEscolherJogada[i].resultadoEsperado || JOGADA.lance != MOVE_NONE
            || JOGADA.score != SCORE_INVALIDO || JOGADA.tipo != TIPO_INVALIDO
            || NOS_VISITADOS != 0 || CANDIDATOS.count != 0) retornoTesteEscolherJogada = 1; // sem sucesso na verificação, a busca não roda
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
        if(!fen_parse(casosV1V2[i].fen, &board)){
            retornoV1V2 = 1;
            continue;
        }
        // Esperado exato do minimax: sem empate por regra, a iteração de limite D visita todo nó
        // até D, e nós terminais não têm filhos, como no perft. Soma das iterações D = 1..iteracoes.
        // O perft ignora a regra dos 50 lances; o minimax corta no empate. Para a soma valer
        // também com a regra real, o halfmove_clock não pode chegar a 100 dentro da árvore
        if(board.halfmove_clock + PROF_TESTE >= 100){
            printf("FEN v1 = v2 com halfmove alto demais para PROF_TESTE: %s\n", casosV1V2[i].fen);
            retornoNos = 1;
            continue;
        }
        uint64_t nosEsperados = 0;
        for(int limite = 1; limite <= casosV1V2[i].iteracoes; limite ++){
            for(int d = 0; d <= limite; d ++) nosEsperados += perft(&board, d);
        }

        fen_parse(casosV1V2[i].fen, &board);
        escolherJogada(&board, VARIANTE_MINIMAX, SORTEIO_DESLIGADO, PROF_TESTE, TEMPO_SEM_LIMITE);
        jogadaMinimax = JOGADA;
        uint64_t nosMinimax = NOS_VISITADOS;

        fen_parse(casosV1V2[i].fen, &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, PROF_TESTE, TEMPO_SEM_LIMITE);
        jogadaAlfaBeta = JOGADA;
        uint64_t nosAlfaBeta = NOS_VISITADOS;

        fen_parse(casosV1V2[i].fen, &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, PROF_TESTE, TEMPO_SEM_LIMITE);
        jogadaRepetida = JOGADA;
        uint64_t nosRepetida = NOS_VISITADOS;

        totalNosMinimax += nosMinimax;
        totalNosAlfaBeta += nosAlfaBeta;

        printf(" %u %u % " PRId32 " % " PRId32 " %" PRIu64 " %" PRIu64 " %" PRIu64 "\n", (unsigned) jogadaMinimax.lance, (unsigned) jogadaAlfaBeta.lance,
               jogadaMinimax.score, jogadaAlfaBeta.score, nosEsperados, nosMinimax, nosAlfaBeta);

        // Contagem de nós: minimax exato; alfa-beta nunca visita mais que o minimax (mesma ordem de lances)
        if(nosMinimax != nosEsperados || nosAlfaBeta > nosMinimax || nosRepetida != nosAlfaBeta) retornoNos = 1;

        // resultado == SUCESSO nas duas: sem isso, duas falhas iguais (MOVE_NONE) passariam no teste
        if(jogadaMinimax.resultado != RESULTADO_SUCESSO || jogadaAlfaBeta.resultado != RESULTADO_SUCESSO
        || jogadaMinimax.lance != jogadaAlfaBeta.lance || jogadaMinimax.score != jogadaAlfaBeta.score
        || jogadaMinimax.tipo != jogadaAlfaBeta.tipo) retornoV1V2 = 1;

        if(jogadaRepetida.resultado != jogadaAlfaBeta.resultado || jogadaRepetida.lance != jogadaAlfaBeta.lance
        || jogadaRepetida.score != jogadaAlfaBeta.score || jogadaRepetida.tipo != jogadaAlfaBeta.tipo) retornoV1V2 = 1;
    }

    // A poda precisa podar: no total, alfa-beta visita estritamente menos nós
    if(totalNosAlfaBeta >= totalNosMinimax) retornoNos = 1;

    printf(" total: %" PRIu64 " %" PRIu64 "\n\n", totalNosMinimax, totalNosAlfaBeta);

    // Teste do limite de tempo: profundidade PROF_MAX, que não termina no prazo.
    // O tempo gasto depende da máquina: só se exige que não passe muito do prazo (FOLGA_TEMPO_MS)

    int retornoTempo = 0;
    const char *fenTempo = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    char fenAntes[MAX_FEN_STRING], fenDepois[MAX_FEN_STRING];
    MoveList lancesRaiz;

    for(size_t i = 0; i < sizeof(casosTempo)/sizeof(casosTempo[0]); i ++){
        if(!fen_parse(fenTempo, &board)){
            retornoTempo = 1;
            continue;
        }
        generate_legal_moves(&board, &lancesRaiz);
        // Nós da iteração 1 completa: raiz + um nó por lance
        uint64_t nosIteracao1 = 1 + (uint64_t) lancesRaiz.count;
        fen_write(&board, fenAntes);

        clock_t inicio = clock();
        escolherJogada(&board, casosTempo[i].variante, SORTEIO_DESLIGADO, PROF_MAX, casosTempo[i].tempoMs);
        long gastoMs = (long)((clock() - inicio) * 1000 / CLOCKS_PER_SEC);

        fen_write(&board, fenDepois);

        bool lanceLegal = false;
        for(int j = 0; j < lancesRaiz.count; j ++){
            if(lancesRaiz.moves[j] == JOGADA.lance) lanceLegal = true;
        }

        printf(" %d %d %ld %u %" PRIu64 "\n", casosTempo[i].variante, casosTempo[i].tempoMs, gastoMs, (unsigned) JOGADA.lance, NOS_VISITADOS);

        if(JOGADA.resultado != RESULTADO_SUCESSO || !lanceLegal || strcmp(fenAntes, fenDepois) != 0
        || NOS_VISITADOS < nosIteracao1 || gastoMs > casosTempo[i].tempoMs + FOLGA_TEMPO_MS) retornoTempo = 1;
    }
    printf("\n");

    // Teste do sorteio: posição inicial, com muitos candidatos empatados em cp 0.
    // Desligado: o lance é o primeiro candidato. Ligado: o lance está entre os candidatos, o score
    // não muda, a mesma semente repete o lance e sementes diferentes não dão sempre o mesmo lance

    int retornoSorteio = 0;
    const char *fenSorteio = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    fen_parse(fenSorteio, &board);
    escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_DESLIGADO, PROF_TESTE, TEMPO_SEM_LIMITE);
    Play jogadaDesligado = JOGADA;
    MoveList candidatosDesligado = CANDIDATOS;

    // Com menos de 2 candidatos, o sorteio não teria o que sortear
    if(jogadaDesligado.resultado != RESULTADO_SUCESSO || candidatosDesligado.count < 2
    || jogadaDesligado.lance != candidatosDesligado.moves[0]) retornoSorteio = 1;

    bool lancesDistintos = false;

    for(unsigned int semente = 1; semente <= NUM_SEMENTES; semente ++){
        iniciarSorteio(semente);
        fen_parse(fenSorteio, &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_LIGADO, PROF_TESTE, TEMPO_SEM_LIMITE);
        Play jogadaSorteada = JOGADA;

        bool candidato = false;
        for(int j = 0; j < CANDIDATOS.count; j ++){
            if(CANDIDATOS.moves[j] == jogadaSorteada.lance) candidato = true;
        }

        iniciarSorteio(semente);
        fen_parse(fenSorteio, &board);
        escolherJogada(&board, VARIANTE_ALFA_BETA, SORTEIO_LIGADO, PROF_TESTE, TEMPO_SEM_LIMITE);

        printf(" %u %d %u %u\n", semente, CANDIDATOS.count, (unsigned) jogadaSorteada.lance, (unsigned) JOGADA.lance);

        if(jogadaSorteada.lance != jogadaDesligado.lance) lancesDistintos = true;

        if(jogadaSorteada.resultado != RESULTADO_SUCESSO || !candidato || CANDIDATOS.count != candidatosDesligado.count
        || jogadaSorteada.score != jogadaDesligado.score || jogadaSorteada.tipo != jogadaDesligado.tipo
        || JOGADA.lance != jogadaSorteada.lance) retornoSorteio = 1;
    }

    if(!lancesDistintos) retornoSorteio = 1;
    printf("\n");

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
    if(retornoTempo) printf("Erro no limite de tempo\n");
    if(retornoSorteio) printf("Erro no sorteio\n");

    if(retornoConversao || retornoEvaluation || retornoParser || retornoTesteEscolherJogada || retornoV1V2 || retornoNos || retornoTempo
    || retornoSorteio) return 1;
    return 0;
}
