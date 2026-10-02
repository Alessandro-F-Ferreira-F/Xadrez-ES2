# Contexto do Projeto — Chess Engine em C

> Documento de contexto para retomar o projeto ou apresentá-lo a quem for trabalhar nele,
> sem precisar ler a codebase inteira.
> Estado em **1º de outubro de 2026**: commit `8344d1a` ("unmake_move() implementada e
> testada"), **mais as alterações ainda não commitadas na árvore de trabalho** — que são,
> principalmente, a geração do cavalo e a migração de `is_own`/`is_enemy` de `piece.h` para
> `board.h`.
>
> **Esta revisão é a primeira em que o motor gera e aplica lances corretamente, de ponta a
> ponta.** `unmake_move` existe, o cavalo existe, e o par gerador + make/unmake bate o
> *perft* de referência nas seis posições-padrão — verificado com evidência (§2), não por
> leitura. O que falta para a demo deixou de ser "regras" e passou a ser **legalidade**: o
> motor aceita lance ilegal, e o primeiro lance que captura um rei derruba o processo (§5,
> Bug #1).
>
> `docs/roadmap-motor.md` e `docs/next_steps.md` **não** foram atualizados junto com este
> documento e ainda descrevem o estado de 16/09 (cavalo ausente, `unmake_move` sem corpo).
> Em caso de conflito, vale este aqui.

---

## 1. O que é este repositório

Um **motor de xadrez em C**, escrito do zero como projeto de aprendizado — e o repositório
oficial do grupo na disciplina de Engenharia de Software 2. O objetivo declarado não é força
de jogo: é entender como uma engine funciona por dentro, e fazê-la funcionar em equipe.

A fronteira do repositório é estreita e deliberada: **um binário que fala um protocolo texto
por stdin/stdout.** Nada além disso vive aqui.

- O **cliente** é de outra equipe, em `interface/` (C++/SFML). A IA tem pseudocódigo em
  `IA/`, ainda não ligado ao motor.
- Este diretório (`engine/`) entrega um executável que, hoje, na prática, é um **menu
  interativo** (`ui()` em `main.c`), não ainda o protocolo real.

A fase atual é **protótipo/demo**: um binário que joga xadrez legalmente do início ao fim,
com uma IA qualquer. Correção das regras vem primeiro; força de jogo vem por último, e só se
sobrar tempo. Bitboards, transposition table e magic bitboards estão explicitamente fora de
escopo.

---

## 2. Estado atual, honestamente

O projeto tem **~2550 linhas** em **12 módulos** + `main.c` (era ~1990 em 11 na revisão de
16/09). O crescimento veio de três frentes: `unmake_move` (`makemove.c` 129 → 185), o cavalo
(`movegen.c` 235 → 267, `square.c` 101 → 142) e um módulo novo, `io.h/c` (wrapper de string
e leitura de stdin, 279 linhas), feito para o REPL e para o futuro protocolo.

**O resumo desta revisão: as regras de movimento estão completas e corretas; a legalidade não
existe; e há um caminho reproduzível que derruba o processo.** O build compila com
**14 avisos** e zero erros (eram 8 em 16/09 — §5, "Sobre o Makefile e as flags").

### Funciona e está verificado com evidência

Tudo nesta tabela foi reproduzido nesta revisão — compilando sob ASan/UBSan e rodando um
harness descartável em `/tmp` (fora do repositório, como manda o `CLAUDE.md`).

| Área | Estado |
|---|---|
| `fen_parse` / `fen_write` | Os seis campos, validação completa, round-trip idêntico. Módulo maduro, sem mudança desde 07/09. Ver `docs/fen.md` |
| **Cavalo** | **Implementado** (árvore de trabalho, ainda não commitado). `KNIGHT_ATTACKS[64][8]` preenchida em `init_knight_attacks()` (`square.c:96`) via `offset_square`, com `SQ_NONE` nos saltos que saem do tabuleiro; `generate_knight_moves` (`movegen.c:128`) **checa `SQ_NONE` antes** de indexar o tabuleiro. Medido: a1 = 2 destinos, b1 = 3, c3 = 8, h8 = 2, **soma = 336**; todo destino é de fato um salto de cavalo |
| **`make_move`** | Completo: captura, roque movendo as duas peças, en passant, promoção nas quatro peças, `castling_rights` por origem **e** destino, `ep_square`, relógios, `king_square` |
| **`unmake_move`** | **Implementado** (`makemove.c:131`, commit `8344d1a`). Inverte o lado primeiro, desfaz promoção, devolve a torre do roque por `CASTLE_POSITIONS`, repõe o peão do en passant na casa certa, restaura os campos do `Undo` |
| **Round-trip make/unmake** | Em **todo nó** do perft abaixo: `fen_write` antes do `make` == `fen_write` depois do `unmake`, e `board_check_invariants` verde depois de cada um. **Zero falhas** em mais de 16 milhões de pares make/unmake |
| **Perft nas 6 posições de referência** | Ver a tabela abaixo. Todas batem |
| `board_check_invariants` | Código de peça válido, um rei de cada cor, cache `king_square` contra varredura, nenhum peão na 1ª/8ª. **4 das 6** checagens de `docs/guides.md` (Bug #7) |
| Roque (geração) | Direito presente **e** casas entre rei e torre vazias. Não verifica xeque nem casa atacada — de propósito, é trabalho do filtro (§3) |
| Peão | Push, push duplo aninhado, captura, promoção nas 4 peças (com e sem captura), en passant |
| Binário sob ASan/UBSan | Limpo em todo o perft acima. **Não** limpo nos caminhos dos Bugs #1 e #3 |

**A bateria que prova o gerador + make/unmake.** O motor só gera lances pseudo-legais, então o
harness acrescentou **o seu próprio** filtro de legalidade (um `is_square_attacked` escrito
do zero, só no harness, mais a regra de não rocar em xeque nem através de casa atacada). Os
números de referência são da [Chess Programming Wiki — Perft Results](https://www.chessprogramming.org/Perft_Results):

| Posição | Pseudo-legais (prof. 1) | Perft | Resultado | Esperado |
|---|---|---|---|---|
| Inicial | 20 | prof. 5 | 4 865 609 | ✅ 4 865 609 |
| Kiwipete | 48 | prof. 4 | 4 085 603 | ✅ 4 085 603 |
| Posição 3 | 16 (14 legais) | prof. 5 | 674 624 | ✅ 674 624 |
| Posição 4 | 38 (6 legais — em xeque) | prof. 4 | 422 333 | ✅ 422 333 |
| Posição 5 | 44 | prof. 4 | 2 103 487 | ✅ 2 103 487 |
| Posição 6 | 46 | prof. 4 | 3 894 594 | ✅ 3 894 594 |

O que isso prova: geração pseudo-legal, `make_move` e `unmake_move` estão corretos em roque,
en passant, promoção e direitos de roque — as posições 2, 4 e 5 existem exatamente para pegar
esses casos. O que **não** prova: nada sobre o filtro de legalidade do motor, porque ele não
existe. O filtro usado foi o do harness.

### Incompleto ou ausente

| Área | Estado |
|---|---|
| **Filtro de legalidade** | **Não existe.** Não há `is_square_attacked`, `in_check` nem `generate_legal`. O REPL aceita lance que deixa o próprio rei em xeque — e, no lance seguinte, aceita a captura do rei, que derruba o processo (Bug #1) |
| Perft no motor | Não existe (o da tabela acima é do harness) |
| Testes no repositório | **Não existem.** `test/test.c` foi apagado em `006c7ae`; sobrou `test/test_api.c`, **vazio**. `make test` **falha** (`test/test.c: No such file or directory`). Ver Bug #8 |
| Protocolo UCI | Não existe — só o menu `ui()`. `io.h/c` é a base para ele, mas tem dois bugs (Bugs #4, #5) |
| Corpus de FEN | Não existe como arquivo. São `#define TEST_FEN_*` em `main.c` |
| `CMakeLists.txt` | Existe, mas é um esqueleto quebrado (`add_executable(engine )` sem fontes — `cmake` falha com `No SOURCES given to target`). O build oficial continua sendo o Makefile |

---

## 3. Decisões arquiteturais firmadas

Escolhas conscientes com trade-off avaliado. Mudá-las agora custa caro.

### Representação: mailbox de 64 casas

`Piece array[64]`, não bitboards. `array[64]` = 64 bytes = uma linha de cache; varredura
completa medida em ~9,5 ns contra ~12,3 ns de uma piece list. Sem mudança.

### Indexação: `a1 = 0` (Little-Endian Rank-File)

`sq = rank * 8 + file`; `h8 = 63`. `PAWN_PUSH[WHITE] = +8`, `PAWN_PUSH[BLACK] = -8`. Sem
mudança.

### Codificação da peça

`piece.h`: `PIECE_MAKE(color, type) = (color << 3) | type`, com `WHITE = 1`, `BLACK = 0`,
`EMPTY = 0`. As duas armadilhas continuam valendo: `PIECE_COLOR(EMPTY) == BLACK`, e comparar
o valor cru contra um `PieceType` funciona por acidente para as pretas e falha para as
brancas.

### `is_own` / `is_enemy` / `is_empty` mudaram de lugar e de assinatura — **mudança desta revisão**

Antes (16/09), em `piece.h`, perguntando sobre uma **peça** e uma **cor**:

```c
static inline bool is_own(Piece p, Color c);
static inline bool is_enemy(Piece p, Color c);
```

Agora, em `board.h`, perguntando sobre uma **casa** de um **tabuleiro**, com `assert` de
casa dentro do tabuleiro:

```c
static inline bool is_empty(const Board *b, int sq);  /* era função em board.c */
static inline bool is_own  (const Board *b, int sq);   /* cor == b->side_to_move */
static inline bool is_enemy(const Board *b, int sq);   /* cor != b->side_to_move */
```

O ganho é real: o `assert(!SQ_OFFBOARD(sq))` transforma índice inválido em parada imediata
em vez de leitura fora do tabuleiro, e `piece.h` voltou a ser só codificação de peça.
`board.h` passou a incluir `square.h` e `<assert.h>` por causa disso.

**O custo, que vale saber antes da Fase 5:** a cor agora é **implícita** —
`b->side_to_move`. Dois lugares vão tropeçar nisso:

- **Depois de `make_move`, `side_to_move` já virou.** `is_own(b, sq)` passa a significar "é
  do adversário de quem acabou de jogar". O filtro de legalidade roda exatamente nesse
  instante.
- **`is_square_attacked(b, sq, by)` recebe a cor como parâmetro**, e ela nem sempre é o lado
  a jogar. Esses helpers não servem lá; vai ser preciso uma variante com `Color` explícita
  (que é, na prática, a assinatura antiga de volta, com outro nome).

### Codificação do lance: `u16` de 6+6+4 bits

```
       tipo (4)   destino (6)   origem (6)
u16 = [0000]      [000000]      [000000]
       <<12          <<6           <<0
```

Esquema da Chess Programming Wiki ([Encoding Moves](https://www.chessprogramming.org/Encoding_Moves)):
bit 3 (`& 8`) é promoção, bit 2 (`& 4`) é captura, e os dois bits baixos, quando há
promoção, indexam a peça (`KNIGHT + (type & 3)`). `MV_CASTLE_KING = 2`, `MV_CASTLE_QUEEN = 3`
e `MV_EP_CAPTURE = 5` **são códigos, não bits** — `move_is_castle` e `move_is_ep_capture`
comparam por igualdade (`move.c:32-44`). Quem for adicionar um predicado novo sobre
`MoveType`: **teste por `&` só nos dois bits que são bits.**

### Direções: tabela de distância até a borda

`SQ_TO_EDGE[64][8]`, ordem do enum como carga estrutural (ortogonais 0–3, diagonais 4–7),
para que torre, bispo e rainha usem uma única função parametrizada por intervalo. O rei usa a
mesma tabela com o laço parando em 1.

### Cavalo por tabela pré-computada — ✅ implementado nesta revisão

Decidido em 10/09, feito agora. `KNIGHT_ATTACKS[64][8]` (o nome mudou: era
`KNIGHT_TARGETS`), preenchida no init a partir de `KNIGHT_VECTORS[8][2]` (pares
`{Δfileira, Δcoluna}`) por `offset_square` — a **mesma** função que já construía
`PAWN_ATTACKS`, então não nasceu um segundo mecanismo de borda. Saltos para fora do
tabuleiro ficam `SQ_NONE`, e o gerador pula esses antes de qualquer acesso ao tabuleiro.

A tabela tem sempre 8 entradas por casa, com buracos `SQ_NONE` — não há `KNIGHT_COUNT`. É
uma variação legítima do que o roadmap sugeria; o custo é um `if` por salto.

`KING_TARGETS[64][8]` **continua alocada, zerada e sem leitor** — o rei usa `SQ_TO_EDGE`,
como decidido. Apagar.

### Pseudo-legal primeiro, legalidade depois

Sem mudança de decisão. O gerador de roque verifica direito e casas vazias, e **não**
verifica se o rei está em xeque, atravessa casa atacada ou chega em casa atacada. Atenção: o
filtro "aplica e vê se o rei ficou atacado" só cobre a **casa final**. As outras duas
condições do roque (não estar em xeque; não atravessar casa atacada) têm de ser verificadas
explicitamente — o harness desta revisão precisou delas para bater o perft da Kiwipete.

### `Undo` é do chamador — ✅ implementado nos dois lados

```c
typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);
void unmake_move(Board *b, Move move, const Undo *u);
```

A decisão de 10/09 continua inteira: a pilha da recursão é a pilha de undo; não existe pilha
global. **Mudou um detalhe desde 16/09:** no en passant, `make_move` agora grava em
`u->captured` o **peão realmente capturado** (`makemove.c:109`), e não `EMPTY`. O
`unmake_move` o repõe em `u->ep_square - PAWN_PUSH[lado]`. A assimetria que o documento
anterior descrevia deixou de existir.

**Limite conhecido:** `unmake_move` só recalcula `king_square` do lado que jogou. Se o lance
desfeito capturou um rei, o cache do outro lado fica errado — ver Bug #2.

### Contrato de validação: fronteira valida, núcleo confia

Sem mudança. `make_move` é primitiva, não serviço. A consequência concreta continua sendo a
mesma: hoje **nenhuma** camada impede um lance ilegal — o REPL entrega a `make_move`
qualquer lance que esteja na lista pseudo-legal. Isso é esperado até a Fase 5; o que não é
esperado é o processo cair (Bug #1).

### `board_check_invariants` sem out-param

`bool board_check_invariants(const Board *b)`, com as falhas num log global em `utils.c`
(`fail_msg` / `print_fail_log`). Sem mudança desde 16/09, e as duas consequências continuam
abertas (Bug #7).

### Testes como subcomando do binário

**Ainda não implementado.** A decisão de 10/09 (`./main.out test`, `./main.out perft N`)
continua de pé. E agora o caso a favor dela é mais forte: o harness desta revisão — perft
nas seis posições, com round-trip e invariantes em todo nó — tem ~150 linhas e roda em
10 segundos. Ele mora em `/tmp` e some. Dentro do repositório, seria o portão da Fase 4.

### Sistema de build: Makefile

CMake fica para quando o cliente precisar integrar. O `CMakeLists.txt` que apareceu na raiz
de `engine/` é um esqueleto sem fontes e não compila (Bug #8) — ou é completado, ou é
apagado, mas não deve ficar como está, porque parece um segundo sistema de build.

---

## 4. Mapa dos módulos

Headers em `include/`, implementação em `src/`. Todo `.c` inclui o próprio `.h` primeiro, e
todo header compila sozinho (`gcc -fsyntax-only -x c include/foo.h` — verificado para os 11).

| Arquivo | Papel | Linhas (.h/.c) | Nota |
|---|---|---|---|
| `types.h` | Typedefs de largura fixa, `MAX_*`, `MIN`/`MAX`, `MemoryZero*` | 57 / — | Ainda arrasta `ctype.h stdio.h stdlib.h string.h`. `PrintSize` e `MemoryZero*` sem uso |
| `log.h/c` | `LOG_ERROR` (sempre) / `LOG_DEBUG` (só com `-DDEBUG`) | 20 / 20 | Correto |
| `piece.h/c` | `Color`, `PieceType`, `Piece`, macros, char ↔ peça | 42 / 27 | **Perdeu** `is_own`/`is_enemy` (foram para `board.h`) |
| `square.h/c` | Coordenadas, `SQ_TO_EDGE`, `DIR_OFFSET`, `PAWN_PUSH`, `PAWN_ATTACKS`, **`KNIGHT_ATTACKS`** | 47 / 142 | Cavalo novo. `KING_TARGETS` órfã; `DIR_CHARMAP` sem uso; `FILE_DIST` sem uso |
| `board.h/c` | `Board`, `CastleRights`, `CASTLE_POSITIONS`, `BOARD_START_POS`, `is_empty`/`is_own`/`is_enemy`, `board_find_king`, `board_check_invariants`, `board_clear`, `board_new`, `board_print`, `fill_sq` | 80 / 156 | `board_new` monta a posição inicial (a inversão de fileira está correta). Bug #7 |
| `fen.h/c` | `fen_parse`, `fen_write`, `START_FEN` | 13 / 612 | Maduro. `castling_matches_board` (`fen.c:385`) segue `static` — é o que o Bug #7 precisa |
| `move.h/c` | `Move`, `MoveType`, `MoveList`, encode/decode, predicados, `movelist_*`, str ↔ lance | 58 / 176 | Bug #9 |
| `makemove.h/c` | `Undo`, `make_move`, `unmake_move` | 18 / 185 | **Os dois completos**. Bug #2 |
| `movegen.h/c` | Peão, deslizantes, **cavalo**, rei, roque, `generate_all_moves` | 12 / 267 | Bugs #1, #10, #11 |
| `io.h/c` | `String` (fatia sem posse), trim/split/parse, `read_line`/`read_word`/`read_int`, `join_args` | 52 / 227 | **Módulo novo.** Bugs #4, #5, #12 |
| `utils.h/c` | `fail_msg`/`print_fail_log`, `COLOR_CHAR`, `strslc`, `clear_screen`, `get_fen`, `get_int` | 19 / 107 | Bug #7. `utils.h` inclui `board.h` sem precisar |
| `main.c` | Menu interativo `ui()` (9 opções) + `main()` | — / 210 | Bug #3. FEN de partida é `#define` |
| `test/test_api.c` | — | — / 0 | Vazio |

**Problemas estruturais que atravessam módulos:**

- **Seis funções com linkage externo que deveriam ser `static`** (são os avisos de
  `-Wmissing-prototypes`): `print_move` (`move.c:162`), `check_pawn_promotion`
  (`movegen.c:7`), `genenare_moves_from_direction` (`movegen.c:79`), `check_allowed_castles`
  (`movegen.c:184`), `string_to_cstr_static` e `string_split_next` (`io.c:42`, `io.c:130`).
  E uma que deveria estar **no header**: `generate_knight_moves` (`movegen.c:128`) é pública
  mas `movegen.h` não a declara.
- **Duas formas de achar o rei no mesmo arquivo**, ainda: `generate_king_moves` lê o cache
  `b->king_square[side]` e `check_allowed_castles` chama `board_find_king` (varredura). E
  `make_move`/`unmake_move` refazem a varredura completa a cada lance, em vez de atualizar o
  cache só quando a peça movida é rei — o cache deixou de ser cache.
- **`const` regrediu:** `generate_pawn_moves` agora recebe `Board *` (era `const Board *`),
  e `generate_all_moves` continua `Board *`. Nenhuma das duas escreve no tabuleiro.
- O typo `genenare_moves_from_direction` continua.
- Globais sem declaração em header: `PIECE_CHAR` (`piece.c:5`) e `KNIGHT_VECTORS`
  (`square.c:33`) têm linkage externo e ninguém os declara — deveriam ser `static`.

---

## 5. Bugs abertos

Numeração nova nesta revisão. Entre parênteses, o número antigo quando o bug já existia.

**Fechados desde 16/09:** antigo #1 (`unmake_move` sem corpo — implementado e verificado) e
antigo #2 (cavalo não gerado — implementado e verificado). O antigo #3 **continua aberto e
ficou mais visível**: agora é o Bug #1 abaixo.

### Bloqueantes para a demo

| # | Onde | Problema |
|---|---|---|
| 1 | `movegen.c:213-222`, `movegen.c:187-188` | **O processo cai quando um rei é capturado.** (antigo #3) Sem filtro de legalidade, o REPL aceita lance que deixa o próprio rei em xeque; o adversário então captura o rei (é pseudo-legal), `make_move` grava `king_square = SQ_NONE`, e o próximo `generate_all_moves` lê `SQ_TO_EDGE[-1][dir]`. **Reproduzido numa partida real** a partir da posição inicial: `f2f3 e7e5 g2g4 d8h4 a2a3 h4e1`, e a opção 6 do menu dá `movegen.c:222: runtime error: index -1 out of bounds` + `AddressSanitizer: global-buffer-overflow` (lendo o fim de `KNIGHT_ATTACKS`). No build sem sanitizer o processo aborta em `check_allowed_castles: Assertion 'king_sq != SQ_NONE' failed`. A correção de verdade é a Fase 5 (o filtro torna a captura de rei inalcançável); até lá, `generate_king_moves` precisa tratar `SQ_NONE` em vez de confiar em pré-condição |
| 2 | `makemove.c:185` | **`unmake_move` não restaura o `king_square` do lado capturado.** Só recalcula `king_square[moved_side]`. Se o lance desfeito capturou um rei, o tabuleiro volta certo mas o cache do outro lado fica `SQ_NONE`. Reproduzido: em `R6k/8/8/8/8/8/8/K7 w - - 0 1`, `make(a8h8)` + `unmake` devolve a FEN idêntica, mas `king_square[BLACK] = -1` (o rei está em 63) e `board_check_invariants` falha com `Black king square cache does not match`. Só acontece junto com o Bug #1 — mas é exatamente o caso que o filtro de legalidade vai exercitar se rodar sobre posição já ilegal |

### Graves

| # | Onde | Problema |
|---|---|---|
| 3 | `main.c:168-175` | **A opção 7 do menu ("Edit square") escreve fora do tabuleiro.** `read_coord()` devolve `SQ_NONE` para coordenada inválida e o código faz `b->array[-1] = p` sem checar. Reproduzido: entrada `7`, `z9`, `.` → `main.c:174: index -1 out of bounds` + `AddressSanitizer: stack-buffer-underflow` (WRITE). Além disso, a edição **não atualiza `king_square`**: reproduzido — mover o rei branco de e1 para e4 pela opção 7 faz o gerador produzir **zero** lances de rei (ele continua gerando a partir de e1, que está vazia). Também não reconcilia direitos de roque/en passant, nem zera `last_move` (um "Unmake" depois da edição usa um `Undo` de outra posição). `-Wconversion` aponta duas linhas exatamente aqui (`char pc = fgetc(...)` perde o `EOF`) |
| 4 | `io.c:181` | **`read_word` usa buffer não inicializado em fim de arquivo.** `read_line` devolve `0` em EOF e `-1` em linha longa; `read_word` só testa `-1`. Em EOF, `temp` não foi escrito e é lido mesmo assim. Reproduzido no build normal: com a entrada `e2e4\n`, a primeira chamada devolve `e2e4`, e **a segunda e a terceira, já em EOF, devolvem `e2e4` de novo, com sucesso** — lixo de pilha que por acaso era a palavra anterior. Nenhum sanitizer pega isso (ver `CLAUDE.md` §7). Importa porque `io.c` é a base do protocolo: um cliente que fecha o pipe faria o motor repetir o último comando |
| 5 | `io.c:205-228` | **`join_args` não coloca separador entre os argumentos**, embora o comentário diga que sim (`len + n + 2 -> ... + espaço + \0`). Reproduzido: juntando os seis campos da FEN inicial sai `rnbqkbnr/.../RNBQKBNRwKQkq-01`, que `fen_parse` rejeita (`too few fields`). Ainda sem chamador — mas é a função feita para montar `position fen ...` |
| 6 | `movegen.c:5` | `#include "../include/assert.h"` aponta para um arquivo **que não existe**. Compila por acidente: o GCC tenta o caminho relativo em cada diretório de sistema e `/usr/include/` + `../include/assert.h` cai em `/usr/include/assert.h` (confirmado com `gcc -H`). Em outro compilador ou outra árvore de includes, é erro de build. E é redundante — `board.h` já inclui `<assert.h>` |
| 7 | `board.c:41`, `utils.c:14` | (antigos #6 e #7) `board_check_invariants` cobre **4 das 6** checagens de `docs/guides.md`: faltam `ep_square` coerente com `side_to_move` e direitos de roque coerentes com rei/torres (esta **já está escrita**: `castling_matches_board`, `static` em `fen.c:385`). E `fail_msg` acumula num log global **que nunca é zerado** (teto 128) e recebe `char *` em vez de `const char *` — os 6 avisos de `-Wwrite-strings` |
| 8 | `Makefile:36-37`, `CMakeLists.txt` | **Não há portão de teste, e os dois alvos de build "extras" estão quebrados.** `make test` compila `test/test.c`, apagado em `006c7ae` → `fatal error: test/test.c: No such file or directory`. `CMakeLists.txt` não lista fontes → `No SOURCES given to target: engine` |

### Menores / higiene

| # | Onde | Problema |
|---|---|---|
| 9 | `move.c:112` | (antigo #5) `promotion_type = EMPTY;` atribui `PieceType` a `MoveType` (`-Wenum-conversion`). Funciona porque `EMPTY == MV_QUIET == 0`. Na mesma função: promoção em maiúscula (`e7e8Q`) ou letra inválida (`e7e8x`) vira lance quieto em silêncio — no REPL, o usuário recebe "invalid move" sem saber por quê |
| 10 | `movegen.c:128-157` | No cavalo novo: `piece_to` é atribuída e nunca lida (`-Wunused-but-set-variable`); o filtro de cor usa `PIECE_COLOR(piece) != side` em vez de `is_own(b, from)` (correto só porque a linha anterior já descartou casa vazia — a mesma armadilha "lembrada, não encapsulada" do peão); e o último `else if (is_enemy(...))` é sempre verdadeiro. Nenhum desses produz lance errado — o perft prova isso |
| 11 | `movegen.c:56`, `movegen.c:190` | (antigos #11 e #10) `piece = b->array[to];` virou atribuição morta desde que a captura passou a usar `is_enemy(b, to)`. `static const enum {LEFT, RIGHT};` — "useless storage class specifier" |
| 12 | `io.c:112`, `io.c:131`, `io.c:121` | `char *p = line;` descarta o `const` do parâmetro (aviso do GCC mesmo sem flags extras — em C isso é violação de restrição). `if (*p \|\| *p == '\0')` é sempre verdadeiro. `string_to_cstr` não confere o `malloc`; `string_parse_int` não detecta estouro de `int` |
| 13 | `makemove.c:8`, `makemove.c:87-99` | (antigos #12, #13) `check_rook_squares` também trata `E1`/`E8` (o nome mente). A torre do roque é posta com `fill_sq(b, "d1", ...)` — parsing de string no caminho mais quente, com o `bool` de retorno descartado. `CASTLE_POSITIONS` (usada no `unmake`) já tem as casas como números; o `make` podia usar a mesma tabela, e as duas metades ficariam simétricas |
| 14 | `move.c:125` | (antigo #14) `movelist_add` loga e descarta quando a lista enche, em vez de `assert` |
| 15 | `main.c:200` | Se a FEN inicial falhar, `b = (Board){0}` deixa `ep_square = 0` (= a1, não `SQ_NONE`) e os dois reis "em a1". Hoje inalcançável (a FEN é fixa e válida); `board_new(&b)` seria o certo |
| 16 | vários | Sobras: `KING_TARGETS` órfã, `DIR_CHARMAP` e `CASTLE_TYPE_MASK` sem uso (antigos #8, #9), `read_move` em `main.c:20` sem uso, `PrintSize`/`FILE_DIST`/`MemoryZero*` sem uso. Comentários desatualizados: `main.c:93-97` e `Makefile:31` dizem que `LOG_ERROR` some fora do debug (não é verdade desde 10/09); o cabeçalho de `fen.c` ainda diz `board.c`/`parse_fen`; `TEST_FEN_EN_PASSANT` tem `-` no campo de en passant |
| 17 | `fen.c` | `fen_parse` aceita posição em que **o lado que não joga está em xeque** (ex.: `R6k/8/8/8/8/8/8/K7 w - - 0 1`) — posição ilegal, e é a porta de entrada por FEN dos Bugs #1/#2. Só dá para rejeitar quando `is_square_attacked` existir; entra junto com a Fase 5 |

### Sobre o Makefile e as flags

`make debug` hoje: **14 avisos**, zero erros (eram 8 em 16/09). A conta:

- 7 de `-Wmissing-prototypes` — seis funções que deveriam ser `static` e uma
  (`generate_knight_moves`) que deveria estar em `movegen.h` (§4);
- 2 de `-Wdiscarded-qualifiers` em `io.c` (Bug #12);
- 1 `-Wenum-conversion` (Bug #9), 1 `-Wunused-but-set-variable` (Bug #10),
  1 "useless storage class" (Bug #11), 1 `-Wunused-const-variable` (`DIR_CHARMAP`),
  1 `-Wunused-function` (`read_move`).

Medido nesta revisão, as quatro flags que o `CLAUDE.md` §7 pede e o Makefile ainda não liga:

| Flag | Custo | O que pega |
|---|---|---|
| `-Wshadow` | **0** | De graça |
| `-Wcast-qual` | **0** | De graça |
| `-Wwrite-strings` | +6 | Todos `fail_msg(char *)` recebendo literal (Bug #7). Uma palavra fecha os seis |
| `-Wconversion` | +2 | **Os dois em `main.c:172-174`, exatamente no caminho do Bug #3.** É a flag apontando o bug antes de alguém achá-lo |

Recomendação: ligar `-Wshadow`, `-Wcast-qual`, `-Wwrite-strings` e `-Wconversion` **já** — o
custo total são oito avisos, e todos apontam para bug real ou para a correção de uma
palavra. (Os oito `&= ~(CASTLE_*)` em `u8` que `-Wconversion` acusava em 16/09 não aparecem
mais nesta versão do GCC sob `-Wconversion`; só sob `-Wsign-conversion`, que não está no
acordo.)

---

## 6. Decisões em aberto

| Decisão | Situação |
|---|---|
| **Sistema de build** | Makefile. Decidir o destino do `CMakeLists.txt` quebrado (completar ou apagar) |
| **Cor implícita vs. explícita nos helpers de casa** | Nova. `is_own`/`is_enemy` usam `side_to_move`; `is_square_attacked` vai precisar de cor explícita. Decidir antes da Fase 5 se nasce uma variante `(b, sq, color)` ou se os helpers atuais ganham o parâmetro (§3) |
| **`Move` sem score** | Confirmado. Entra quando começar a IA, em outra estrutura |
| **Camada de integração cliente ↔ motor** | **Ainda em aberto, e bloqueia a Fase 6a.** O cliente C++ em `interface/` roda o motor como subprocesso. Decide se o protocolo precisa de um comando `legalmoves`. Conversa curta com a equipe do cliente, antes de escrever o protocolo |
| **Onde mora o corpus de FEN** | `const char *[]` num `.c` (recomendado em `next_steps.md` §B6) ou arquivo lido em runtime. As seis posições da tabela do §2 são o começo natural |

---

## 7. Próximos passos, em ordem

1. **Commitar o cavalo** (está só na árvore de trabalho) — em mensagem em português, num PR.
   Ele está verificado; perder isso num `checkout` seria caro.
2. **Trazer o perft e o round-trip para dentro do repositório** como subcomando
   (`./main.out perft N`, `./main.out test`) e consertar `make test` (Bug #8). É o mesmo
   código do harness desta revisão, e os números do §2 viram o critério automático. Fecha o
   portão da Fase 4 de verdade — hoje ele só fechou "em /tmp".
3. **Filtro de legalidade (Fase 5):** `is_square_attacked` por busca reversa,
   `generate_legal` por aplica-e-testa, e as duas checagens extras do roque (não estar em
   xeque, não atravessar casa atacada). Isso fecha o Bug #1 pela raiz e destrava mate e
   afogamento. Junto: rejeitar em `fen_parse` a posição com o lado que não joga em xeque
   (Bug #17). Decidir antes a questão da cor explícita (§6).
4. **Guardas baratas enquanto o filtro não chega:** `SQ_NONE` em `generate_king_moves`
   (Bug #1), recalcular os dois reis no `unmake_move` (Bug #2), validar a coordenada e
   atualizar o cache na opção 7 do menu (Bug #3). São poucas linhas e tiram três caminhos
   de crash da demo.
5. **`io.c` antes do protocolo:** tratar EOF em `read_word` (Bug #4) e pôr o separador em
   `join_args` (Bug #5). O protocolo vai ser construído em cima dos dois.
6. **Fechar os 14 avisos** e ligar as quatro flags (§5). Custo: uma sessão curta.
7. Só então: protocolo (Fase 6a) e IA aleatória — que, com o filtro pronto, é "sortear um
   elemento de `generate_legal`".

---

## 8. Referências ativas

- **Chess Programming Wiki**: [Square Attacked By](https://www.chessprogramming.org/Square_Attacked_By)
  (a etapa imediata), [Legal Move](https://www.chessprogramming.org/Legal_Move),
  [Castling](https://www.chessprogramming.org/Castling) (as três condições de casa atacada),
  [Perft](https://www.chessprogramming.org/Perft) e
  [Perft Results](https://www.chessprogramming.org/Perft_Results) (as seis posições do §2),
  [Encoding Moves](https://www.chessprogramming.org/Encoding_Moves).
- **TSCP** — `attack()` e `in_check()` em mailbox legível; boa leitura para o
  `is_square_attacked`. Note que ele usa pilha global de histórico, o desenho oposto ao
  nosso.
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas, ler inteira antes do protocolo.
  Em especial o que acontece quando a entrada acaba (Bug #4).
- `man gcc`, seção *Options to Request or Suppress Warnings*; e, para o Bug #6, a seção
  *Search Path* de `man cpp` / documentação do GCC sobre `#include "..."`.

**Pendências de documentação:** `docs/roadmap-motor.md` e `docs/next_steps.md` ainda
descrevem 16/09 (cavalo ausente, `unmake_move` sem corpo, Fases 2/3/4 parciais). O cabeçalho
de `docs/fen.md` ainda diz "módulo `src/board.c`".

---

## Apêndice — referência rápida dos módulos

Typedefs, enums, macros e assinaturas, na ordem de dependência. Comentários `/* … */` ao lado
de uma linha apontam para o bug correspondente no §5. Includes, guards e linhas em branco
omitidos. Reflete a **árvore de trabalho em 01/10** (commit `8344d1a` + alterações não
commitadas).

### `types.h` — vocabulário mínimo

```c
typedef uint64_t u64;  typedef uint32_t u32;  typedef uint16_t u16;  typedef uint8_t u8;
typedef int16_t i16;

#define INPUT_STR_SIZE 128
#define MAX_FEN_STRING 256
#define BOARD_SIZE     64
#define BOARD_WIDTH    8
#define MAX_MOVES      256
#define MAX_SEARCH_PLY 64
#define MAX_GAME_PLY   1024
#define NUM_COLORS     2

#define MemoryZero(addr, size)     memset((addr), 0x0, (size))
#define MemoryZeroStruct(addr, st) MemoryZero((addr), sizeof(st))
#define PrintSize(type)            /* sem uso */
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
```

### `piece.h` — codificação de peça

```c
typedef enum { WHITE = 1, BLACK = 0 } Color;
typedef enum { EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3,
               ROOK = 4, QUEEN = 5, KING = 6 } PieceType;
typedef u8 Piece;

#define PIECE_MAKE(color, type) ((Piece)((((color) & 1u) << 3) | ((type) & 7u)))
#define PIECE_TYPE(p)           ((PieceType)((p) & PIECE_TYPE_MASK))
#define PIECE_COLOR(p)          ((Color)(((unsigned)(p) >> 3) & PIECE_COLOR_MASK))
#define NO_PIECE                ((Piece)0)

char  piece_to_char(Piece p);
Piece piece_from_char(char c);
/* is_own / is_enemy NAO moram mais aqui -- ver board.h */
```

### `square.h` — geometria do tabuleiro

```c
#define SQ_NONE (-1)
#define RANK_OF(sq)       ((sq) / BOARD_WIDTH)
#define FILE_OF(sq)       ((sq) % BOARD_WIDTH)
#define FILE_DIST(d, o)   /* sem uso */
#define SQ_AT(rank, file) (int)((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq)   (((sq) < 0) || ((sq) >= BOARD_SIZE))

enum { SQ_A1 = 0, SQ_E1 = 4, SQ_H1 = 7, SQ_A8 = 56, SQ_E8 = 60, SQ_H8 = 63,
       SQ_C1 = 2, SQ_G1 = 6, SQ_C8 = 58, SQ_G8 = 62 };

typedef enum { DIR_N, DIR_S, DIR_E, DIR_W,
               DIR_NE, DIR_SW, DIR_SE, DIR_NW, NUM_DIRS } Direction;

extern const int DIR_OFFSET[NUM_DIRS];              /* {+8,-8,+1,-1,+9,-9,-7,+7} */
extern const int PAWN_PUSH[NUM_COLORS];             /* [BLACK]=-8 [WHITE]=+8 */
extern int SQ_TO_EDGE[BOARD_SIZE][NUM_DIRS];
extern int KNIGHT_ATTACKS[BOARD_SIZE][8];           /* SQ_NONE nos saltos para fora */
extern int KING_TARGETS[BOARD_SIZE][8];             /* orfa: zerada, sem leitor -- apagar */
extern int PAWN_ATTACKS[NUM_COLORS][BOARD_SIZE][2]; /* SQ_NONE nas bordas */

void init_square_tables(void);   /* SQ_TO_EDGE + PAWN_ATTACKS + KNIGHT_ATTACKS */
int  sq_from_coord(const char *coord);
void sq_to_coord(int sq, char out[3]);
```

### `board.h` — o tabuleiro

```c
static const char BOARD_START_POS[64];   /* ordem VISUAL: indice 0 = a8 */

enum CastleRights {
    CASTLE_NONE = 0, CASTLE_WK = 1, CASTLE_WQ = 2, CASTLE_BK = 4, CASTLE_BQ = 8,
    CASTLE_WHITE = CASTLE_WK | CASTLE_WQ,
    CASTLE_BLACK = CASTLE_BK | CASTLE_BQ,
    CASTLE_ALL   = CASTLE_WHITE | CASTLE_BLACK
};

/* [cor][0 = lado da dama, 1 = lado do rei][0 = casa da torre DEPOIS do roque,
                                            1 = casa de origem da torre]      */
static const int CASTLE_POSITIONS[NUM_COLORS][2][2];

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int   king_square[2];    /* cache derivado */
    u8    castling_rights;   /* bitmask CASTLE_* */
    int   ep_square;         /* SQ_NONE se nao houver */
    int   halfmove_clock;
    int   fullmove_number;
} Board;

int  board_find_king(const Board *b, Color c);   /* -1 se nao achar */
bool board_check_invariants(const Board *b);     /* 4 das 6 checagens -- Bug #7 */
void board_clear(Board *b);
void board_new(Board *new);                      /* posicao inicial */
void board_print(const Board *board);
bool fill_sq(Board *b, const char *sq_str, Piece p);

/* todos com assert(!SQ_OFFBOARD(sq)); a cor de referencia e b->side_to_move */
static inline bool is_empty(const Board *b, int sq);
static inline bool is_own  (const Board *b, int sq);
static inline bool is_enemy(const Board *b, int sq);
```

`board_clear` faz `memset` e restaura `ep_square`, `king_square[2]` (para `SQ_NONE`) e
`fullmove_number`; `side_to_move` fica `0`, que é `BLACK`. `board_new` chama `board_clear` e
monta a posição inicial com `side_to_move = WHITE` e `CASTLE_ALL`.

### `fen.h`

```c
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
bool fen_parse(const char *fen_string, Board *out);   /* so escreve *out se tudo validar */
void fen_write(const Board *board, char fen_out[MAX_FEN_STRING]);
```

### `move.h` — o lance e a lista de lances

```c
typedef u16 Move;
#define MOVE_NONE ((Move)0)

typedef enum {
    MV_QUIET       =  0, MV_DOUBLE_PUSH =  1, MV_CASTLE_KING =  2, MV_CASTLE_QUEEN =  3,
    MV_CAPTURE     =  4, MV_EP_CAPTURE  =  5, /* 6 e 7 nao existem */
    MV_PROMO_N     =  8, MV_PROMO_B     =  9, MV_PROMO_R     = 10, MV_PROMO_Q      = 11,
    MV_PROMO_CAP_N = 12, MV_PROMO_CAP_B = 13, MV_PROMO_CAP_R = 14, MV_PROMO_CAP_Q  = 15
} MoveType;

typedef struct { Move moves[MAX_MOVES]; int count; } MoveList;

Move      encode_move(int from, int to, MoveType type);
int       move_from(Move m);
int       move_to(Move m);
MoveType  move_type(Move m);
bool      move_is_capture(Move m);      /* & 4  -- bit de verdade  */
bool      move_is_promotion(Move m);    /* & 8  -- bit de verdade  */
bool      move_is_castle(Move m);       /* IGUALDADE, nao mascara */
bool      move_is_ep_capture(Move m);   /* IGUALDADE, nao mascara */
PieceType move_promo_type(Move m);      /* KNIGHT + (type & 3)    */

void movelist_clear(MoveList *l);
void movelist_add(MoveList *l, Move m);           /* loga e descarta se cheia -- Bug #14 */
int  movelist_find(MoveList *l, const char *uci); /* indice, -1 se nao achar */
void move_to_str(Move m, char out[6]);
Move move_from_str(const char *in);               /* Bug #9 */
void print_moves(MoveList *list);
```

`movelist_find` casa por origem, destino **e** `move_promo_type` — é o que faz `e7e8q` achar
a promoção certa entre as quatro, e `e1g1` achar o roque.

### `makemove.h` — aplicar e desfazer

```c
typedef struct {
    Piece captured;          /* no en passant: o peao capturado (nao EMPTY) */
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);
void unmake_move(Board *b, Move move, const Undo *u);   /* Bug #2 */
```

Ordem interna de `make_move`: decodifica → **salva o `Undo` antes de tocar no tabuleiro** →
move a peça e inverte o lado → `ep_square` → `king_square` (varredura) → direitos de roque
(origem e destino) → torre do roque → peão do en passant → promoção → `halfmove_clock` →
`fullmove_number`.

Ordem interna de `unmake_move`: inverte o lado → se promoção, volta a peça em `to` para peão
→ se roque, devolve a torre por `CASTLE_POSITIONS` → devolve a peça para `from` → repõe a
capturada (em `to`, ou atrás de `to` no en passant) → restaura `ep_square`,
`castling_rights`, `halfmove_clock`, decrementa `fullmove_number` se as pretas jogaram →
recalcula `king_square` **só do lado que jogou**.

### `movegen.h` — geração de lances

```c
void generate_pawn_moves   (Board *board, MoveList *list);   /* const perdido -- §4 */
void generate_sliding_moves(const Board *board, MoveList *list);
void generate_king_moves   (const Board *b, MoveList *list); /* Bug #1 */
void generate_all_moves    (Board *b, MoveList *list);       /* limpa a lista; peao,
                                                                deslizantes, rei, cavalo */
/* generate_knight_moves(const Board *, MoveList *) existe em movegen.c mas NAO esta
   declarada aqui -- §4 */
```

Não há `generate_legal` — todo lance aqui é pseudo-legal.

### `io.h` — strings e leitura

```c
typedef struct { const char *data; size_t len; } String;   /* fatia, nao dona */

String string_make(const char *data, size_t len);
String string_from_cstr(const char *cstr);
char  *string_to_cstr(String s);            /* malloc -- o chamador libera */
void   string_trim(String *s);              /* + _left, _right, chop_left, chop_right */
bool   string_eq(String a, String b);
bool   string_starts_with(String s, String prefix);
bool   string_parse_int(String s, int *out);
int    string_split(const char *line, String argv[], int max_split);

int  read_line(char *buf, size_t cap);      /* 1 ok, 0 EOF, -1 linha longa */
bool read_word(char *buf, size_t cap);      /* Bug #4 */
bool read_int(int *out);
bool join_args(int argc, char **argv, int start, char *out_buf, size_t buf_cap);  /* Bug #5 */
```

### `log.h` / `utils.h`

```c
void log_emit(const char *level, const char *file, int line,
              const char *func, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

#define LOG_ERROR(...) /* sempre ativo */
#define LOG_DEBUG(...) /* so com -DDEBUG */

extern const char *COLOR_CHAR[2];   /* {"BLACK", "WHITE"} */
void print_piece_chart(void);
void get_fen(char fen[MAX_FEN_STRING]);
int  get_int(const char *msg);
void clear_screen(void);
void strslc(const char *src, char *dest, int start, int end);
void fail_msg(char *msg);           /* deveria ser const char * -- Bug #7 */
void print_fail_log(void);          /* e falta um reset -- Bug #7 */
```

---

*Este documento é escrito à mão. A próxima revisão deveria ser disparada por um evento
concreto — o filtro de legalidade passando no perft das seis posições **dentro** do
repositório — não por passagem de tempo.*
