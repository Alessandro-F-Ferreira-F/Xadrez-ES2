# Contexto do Projeto — Chess Engine em C

> Documento de contexto para retomar o projeto ou apresentá-lo a quem for trabalhar nele,
> sem precisar ler a codebase inteira.
> Estado em **13 de setembro de 2026**, revisão pós-refatoração (commit `8f8a52d`,
> "organização do código; módulos bem divididos").
>
> **`docs/next_steps.md` está desatualizado em relação a este documento.** Ele foi escrito
> em 10 de setembro contra o `movegen.c` monolítico de antes da refatoração — os números de
> linha que cita não existem mais nos arquivos atuais, e vários bugs que ele descreve foram
> fechados ao mesmo tempo em que outros, novos, apareceram no meio do split. As decisões
> arquiteturais de lá (Undo do chamador, split de módulos) **continuam valendo** — só a lista
> de bugs e a numeração de linha precisam de uma nova passada antes de servir de guia de
> novo. Este documento é a fonte de verdade sobre o estado atual.

---

## 1. O que é este repositório

Um **motor de xadrez em C**, escrito do zero como projeto de aprendizado — agora também o
repositório oficial do grupo, na disciplina de Engenharia de Software 2. O objetivo declarado
não é força de jogo — é entender como uma engine funciona por dentro, e fazê-la funcionar em
equipe.

A fronteira do repositório é estreita e deliberada: **um binário que fala um protocolo texto
por stdin/stdout.** Nada além disso vive aqui.

- O **cliente** é de outra equipe, em outro repositório (ou outra parte deste, fora de `engine/`).
- Este repo entrega um executável que recebe comandos por linha e responde por linha —
  hoje, na prática, um menu interativo (`ui()`), não ainda o protocolo real.

A fase atual é **protótipo/demo**: um binário que joga xadrez legalmente do início ao fim,
com uma IA qualquer. Correção das regras vem primeiro; força de jogo vem por último, e só
se sobrar tempo. Bitboards, transposition table e magic bitboards estão explicitamente
fora de escopo.

---

## 2. Estado atual, honestamente

O projeto tem **~1640 linhas** em **10 módulos**. Cresceu bastante desde a última revisão
(~750 linhas, 5 módulos) — a refatoração de 12/09 dividiu `board.c` como planejado, e foi
além: `piece.h/c` também saiu do que antes era `types.h`. É uma estrutura bem mais alinhada
com o desenho de longo prazo do que a anterior — mas o meio do caminho de um split grande é
exatamente onde bugs de "esqueci de portar isso direito" se escondem, e vários apareceram.

**O resumo mais importante desta revisão: o build compila (28 warnings, zero erros), mas o
ainda há bugs lógicos, e funcionalidades inexistentes ou incompletas.**

### Funciona e está verificado

| Área | Estado |
|---|---|
| `fen_parse` (`fen.c`, ex-`parse_fen`) | Os seis campos, validação completa — sem mudança de comportamento desde a última revisão. Ver `docs/fen.md` (nota: o cabeçalho desse documento ainda diz `src/board.c`; o módulo mudou de nome, ver §8) |
| `fen_write` (ex-`board_to_fen`) | Round-trip idêntico, os 6 campos |
| `Board` | Completa: `castling_rights`, `ep_square`, `halfmove_clock`, `fullmove_number` |
| Indexação `a1 = 0` | Inalterada e correta |
| `SQ_TO_EDGE`, `PAWN_ATTACKS` | Tabelas geradas por `init_square_tables()`, verificadas na forma (não há teste automatizado, mas os valores batem com o cálculo manual em casos conferidos) |
| Headers auto-contidos | Os 10 compilam sozinhos com `gcc -fsyntax-only -x c` — verificado nesta revisão |
| Nomenclatura `módulo_verbo` | Consistente: `fen_parse`, `fen_write`, `board_print`, `board_clear`, `move_from`, `move_to`, `movelist_add`, `sq_from_coord`. Ganho real de legibilidade sobre a versão anterior |
| Codificação de lance (desenho) | O esquema de 4 bits de tipo (`MoveType`) é o padrão da Chess Programming Wiki — captura é bit 2, promoção é bit 3, os dois bits baixos da promoção codificam a peça. Ver §3. |

### Incompleto ou ausente (sem mudança desde a última revisão)

| Área | Estado |
|---|---|
| Cavalo e rei | Tabelas (`KNIGHT_TARGETS`, `KING_TARGETS`) **declaradas e alocadas**, mas `init_square_tables()` só preenche `SQ_TO_EDGE` e `PAWN_ATTACKS` — as duas ficam com zero em todo lugar. Geração de lances para essas peças não existe |
| Roque, en passant | Não gerados |
| `is_square_attacked`, filtro de legalidade | Não existem. Todo lance é pseudo-legal na melhor das hipóteses |
| Perft | Não existe |
| Protocolo UCI | Não existe |
| Testes automatizados | Nenhum |

### Novo nesta revisão: regressões e bugs introduzidos pelo split

Isto é o que mudou de fato. Detalhado com evidência no §5; resumo aqui:

| Área | O que quebrou |
|---|---|
| `make_move` / `unmake_move` | `make_move` não preenche o `Undo` (nem sequer guarda a peça capturada); `unmake_move` é um `return;` vazio — regressão em relação ao "esqueleto" da revisão anterior, que ao menos desfazia o movimento da peça |
| `board_check_invariants`, `board_find_king` | Declaradas em `board.h` (e re-declaradas, sem motivo, no topo de `board.c`) — **sem implementação em lugar nenhum do repositório** |

---

## 3. Decisões arquiteturais firmadas

Estas são escolhas conscientes com trade-off avaliado. Mudá-las agora custa caro. Não
mudaram desde a última revisão, exceto onde marcado.

### Representação: mailbox de 64 casas

Um `Piece array[64]`, não bitboards — inalterado. `array[64]` = 64 bytes = uma linha de
cache; varredura completa medida em ~9,5 ns contra ~12,3 ns de uma piece list. Sem mudança.

### Indexação: `a1 = 0` (Little-Endian Rank-File)

Sem mudança. `sq = rank * 8 + file`; peão branco avança `+8`, preto `-8` — **na intenção**.
Ver Bug #2: o array que deveria expressar isso está com os índices trocados.

### Codificação da peça

Sem mudança de desenho, só de lugar e nome: vivia em `types.h` (`MAKE_PIECE`/`TYPE_OF`/
`COLOR_OF`), agora vive em `piece.h` (`PIECE_MAKE`/`PIECE_TYPE`/`PIECE_COLOR`). As duas
armadilhas de sempre continuam valendo: `PIECE_COLOR(EMPTY) == BLACK`, e comparar o valor
cru contra um `PieceType` funciona por acidente para pretas e falha para brancas.

`piece.h` ganhou também `piece_is_empty`, `piece_is_own`, `piece_is_enemy` como `static
inline` — o equivalente aos antigos `is_own`/`is_enemy` de `movegen.c`, só que expostos como
API do módulo em vez de `static` local não utilizada. **`movegen.c` ainda tem sua própria
cópia local** (`is_own`/`is_enemy`, linhas 10-11) que poderia ser deletada em favor da de
`piece.h` — duas implementações da mesma checagem é o tipo de duplicação que diverge.

### Codificação do lance — decisão fechada nesta janela, superando a descrição anterior

**Esta seção substitui integralmente a versão anterior deste documento**, que descrevia um
`u32` com 8 bits por campo (origem/destino/promoção/flags) e um `enum Promotion` separado.
Esse desenho não existe mais no código. O que existe agora, em `move.h`:

```
       tipo (4)   destino (6)   origem (6)
u16 = [0000]      [000000]      [000000]
       <<12          <<6           <<0
```

```c
typedef enum {
    MV_QUIET         =  0,
    MV_DOUBLE_PUSH   =  1,
    MV_CASTLE_KING   =  2,
    MV_CASTLE_QUEEN  =  3,
    MV_CAPTURE       =  4,
    MV_EP_CAPTURE    =  5,
    /* 6 e 7 nao existem */
    MV_PROMO_N       =  8, MV_PROMO_B = 9, MV_PROMO_R = 10, MV_PROMO_Q = 11,
    MV_PROMO_CAP_N   = 12, MV_PROMO_CAP_B = 13, MV_PROMO_CAP_R = 14, MV_PROMO_CAP_Q = 15
} MoveType;
```

É o esquema de 4 bits da Chess Programming Wiki ([Encoding Moves](https://www.chessprogramming.org/Encoding_Moves)):
bit 3 (`& 8`) é promoção, bit 2 (`& 4`) é captura, e os dois bits baixos, quando há
promoção, indexam a peça (`KNIGHT + (type & 3)`). `move_is_capture`, `move_is_promotion` e
`move_promo_type` em `move.c` implementam exatamente essa aritmética, e implementam
**corretamente** — conferido bit a bit nesta revisão.

Isso também fecha, na prática, a "Decisão em aberto" do documento anterior ("Lance em 16 vs
32 bits"): o formato lógico agora é de 16 bits (6+6+4), como a literatura recomenda.

### Direções: tabela de distância até a borda

Sem mudança. `SQ_TO_EDGE[64][8]`, ordem do enum como carga estrutural. Vive em `square.h/c`
agora, junto com `PAWN_ATTACKS` e a futura `KNIGHT_TARGETS`/`KING_TARGETS`.

### Pseudo-legal primeiro, legalidade depois

Sem mudança de decisão. Na prática, hoje nem o pseudo-legal está confiável (§5).

### Apply/undo com pilha, não cópia do tabuleiro — `Undo` é do chamador

**Decisão de 10/09, agora com a assinatura implementada** em `makemove.h`:

```c
typedef struct {
    Piece captured;
    u8    castling_rights;
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);
void unmake_move(Board *b, Move move, const Undo *u);
```

A assinatura é exatamente a decidida em 10/09 — o argumento completo (sete pontos, pilha da
recursão como pilha de undo, compilador verificando o pareamento) continua valendo e não
precisa ser reescrito aqui. **O que falta é o corpo.** Hoje `make_move` move a peça e inverte
o lado — só isso — e nunca escreve em `*u`; `unmake_move` é um `return;` vazio. A estrutura
certa já existe; a Etapa B do `next_steps.md` (make/unmake completos) continua sendo o
próximo trabalho real, só que a partir de uma base pior do que a de 10/09 (que ao menos
desfazia o movimento da peça).

Nota de nomenclatura: o header se chama `makemove.h`, mas o *include guard* é `UNDO_H`
(`#ifndef UNDO_H` / `#define UNDO_H`). Inofensivo hoje, mas se algum dia existir um
`undo.h` de verdade, a colisão de guard vai silenciosamente pular a inclusão de um dos dois.

### Contrato de validação: fronteira valida, núcleo confia

Sem mudança de decisão. `make_move` continua **não** chamando `find_move`/equivalente —
isso é bom, mas hoje é mais por `make_move` fazer pouco do que por disciplina proposital.

### Split de `board.c` — fechado, e foi além do planejado

**Decidido em 10/09, executado em 12/09.** O plano original (`next_steps.md` D3) previa três
módulos saindo de `board.c`: `square.h/c`, `fen.h/c`, `board.h/c`. O resultado real tem
**quatro**, porque a codificação de peça também saiu de `types.h` para seu próprio módulo:

```
types.h      vocabulário mínimo: typedefs de largura fixa, MAX_*, MIN/MAX
piece.h/c    Color, PieceType, Piece, PIECE_MAKE/TYPE/COLOR, piece_to_char
square.h/c   sq_from_coord, sq_to_coord, SQ_TO_EDGE, Direction, PAWN_ATTACKS
fen.h/c      fen_parse, fen_write
board.h/c    Board, CastleRights, board_print, board_clear (board_find_king e
             board_check_invariants: DECLARADAS, sem corpo — ver Bug #11)
move.h/c     Move, MoveType, MoveList, encode/decode, movelist_*, move_to_str
makemove.h/c Undo, make_move, unmake_move
movegen.h/c  generate_pawn_moves, generate_sliding_moves
log.h/c      log_emit, LOG_ERROR, LOG_DEBUG
utils.h/c    impressão de debug, leitura de stdin, strslc
```

Divisão mais fina do que o planejado, e no geral uma divisão coerente — `piece.h` isolado é
uma responsabilidade genuína (codificação de peça, independente de tabuleiro ou de casa).
Um `gcc -fsyntax-only -x c` em cada um dos 10 headers passa sem erro — verificado nesta
revisão.

### Testes como subcomando do binário

**Ainda não implementado.** A decisão de 10/09 continua de pé (`./main.out test`,
`./main.out perft N`), mas `main.c` não tem esses subcomandos — só o menu interativo `ui()`.
Isso é o item mais alto de retorno da lista de próximos passos: sem ele, todo bug desta
revisão foi achado por leitura de código e por sessão manual no menu, não por um portão
automático.

---

## 4. Mapa dos módulos

| Arquivo | Papel | Linhas (.h/.c) | Nota |
|---|---|---|---|
| `types.h` | Typedefs de largura fixa, `MAX_*`, `MIN`/`MAX` | 58 / — | Ainda arrasta `ctype.h stdio.h stdlib.h string.h` que quase ninguém usa direto — a limpeza sugerida em 10/09 (A2) não foi feita |
| `log.h/c` | `LOG_ERROR`/`LOG_DEBUG` via `log_emit`, com `__FILE__/__LINE__/__func__` capturados na macro | 20 / 20 | Sem mudança, continua correto |
| `piece.h/c` | `Color`, `PieceType`, `Piece`, macros de codificação, `piece_to_char`/`piece_from_char` | 54 / 27 | Novo módulo desde a última revisão |
| `square.h/c` | Coordenadas, `SQ_TO_EDGE`, `PAWN_ATTACKS`, `KNIGHT_TARGETS`/`KING_TARGETS` (declaradas, não populadas) | 48 / 101 | `PAWN_PUSH` mora aqui e está com os índices trocados (Bug #2) |
| `board.h/c` | `Board`, `CastleRights`, impressão, limpeza | 36 / 56 | `board_find_king`/`board_check_invariants` sem corpo (Bug #11); `fen_write` chamado sem incluir `fen.h` (Bug #12) |
| `fen.h/c` | `fen_parse`, `fen_write` | 12 / 612 | Módulo maduro, sem mudança de comportamento. **De longe o maior arquivo do projeto** — candidato a ganhar sua própria bateria de testes primeiro |
| `move.h/c` | `Move`, `MoveType`, `MoveList`, `encode_move`/decodificação, `movelist_*`, conversão de/para string | 53 / 147 | `encode_move` quebrado (Bug #1); `move_from_str` inutilizável como está (Bug #7) |
| `makemove.h/c` | `Undo`, `make_move`, `unmake_move` | 18 / 21 | Esqueleto mínimo — nem captura é guardada. Guard do header é `UNDO_H`, não `MAKEMOVE_H` |
| `movegen.h/c` | Geração de lances de peão e de peças deslizantes | 12 / 119 | Onde vivem os Bugs #2 a #6, #10, #13 (parte) |
| `utils.h/c` | Impressão de debug, leitura de stdin, `strslc` | 26 / 81 | Sem mudança relevante |
| `main.c` | Menu interativo `ui()` + `main()` | — / 117 | Bugs #7 (indireto), #8, #9 (uso) |

**Problemas estruturais que atravessam módulos:**

- **Seis funções com linkage externo que só são usadas dentro do próprio arquivo e deveriam
  ser `static`** (violação direta do §7 do `CLAUDE.md`, e exatamente a classe de bug que
  `-Wmissing-prototypes` existe para achar — e já está achando, ver os warnings do build):
  `ui` (`main.c`), `print_move` e `print_moves` (`move.c`), `check_pawn_promotion` e
  `genenare_moves_from_direction` (`movegen.c`).
- **Duas chamadas entre módulos sem o include correspondente**, sobrevivendo só por sorte de
  assinatura bater no link (`-Wimplicit-function-declaration`): `board.c` chama `fen_write`
  sem incluir `fen.h`; `main.c` chama `print_moves` sem que `movegen.h` ou `move.h` a
  declarem para quem inclui só `movegen.h`. Ver Bug #12.
- O typo `genenare_moves_from_direction` (por `generate_...`) continua no código, já
  documentado desde a revisão anterior.

---

## 5. Bugs abertos

Nesta revisão, cada bug crítico foi **reproduzido com evidência** — warning de compilação,
ASan/UBSan, ou aritmética direta das macros — em vez de inferido por leitura. Comando usado
para os warnings: `make` (conjunto de flags do `CLAUDE.md` §7, hoje com só uma das cinco
flags novas ligada — ver nota de Makefile no fim desta seção). Para os crashes: `make debug`
(ASan + UBSan) e entrada manual via `stdin`.


### Graves — não crasham na hora, mas quebram a regra ou a arquitetura

| # | Onde | Problema |
|---|---|---|
| 1 | `main.c:62` | `if (!movelist_find(&l, move_str))` trata o índice devolvido como booleano: índice `0` (achado na primeira posição) é falso, `-1` (não achado) é verdadeiro — os dois ramos estão invertidos. E o lance de verdade nunca é lido: a variável `move` usada em seguida (`main.c:65`) nunca é atribuída |
| 2 | `main.c:65` (uso), `makemove.c:6-18` | `make_move(b, move, &u)` é chamado com `move` não inicializada — `-Wmaybe-uninitialized` confirma. E mesmo que fosse o lance certo, `make_move` não preenche `*u`: `prev_on_target` (a peça capturada) é calculada e descartada (`-Wunused-variable`); `unmake_move` (`makemove.c:20-22`) é `{ return; }` — não desfaz nada |
| 3 | `board.h:29-30`, `board.c:8-9` | **`board_find_king` e `board_check_invariants` estão declaradas — duas vezes, inclusive: no header e de novo, sem motivo, logo no topo de `board.c` — mas não têm corpo em lugar nenhum do repositório.** Chamar qualquer uma das duas é erro de link. É exatamente a função que `next_steps.md` (Etapa B7) descreve como o próximo passo natural, com a assinatura já pronta para devolver **qual** invariante quebrou (`const char **fail_msgs`) — a implementação é que ficou para trás |

### Menores / higiene — baixo risco, valem a correção quando o arquivo for aberto de qualquer forma

| # | Onde | Problema |
|---|---|---|
| 5 | `main.c:84-87` | `case 6` (Unmake Move) cai para `default` sem `break` explícito antes — inofensivo hoje porque `default` só tem `break`, mas é o tipo de fallthrough que vira bug real na próxima vez que alguém adicionar um `case 7` |
| 6 | `square.h:43`, `square.c:56,71` | `init_square_tables()` (e o `static init_pawn_attacks()`) declarados com `()` em vez de `(void)` — sozinhos respondem por 8 dos 28 warnings do build, um por unidade de tradução que inclui `square.h` |
| 7 | `square.h:33,37` | `extern int SQ_TO_EDGE[...]` declarado duas vezes no mesmo header |
| 8 | `square.c:8` | `DIR_CHARMAP` definido e nunca usado (`-Wunused-const-variable`) — sobrou de uma função de debug que não existe mais |
| 9 | `board.h:7` | `enum ClastleRights` — typo no nome do tag (`Clastle`). Cosmético: o tag não é referenciado em lugar nenhum |

### Sobre o Makefile

O acordo do `CLAUDE.md` §7 é cinco flags novas: `-Wmissing-prototypes`, `-Wconversion`,
`-Wshadow`, `-Wwrite-strings`, `-Wcast-qual`. **Só a primeira foi ligada.** Vale religar as
outras quatro — `-Wconversion` em particular já pegou dois bugs reais neste projeto antes
(documentado no `next_steps.md` §3), e o Bug #2 desta revisão (índices de `Color` invertidos
alimentando aritmética de ponteiro/array) é exatamente o tipo de coisa que conversões
implícitas de sinal deixam passar batido.

---

## 6. Decisões em aberto (com observações)

| Decisão | Situação |
|---|---|
**Sistema de Build**: Somente Makefile por enquanto, migrar para Cmake depois.<br>
**`Move` sem score**: Será implementado quando começar na IA.<br>

---

## 7. Próximos passos, em ordem

A ordem original do `next_steps.md` (Etapas A-E) continua correta em espírito, mas os itens
abaixo são **pré-requisitos que apareceram nesta revisão** e vêm antes de continuar qualquer
feature nova — não porque sejam mais importantes que cavalo/rei/roque, mas porque sem eles
nem o que já existe está confiável para construir em cima:

1. **Consertar a lógica de `main.c` (Bugs #8, #9)** — usar o índice/lance devolvido por
   `movelist_find` em vez de tratá-lo como booleano, e efetivamente aplicar o lance
   encontrado, não uma variável não inicializada.
2. **Implementar `board_check_invariants` (Bug #11)** — a assinatura já existe e já prevê
   devolver qual condição quebrou; falta o corpo. `castling_matches_board` (hoje `static` em
   `fen.c`) já faz a parte de roque; expor o suficiente dela é a maior parte do trabalho.

3.  Só depois disso, retomar o roteiro do `next_steps.md`: `make_move`/`unmake_move`
    completos (Etapa B, com o corpo real desta vez), cavalo e rei (Etapa C), roque/en
    passant/promoção múltipla no gerador (Etapa D), e o subcomando `test`/`perft` que a
    decisão de 10/09 já previa e que continua sendo a lacuna mais cara: **nenhum destes
    bugs precisaria ter sido achado por sessão manual se o corpus de ~30 FEN e o round-trip
    make/unmake já existissem como portão de build.**

### Uma recomendação de processo, não de código

A partir desse momento, é válido considerar testes básicos dos módulos: teste fen round-trip; teste make/unmake move;
teste de invariantes do tabuleiro.

---

## 8. Referências ativas

Sem mudança em relação à revisão anterior — continuam as certas para a próxima etapa:

- **Chess Programming Wiki**: [Encoding Moves](https://www.chessprogramming.org/Encoding_Moves)
  (o esquema que `move.h` já implementa — vale reler à luz do Bug #1),
  [Move List](https://www.chessprogramming.org/Move_List),
  [Pawn Push](https://www.chessprogramming.org/Pawn_Push),
  [Make Move](https://www.chessprogramming.org/Make_Move),
  [Square Attacked By](https://www.chessprogramming.org/Square_Attacked_By),
  [Perft](https://www.chessprogramming.org/Perft) e
  [Perft Results](https://www.chessprogramming.org/Perft_Results).
- **Especificação UCI** (Stefan Meyer-Kahlen).
- **TSCP** — para comparar o desenho de `make_move`/`unmake_move` quando a Etapa B for retomada.
- `man gcc`, seção *Options to Request or Suppress Warnings*.

**Pendência de documentação, fora do escopo desta revisão:** o cabeçalho de `docs/fen.md`
ainda diz "Documento de referência do módulo `src/board.c`" — o módulo se chama `fen.c`
desde o split de 12/09. Correção de uma linha, sinalizada aqui para quem for mexer no
arquivo por outro motivo.

---

## Apêndice — referência rápida dos módulos

A tabela do §4 traz papel e linhas de cada arquivo; esta seção traz as partes que valem a
pena ter à mão sem abrir o arquivo — typedefs, enums, macros e assinaturas — na ordem de
dependência (`types.h` primeiro, o que só depende dele por último). Comentários com `/* ... */`
ao lado de uma linha apontam para o bug correspondente no §5, quando existe um. Includes,
guards e linhas em branco foram omitidos; o conteúdo abaixo reflete o estado **depois** do
commit `04fdba4` ("fix: bugs de verificação; movimento dos peões corrigido; lance de u32
para u16") — ou seja, já com boa parte do §5 anterior corrigida.

### `types.h` — vocabulário mínimo

```c
typedef uint64_t u64;  typedef uint32_t u32;  typedef uint16_t u16;  typedef uint8_t u8;
typedef int16_t i16;

#define MAX_FEN_STRING 256
#define BOARD_SIZE     64
#define BOARD_WIDTH    8
#define MAX_MOVES      256
#define MAX_SEARCH_PLY 64      /* profundidade de busca */
#define MAX_GAME_PLY   1024    /* plies de uma partida */
#define NUM_COLORS     2

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
```

Ainda inclui `ctype.h stdio.h stdlib.h string.h` de que quase nada aqui precisa
diretamente — a limpeza sugerida em 10/09 (`next_steps.md` A2) continua pendente.

### `piece.h` — codificação de peça

```c
typedef enum { WHITE = 1, BLACK = 0 } Color;

typedef enum {
    EMPTY = 0, PAWN = 1, KNIGHT = 2, BISHOP = 3, ROOK = 4, QUEEN = 5, KING = 6
} PieceType;

typedef u8 Piece;

#define PIECE_MAKE(color, type) ((Piece)((((color) & 1u) << 3) | ((type) & 7u)))
#define PIECE_TYPE(p)           ((PieceType)((p) & PIECE_TYPE_MASK))
#define PIECE_COLOR(p)          ((Color)(((unsigned)(p) >> 3) & PIECE_COLOR_MASK))

#define NO_PIECE ((Piece)0)

static inline bool is_own(Piece p, Color c)   { return p != EMPTY && PIECE_COLOR(p) == c; }
static inline bool is_enemy(Piece p, Color c) { return p != EMPTY && PIECE_COLOR(p) != c; }

char  piece_to_char(Piece p);
Piece piece_from_char(char c);
```

`is_own`/`is_enemy` voltaram a morar aqui como a única cópia (o commit `04fdba4` removeu as
que tinham sido duplicadas em `movegen.c`, e usa esta em `generate_sliding_moves`). A
armadilha de sempre continua valendo: `PIECE_COLOR(EMPTY) == BLACK`.

### `square.h` — geometria do tabuleiro

```c
#define SQ_NONE (-1)
#define RANK_OF(sq)         ((sq) / BOARD_WIDTH)
#define FILE_OF(sq)         ((sq) % BOARD_WIDTH)
#define SQ_AT(rank, file)   ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq)     (((sq) < 0) || ((sq) >= BOARD_SIZE))

enum { SQ_A1 = 0, SQ_E1 = 4, SQ_H1 = 7, SQ_A8 = 56, SQ_E8 = 60, SQ_H8 = 63 };

typedef enum {
    DIR_N, DIR_S, DIR_E, DIR_W, DIR_NE, DIR_SW, DIR_SE, DIR_NW, NUM_DIRS
} Direction;

extern const int DIR_OFFSET[NUM_DIRS];
extern const int PAWN_PUSH[NUM_COLORS];            /* [WHITE]=+8 [BLACK]=-8 desde 04fdba4 */
extern int SQ_TO_EDGE[BOARD_SIZE][NUM_DIRS];       /* declarado 2x no header — Bug #7 */
extern int KNIGHT_TARGETS[BOARD_SIZE][8];          /* alocada, NAO populada ainda */
extern int KING_TARGETS[BOARD_SIZE][8];            /* alocada, NAO populada ainda */
extern int PAWN_ATTACKS[NUM_COLORS][BOARD_SIZE][2];

void init_square_tables();                          /* '()' em vez de '(void)' — Bug #6 */
int  sq_from_coord(const char *coord);
void sq_to_coord(int sq, char out[3]);
```

### `board.h` — o tabuleiro

```c
enum ClastleRights {                                 /* typo no tag — Bug #9 */
    CASTLE_WK = 1, CASTLE_WQ = 2, CASTLE_BK = 4, CASTLE_BQ = 8,
    CASTLE_WHITE = CASTLE_WK | CASTLE_WQ,
    CASTLE_BLACK = CASTLE_BK | CASTLE_BQ,
    CASTLE_ALL   = CASTLE_WHITE | CASTLE_BLACK
};

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int   king_square[2];

    u8    castling_rights;    /* bitmask CASTLE_* */
    int   ep_square;          /* SQ_NONE se não houver */
    int   halfmove_clock;
    int   fullmove_number;
} Board;

int  board_find_king(const Board *b, Color c);                       /* sem corpo — Bug #4 */
bool board_check_invariants(const Board *b, const char **fail_msgs); /* sem corpo — Bug #4 */
void board_clear(Board *b);
void board_print(const Board *board);
```

### `fen.h` — a interface inteira de um módulo de 612 linhas

```c
bool fen_parse(const char *fen_string, Board *out);
void fen_write(const Board *board, char fen_out[MAX_FEN_STRING]);
```

Duas funções só — todo o resto de `fen.c` (as seis funções `parse_*` por campo,
`castling_matches_board`, `split_fen_fields`) é `static`, invisível fora do módulo. Ver
`docs/fen.md` para o desenho de cada validação.

### `move.h` — o lance e a lista de lances

```c
typedef u16 Move;                     /* era u32 antes do commit 04fdba4 */
#define MOVE_NONE ((Move)0)

typedef enum {
    MV_QUIET        =  0, MV_DOUBLE_PUSH  =  1, MV_CASTLE_KING = 2, MV_CASTLE_QUEEN = 3,
    MV_CAPTURE      =  4, MV_EP_CAPTURE   =  5, /* 6 e 7 nao existem */
    MV_PROMO_N      =  8, MV_PROMO_B      =  9, MV_PROMO_R     = 10, MV_PROMO_Q      = 11,
    MV_PROMO_CAP_N  = 12, MV_PROMO_CAP_B  = 13, MV_PROMO_CAP_R = 14, MV_PROMO_CAP_Q  = 15
} MoveType;

typedef struct {
    Move moves[MAX_MOVES];
    int  count;
} MoveList;

Move     encode_move(int from, int to, MoveType type);
int      move_from(Move m);
int      move_to(Move m);
MoveType move_type(Move m);

bool      move_is_capture(Move m);
bool      move_is_promotion(Move m);
PieceType move_promo_type(Move m);

void movelist_clear(MoveList *l);
void movelist_add(MoveList *l, Move m);
int  movelist_find(MoveList *l, const char *uci);

void move_to_str(Move m, char out[6]);
Move move_from_str(const char *in);
```

`encode_move` agora desloca `type` de verdade (`type << MOVE_TYPE_SHIFT`, corrigido no
`04fdba4` — antes fazia OR do valor cru `12`, corrompendo a origem). Nota residual não
coberta por esse commit: `move_from_str` lê o caractere de promoção em `in[5]` — para um
lance de 5 caracteres bem formado (`"e7e8q"`, sem `\n` sobrando por causa do buffer de 6
bytes em `main.c`), a peça promovida está em `in[4]`, não `in[5]`; vale conferir com um
caso de teste de promoção antes de confiar nele.

### `makemove.h` — aplicar e desfazer

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

Guard corrigido para `MAKEMOVE_H` no `04fdba4` (era `UNDO_H`). Assinatura pronta; corpo
ainda é o esqueleto descrito no §3 — `*u` não é preenchido, `unmake_move` não desfaz nada.

### `movegen.h` — geração de lances

```c
void generate_pawn_moves(const Board *board, MoveList *list, Color side);
void generate_sliding_moves(const Board *board, MoveList *list);
```

Só duas funções públicas — cavalo, rei, roque e en passant ainda não têm entrada aqui.
`generate_sliding_moves` ganhou o filtro `is_own(piece, board->side_to_move)` no `04fdba4`;
`generate_pawn_moves` continua recebendo `side` como parâmetro em vez de ler
`board->side_to_move` — por isso `main.c` ainda a chama duas vezes, uma por cor (§5, Bug #3).

### `log.h` — logging

```c
void log_emit(const char *level, const char *file, int line,
              const char *func, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

#define LOG_ERROR(...) log_emit("ERROR", __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef DEBUG
#  define LOG_DEBUG(...) log_emit("DEBUG", __FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#  define LOG_DEBUG(...) ((void)0)
#endif
```

### `utils.h` — debug e entrada

```c
extern const char *COLOR_CHAR[2];   /* {"BLACK", "WHITE"} */

void print_piece_chart(void);
void get_fen(char fen[MAX_FEN_STRING]);
void strslc(const char *src, char *dest, int start, int end);
void clear_screen(void);
int  get_int(const char *msg);
```

---

*Este documento é escrito à mão. A próxima revisão deveria ser disparada por um evento
concreto — Etapa A do `next_steps.md` fechada, ou os bugs restantes do §5 corrigidos — não
por passagem de tempo.*
