#define __USE_MINGW_ANSI_STDIO 1

#include "fen.h"
#include "square.h"
#include "ia.h"
#include "stubs.h"
#include "movegen.h"
#include "niveis.h"
#include "ia_test_common.h"
#include "../../tools/ctest/test_api.h"

#include <stdbool.h>
#include <string.h>
#include <time.h>

#define FOLGA_TEMPO_MS 50

typedef struct CasoTempo {
    int variante;
    int quiescencia;
    int tempoMs;
} CasoTempo;

static const CasoTempo casosTempo[] = {
    {VARIANTE_ALFA_BETA, QUIESCENCIA_DESLIGADA, 1},
    {VARIANTE_ALFA_BETA, QUIESCENCIA_DESLIGADA, 100},
    {VARIANTE_MINIMAX,   QUIESCENCIA_DESLIGADA, 100},
    {VARIANTE_ALFA_BETA, QUIESCENCIA_LIGADA,    100},
    {VARIANTE_MINIMAX,   QUIESCENCIA_LIGADA,    100},
};

typedef struct CasoNivel {
    int nivel;
    ParametrosNivel esperado;
    int respostaMaxMs;
} CasoNivel;

static const CasoNivel casosNiveis[] = {
    {NIVEL_FACIL,   {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_DESLIGADA, 2,        180}, RESPOSTA_MAX_BASICO_MS},
    {NIVEL_MEDIO,   {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_LIGADA,    3,        180}, RESPOSTA_MAX_BASICO_MS},
    {NIVEL_DIFICIL, {VARIANTE_ALFA_BETA, SORTEIO_LIGADO, QUIESCENCIA_LIGADA,    PROF_MAX, 950}, RESPOSTA_MAX_DIFICIL_MS},
};

static const int niveisInvalidos[] = {NIVEL_FACIL - 1, NIVEL_DIFICIL + 1, -1};

static bool mesmosParametros(const ParametrosNivel *a, const ParametrosNivel *b) {
    return a->variante == b->variante && a->randomizador == b->randomizador &&
           a->quiescencia == b->quiescencia && a->profundidade == b->profundidade &&
           a->tempoMs == b->tempoMs;
}

TEST(ia_limite_tempo) {
    Board board;
    MoveList lancesRaiz;
    char fenAntes[MAX_FEN_STRING], fenDepois[MAX_FEN_STRING];
    varTesteEmpate = SEM_EMPATE;

    for (size_t i = 0; i < sizeof(casosTempo) / sizeof(casosTempo[0]); i++) {
        ASSERT(fen_parse(FEN_KIWIPETE, &board));
        generate_legal_moves(&board, &lancesRaiz);
        uint64_t nosIteracao1 = 1 + (uint64_t)lancesRaiz.count;
        fen_write(&board, fenAntes);

        clock_t inicio = clock();
        escolherJogada(&board, casosTempo[i].variante, SORTEIO_DESLIGADO,
                       casosTempo[i].quiescencia, PROF_MAX, casosTempo[i].tempoMs);
        long gastoMs = (long)((clock() - inicio) * 1000 / CLOCKS_PER_SEC);

        fen_write(&board, fenDepois);

        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT(ia_contem_lance(&lancesRaiz, JOGADA.lance));
        ASSERT_EQ_STR(fenAntes, fenDepois);
        ASSERT(CANDIDATOS.count > 0);
        for (int j = 0; j < CANDIDATOS.count; j++) {
            ASSERT(ia_contem_lance(&lancesRaiz, CANDIDATOS.moves[j]));
        }
        ASSERT(ia_contem_lance(&CANDIDATOS, JOGADA.lance));
        if (casosTempo[i].quiescencia == QUIESCENCIA_DESLIGADA) {
            ASSERT(PROF_ALCANCADA >= 1);
        }
        if (PROF_ALCANCADA == 0) {
            ASSERT(!PARCIAL_USADA);
        }
        if (casosTempo[i].variante == VARIANTE_MINIMAX) {
            ASSERT(!PARCIAL_USADA);
        }
        ASSERT(NOS_VISITADOS >= nosIteracao1);
        ASSERT_MSG(gastoMs <= casosTempo[i].tempoMs + FOLGA_TEMPO_MS,
                   "Tempo excedido: %ld ms (teto %d + %d ms)",
                   gastoMs, casosTempo[i].tempoMs, FOLGA_TEMPO_MS);
    }
}

TEST(ia_niveis) {
    Board board;
    ParametrosNivel parametros;
    const ParametrosNivel sentinelaNivel = {-7, -7, -7, -7, -7};
    varTesteEmpate = SEM_EMPATE;

    for (size_t i = 0; i < sizeof(niveisInvalidos) / sizeof(niveisInvalidos[0]); i++) {
        parametros = sentinelaNivel;
        ASSERT(!parametrosNivel(niveisInvalidos[i], &parametros));
        ASSERT(mesmosParametros(&parametros, &sentinelaNivel));
    }

    for (size_t i = 0; i < sizeof(casosNiveis) / sizeof(casosNiveis[0]); i++) {
        parametros = sentinelaNivel;
        ASSERT(parametrosNivel(casosNiveis[i].nivel, &parametros));
        ASSERT(mesmosParametros(&parametros, &casosNiveis[i].esperado));
        ASSERT(fen_parse(FEN_KIWIPETE, &board));

        clock_t inicio = clock();
        escolherJogada(&board, parametros.variante, parametros.randomizador,
                       parametros.quiescencia, parametros.profundidade, parametros.tempoMs);
        long gastoMs = (long)((clock() - inicio) * 1000 / CLOCKS_PER_SEC);

        ASSERT_EQ_INT(JOGADA.resultado, RESULTADO_SUCESSO);
        ASSERT(ia_contem_lance(&CANDIDATOS, JOGADA.lance));
        ASSERT(PROF_ALCANCADA <= parametros.profundidade);
        ASSERT_MSG(gastoMs < casosNiveis[i].respostaMaxMs,
                   "Nivel %d estourou RNF01: %ld ms >= %d ms",
                   casosNiveis[i].nivel, gastoMs, casosNiveis[i].respostaMaxMs);
    }
}

int main(void) {
    init_square_tables();
    return test_run_all();
}
