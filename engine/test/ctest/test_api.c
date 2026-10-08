#include "test_api.h"
#include <inttypes.h>

/* 
Fase 1: Núcleo mínimo

Marco: dois testes (um passando, um falhando de propósito) rodando e reportando corretamente, com exit code certo.

Mecanismos de C para estudar:

Registro automático: __attribute__((constructor)) (GCC/Clang) faz uma função rodar antes do main. Cada TEST(nome) gera uma função de teste e um construtor que a adiciona a uma lista.
Macros de geração de nomes: ## (concatenação) e # (stringificação) do pré-processador.
Armazenamento: array estático com capacidade fixa ou lista encadeada? Para uso pessoal, um array de 1024 entradas basta. Que erro você quer dar se estourar?
Alternativa sem constructor: X-macros. É 100% padrão C, mas exige manter uma lista em um lugar só. Vale ler para comparar.

Pergunta para você decidir: o que o main do runner precisa fazer? (Percorrer o registro, executar, contar, imprimir resumo, retornar o código.)
*/

#include "test_api.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define C_RED    "\033[31m"
/* 256 cores (38;5;N): independe da paleta do tema, que costuma remapear o verde ANSI. */
#define C_GREEN  "\033[1;38;5;40m"
#define C_YELLOW "\033[33m"
#define C_BOLD   "\033[1m"
#define C_RESET  "\033[0m"

#define RULE "------------------------------------------"
#define MAX_FAILS 64

typedef struct Failure {
    const char *file;
    int line;
    const char *msg;
} Failure;

static TestCase g_tests[MAX_TESTS];
static size_t g_ntest = 0;
static int g_failed;
static int g_color;

/* Falhas do teste em execução: guardadas e impressas depois da linha [ FAIL ]. */
static Failure g_fails[MAX_FAILS];
static size_t g_nfails;

/* Devolve o código ANSI só se a saída for um terminal (e NO_COLOR não existir). */
static const char *paint(const char *code) {
    return g_color ? code : "";
}

void test_fail(const char *file, int line, const char *msg) {
    if (g_nfails < MAX_FAILS) {
        g_fails[g_nfails++] = (Failure){.file = file, .line = line, .msg = msg};
    }
    g_failed = 1;
}

void test_register(const char *name, test_fn fn, const char *file, int line) {
    if (g_ntest >= MAX_TESTS) {
        fprintf(stderr, "Too many tests\n");
        return;
    }
    TestCase new = {.name = name, .fn = fn, .file = file, .line = line};
    g_tests[g_ntest++] = new;
    return;
}

int test_run_all(void) {
    size_t failed_count = 0;
    size_t width = 0;   /* maior nome, para alinhar a coluna do resultado */

    g_color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");

    for (size_t i = 0; i < g_ntest; i++) {
        size_t len = strlen(g_tests[i].name);
        if (len > width) width = len;
    }

    printf("%sRunning %zu tests%s\n%s\n",
           paint(C_BOLD), g_ntest, paint(C_RESET), RULE);

    for (size_t i = 0; i < g_ntest; i++) {
        g_failed = 0;
        g_nfails = 0;

        /* fflush: se o teste travar ou abortar, o nome já está na tela. */
        printf("[ RUNNING ] %-*s . . .   ", (int)width, g_tests[i].name);
        fflush(stdout);
        g_tests[i].fn();

        if (g_failed) {
            printf("%s[ FAIL ]%s\n", paint(C_RED), paint(C_RESET));
            for (size_t j = 0; j < g_nfails; j++) {
                printf("\tat %s%s:%d%s  %s%s%s\n",
                       paint(C_YELLOW), g_fails[j].file, g_fails[j].line, paint(C_RESET),
                       paint(C_RED), g_fails[j].msg, paint(C_RESET));
            }
            failed_count++;
        } else {
            printf("%s[ OK ]%s\n", paint(C_GREEN), paint(C_RESET));
        }
    }

    printf("%s\n", RULE);
    printf("%sPASSED %zu/%zu%s", paint(failed_count ? C_RED : C_GREEN),
           g_ntest - failed_count, g_ntest, paint(C_RESET));
    if (failed_count) {
        printf("   %sFAILED %zu%s", paint(C_RED), failed_count, paint(C_RESET));
    }
    printf("\n");
    return failed_count ? 1 : 0;
}