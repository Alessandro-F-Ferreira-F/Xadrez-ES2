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




void test_register(const char *name, test_fn fn, const char *file, int line) {
    if (g_ntest >= MAX_TESTS) {
        fprintf(stderr, "Too many tests\n");
        return;
    }
    TestCase new = {.name = name, .fn = fn, .file = file, .line = line};
    g_tests[g_ntest++] = new;
    return;
}


// int main(void) {


//     int CONCAT(a, b);
//     ab = 5;
//     printf("%d\n", ab);

//     return 0;
// }