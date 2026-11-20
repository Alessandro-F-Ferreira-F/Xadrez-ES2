#include "test_api.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>



#define C_RED    "\033[31m"
/* 256 cores (38;5;N): independe da paleta do tema, que costuma remapear o verde ANSI. */
#define C_GREEN  "\033[1;38;5;40m"
#define C_YELLOW "\033[33m"
#define C_BOLD   "\033[1m"
#define C_RESET  "\033[0m"

#define RULE "------------------------------------------"
#define MAX_FAILS 64
#define MSG_SIZE 512
/* Quantos caracteres antes/depois da diferença o EQ_STR mostra. */
#define DIFF_WINDOW 60

typedef struct Failure {
    const char *file;
    int line;
    char msg[MSG_SIZE];
} Failure;

static TestCase g_tests[MAX_TESTS];
static size_t g_ntest = 0;
static size_t g_not_registered = 0;   /* TESTs que não couberam em MAX_TESTS */
static int g_failed;
static int g_color;

/* Falhas do teste em execução: guardadas e impressas depois da linha [ FAIL ]. */
static Failure g_fails[MAX_FAILS];
static size_t g_nfails;
static size_t g_not_shown;            /* falhas além de MAX_FAILS no teste atual */

/* Devolve o código ANSI só se a saída for um terminal (e NO_COLOR não existir). */
static const char *paint(const char *code) {
    return g_color ? code : "";
}

void test_fail(const char *file, int line, const char *fmt, ...) {
    g_failed = 1;
    if (g_nfails >= MAX_FAILS) {
        g_not_shown++;
        return;
    }

    Failure *f = &g_fails[g_nfails++];
    f->file = file;
    f->line = line;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(f->msg, sizeof(f->msg), fmt, ap);
    va_end(ap);

    /* Mensagem cortada: termina com "..." e fecha qualquer cor que ficou aberta. */
    if (n >= (int)sizeof(f->msg)) {
        char tail[16];
        snprintf(tail, sizeof(tail), "...%s", paint(C_RESET));
        size_t tail_len = strlen(tail);
        memcpy(f->msg + sizeof(f->msg) - 1 - tail_len, tail, tail_len + 1);
    }
}

void test_register(const char *name, test_fn fn, const char *file, int line) {
    if (g_ntest >= MAX_TESTS) {
        g_not_registered++;       /* reportado (e vira falha) em test_run_all */
        return;
    }
    TestCase new = {.name = name, .fn = fn, .file = file, .line = line};
    g_tests[g_ntest++] = new;
    return;
}

/* TEST_FILTER vazio ou ausente seleciona tudo. */
static bool selected(const TestCase *t, const char *filter) {
    return !filter || strstr(t->name, filter) != NULL;
}

int test_run_all(void) {
    static size_t failed_idx[MAX_TESTS];
    size_t failed_count = 0;
    size_t run_count = 0;
    size_t width = 0;   /* maior nome, para alinhar a coluna do resultado */

    g_color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");

    const char *filter = getenv("TEST_FILTER");
    if (filter && !filter[0]) filter = NULL;

    for (size_t i = 0; i < g_ntest; i++) {
        if (!selected(&g_tests[i], filter)) continue;
        run_count++;
        size_t len = strlen(g_tests[i].name);
        if (len > width) width = len;
    }

    printf("%sRunning %zu tests%s", paint(C_BOLD), run_count, paint(C_RESET));
    if (filter) printf(" (filter: %s)", filter);
    printf("\n%s\n", RULE);

    if (g_not_registered) {
        printf("%sERROR: %zu test(s) not registered (MAX_TESTS = %d)%s\n",
               paint(C_RED), g_not_registered, MAX_TESTS, paint(C_RESET));
    }

    /* Nenhum teste rodou: pode ser um arquivo esquecido no build, então não é sucesso. */
    if (run_count == 0) {
        printf("%sNo tests to run%s\n", paint(C_RED), paint(C_RESET));
        return 1;
    }

    for (size_t i = 0; i < g_ntest; i++) {
        if (!selected(&g_tests[i], filter)) continue;

        g_failed = 0;
        g_nfails = 0;
        g_not_shown = 0;

        /* fflush: se o teste travar ou abortar, o nome já está na tela. */
        printf("\n[ RUNNING ] %-*s . . .   ", (int)width, g_tests[i].name);
        fflush(stdout);

        clock_t start = clock();
        g_tests[i].fn();
        long ms = (long)((clock() - start) * 1000 / CLOCKS_PER_SEC);

        if (g_failed) {
            printf("%s[ FAIL ]%s (%ld ms)\n", paint(C_RED), paint(C_RESET), ms);
            for (size_t j = 0; j < g_nfails; j++) {
                /* Só a primeira linha da mensagem (a expressão) fica vermelha;
                   as linhas seguintes (valores) usam a cor padrão do terminal. */
                const char *msg = g_fails[j].msg;
                const char *rest = strchr(msg, '\n');
                int head_len = rest ? (int)(rest - msg) : (int)strlen(msg);

                printf("\tat %s%s:%d%s  %s%.*s%s%s\n",
                       paint(C_YELLOW), g_fails[j].file, g_fails[j].line, paint(C_RESET),
                       paint(C_RED), head_len, msg, paint(C_RESET),
                       rest ? rest : "");
            }
            if (g_not_shown) {
                printf("\t... and %zu more failure(s) not shown\n", g_not_shown);
            }
            failed_idx[failed_count++] = i;
        } else {
            printf("%s[ OK ]%s (%ld ms)\n", paint(C_GREEN), paint(C_RESET), ms);
        }
    }

    printf("%s\n", RULE);
    printf("%sPASSED %zu/%zu%s", paint(failed_count ? C_RED : C_GREEN),
           run_count - failed_count, run_count, paint(C_RESET));
    if (failed_count) {
        printf("   %sFAILED %zu%s", paint(C_RED), failed_count, paint(C_RESET));
    }
    printf("\n");

    if (failed_count) {
        printf("\nFailed tests:\n");
        for (size_t k = 0; k < failed_count; k++) {
            printf("  - %s\n", g_tests[failed_idx[k]].name);
        }
    }
    return (failed_count || g_not_registered) ? 1 : 0;
}


/*
 * CHECKS
 */

bool test_check(bool ok, const char *file, int line, const char *msg) {
    if (ok) return true;
    test_fail(file, line, "%s", msg);     // msg vai como ARGUMENTO de %s
    return false;
}

bool test_check_i64(const char *file, int line, const char *ea, const char *eb,
                    int64_t a, int64_t b) {
    if (a == b) return true;
    test_fail(file, line, "EQ_INT(%s, %s): %" PRId64 " != %" PRId64, ea, eb, a, b);
    return false;
}

bool test_check_u64(const char *file, int line, const char *ea, const char *eb,
                    uint64_t a, uint64_t b) {
    if (a == b) return true;
    test_fail(file, line, "EQ_U64(%s, %s): %" PRIu64 " != %" PRIu64, ea, eb, a, b);
    return false;
}

/* Uma linha "rótulo: prefixo[X]resto" com o byte i destacado, mostrando só uma
   janela de DIFF_WINDOW caracteres de cada lado ("..." quando corta). */
static void diff_line(char *out, size_t cap, const char *label, const char *s, size_t i) {
    size_t start = i > DIFF_WINDOW ? i - DIFF_WINDOW : 0;
    /* Se a string acabou em i, usa um espaço (invisível em vermelho; vale o índice). */
    const char c[2] = {s[i] ? s[i] : ' ', '\0'};
    const char *rest = s[i] ? s + i + 1 : "";
    size_t rest_len = strlen(rest);
    size_t rest_shown = rest_len > DIFF_WINDOW ? DIFF_WINDOW : rest_len;

    snprintf(out, cap, "%.40s: %s%.*s%s%s%s%.*s%s",
             label,
             start ? "..." : "", (int)(i - start), s + start,
             paint(C_RED), c, paint(C_RESET),
             (int)rest_shown, rest, rest_len > DIFF_WINDOW ? "..." : "");
}

bool test_check_str(const char *file, int line, const char *ea, const char *eb, const char *a, const char *b) {
    if (a == b) return true;                       /* inclui os dois NULL */
    if (a && b && strcmp(a, b) == 0) return true;

    /* ea/eb vêm de #a/#b: se começam com aspas, o argumento era um literal e o
       valor já aparece no cabeçalho, então a linha usa um rótulo curto. */
    const char *label_a = (ea[0] == '"') ? "str_a" : ea;
    const char *label_b = (eb[0] == '"') ? "str_b" : eb;

    if (!a || !b) {
        test_fail(file, line, "EQ_STR(%s, %s):\n\t%.40s: %s\n\t%.40s: %s",
                  ea, eb, label_a, a ? a : "(null)", label_b, b ? b : "(null)");
        return false;
    }

    /* i = índice do primeiro byte diferente (ou do '\0' da string mais curta). */
    size_t i = 0;
    while (a[i] && a[i] == b[i]) i++;

    char line_a[256], line_b[256];
    diff_line(line_a, sizeof(line_a), label_a, a, i);
    diff_line(line_b, sizeof(line_b), label_b, b, i);

    test_fail(file, line, "EQ_STR(%s, %s):\n\t%s\n\t%s\n\tfirst diff at index %zu",
              ea, eb, line_a, line_b, i);
    return false;
}

/* Corpo do EXPECT_MSG: só é chamada quando a condição falhou. */
bool test_msg_fail(const char *file, int line, const char *expr, const char *fmt, ...) {
    char buf[256];

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    test_fail(file, line, "MSG(%s):\n\t%s", expr, buf);
    return false;
}
