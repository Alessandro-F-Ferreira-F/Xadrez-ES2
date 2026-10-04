# REPL de depuração da engine — o que implementar

Guia para escrever o REPL em `src/main.c` (ou em `src/cli.c`, ver §7). Descreve **quais
comandos** ter, **que funções da engine cada um chama** e **quais funções auxiliares você
precisa escrever**. Sem corpos de função — só o desenho.

O REPL **não** é o protocolo stdin/stdout do cliente gráfico. É uma bancada de testes para
exercitar FEN, geração de lances, make/unmake e perft à mão.

> **Estado em 4 de outubro de 2026.** O que existe hoje em `src/main.c` não é este desenho: é
> um **menu numérico** (`ui()`, 13 opções). Ele cobre boa parte dos comandos da §2 por outro
> caminho:
>
> | Comando deste guia | No menu de hoje |
> |---|---|
> | `fen <fen>` | opção 1 |
> | `move <uci>` / `undo` | opções 2 / 3 (desfaz só o último lance) |
> | `new` | opção 4 |
> | `moves` | opção 6 (todos) e 5 (de uma casa) — já **legais**, não pseudo-legais |
> | `perft <n>` / `divide <n>` | opções 10 / 13 (por make/unmake, não copy-make — e está provado correto) |
> | `roundtrip` | opção 12 (o teste de `test/test.c`, sobre 18 FENs fixas) |
> | `check` | não existe |
> | — | opção 7 (editar casa) e 11 (casa atacada?), que este guia não previa |
>
> O desenho por comandos continua sendo o alvo quando o menu sair do default: o
> `next_steps.md` (Etapa 10.5) move a bancada para um `repl.c`, chamado por
> `./main.out repl`, porque o default do binário passa a ser o protocolo UCI. As §1–§5 e §8
> abaixo continuam válidas para esse momento; a §6 e a §7 foram atualizadas.

---

## 1. Ideia central: o REPL como verificador

O valor de um REPL de engine não está em jogar, está em **desconfiar das peças que você está
testando**. Dois princípios orientam os comandos abaixo:

1. **Guardar um snapshot do `Board` antes de cada lance jogado.** Com isso, `undo` e
   `roundtrip` conseguem *comparar* o que `unmake_move` devolveu contra a verdade, em vez de
   confiar nele. Se divergir, o REPL mostra as duas FENs e restaura pelo snapshot — a sessão
   continua útil mesmo com o bug.
2. **`perft` por copy-make, não por `unmake_move`.** Copiar o `Board` a cada nó faz a contagem
   não depender da função que está em teste. Assim um bug de unmake não contamina o perft, e
   `roundtrip` isola o bug de unmake.

---

## 2. Comandos

| Comando | Atalho | Argumentos | O que faz |
|---|---|---|---|
| `help` | `h` | — | Lista os comandos (gerada a partir da tabela, ver §4) |
| `show` | `s` | — | Tabuleiro, FEN e lances jogados |
| `new` | — | — | Volta à posição inicial |
| `fen` | `f` | `[<fen>]` | Sem argumento imprime a FEN atual; com argumento, carrega |
| `moves` | `m` | — | Lista os lances gerados, com etiqueta de tipo |
| `move` | `mv` | `<uci>` | Joga um lance (`e2e4`, `e7e8q`) |
| `undo` | `u` | `[n]` | Desfaz n lances **e confere** `unmake_move` |
| `check` | `c` | — | Roda `board_check_invariants` |
| `perft` | `p` | `<n>` | Conta nós até a profundidade n |
| `divide` | `d` | `<n>` | Perft por lance da raiz |
| `roundtrip` | `rt` | `[n]` | Testa que `make_move` + `unmake_move` é a identidade |
| `quit` | `q` | — | Sai (também `exit` e Ctrl-D) |

Extra: um lance UCI digitado sozinho (`e2e4`) equivale a `move e2e4`. Só vale tentar isso
**depois** de não achar comando com aquele nome.

### 2.1 Detalhes por comando

**`show`** — `board_print` já imprime tabuleiro, lado, roque, en passant e relógios. Falta só
a FEN (`fen_write`) e, se quiser, a lista de lances jogados convertidos com `move_to_str`.

**`new` / `fen <fen>`** — ambos terminam em `fen_parse` + zerar o histórico. Detalhe que
importa: `fen_parse` **só escreve no `Board` de saída se a FEN inteira validar**, então passe
um `Board` temporário e só copie para a sessão em caso de sucesso — a posição atual sobrevive a
uma FEN inválida. Os motivos da rejeição saem por `LOG_ERROR`; a mensagem "FEN rejeitada" é
sua.

A FEN tem espaços. O argumento de `fen` é **o resto da linha**, não uma palavra só (ver §5.1).

**`moves`** — chama `generate_all_moves` e imprime cada lance com `move_to_str`. Vale mostrar
o tipo (captura, roque, en passant, promoção): uma tabela de 16 strings indexada por
`MoveType` resolve. Os valores 6 e 7 do enum não existem; marque como `?` para o dia em que
aparecerem. Mostre também **quantos** lances saíram e deixe explícito que são
**pseudo-legais**.

`print_moves` (em `move.c`) já existe mas imprime um lance por linha e sem tipo; para o REPL
vale escrever o seu.

**`move <uci>`** — o fluxo é: gerar a lista → `movelist_find` → pegar `list.moves[idx]` (o
lance **gerado**, não o decodificado do texto, porque só o gerador sabe se é captura, en
passant ou roque) → guardar snapshot → `make_move`.

Armadilhas:
- `move_from_str("e7e8")` devolve um lance sem promoção, que **não casa** com nenhum dos quatro
  lances de promoção gerados. A mensagem de erro deve dizer que promoção exige sufixo
  (`e7e8q`).
- **Captura de rei.** Sem filtro de legalidade, o gerador produz lances que capturam o rei.
  Aplicar um deixa o tabuleiro sem rei, e aí `generate_king_moves` usa `king_square == -1` como
  índice e `check_allowed_castles` dispara o `assert`. Recuse esses lances no REPL e explique
  por quê. Some quando o filtro de legalidade existir.
- Limite do histórico: cheque `ply < MAX_GAME_PLY` antes de empilhar.

**`undo [n]`** — para cada lance: decrementar `ply`, chamar `unmake_move(board, move, &undo)`
e **comparar com o snapshot**. Divergiu → `print_mismatch` e restaurar o snapshot.

**`check`** — chama `board_check_invariants`. Em caso de falha, `print_fail_log()` (em
`utils.c`) mostra os motivos. Note que o log de falhas é global e acumula entre chamadas; não
há função para limpá-lo.

**`perft <n>`** — recursão por copy-make. Nas folhas (`depth == 1`) basta somar
`list.count` (bulk counting). Imprima nós, tempo e nós/s (`clock()`).

**`divide <n>`** — mesma recursão, mas imprimindo `lance: contagem` para cada lance da raiz e
o total. É a ferramenta de bissecção: compara contra `go perft N` do Stockfish, acha a raiz que
diverge, entra nela (`fen` + `move`) e repete.

**`roundtrip [n]`** — para cada lance gerado, em profundidade n: guardar cópia → `make_move` →
(recursão) → `unmake_move` → `board_equal` contra a cópia. Se falhar, reportar (limite de ~10
relatos) e **restaurar pela cópia** antes de seguir, para a varredura continuar sobre um estado
são. Ao final: quantos lances testados, quantos falharam.

É o teste mais valioso do REPL. No estado atual do repositório ele falha em 100% dos lances —
porque `unmake_move` ainda não está escrito — e vai guiando você até zerar.

---

## 3. Funções que você precisa escrever

Todas `static` (nenhuma é exportada). Assinaturas sugeridas:

### Comparação e diagnóstico
```c
static bool board_equal(const Board *a, const Board *b);
static bool captures_king(const Board *b, Move m);
static void print_mismatch(const char *ctx, Move m,
                           const Board *expected, const Board *got);
static void print_fen_of(const Board *b);
```
- `board_equal`: compare **campo a campo**, não com `memcmp(a, b, sizeof(Board))`. A struct tem
  padding (entre `side_to_move`/`king_square`/`castling_rights`) cujo conteúdo é indeterminado.
  O `array` pode usar `memcmp`. Não esqueça `fullmove_number` e `halfmove_clock`: `Undo` não
  guarda o fullmove, então é `unmake_move` quem tem de derivá-lo, e é exatamente o tipo de
  bug que você quer que o teste pegue.
- `captures_king`: `TYPE_OF` da peça na casa de destino. Use `PIECE_TYPE`, não compare o valor
  cru (armadilha da seção 5 do `CLAUDE.md`).

### Sessão
```c
typedef struct {
    Board board;
    Board before[MAX_GAME_PLY];   /* posição antes de cada lance */
    Move  moves[MAX_GAME_PLY];
    Undo  undos[MAX_GAME_PLY];
    int   ply;
} Session;

static void reset_to(Session *s, const Board *b);
static int  play_move(Session *s, const char *uci);
```
`Session` tem ~110 KB: declare como `static Session session;` na `main`, não como local. Com
ASan ligado o frame estoura o limite `-Wframe-larger-than=16384` do `make debug`.

### Perft e teste
```c
static u64  perft(const Board *b, int depth);
static void roundtrip(Board *b, int depth, RoundtripStats *st);   /* ver struct abaixo */
static double elapsed_ms(clock_t start);
```
```c
typedef struct { u64 moves; u64 failures; } RoundtripStats;
```
Na `perft` e no `roundtrip`, lance que captura rei é **folha**: conta como nó, não expande
(mesmo motivo da captura de rei em `move`).

### Comandos e despacho
```c
enum { CMD_OK = 0, CMD_QUIT = 1 };

typedef struct {
    const char *name;
    const char *alias;    /* pode ser NULL */
    const char *usage;
    const char *desc;
    int (*fn)(Session *s, char *args);
} Command;

static const Command *find_command(const char *word);
static int dispatch(Session *s, char *line);
```
Um `cmd_*` por comando, todos com a mesma assinatura `int cmd_x(Session *, char *args)`.
Devolvem `CMD_OK` ou `CMD_QUIT`.

---

## 4. Tabela de comandos em vez de `if/else`

Uma tabela `static const Command COMMANDS[]` terminada por `{NULL, ...}` faz `help` ser
**gerado da própria tabela** (nunca fica desatualizado) e `find_command` virar um laço.

Duas armadilhas de C:
- Uma declaração antecipada `static const Command COMMANDS[];` (sem tamanho) **não compila**.
  Se `cmd_help` precisa percorrer a tabela, declare só o **protótipo** de `cmd_help` antes e
  defina-o **depois** da tabela.
- O `.fn` de cada entrada exige que os `cmd_*` já estejam declarados acima da tabela.

---

## 5. Entrada: o que já existe em `io.h` e o que usar

### 5.1 Tokenização

A linha chega inteira (`read_line`). Você precisa de "primeira palavra + resto da linha":

```c
static char *next_word(char **p);   /* devolve a palavra; avança *p para a próxima */
static void  rtrim(char *s);
static bool  parse_int(const char *str, int *out);
```

Por que não usar as funções que já existem no `io.c`:

- **`read_line`** — **use**. Trata linha longa demais, `\r` do Windows e última linha sem `\n`.
  Devolve 1/0/−1 (ok/EOF/longa demais).
- **`read_word` / `read_int`** — **não servem aqui**. Cada uma lê *uma linha inteira* e devolve
  só a primeira palavra; chamar duas em sequência consome duas linhas. Foram pensadas para
  prompt de "digite um número".
- **`join_args`** — **não insere espaço** entre os argumentos (concatena `"a"`, `"b"` como
  `"ab"`). Uma FEN reconstruída com ela vira lixo. Se quiser reaproveitá-la, corrija-a antes.
- **`string_split`** — devolve `String` (ponteiro + tamanho, **sem `'\0'`**); para chamar
  `fen_parse` você teria de copiar. Além disso `char *p = line;` descarta o `const` (warning
  `-Wdiscarded-qualifiers` já presente no build).
- **`string_to_cstr_static` e `string_split_next`** existem no `io.c` mas **não estão
  declaradas em `io.h`** — o build dá `-Wmissing-prototypes`. Declare em `io.h` ou torne `static`.
- **`string_parse_int`** faz `malloc` só para chamar `strtol`. Para o REPL, `strtol` direto com
  checagem de `endptr` é menos código.

Para `strtol`, cheque (a) que `endptr != str`, (b) que `*endptr == '\0'` e (c) o intervalo antes
de converter para `int`.

### 5.2 `isatty` e prompt

`isatty(fileno(stdin))` decide se imprime banner e prompt. Com `-std=c11` esses símbolos POSIX
ficam escondidos: é preciso `#define _POSIX_C_SOURCE 200809L` **antes do primeiro `#include`**
e `#include <unistd.h>`. Vale a pena: sem o prompt, `printf '...' | ./build/main.out` fica
legível, e é assim que você automatiza testes (ver §8).

### 5.3 Buffering

`setvbuf(stdout, NULL, _IOLBF, 0)` no início da `main` (recomendação do `CLAUDE.md` §9).
Quando a stdout for um pipe, sem isso a saída fica presa no buffer de bloco.
Se imprimir prompt sem `\n`, dê `fflush(stdout)` antes de ler.

---

## 6. Armadilhas da engine que o REPL encontra

**Estado medido em 2026-10-04.** As quatro armadilhas medidas em 30/09 (`unmake_move`
incompleto, sem filtro de legalidade, cavalo ausente, perft que não batia) **foram todas
resolvidas** — o motor gera, aplica, desfaz e filtra lances corretamente, e o perft bate nas
seis posições de referência (`project_context.md` §2). O que sobra para o REPL encontrar:

1. **`init_square_tables()` precisa ser chamada na `main`, antes de tudo.** Continua valendo:
   nenhum outro lugar chama, e sem ela `SQ_TO_EDGE`, `KNIGHT_ATTACKS` e `PAWN_ATTACKS` são
   zeros e a geração devolve lixo sem aviso.
2. **FEN com o lado que não joga em xeque é aceita** (`project_context.md` Bug #1) — e o
   filtro aceita a captura do rei nela, porque capturar o rei não deixa o rei de quem joga em
   xeque. O lance seguinte derruba o processo num `assert`. O REPL deve continuar recusando
   captura de rei (§2.1) até o `fen_parse` rejeitar essa posição.
3. **Editar casa sem validar a coordenada escreve fora do tabuleiro**, e editar o rei sem
   atualizar `king_square` faz o gerador partir da casa errada (Bug #2, opção 7 do menu atual).
   Um comando de edição no REPL novo precisa das duas coisas.
4. **`read_word` em fim de entrada** devolve a palavra anterior de novo, com sucesso (Bug #3).
   Um REPL alimentado por pipe (`printf ... | ./build/main.out`) entraria em laço. Use
   `read_line` e trate o retorno `0`.
5. **`is_square_attacked(b, sq, side)` recebe a cor do dono da casa**, não do atacante. Um
   comando "esta casa está atacada?" precisa dizer isso ao usuário.

---

## 7. Onde colocar: `main.c` ou `cli.c`

*Atualização de 04/10:* o esboço `src/cli.c` não existe mais; a bancada virou o menu numérico
de `main.c`. A decisão abaixo continua de pé e ficou mais urgente: quando o protocolo UCI
entrar, ele é o default do binário, e o menu precisa sair de `main.c` — `next_steps.md` Etapa
10.5 recomenda `repl.c`, levando junto `read_coord`, `wait_enter` e `read_move` de `utils.c`
(o que também conserta o `make test`, Bug #4). O texto original:

Hoje existe um esboço seu em `src/cli.c` (struct `Session` + protótipos `cmd_*` sem definição)
e o Makefile compila `src/*.c`, então ele gera 14 warnings `-Wunused-function`. Duas opções:

- **Recomendo `cli.c`**: separa a bancada (REPL) do `main`, que fica só com `init_square_tables`,
  `setvbuf` e uma chamada `cli_run()`. É a mesma fronteira que o `CLAUDE.md` §3 pede para
  reduzir atrito de merge: o time mexe na `main` sem conflitar com o REPL. Exigiria um
  `include/cli.h` e o primeiro include de `cli.c` ser o próprio header (convenção do §7).
- `main.c` direto é mais rápido e suficiente enquanto o REPL é só seu.

---

## 8. Ordem sugerida de implementação

1. Laço de leitura + `dispatch` + `quit` + `help`. Teste: `printf 'help\nq\n' | ./build/main.out`.
2. `init_square_tables`, `show`, `new`, `fen`. Teste: round-trip FEN → `fen` imprime a mesma string.
3. `moves`. Teste: posição inicial; compare o número com o esperado (hoje 16; 20 com cavalo).
4. `move` + histórico com snapshot. Teste: `e2e4`, `e7e5`, `show`.
5. `undo` **com `board_equal`**. Vai falhar — é o ponto. Comece a implementar `unmake_move`
   guiado pelas divergências.
6. `roundtrip`. Na profundidade 1, depois 2, 3, sobre FENs variadas.
7. `perft` e `divide`. Só faz sentido comparar com tabelas oficiais depois do filtro de
   legalidade; até lá, use para comparar **dois estados do seu próprio código**.
8. `check`.

Build de debug (sanitizers) para rodar tudo: `make debug`. Lembre que o `CLAUDE.md` pede `-Og`,
não `-g` sozinho, para os avisos de fluxo de dados funcionarem.

### Posições para testar

| Posição | FEN | Serve para |
|---|---|---|
| Inicial | `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1` | linha de base |
| Kiwipete | `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1` | roque, en passant, tudo |
| Promoção | `7k/4P3/8/8/8/8/8/4K3 w - - 0 1` | quatro promoções |

Deixe o rei preto longe de e8 na posição de promoção: com ele em e8 o peão fica bloqueado e o
teste não testa nada.

---

## 9. Referências

- **Chess Programming Wiki**: *Perft*, *Perft Results* (valores de referência), *Unmake Move*.
- `man isatty`, `man 3 fileno`, `man feature_test_macros` (por que `_POSIX_C_SOURCE`).
- `man strtol`, seção sobre tratamento de erro com `endptr`.
- `CLAUDE.md` §5 — decisão de `Undo` sob responsabilidade do chamador e de `make_move` como
  primitiva (não valida), que explica por que a validação de lance vive no REPL e não no core.
