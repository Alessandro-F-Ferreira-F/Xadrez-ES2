#ifndef TEST_API_H
#define TEST_API_H

/*
 * API mínima de testes.
 *
 *   TEST(nome) { ... }          registra um teste (automático, antes do main)
 *   int main(void) { return test_run_all(); }
 *
 * Duas famílias de checagem:
 *   EXPECT_*  registra a falha e CONTINUA; vale 1 (passou) ou 0 (falhou)
 *   ASSERT_*  registra a falha e ENCERRA o teste (return;), então só serve
 *             direto no corpo de um TEST ou em função void
 *
 * Variáveis de ambiente lidas por test_run_all:
 *   TEST_FILTER=texto   roda só os testes cujo nome contém "texto"
 *   NO_COLOR=1          desliga as cores
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_TESTS 1024

typedef void (*test_fn)(void);

typedef struct TestCase {
    const char *name;
    test_fn fn;
    const char *file;
    int line;
} TestCase;



void test_fail(const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));
void test_register(const char *name, test_fn fn, const char *file, int line);
int test_run_all(void);

/* Usadas pelas macros abaixo; não chame direto. */
bool test_check(bool ok, const char *file, int line, const char *msg);
bool test_check_i64(const char *file, int line, const char *ea, const char *eb, int64_t a, int64_t b);
bool test_check_u64(const char *file, int line, const char *ea, const char *eb, uint64_t a, uint64_t b);
bool test_check_str(const char *file, int line, const char *ea, const char *eb, const char *a, const char *b);
bool test_msg_fail(const char *file, int line, const char *expr, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));


#define EXPECT(expr) test_check((expr), __FILE__, __LINE__, "EXPECT(" #expr ")")
#define EXPECT_EQ(a, b) test_check((a) == (b), __FILE__, __LINE__, "EXPECT_EQ(" #a ", " #b ")")
/* Com valores na mensagem. _INT para inteiros com sinal, _U64 para sem sinal. */
#define EXPECT_EQ_INT(a, b) test_check_i64(__FILE__, __LINE__, #a, #b, (a), (b))
#define EXPECT_EQ_U64(a, b) test_check_u64(__FILE__, __LINE__, #a, #b, (a), (b))
#define EXPECT_EQ_STR(a, b) test_check_str(__FILE__, __LINE__, #a, #b, (a), (b))
/* Texto de contexto no estilo printf. Os argumentos só são avaliados se a
   condição falhar, então é barato usar dentro de laços. */
#define EXPECT_MSG(cond, ...) \
    ((cond) ? true : test_msg_fail(__FILE__, __LINE__, #cond, __VA_ARGS__))

#define ASSERT(expr)        do { if (!EXPECT(expr)) return; } while (0)
#define ASSERT_EQ(a, b)     do { if (!EXPECT_EQ(a, b)) return; } while (0)
#define ASSERT_EQ_INT(a, b) do { if (!EXPECT_EQ_INT(a, b)) return; } while (0)
#define ASSERT_EQ_U64(a, b) do { if (!EXPECT_EQ_U64(a, b)) return; } while (0)
#define ASSERT_EQ_STR(a, b) do { if (!EXPECT_EQ_STR(a, b)) return; } while (0)
#define ASSERT_MSG(cond, ...) do { if (!EXPECT_MSG(cond, __VA_ARGS__)) return; } while (0)



#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define STR(str) #str

#define TEST(name) \
    static void CONCAT(test_, name)(void); \
    static void __attribute__((constructor)) CONCAT(register_, name)(void) { \
        test_register(STR(name), CONCAT(test_, name), __FILE__, __LINE__);\
    } \
    static void CONCAT(test_, name)(void)

#endif /* TEST_API_H */
