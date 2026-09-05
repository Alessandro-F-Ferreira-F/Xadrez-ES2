# Contexto do Projeto — Chess Engine em C

> Documento de contexto para retomar o projeto ou apresentá-lo a alguém (humano ou LLM)
> sem precisar ler a codebase inteira.
> Estado em **5 de setembro de 2026**, commit `0c4a88a`.
> Fonte de verdade das decisões: `CLAUDE.md`. Este arquivo interpreta e resume.

---

## 1. O que é este repositório

Um **motor de xadrez em C**, escrito do zero como projeto de aprendizado. O objetivo
declarado não é força de jogo — é entender como uma engine funciona por dentro.

A fronteira do repositório é estreita e deliberada: **um binário que fala um protocolo
texto por stdin/stdout.** Nada além disso vive aqui.

- O **cliente desktop (C++)** é de outra equipe, em outro repositório.
- O **backend (Node/TS)** também não vive aqui.
- Este repo entrega um executável que recebe comandos por linha e responde por linha.

A fase atual é **protótipo/demo**: um binário que joga xadrez legalmente do início ao fim,
com uma IA qualquer. Correção das regras vem primeiro; força de jogo vem por último, e só
se sobrar tempo. Bitboards, transposition table e magic bitboards estão explicitamente
fora de escopo.

---

## 2. Estado atual, honestamente

O projeto tem **~750 linhas** em 5 módulos e 6 commits. O parser de FEN é a parte madura;
a geração de lances mal começou.

### Funciona e está verificado

| Área | Estado |
|---|---|
| `parse_fen` | Valida e constrói o tabuleiro numa passada só. Rejeita as posições ilegais testadas (peão em back rank, rei duplicado, rank incompleta, contagem de peças) |
| Indexação `a1 = 0` | Convertida e verificada: round-trip FEN idêntico, `king_square` correto, tabuleiro na orientação certa |
| `board_to_fen` | Emite o campo de posição corretamente |
| `SQ_TO_EDGE` | Tabela de distância até a borda, verificada casa a casa contra valores calculados à mão |
| Conversão de coordenadas | `sq_from_coord` / `coord_from_sq` |
| Camadas de validação | `find_move` valida na fronteira; `make_move` confia na entrada |
| Headers | Os cinco são auto-contidos (cada um compila sozinho) |

### Incompleto ou ausente

| Área | Estado |
|---|---|
| `Board` | Faltam `castling_rights`, `ep_square`, `halfmove_clock`, `fullmove_number` |
| `parse_fen` | Lê os campos 3 a 6 da FEN e os **descarta** |
| `board_to_fen` | Emite só a posição, não os seis campos |
| Geração de lances | Só peão, e só empurrada simples. Sem captura, dupla, en passant ou promoção |
| Peças deslizantes | `generate_sliding_moves` existe vazia |
| Cavalo e rei | Não existem |
| `side_to_move` | A geração **ignora de quem é a vez** e produz lances das duas cores |
| `make_move` | Move a peça e nada mais: não atualiza `king_square`, não inverte `side_to_move`, não trata captura/roque/en passant |
| `unmake_move` | Não existe. Sem ele não há busca |
| Legalidade | Sem `is_square_attacked`, sem filtro de xeque. Todo lance gerado é pseudo-legal |
| Perft | Não existe. **Nada da geração está validado ainda** |
| Protocolo | Não existe. A interação é um `printf`/`fgets` no `main` |
| Testes | Nenhum automatizado |

---

## 3. Decisões arquiteturais firmadas

Estas são escolhas conscientes com trade-off avaliado. Mudá-las agora custa caro.

### Representação: mailbox de 64 casas

Um `Piece array[64]`, não bitboards. Bitboards são mais rápidos, mas mailbox é muito mais
fácil de acertar e de depurar — e correção vem antes de velocidade nesta fase.

Efeito colateral feliz, medido: `array[64]` são **64 bytes = exatamente uma linha de cache**.
Varrer o tabuleiro inteiro custa ~9,5 ns. Uma *piece list* (iterar só as ~30 casas ocupadas)
foi medida em ~12,3 ns — **mais lenta**, porque a indireção custa mais que a varredura que
economiza. Decisão registrada: não implementar piece lists.

### Indexação: `a1 = 0` (Little-Endian Rank-File)

```
sq = rank * 8 + file        rank 0 = 1ª fileira,  file 0 = coluna a
a1 = 0    h1 = 7    a8 = 56    h8 = 63
```

```
    a   b   c   d   e   f   g   h
8  56  57  58  59  60  61  62  63
7  48  49  50  51  52  53  54  55
6  40  41  42  43  44  45  46  47
5  32  33  34  35  36  37  38  39
4  24  25  26  27  28  29  30  31
3  16  17  18  19  20  21  22  23
2   8   9  10  11  12  13  14  15
1   0   1   2   3   4   5   6   7
```

É a convenção da Chess Programming Wiki, dos valores de perft publicados e das
piece-square tables prontas — evita traduzir mentalmente cada exemplo lido.

Consequências: peão branco avança `+8`, preto `-8`. A FEN é lida da fileira 8 para a 1,
então a **primeira** linha da FEN preenche os índices 56–63.

### Codificação da peça num `u8`

```c
MAKE_PIECE(color, type) = (color << 3) | type
TYPE_OF(piece)          = piece & 0x7
COLOR_OF(piece)         = (piece >> 3) & 0x1
```

Com `WHITE = 1`, `BLACK = 0`, `EMPTY = 0`. Peão branco = 9, peão preto = 1.

**Armadilha conhecida:** `COLOR_OF(EMPTY) == BLACK`. Casa vazia se parece com peça preta.
Sempre teste "está vazia" **antes** de testar a cor.

**Armadilha relacionada:** comparar o valor cru contra um `PieceType` (`if (array[sq] == PAWN)`)
funciona por acidente para as pretas e falha para as brancas, porque `BLACK = 0` faz o nibble
de cor sumir. Use sempre `TYPE_OF()`.

### Codificação do lance num `u32`

```
       origem    destino   promoção   flags
u32 = [00000000][00000000][00000000][00000000]
        <<24      <<16       <<8       <<0
```

8 bits por campo. É folgado — casas cabem em 6 bits e o padrão da literatura é um `u16`
com 6+6+4. A folga foi mantida deliberadamente por enquanto: cada campo cai num byte
alinhado, o que facilita a depuração. **Decisão em aberto** (ver §6).

`MoveDescription` é a forma desempacotada, devolvida por `decode_move`.

### Direções: tabela de distância até a borda

`SQ_TO_EDGE[64][8]` guarda, para cada casa e cada direção, quantas casas existem até a
borda. Resolve o problema de wrap-around sem 0x88 nem mailbox 10x12 — as três abordagens
resolvem o mesmo problema, e trocar depois custa mais do que ganha.

**A ordem do `enum Direction` é carga estrutural, não estética:**

```
índice:   0     1     2     3     4      5      6      7
        DIR_N DIR_S DIR_E DIR_W DIR_NE DIR_SW DIR_SE DIR_NW
        └──── ortogonais ────┘  └────── diagonais ──────┘

DIR_OFFSET = { 8,  -8,   1,   -1,    9,    -9,    -7,     7 }
```

Torre usa as direções 0–3, bispo 4–7, rainha 0–7. Isso permite **uma única função**
parametrizada por intervalo de direção, em vez de três. Reordenar o enum quebra isso em
silêncio.

Note a distinção entre **índice** (o valor do enum, usado para indexar as tabelas) e
**offset** (o delta somado ao índice da casa). Confundir os dois foi bug real neste projeto.

### Pseudo-legal primeiro, legalidade depois

Gerar todos os lances que respeitam o movimento da peça, e só depois filtrar os que deixam
o próprio rei em xeque. É mais fácil de acertar do que gerar apenas lances legais direto.

O filtro é: aplica o lance, pergunta se o rei do lado que jogou está atacado, desfaz.

### Apply/undo com pilha, não cópia do tabuleiro

Cada nó de busca aplica o lance sobre o `Board` existente e o desfaz depois, guardando o
que se perdeu numa struct `Undo` na pilha da recursão. **Não** copiar o `Board` inteiro
por nó.

O `Undo` precisa guardar exatamente o que **não é recalculável** a partir do tabuleiro
depois do lance:

```c
typedef struct {
    Piece captured;         /* o tabuleiro depois não sabe o que estava ali */
    u8    castling_rights;  /* mover a torre destrói o direito, sem deixar rastro */
    int   ep_square;
    int   halfmove_clock;
} Undo;
```

Esta decisão dói para trocar depois. Respeitar desde o início.

### Contrato de validação: fronteira valida, núcleo confia

Estabelecido nesta sessão e já implementado:

```
uci.c / main.c   →  recebe texto do usuário. VALIDA aqui, com find_move.
                 ↓
movegen.c        →  make_move(Board*, u32) confia na entrada. Não verifica nada.
```

`make_move` é uma **primitiva**, não um serviço — como `free(p)`, que não confere se `p`
veio de um `malloc`. A pré-condição é "este `u32` saiu do gerador para esta posição", e
ela é satisfeita por construção nos dois únicos chamadores:

1. **A busca / o filtro de legalidade** — itera a lista que o gerador acabou de produzir.
2. **A fronteira do protocolo** — chama `find_move`, que gera, acha, e devolve o lance.

`make_move` **não pode** validar, por dois motivos. O decisivo é que o filtro de legalidade
precisa aplicar lances que talvez sejam ilegais — é aplicando que se descobre. Um `make_move`
que recusa lances ilegais torna o filtro impossível de escrever, e fecha um ciclo de
dependência: geração → validação → make → geração. O segundo motivo é custo: na busca,
regenerar todos os lances dentro do `make_move` transformaria cada nó em O(n²).

`find_move` devolve **o lance como o gerador o produziu**, não um booleano. Quem digitou
`e5d6` não sabe se aquilo é captura comum ou en passant; o gerador sabe, porque foi ele
que marcou a flag. Devolver só `true` jogaria fora exatamente o que o `make_move` precisa.

### Motor stateless entre comandos

A posição chega sempre como FEN completa, não como histórico incremental. Qualquer posição
vira reproduzível isoladamente em teste, e cliente e motor não podem dessincronizar.

**Limite conhecido:** repetição tripla **não é detectável a partir de uma FEN** — ela é uma
fotografia sem histórico. A regra dos 50 lances funciona (vem do `halfmove_clock`), mas
repetição não. É por isso que o comando `position` do UCI aceita o sufixo `moves`:

```
position fen <fen> moves e2e4 e7e5 g1f3
```

O motor parte da FEN e reaplica os lances, construindo o histórico. Continua stateless —
tudo veio no comando — mas agora dá para detectar empate por repetição.

O invariante a defender: **protocolo stateless, processo stateful.** Nenhum comando *precisa*
do anterior; o processo pode cachear o que quiser.

---

## 4. Mapa dos módulos

```
types.h ──┬── board.h ──┬── utils.h ──┬── movegen.h
          │             │             │
          └── log.h ────┘             └── (main.c usa todos)
```

| Arquivo | Papel | Linhas |
|---|---|---|
| `types.h` | Vocabulário: `Board`, `Move`, `MoveList`, `Piece`, `Color`, `Direction`, e as macros de codificação | 98 |
| `log.h` / `log.c` | `LOG_ERROR(msg)` com `__FILE__`/`__LINE__`/`__func__` capturados **na macro**, não na função | 17 / 5 |
| `board.h` / `board.c` | FEN (parse, serialize, validação) **e** conversão de coordenadas | 18 / 324 |
| `movegen.h` / `movegen.c` | Direções, codificação de lance, geração, `find_move`, `make_move` | 20 / ~215 |
| `utils.h` / `utils.c` | Impressão de debug, leitura de stdin, `strslc` | 18 / ~70 |
| `main.c` | REPL provisório de teste | 66 |

**Problemas estruturais conhecidos:**

- `board.c` é dois módulos num arquivo só (FEN + coordenadas), e é o maior do projeto.
  Coordenada não é detalhe de FEN — o protocolo UCI (`e2e4`), a leitura de lances e a
  impressão de en passant vão todos precisar dela. Pertence ao vocabulário central.
- `types.h` arrasta seis headers da libc (`ctype`, `stdbool`, `stdint`, `stdio`, `stdlib`,
  `string`). Todo módulo ganha tudo de graça, e o compilador para de avisar quando surge
  acoplamento novo. Só `stdbool.h` e `stdint.h` são realmente necessários lá.
- `utils.h` define uma função `static` **dentro do header** (`clear_screen`). Isso duplica
  a função em cada unidade de tradução e gera aviso de "definida mas não usada" em todas.

---

## 5. Bugs abertos

| Onde | Problema |
|---|---|
| `main.c:56` | `find_move(..., movedesc.flags)` — o 5º parâmetro é `u32 *out`, e está recebendo um `u8`. Hoje `flags` é sempre 0, então vira `NULL` e o guard interno salva; no dia em que uma flag for marcada, vira escrita através de ponteiro-lixo. GCC avisa: `-Wint-conversion` |
| `main.c:59` | Passa `move` (o digitado) ao `make_move`, não o lance devolvido por `find_move` — o que anula o propósito do `out` |
| `main.c:50` | `char move_out[5]` não comporta promoção: `e7e8q` são 5 caracteres + NUL |
| `types.h:33` | `SQ_OFFBOARD(sq)` sem parênteses no parâmetro. `SQ_OFFBOARD(a & 1)` expande para `a & 1 < 0`, que o C lê como `a & (1 < 0)` |
| `movegen.c` | `add_move` usa `if (count > GEN_MOVES_MAX)` — deveria ser `>=`, senão o índice `GEN_MOVES_MAX` escreve uma posição além do array |
| `movegen.c` | `str_to_move` não checa `sq_from_coord` devolvendo `-1`. Digitar `z9z9` produz um lance com origem 255 |
| `movegen.c` | A geração ignora `side_to_move` |
| `board.c` | `coord_from_sq` — no ramo de erro escreve `out[3]` num buffer de 3 bytes, e não tem `return`, então sobrescreve os `'X'` com lixo. `sq > BOARD_SIZE` deveria ser `>=` |
| `log.h:11` | `LOG_ERROR` está sob `#ifdef DEBUG` e vira `((void)0)` no build padrão. **Todo erro é engolido em release** — inclusive FEN inválida e lance ilegal |
| `Makefile` | `all` e `debug` usam `-Wall -g` sem `-O`. O GCC só faz análise de fluxo de dados com otimização ligada, então `-Wuninitialized` e família **não disparam**. Custou uma sessão de depuração |

### Sobre o Makefile, especificamente

Das últimas seis sessões, **todo bug encontrado era detectável em tempo de compilação**.
A recomendação é trocar `-g` por `-Og -g` e ligar o conjunto completo:

```
-std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Og -g
-fsanitize=address,undefined
```

`-Og` é o nível de otimização feito para depuração: liga as análises que alimentam os avisos
sem tornar o código irrastreável no gdb. Note que **nenhum sanitizer pega leitura de memória
não inicializada** — ASan pega fora-dos-limites e use-after-free, UBSan pega UB aritmético.
Quem pega é o MemorySanitizer, que é só do clang e exige recompilar a libc. Na prática, o
aviso do compilador é a única defesa viável.

---

## 6. Decisões em aberto

| Decisão | Situação |
|---|---|
| **Campos que faltam na `Board`** | `castling_rights`, `ep_square`, `halfmove_clock`, `fullmove_number`. Bloqueia roque e en passant — que são justamente os casos que o perft existe para pegar. São também exatamente os campos que o `Undo` precisa restaurar: definir a `Board` completa é definir metade do make/unmake |
| **Lance em 16 vs 32 bits** | Hoje 32. O padrão é 16 (6+6+4). Corta a `MoveList` pela metade. Momento barato de decidir: só `encode_move` e `decode_move` tocam o formato |
| **Sistema de build** | Makefile vs. CMake. O documento de arquitetura pede CMake por causa da integração com a equipe do cliente |
| **Onde vive `side_to_move` na geração** | Decidir antes de escrever as outras cinco peças, senão a mesma checagem é escrita seis vezes. A opção limpa é o laço externo pular tudo que não é da cor da vez, uma vez só |

---

## 7. Próximos passos, em ordem

Os primeiros itens são pré-requisitos reais dos seguintes.

1. **Fechar os bugs de `main.c`** — o `find_move` com `flags` no lugar de `&out`, e usar o
   lance devolvido no `make_move`.
2. **Completar a `Board` e o ciclo da FEN** — os quatro campos, `parse_fen` lendo os campos
   3 a 6, `board_to_fen` emitindo os seis. Enquanto o serializador só emite a posição, o
   round-trip não prova nada sobre roque e en passant.
3. **Fixar a assinatura de `generate_moves`** antes de gerar mais lances:
   `int generate_moves(const Board *board, MoveList *out)`. `const` porque geração não
   modifica nada; retorno é a contagem.
4. **Peças deslizantes** — uma função só, parametrizada por intervalo de direção (torre 0–3,
   bispo 4–7, rainha 0–7). Laço interno de três saídas: vazia → adiciona e continua;
   inimiga → adiciona e para; amiga → para.
5. **Cavalo e rei por tabela** pré-computada de destinos. Peão por último, é o que tem mais
   casos (dupla, duas capturas, en passant, promoção em quatro peças).
6. **`is_square_attacked` → `make_move`/`unmake_move` completos → filtro de legalidade.**
   Adicionar um `assert` de debug que recalcula `king_square` do zero e compara com o cache
   — campo denormalizado que dessincroniza é dos bugs mais chatos de rastrear.
7. **Perft, e perft divide.** Comparar com os valores publicados (inicial, Kiwipete).
   `perft divide` é a técnica de depuração: mostra a contagem por lance de raiz, você compara
   com o Stockfish (`go perft N`), acha o que diverge, entra nele e repete — busca binária
   dentro da árvore. **Nada de busca ou avaliação antes do perft bater.**
8. **Protocolo stdin/stdout.** Conjunto mínimo: `uci`, `isready`, `quit`, `position`,
   `legalmoves`, `go movetime`. Só depois disso, a IA.

### Armadilha do protocolo, para quando chegar lá

Quando a stdout é um terminal, a libc usa buffer de linha e cada `printf` sai na hora.
Quando é um **pipe** — exatamente o caso do cliente C++ rodando o motor como subprocesso —
ela muda para buffer de bloco de 4 KB. O `bestmove` fica preso, o cliente espera para
sempre, e não há sintoma para depurar. Duas linhas no começo do `main` resolvem:

```c
setvbuf(stdout, NULL, _IOLBF, 0);
setvbuf(stdin,  NULL, _IONBF, 0);
```

Junto: cuidado com `\r\n` se a equipe do cliente estiver no Windows.

---

## 8. Referências ativas

- **Chess Programming Wiki** — verbetes centrais para o que vem a seguir:
  [Encoding Moves](https://www.chessprogramming.org/Encoding_Moves),
  [Move List](https://www.chessprogramming.org/Move_List),
  [Pawn Push](https://www.chessprogramming.org/Pawn_Push),
  [Make Move](https://www.chessprogramming.org/Make_Move),
  [Square Attacked By](https://www.chessprogramming.org/Square_Attacked_By),
  [Perft](https://www.chessprogramming.org/Perft) e
  [Perft Results](https://www.chessprogramming.org/Perft_Results).
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas de texto puro, vale ler inteira.
- **TSCP** (Tom Kerrigan's Simple Chess Program) — engine mailbox completa em ~2000 linhas
  de C legível. Já está no repositório, em `tscp183b/`.
- `man gcc`, seção *Options to Request or Suppress Warnings* — em especial a nota sob
  `-Wmaybe-uninitialized` sobre a dependência do nível de otimização.

---

# Apêndice — Headers

A interface completa do projeto. Cinco headers, ~120 linhas de declaração descrevendo
~750 linhas de código.

## `src/types.h`

```c
#ifndef TYPES_H
#define TYPES_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int16_t i16;

#define INPUT_STR_SIZE 128
#define BOARD_SIZE 64
#define BOARD_WIDTH 8
#define GEN_MOVES_MAX 256

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define MemoryZero(addr, size) memset((addr), 0x0, (size))
#define MemoryZeroStruct(addr, st) MemoryZero((addr), sizeof(st))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define SQ_NONE (-1)
#define RANK_OF(sq) ((sq) / BOARD_WIDTH)
#define FILE_OF(sq) ((sq) % BOARD_WIDTH)
#define FILE_DIST(dest, orig) (abs(FILE_OF(dest) - FILE_OF(orig)))
#define SQ_FROM_RF(rank, file) ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq) (((sq < 0) || (sq >= 64)) ? 1 : 0)

#define MOVE_MASK 0xFFUL
#define MAX_FEN_STRING 256

typedef enum {
    WHITE = 1,
    BLACK = 0
} Color;

typedef enum {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
} PieceType;

typedef uint8_t Piece;

#define PIECE_TYPE_MASK 0x7
#define COLOR_MASK 0x1
#define MAKE_PIECE(color, type) \
    ((Piece)((((color) & COLOR_MASK) << 3) | ((type) & PIECE_TYPE_MASK)))
#define TYPE_OF(piece) ((PieceType)((piece) & PIECE_TYPE_MASK))
#define COLOR_OF(piece) ((Color)(((piece) >> 3) & COLOR_MASK))

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int king_square[2];
} Board;

typedef enum {
    DIR_N,
    DIR_S,
    DIR_E,
    DIR_W,
    DIR_NE,
    DIR_SW,
    DIR_SE,
    DIR_NW,
    DIR_COUNT
} Direction;

typedef struct {
    u8 origin_sq;
    u8 target_sq;
    u8 promotion;
    u8 flags;
} MoveDescription;

typedef struct {
    u32 move;
    int score;
} Move;

typedef struct {
    Move moves[GEN_MOVES_MAX];
    int count;
} MoveList;

#endif
```

## `src/log.h`

```c
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>

void log_msg(const char *msg, const char *file, int line, const char *func);

// #define LOG_ERROR(msg) log_msg((msg), __FILE__, __LINE__, __func__)

#ifdef DEBUG
    #define LOG_ERROR(msg) log_msg((msg), __FILE__, __LINE__, __func__)
#else
    #define LOG_ERROR(msg) ((void)0)
#endif

#endif
```

> `__FILE__`, `__LINE__` e `__func__` são capturados **na macro**, não dentro de `log_msg`.
> Se ficassem na função, ela reportaria a si mesma em toda chamada. Isso já foi bug aqui.

## `src/board.h`

```c
#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "log.h"

bool parse_fen(const char *fen_string, Board *out);
void board_to_fen(Board *board, char fen_out[MAX_FEN_STRING]);

int sq_from_coord(const char *coord);
void coord_from_sq(int sq, char out[3]);

extern const char PIECE_CHAR[17];

#endif
```

> `PIECE_CHAR` é `".pnbrqk..PNBRQK."` — indexado **diretamente pelo valor codificado da peça**.
> Peão branco = 9 → `'P'`; peão preto = 1 → `'p'`. Sem `if` nenhum.

## `src/movegen.h`

```c
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "types.h"
#include "utils.h"

void precompute_move_data();
void add_move(u32 move, MoveList *list);
void generate_pawn_moves(Board *board, MoveList *list);
void print_moves(MoveList *list);
void print_square_directions(char sq_str[3]);
bool find_move(Board *board, int origin_sq, int target_sq, int promo, u32 *out);
u32 encode_move(int origin_sq, int target_sq, int promo, int flags);
MoveDescription decode_move(u32 move);
u32 str_to_move(char move_in[5]);
void make_move(Board *b, u32 move);

#endif
```

> `precompute_move_data()` deveria ser `(void)` — em C, `()` declara "argumentos não
> especificados", não "nenhum argumento". `-Wstrict-prototypes` avisa.

## `src/utils.h`

```c
#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "log.h"
#include "board.h"

void print_piece_chart();
void print_board(const Board *board);
void get_fen(char fen[MAX_FEN_STRING]);
int get_int(char msg[INPUT_STR_SIZE]);
void strslc(const char *src, char *dest, int start, int end);

static void clear_screen() {
    printf("\e[1;1H\e[2J");
    fflush(stdout);
}

#endif
```

> `clear_screen` é uma função `static` **definida dentro de um header** — cada unidade de
> tradução ganha a própria cópia, e todas avisam "definida mas não usada". Deveria ser
> `static inline`, ou ir para o `.c`. O `\e` também não é padrão C (é extensão GNU);
> a forma portável é `\x1b`.

---

*Este documento é escrito à mão — o apêndice de headers precisa ser atualizado manualmente
quando as interfaces mudarem. `make context` junta `CLAUDE.md` + este arquivo em
`build/context.md`, pronto para colar numa conversa.*
