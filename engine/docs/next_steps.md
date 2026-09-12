# Próximos Passos — do `make_move` até o protocolo

> Guia de implementação e arquitetura. **Documento vivo**: cada etapa tem critério de saída
> objetivo; marque conforme avança e registre aqui o que mudou de opinião no caminho.
>
> Estado base: **10 de setembro de 2026**.
> Contexto e decisões anteriores: `project_context.md`. Fases de longo prazo: `roadmap-motor.md`.
>
> Escopo deste documento: `make_move`/`unmake_move`, cavalo e rei, polimento do gerador
> pseudo-legal, e UCI mínimo. **A geração de lances legais fica de fora de propósito** — ela
> depende de tudo isto estar correto primeiro.

---

## 1. Estado em 2026-09-10

O que mudou desde o `project_context.md` (07-09):

| Área | Estado |
|---|---|
| Deslizantes | Torre, bispo e dama geram por `SQ_TO_EDGE`. Ainda sem filtro de cor |
| `MoveHistory` + `push_undo`/`pop_undo` | Existem, são globais em `movegen.c`, e **estão quebrados** (§3) |
| `unmake_move` | Esqueleto: move a peça de volta, restaura a captura, e para aí |
| `make_move` | Move a peça, inverte o lado, e chama `find_move` — inversão de camada (§2, D4) |
| `LOG_ERROR` | **Saiu do `#ifdef DEBUG`.** O build padrão já não engole erros. Item do `CLAUDE.md` §8 fechado |
| `board.c` | 324 → **721 linhas**. FEN + coordenadas no mesmo arquivo |
| Cavalo, rei, en passant, promoção múltipla, roque | Ausentes |
| Perft, protocolo, testes automatizados | Ausentes |

Build atual: **4 warnings** com o conjunto do `CLAUDE.md` §7, e três deles são bugs reais (§3).

---

## 2. Decisões fechadas nesta sessão

### D1 — O `Undo` é do chamador. Não existe pilha global de undo.

```c
void make_move  (Board *b, u32 move, Undo *u);
void unmake_move(Board *b, u32 move, const Undo *u);
```

```c
typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;
```

O `Undo` guarda **só o irrecuperável**: o que o tabuleiro pós-lance não permite recalcular.
Fora dele ficam duas coisas, por motivos diferentes:

- **O lance.** Quem desfaz acabou de fazer; tem o `u32` na mão. Guardá-lo é uma segunda cópia
  que pode divergir da primeira.
- **`fullmove_number`.** É derivável: incrementa exatamente quando as pretas jogam.

E o campo `Move` de hoje sai: o `score` é artefato de ordenação de lances e não significa nada
num undo. Medido: `Undo` cai de **20 para 12 bytes**, e o acesso deixa de ser `u.move.move`.

Os tipos dos campos **espelham os da `Board`** — `int ep_square`, `int halfmove_clock`, não
`u8`. Isso não é detalhe: é o que torna o Bug 4 (§3) inexprimível em vez de corrigido.

#### Por que esta é a opção certa

A pergunta original era "a engine deve guardar um histórico das jogadas, ou o `Undo` serve só
para checar legalidade?". A resposta é que **são três necessidades diferentes, com três tempos
de vida diferentes** — e fundir as três numa estrutura só é a origem da confusão, não do bug:

| Necessidade | Tempo de vida | O dado que ela realmente precisa |
|---|---|---|
| Filtro de legalidade / busca | um nó de recursão | 1 `Undo`, e só até o `unmake` |
| `position fen … moves e2e4 …` | um comando | **nada.** Só aplica, nunca desfaz |
| Repetição tripla | a partida | lista de `u64` (chaves Zobrist), **não** `Undo`s |
| Menu "Unmake Move" do REPL | a sessão | pares `(lance, Undo)` — este sim é um histórico |

Os sete argumentos a favor do `Undo` local, em ordem de peso:

1. **A pilha da recursão já é a pilha de undo.** A busca é uma estrutura de pilha; um
   `MoveHistory` explícito é uma segunda cópia de informação que o registro de ativação do C
   já mantém, e duas cópias da mesma verdade podem dessincronizar. Com `Undo u;` local, a
   dessincronização é **inexprimível**: existe exatamente um `Undo` por nó ativo porque existe
   exatamente um frame por nó ativo. Não é disciplina, é aritmética.
2. **O compilador passa a verificar o pareamento.** `unmake_move(b, m, &u)` não compila sem o
   `u` no escopo. Com `pop` interno, `unmake_move(b)` compila em qualquer lugar — inclusive num
   ramo que nunca fez lance. Isso converte um bug de runtime (que aparece como perft errado
   três níveis abaixo) em erro de compilação. É o melhor negócio disponível em C.
3. **A falha fica local no tempo.** Cenário concreto: `go movetime 1000` estoura e a busca
   retorna de um nó profundo sem desfazer. Nos dois desenhos a `Board` fica corrompida — mas
   com pilha global o `ply` também fica alto, e ele é uma *segunda* fonte de verdade sobre a
   profundidade que **sobrevive ao comando**. O próximo `pop_undo`, talvez no comando seguinte,
   lê um registro velho e desfaz um lance que nunca foi feito.
4. **O teste-portão deixa de depender de ordem.** O round-trip são ~1000 pares `make/unmake`
   sobre 30 posições. Com estado global, o teste N+1 começa com o `ply` que o N deixou, e uma
   falha vira não-reproduzível isoladamente — a pior propriedade que uma suíte pode ter.
5. **A assinatura para de mentir.** `make_move(Board *b, u32 move)` promete mexer em `b`; ele
   também mexe em `g_history`. Função cuja assinatura sub-declara os efeitos é o mecanismo pelo
   qual uma codebase "fica confusa".
6. **Memória, medido:** `Undo` local custa 12 bytes por frame ativo — profundidade 8 são 96
   bytes. `MoveHistory` são **5124 bytes** vivos para sempre, e não podem ser locais (é para
   isso que existe o `-Wframe-larger-than=16384` no Makefile).
7. **`MAX_PLY = 256` está errado para um dos dois usos**, e isso é o sintoma da fusão.
   Profundidade de busca nunca precisa de 256; uma *partida* passa de 256 plies sem esforço
   (130 lances). Uma constante servindo aos dois significa que um dos dois está errado.

**O que se perde, honestamente:** chamadas mais longas, e `unmake_move` precisa do lance de
volta como parâmetro. Custo real, e pequeno. Vale saber que **os dois desenhos rodam em engines
de verdade**: o Stockfish usa `pos.do_move(m, st)` com `StateInfo st;` local — é este desenho;
o TSCP usa `hist_dat[hply]` global. O TSCP se dá bem porque são ~2000 linhas escritas uma vez
por uma pessoa. Aqui o `CLAUDE.md` §3 diz que mais gente entra em `movegen.c`, e estado global
é o que transforma "dois commits" em "dois commits que se corrompem".

### D2 — O `MoveHistory` sai de `movegen.c` e vira a estrutura de quem precisa dele

O instinto de que existe *algo* persistente estava certo. O erro foi supor que esse algo é o
mesmo objeto que o filtro de legalidade usa, e colocá-lo no núcleo.

```c
/* uci.h — camada de protocolo/UI, dona do estado da partida */
typedef struct { u32 move; Undo u; } GamePly;

typedef struct {
    Board   board;
    GamePly ply[MAX_GAME_PLY];
    int     count;
} Game;
```

Separar as constantes, que hoje são uma só:

```c
#define MAX_SEARCH_PLY   64     /* profundidade de busca; 64 é folgado */
#define MAX_GAME_PLY   1024     /* plies de uma partida */
```

Repetição tripla, quando chegar, é uma lista de **chaves de posição** (`u64` Zobrist) desde o
último lance irreversível — não uma lista de `Undo`s. Tipo diferente, estrutura diferente.

### D3 — Quebrar `board.c` agora

```
types.h      vocabulário compartilhado; só <stdbool.h> e <stdint.h>
square.h/c   sq_from_coord, coord_from_sq, SQ_TO_EDGE, KNIGHT_TARGETS, PAWN_ATTACKS
fen.h/c      parse_fen, board_to_fen
board.h/c    Board, print_board, board_check_invariants
movegen.h/c  Move, MoveList, MoveDescription, Undo + geração + make/unmake
uci.h/c      laço de comandos, Game, str_to_move
```

Motivo de ser **agora** e não depois: as tabelas de cavalo e de peão nascem na Etapa C. É mais
barato elas nascerem no lugar certo do que serem movidas depois. E `board.c` é onde o grupo mais
vai conflitar (`CLAUDE.md` §3) — cada semana que passa aumenta o custo do split.

`square.c` não é um saco de utilitários: a responsabilidade dele é **geometria do tabuleiro**,
independente de peça e de posição. Coordenada, distância até a borda e tabelas de salto são
todas a mesma coisa.

### D4 — `make_move` deixa de chamar `find_move`

Hoje `make_move` chama `find_move`. Isso inverte a fronteira que o projeto já decidiu
(`project_context.md` §3: "fronteira valida, núcleo confia") e fecha o ciclo
geração → validação → make → geração.

O motivo decisivo não é estético: **o filtro de legalidade precisa aplicar lances possivelmente
ilegais** — é aplicando que se descobre se o rei fica em xeque. Um `make_move` que recusa lance
ilegal torna o filtro impossível de escrever. O segundo motivo é custo: regenerar todos os
lances dentro do `make_move` faz cada nó de busca virar O(n²).

`make_move` é uma **primitiva**, como `free(p)`: não confere se `p` veio de um `malloc`. A
pré-condição é "este `u32` saiu do gerador para esta posição", satisfeita por construção nos
dois únicos chamadores — a busca/filtro (itera a lista que acabou de gerar) e a fronteira do
protocolo (chama `find_move`, que gera, acha e devolve).

---

## 3. Bugs abertos

Ordenados por gravidade. A coluna da direita é o ponto importante: **5 dos 6 primeiros eram
detectáveis em compilação**, e dois exigem flags que o Makefile ainda não liga.

| # | Onde | Problema | Detectado por |
|---|---|---|---|
| 1 | `movegen.c:43` | **`pop_undo` lê uma casa além do topo.** `push_undo` grava em `undos[ply]` e *depois* incrementa; o topo é `ply - 1`. `pop_undo` lê `undos[ply]` | — (lógica) |
| 2 | `movegen.c:269` | **`u32 *out_move;` não inicializado.** `find_move` escreve através dele; `push_undo(out_move, …)` guarda o **ponteiro truncado a 32 bits** como lance | `-Wuninitialized`, `-Wint-conversion` |
| 3 | `movegen.c:281` | `push_undo` chamado **depois** de mutar o tabuleiro | — (lógica) |
| 4 | `movegen.c:25` | `u8 ep_square` no parâmetro trunca `SQ_NONE`: `-1` → `255` | **`-Wconversion`** (não ligada) |
| 5 | `movegen.c:238` | **`find_move` aceita lance da cor errada** | — (lógica) |
| 6 | `movegen.c:299` | `printf("%s", …)` com um `char` | `-Wformat` |
| 7 | `movegen.c:68` | `add_move` usa `>` em vez de `>=`: escreve em `moves[GEN_MOVES_MAX]` | — |
| 8 | `movegen.c:144` | Duplo avanço do peão **em paralelo** ao simples, sem checar casa intermediária nem destino | — |
| 9 | `movegen.c:154` | Promoção gera só a dama; perft conta as quatro peças | — |
| 10 | `movegen.c:181` | `genenare_moves_from_direction` (typo) testa `board->side_to_move` enquanto o laço externo não filtra cor | — |
| 11 | `movegen.c:313` | `str_to_move` não checa `sq_from_coord() == -1`; ignora o 5º caractere; `char[6]` não cabe `"e7e8q\n"` + NUL | — |
| 12 | `movegen.c:13` | `g_history` tem linkage externo e **nenhuma declaração em header** | **`-Wmissing-prototypes`** não cobre variáveis |
| 13 | `movegen.c:289` | Sentinela `move.move == 0` para "vazio": `0` é codificação válida de `a1a1` | — |
| 14 | `movegen.c:25` | `push_undo` sem limite superior; `pop_undo` em `ply == MAX_PLY` lê índice 256 de 256 | — |
| 15 | `movegen.c:37` | `pop_undo()` declarada com `()` | `-Wstrict-prototypes` |

### Os dois que foram reproduzidos

**Bug 1 + 2, o sintoma que você viu:**

```
$ printf '2\nd7d6\n6\n' | ./build/main.out
error: debug test     [src/movegen.c] [285] [unmake_move]
error: pop undo error [src/movegen.c] [290] [unmake_move]
→ o tabuleiro não voltou
```

Depois do primeiro lance `ply == 1`, e `pop_undo` lê `undos[1]`, que nunca foi escrito —
zeros. Pior: decrementa mesmo assim, então o registro real em `undos[0]` fica inalcançável.
E mesmo que o índice estivesse certo, o que está gravado em `undos[0]` não é lance nenhum: é o
valor do ponteiro `out_move` truncado.

O compilador já dizia as duas coisas:

```
movegen.c:270: warning: 'out_move' is used uninitialized [-Wuninitialized]
movegen.c:281: warning: passing argument 1 of 'push_undo' makes integer
                        from pointer without a cast [-Wint-conversion]
```

**Bug 5, reproduzido:** FEN com as pretas na vez, digitei `e3e4` (peão branco), e foi jogado.
Duas causas somadas — `find_move` gera as duas cores
(`generate_pawn_moves(…, WHITE)` *e* `(…, BLACK)`), e a checagem de lado só faz `LOG_ERROR`
sem `return false`. Além disso o teste `COLOR_OF(p) != side_to_move` cai na armadilha já
documentada: **casa vazia se parece com peça preta**, então em casa vazia o teste passa quando
são as pretas a jogar.

### Bugs de desenho, não de código

**16 — `enum Promotion` começa em 0, e 0 já significa "sem promoção".**

```c
encode_move(sq, target, 0, QUIET_MOVE)                  /* lance comum   */
encode_move(sq, target, KNIGHT_PROMOTION, QUIET_MOVE)   /* == idêntico!  */
```

São o mesmo `u32`. **Recomendação: o campo `promotion` guarda o `PieceType` cru** —
`KNIGHT = 2` … `QUEEN = 5`, e `0` (que é `EMPTY`) significa "nenhuma". Ganho duplo: o zero passa
a ser sentinela legítima, e `MAKE_PIECE(side, desc.promotion)` funciona direto no `make_move`,
sem tabela de tradução no meio. Apague o `enum Promotion`.

**17 — `enum MoveFlag` não tem os roques.** `make_move` precisa saber que tem de mover a torre
também. Inferir isso de "o rei andou duas colunas" é um segundo mecanismo para a mesma
informação — e o `project_context.md` já registra que mecanismo duplicado tem duas formas de
estar errado. Adicione `CASTLE_KING` e `CASTLE_QUEEN`.

Respondendo ao `[CLAUDE?]` no `movegen.c:155` — *"como guardar o resultado da promoção sem saber
a escolha do usuário?"*: você não guarda. O **gerador emite os quatro lances**, um por peça, e
quem escolhe é quem digitou `e7e8q` (o 5º caractere) ou a busca (que avalia os quatro). A
promoção não é uma pergunta pendente no meio da geração; é um lance que se multiplica por
quatro. É por isso que ela é o caso mais caro do peão.

---

## 4. Etapa A — Fundação

**Esforço:** 1 sessão. Nada aqui muda comportamento; tudo aqui muda o que o compilador
consegue pegar depois.

### A1. Flags novas no Makefile

Medido nesta sessão, contra o build atual:

| Flag | Custo | O que pega |
|---|---|---|
| `-Wmissing-prototypes` | +6 avisos | Funções com linkage externo e sem declaração: `push_undo`, `check_pawn_promotion`, `genenare_moves_from_direction`, `print_move`, `print_undo_history`, `ui` |
| `-Wconversion` | +2 avisos | Os dois são bugs reais; um é o Bug 4 |
| `-Wshadow` | **0** | De graça |
| `-Wwrite-strings` | **0** | De graça |
| `-Wcast-qual` | **0** | De graça |

`-Wmissing-prototypes` é a mais valiosa das cinco, e não pelo número: cada uma das 6 funções que
ela acusa é **ou** helper interno que devia ser `static`, **ou** parte da API que devia estar no
header. A flag não conserta nada — ela força a decisão, que hoje está sendo tomada por omissão.

Mover também `-Wframe-larger-than=16384` de `DBGFLAGS` para `CFLAGS`: um `Undo[256]` numa função
é exatamente o erro que ela pega, e ele deixa de ser hipotético a partir de D1.

### A2. Enxugar `types.h`

Verificado: é mais barato do que parece. `FILE_DIST` e `PrintSize` **nunca são usados** em lugar
nenhum; `MemoryZeroStruct` tem **um único uso** (`main.c:71`), substituível por `l = (MoveList){0}`.
Apagando as três macros, as dependências de `abs`, `printf` e `memset` desaparecem e o header cai
para `<stdbool.h>` + `<stdint.h>`.

O ganho é um **mecanismo de detecção**, não estética: cada `.c` volta a incluir o que usa, e o
compilador volta a avisar quando surgir acoplamento novo. Hoje todo módulo ganha seis headers da
libc de graça e ninguém nota quando passa a depender de um novo.

### A3. Mover os tipos de lance para `movegen.h`

`Move`, `MoveList`, `MoveDescription`, `Undo` saem de `types.h`. A regra, que vale para o resto
do projeto: **um tipo mora no header do módulo dono dos seus invariantes.** `MoveList.count` é
invariante de `add_move`; `board.c` não pode quebrá-lo e não devia ver o tipo. `Board` continua
em `types.h` porque todo mundo precisa dela de verdade.

Argumento adicional, de build: `types.h` é incluído por todo `.c`, então toda edição nele
recompila o projeto inteiro. Mantê-lo pequeno é custo de compilação e de acoplamento.

### A4. O split de D3

Num commit que **não muda comportamento nenhum**: build limpo e mesma saída antes e depois.
Commit que move e altera junto não é revisável nem bissetável — e perft existe justamente para
achar qual commit quebrou a geração.

### A5. `add_move` falha alto

```c
void add_move(u32 move, MoveList *list) {
    assert(list->count < GEN_MOVES_MAX);   /* 218 é o máximo legal; estourar é bug alheio */
    list->moves[list->count++].move = move;
}
```

O `>` virar `>=` é a correção óbvia. A decisão embutida é outra: o máximo teórico de lances numa
posição legal é 218, contra um teto de 256. Se a lista enche, **é bug em outro lugar** — geração
duplicada, laço que não termina, corrupção. Descartar em silêncio transforma um bug detectável
num perft levemente errado, que é a pior moeda de troca possível nesta fase.

### A6. `SQ_TO_EDGE` exposto por `extern`, não por accessor

```c
/* square.h */
extern int SQ_TO_EDGE[BOARD_SIZE][DIR_COUNT];
```

Accessor seria mais puro, mas em TU diferente ele não inlina sem LTO — chamada de função no laço
mais quente do motor — e protege contra um erro que ninguém comete (escrever em `SQ_TO_EDGE`).
Não vale.

O risco real é outro: **esquecer `precompute_move_data()`**, que fica fácil com três entry points
(`test`, `perft`, laço UCI). Um assert barato no topo do gerador, sob `DEBUG`, resolve:

```c
assert(SQ_TO_EDGE[0][DIR_N] == 7);   /* a1 tem 7 casas ao norte */
```

### Critério de saída

`make` limpo com as cinco flags novas. `gcc -std=c11 -fsyntax-only -x c src/<cada>.h` passa nos
seis headers. Comportamento idêntico ao de antes.

---

## 5. Etapa B — `make_move` / `unmake_move` 🚧 portão

**Esforço:** 2–3 sessões. **A parte mais delicada do projeto.** Sem ela não há filtro de
legalidade, não há perft e não há busca — e um bug aqui contamina as três, cada uma de forma
diferente.

### B1. A ordem dentro de `make_move`

```
1. decodificar o lance
2. SALVAR no Undo: captured, castling_rights, ep_square, halfmove_clock   ← primeiro
3. mover a peça, e o caso especial:
     captura  → a peça do destino já está em u->captured
     ep       → limpar a casa do peão capturado (NÃO o destino)
     promoção → array[target] = MAKE_PIECE(side, desc.promotion)
     roque    → mover as DUAS peças
4. atualizar king_square, se a peça era rei (inclui o roque)
5. limpar direitos de roque
6. ep_square: a casa atravessada se DOUBLE_PAWN_PUSH, senão SQ_NONE
7. halfmove_clock: 0 se peão ou captura, senão +1
8. fullmove_number: +1 se quem acabou de jogar era BLACK
9. inverter side_to_move
```

**O passo 2 vem primeiro, e é a correção do Bug 3.** Hoje o `push_undo` está no fim e funciona
por acidente, porque `make_move` ainda não toca roque/ep/relógio. No instante em que tocar — que
é agora — ele passa a gravar o estado *pós*-lance, e o `unmake` restaura o que já estava lá.

### B2. `unmake_move` é o espelho, em ordem reversa, com uma assimetria

Inverter `side_to_move` **primeiro**. Todo o resto depende de saber de quem era a vez: de que
lado o peão veio, onde estava o peão capturado no en passant, qual torre volta no roque. Fazer
por último significa calcular tudo com a cor errada.

### B3. Armadilha — en passant

A peça capturada **não está** na casa de destino:

```
Peão branco em e5, pretas acabaram de jogar d7-d5.  ep_square = d6.

  Lance:           e5xd6 en passant
  Peça capturada:  o peão preto em d5, não em d6
  Casa a limpar:   ep_square - PUSH[side]  =  d6 - 8  =  d5
```

No `unmake`, restaurar a peça em `target` faz um peão sumir e outro aparecer — e o tabuleiro
continua com 32 peças, então nenhum assert de contagem pega.

### B4. Armadilha — roque, e a metade que quase todo mundo esquece

Use uma tabela de máscaras em vez de cadeia de `if`:

```c
static const u8 CASTLE_MASK[64];   /* 0xF em tudo, menos nas 6 casas que importam */

b->castling_rights &= CASTLE_MASK[origin] & CASTLE_MASK[target];
```

O `& CASTLE_MASK[origin]` é o óbvio: mover o rei ou a torre mata o direito. **O
`& CASTLE_MASK[target]` é o sutil: capturar a torre em h8 mata o O-O preto**, sem que ela tenha
se movido. Esquecer isso é bug clássico de perft na profundidade 4 — e a tabela faz as duas
metades caírem na mesma linha, então é difícil lembrar de uma e esquecer da outra.

Leitura: Chess Programming Wiki, *Castling Rights* e *Make Move*.

### B5. Armadilha — `king_square` é campo denormalizado

Campo denormalizado que dessincroniza é dos bugs mais chatos de rastrear, porque o sintoma
aparece longe da causa: `is_square_attacked` consulta um rei que não está lá e devolve
"não atacado" para uma posição em xeque. O assert que recalcula do zero e compara com o cache é
o que impede horas de rastreamento, e mora em `board_check_invariants`.

### B6. O teste que define esta etapa

```
para cada FEN do corpus:
    parse_fen; fen_antes = board_to_fen(b)
    generate_pseudo_legal(b, &lista)
    para cada lance m da lista:
        Undo u;
        make_move(b, m, &u)
        unmake_move(b, m, &u)
        assert(board_to_fen(b) == fen_antes)
        assert(board_check_invariants(b))
```

~30 posições × ~35 lances = **~1000 verificações**, e roda em milissegundos. Ele pega
praticamente todo bug de make/unmake — incluindo B3 e B4 — **antes** que o perft precise entrar
em cena. E o motivo pelo qual ele funciona tão bem é que o round-trip da FEN já está verificado:
você está comparando contra um serializador em que confia.

Corpus num `.c` próprio, como array `const char *[]`. Comece com: posição inicial, Kiwipete,
uma com en passant disponível, uma com promoção a um lance, uma com todos os roques vivos, uma
com roque já perdido, e as posições 3–6 dos *Perft Results*.

### B7. `board_check_invariants(const Board *b)`

Em `board.c`. Verifica:

- `king_square[cor]` bate com uma varredura do zero
- exatamente um rei de cada cor
- `ep_square` é `SQ_NONE`, ou está na fileira 3/6 **com o peão correspondente ao lado**
- direitos de roque coerentes com rei e torres nas casas de origem

**O último item já existe:** `castling_matches_board` em `board.c:379`, hoje `static`. Basta
expor — não reescrever. É a mesma verificação que `parse_fen` faz na entrada; usá-la também na
saída de `make_move` é o que fecha o ciclo que a decisão de "FEN inconsistente é rejeitada, não
corrigida" (`docs/fen.md`) abriu de propósito.

### Critério de saída 🚧

`./build/main.out test` verde nas 30 posições, com invariantes passando. **Não avance sem isso.**

---

## 6. Etapa C — Cavalo e rei

**Esforço:** 1 sessão. Trivial depois da Etapa A.

### C1. Construir a tabela iterando rank/file, não índice de casa

```c
/* square.h */
extern int KNIGHT_TARGETS[64][8];
extern int KNIGHT_COUNT[64];
extern int PAWN_ATTACKS[2][64][2];   /* SQ_NONE onde não existe */
```

Para cada `(rank, file)` e cada delta `(dr, df)`, testar `0 <= rank+dr < 8 && 0 <= file+df < 8`
e **só então** calcular `SQ_FROM_RF`. A forma rank/file torna o wraparound **impossível por
construção** — é por isso que ela ganha de testar `abs(FILE_OF(a) - FILE_OF(b)) > 2` em runtime:
o segundo mecanismo funciona, mas convive com `SQ_TO_EDGE` no mesmo arquivo, e aí o gerador tem
duas formas de estar errado na borda.

### C2. Rei sem tabela — e por quê

**Recomendação: não criar `KING_TARGETS`.** Os lances do rei **são** as 8 direções com distância
1, então `SQ_TO_EDGE[sq][dir] > 0` já é o mecanismo certo para ele. Usá-lo não introduz um
segundo mecanismo — que era a preocupação legítima do `roadmap-motor.md` §3.1 — porque é
exatamente o mesmo mecanismo dos deslizantes, com o laço parando em 1.

O cavalo é a única peça fora das 8 direções, e é a única que precisa de tabela própria. Isto é
uma aplicação mais precisa do princípio de §3.1, não uma exceção a ele.

### C3. `PAWN_ATTACKS` ganha o lugar por outro motivo

Ela serve **duas vezes**: na geração de capturas de peão, e **invertida** em
`is_square_attacked` — que é justamente onde a inversão de peão mais confunde ("de que casas um
peão branco ataca `sq`?" é o inverso de "que casas um peão branco em `sq` ataca"). Ter a tabela
torna a inversão explícita, um índice de cor, em vez de mental.

### Critério de saída

Contagem de destinos por casa conferida à mão em quatro casos: a1 (cavalo: 2), b1 (3), c3 (8),
h8 (2). Rei em a1 = 3, e1 = 5, d4 = 8.

---

## 7. Etapa D — Polimento do gerador pseudo-legal 🚧 portão

**Esforço:** 1–2 sessões.

### P1. Uma entrada só

```c
int generate_pseudo_legal(const Board *b, MoveList *out);   /* devolve a contagem */
```

Um laço sobre as 64 casas, `switch (TYPE_OF(p))`, dispatch por peça. Não seis laços de 64. Além
do custo, o laço único é o que faz a checagem de cor acontecer **exatamente uma vez**.

O `const` está certo e é a metade const do par: `generate_legal(Board *b, …)`, que vem depois,
**não pode** ser `const`, porque o filtro muta o tabuleiro para descobrir. Isso não é defeito da
assinatura, é a natureza do filtro.

### P2. `side_to_move` no laço externo, com o helper que já existe

```c
if (!is_own(b->array[sq], b->side_to_move)) continue;
```

`is_own` e `is_enemy` **já estão escritos em `movegen.c:22-23` e nunca são usados** —
`generate_pawn_moves` usa `COLOR_OF(piece) != side` no lugar. Usar o helper é o que torna a
armadilha `COLOR_OF(EMPTY) == BLACK` **inalcançável** em vez de lembrada. Armadilha documentada
ainda é armadilha; armadilha encapsulada não é.

### P3. Tirar o parâmetro `side` de `generate_pawn_moves`

Ele existe só porque `find_move` chama a função duas vezes, uma por cor — que é o Bug 5. Remova
a causa, não o sintoma. A cor vem de `b->side_to_move`, e o `TODO` do `movegen.c:104` fecha.

### P4. Os cinco casos do peão, e o que cada um quebra

| Comportamento | O que quebra a simetria |
|---|---|
| Avanço reto | Destino tem de estar **vazio** — oposto de toda outra peça |
| Captura diagonal | Destino tem de estar **ocupado por inimigo** |
| Avanço duplo | Só da fileira inicial, e **aninhado dentro** do avanço simples |
| Promoção | Um lance de origem gera **quatro** `Move` |
| En passant | Captura numa casa onde **não há peça** |

**Bug 8, o aninhamento:** hoje o duplo avanço é emitido *antes* do simples, em paralelo, e sem
checar casa nenhuma. O peão salta sobre peças. O sintoma é cruel: `perft(1)` da posição inicial
dá **20, correto**, porque na posição inicial não há nada bloqueando — e quebra na profundidade
2 ou 3, longe da causa. O duplo tem de estar **dentro** do `if` do simples.

**Bug 9, a promoção:** quatro lances, e **nos dois casos** — avanço reto para a 8ª fileira
também promove, não só a captura.

**En passant:** se `b->ep_square != SQ_NONE` e um peão da vez ataca a casa (consulta invertida a
`PAWN_ATTACKS`), emitir com `EP_CAPTURE`. Convenção do `ep_square`: definir a **todo** avanço
duplo, não só quando existe peão capaz de capturar. A segunda é otimização que afeta o hash
Zobrist e **não** as contagens de perft — e as posições de referência publicadas usam a primeira.

### P5. Roque na geração

Emitir com as flags novas (`CASTLE_KING`/`CASTLE_QUEEN`), verificando o que é pseudo-legal:
direito presente, e as casas entre rei e torre vazias. O que **não** entra agora: rei não está
em xeque, não atravessa casa atacada, não chega em casa atacada — os três dependem de
`is_square_attacked` e pertencem ao filtro de legalidade. Documente essa divisão no header, ou
alguém vai reimplementar metade dela nos dois lugares.

### P6. Limpeza

- `genenare_moves_from_direction` → renomear (o typo) e tornar `static`
- `find_move`: `return false` na cor errada, testando `EMPTY` **antes** da cor; usar o gerador
  único; manter o contrato de devolver o lance com as flags do gerador
- `str_to_move` → checar `sq_from_coord() == -1`, ler o 5º caractere como promoção, buffer de 8.
  Ela é parsing de texto do usuário: pertence a `uci.c`, não a `movegen.c`

### Critério de saída 🚧

`perft(1)` da posição inicial = **20**; da Kiwipete = **48**. Nessas duas posições as contagens
pseudo-legais coincidem com as legais, então servem como checagem antecipada antes do filtro
existir.

E **rode o teste da Etapa B de novo**: agora que existem promoção, en passant e roque, ele
exercita muito mais caminhos do `make_move` do que quando passou. De portão único ele vira teste
de regressão permanente — é o que vai dizer qual commit quebrou a geração quando mais gente
estiver em `movegen.c`.

---

## 8. Etapa E — UCI mínimo

**Esforço:** 1 sessão. Pode ser feita **em paralelo** com as outras — é o que desbloqueia a
equipe do cliente, que hoje não tem alvo de integração.

### E1. A linha mais importante do arquivo

```c
setvbuf(stdout, NULL, _IOLBF, 0);
```

Quando a stdout é um terminal, a libc usa buffer de linha e cada `printf` sai na hora. Quando é
um **pipe** — exatamente o caso do cliente rodando o motor como subprocesso — ela troca para
buffer de bloco de 4 KB. A resposta fica presa, o cliente espera para sempre, e **não há sintoma
para depurar**: o motor está vivo, correto, e mudo. Se aparecer na apresentação, não tem
depuração possível ao vivo.

### E2. Leitura de linha

- Buffer de **8192**. `position startpos moves …` passa de 1 KB em partida longa
- `line[strcspn(line, "\r\n")] = '\0'` trata `\n` e `\r\n` de uma vez — a spec avisa que o
  terminador varia por sistema, e a equipe do cliente pode estar no Windows

### E3. Conjunto mínimo de comandos

| Comando | Resposta |
|---|---|
| `uci` | `id name …`, `id author …`, `uciok` |
| `isready` | `readyok` |
| `position startpos [moves …]` | — |
| `position fen <FEN> [moves …]` | — |
| `d` | tabuleiro + FEN *(não-UCI, depuração)* |
| `perft N` | contagem *(não-UCI, depuração)* |
| `quit` | encerra |
| desconhecido | **ignora em silêncio** (a spec manda) |

`go` fica para depois, junto com a IA.

`position … moves` é o consumidor natural de `find_move`: os lances chegam em notação de
coordenada (`e2e4`, `e7e8q`) e é o **gerador** que sabe se `e5d6` é captura comum ou en passant,
porque foi ele que marcou a flag. É exatamente por isso que `find_move` devolve o lance em vez
de um `bool`.

Suporte o sufixo `moves` desde já: depende só do `make_move` da Etapa B, e é o que permite
plugar o motor no Cute Chess ou Arena.

### E4. `main.c` encolhe

Passa a ser: `setvbuf` → `precompute_move_data()` → dispatch de `argv`:

```
./main.out            laço UCI  (o default, porque é o que o cliente executa)
./main.out test       round-trip de make/unmake
./main.out perft N    contagem
./main.out repl       o menu interativo de hoje
```

O menu atual é genuinamente útil para depurar à mão — mas não pode mais ser o default, porque o
default é o que o cliente vai executar. Mova para `repl.c`.

### E5. Dívidas conscientes — anotar, não deixar implícitas

- A spec exige processar stdin **enquanto pensa**, para atender `stop`. Um laço bloqueante não
  faz isso. Para busca rasa é irrelevante; resolve-se depois com thread de input.
- **Divergência a resolver com a equipe** (`roadmap-motor.md` §7): o `arquitetura-xadrez.md` diz
  que o cliente nunca fala direto com o motor (sempre via backend Node); o `project_context.md`
  diz que o cliente C++ roda o motor como subprocesso. Isso decide se o protocolo precisa de
  `legalmoves` (para a UI destacar lances) ou se o backend monta isso. Conversa de 15 minutos
  que evita implementar comando que ninguém chama — ou descobrir na integração que falta um.

### Critério de saída

O cliente (ou Cute Chess/Arena) joga uma partida inteira contra o motor sem travar. Antes de
emitir `bestmove`, verificar que o lance está na lista legal: custa uma varredura e garante que
o motor **nunca** devolve lance ilegal, que é requisito funcional do projeto.

---

## 9. Boas práticas para este projeto

### 9.1 Onde vive uma variável global

Três categorias, três regras:

| Categoria | Onde | Exemplo |
|---|---|---|
| Tabela read-only depois do init | `static` no `.c` dono, ou `extern` no header dele; escrita **só** pela função de init | `SQ_TO_EDGE`, `KNIGHT_TARGETS` |
| Estado mutável | **Nunca global.** Mora numa struct passada por ponteiro | `Board`, `Game` |
| Constante | `static const` no `.c`, ou macro no header | `DIR_OFFSET`, `PIECE_CHAR` |

E a regra que fecha o caso do `g_history`: **se não é `static`, tem de estar declarada num
header.** `-Wmissing-prototypes` cobre funções, não variáveis — então esta é por disciplina.
Um global com linkage externo e sem declaração é invisível em revisão de código e alcançável por
`extern` de qualquer `.c`: o pior dos dois mundos.

### 9.2 Onde vive um tipo

No header do **módulo dono dos seus invariantes**. Se um módulo não pode quebrar o invariante,
não devia ver o tipo. `types.h` é para vocabulário que todo mundo precisa de verdade — e é
incluído por todo `.c`, então cada edição nele recompila o projeto inteiro.

### 9.3 Toda função não exportada é `static`

E a flag força a escolha, em vez de confiar na memória. Hoje seis funções em `movegen.c` têm
linkage externo por omissão — nenhuma delas de propósito.

### 9.4 Assert como executável, não como comentário

Todo campo denormalizado merece um assert que o recalcula do zero e compara. `king_square` é o
caso atual; se o Zobrist entrar, `assert(b->hash == compute_hash_from_scratch(b))` no fim de
`make/unmake` é o detector de bug mais sensível que existe para esta fase — ele é um resumo de
64 bits de *tudo* que o `make_move` deveria ter alterado, e dispara no lance exato em vez de
três níveis de busca depois.

### 9.5 A regra de ouro daqui

**Se o bug é detectável em compilação, ligue a flag em vez de lembrar dele.** Cinco dos seis
bugs principais desta sessão eram visíveis ao compilador; dois exigiam flags que o Makefile
ainda não liga. Das últimas sessões, a proporção é parecida — este projeto tem histórico de
bugs que o GCC sabia apontar.

Corolário já registrado no `CLAUDE.md` §7, e que vale repetir: **nenhum sanitizer pega leitura
de memória não inicializada.** ASan pega fora-dos-limites e use-after-free; UBSan pega UB
aritmético. O Bug 2 desta sessão é leitura de valor não inicializado, e quem o pegou foi
`-Wuninitialized` — que só funciona com `-Og` ligado. O aviso do compilador é a única defesa.

### 9.6 `LOG_ERROR` não é tratamento de erro

Logar e seguir — como `find_move` faz hoje na checagem de cor — é **pior** do que não logar: dá
a impressão de que o caso foi tratado. A regra: logar **e** devolver status. Função que pode
falhar devolve se falhou.

### 9.7 Uma função, uma camada

`make_move` chamando `find_move` é a violação concreta (§2, D4). A camada é: protocolo valida,
núcleo confia. Toda vez que uma função do núcleo sentir vontade de validar a entrada, a pergunta
certa é "quem deveria ter validado isso antes?".

### 9.8 Commits

Pequenos, um por etapa, em português (convenção do grupo). **Refatoração que move código nunca
muda comportamento no mesmo commit** — um commit que faz as duas coisas não é revisável nem
bissetável, e perft existe para bissetar. E nunca `--amend` nem rebase no commit de merge do
subtree (`229e25e`).

---

## 10. Ordem, portões e progresso

```
A ─── B 🚧 ─── C ─── D 🚧 ─── E
fundação  make/unmake  cavalo  polimento  UCI
                       e rei              (pode ir em paralelo)
```

| Etapa | Critério de saída | Feito |
|---|---|---|
| A — Fundação | `make` limpo com as 5 flags novas; headers auto-contidos; comportamento idêntico | ☐ |
| B — make/unmake 🚧 | `./main.out test` verde em 30 posições, invariantes passando | ☐ |
| C — Cavalo e rei | Contagem de destinos conferida à mão em a1, b1, c3, h8 | ☐ |
| D — Polimento 🚧 | `perft(1)` inicial = 20, Kiwipete = 48; **e o teste de B de novo** | ☐ |
| E — UCI | Partida inteira contra Cute Chess/Arena sem travar | ☐ |

**B é portão duplo:** passa uma vez no fim de B, e volta a rodar como regressão depois de D —
quando há promoção, en passant e roque para exercitar, ele testa muito mais do que testava.

Depois de E, o próximo documento é sobre `is_square_attacked` e o filtro de legalidade. Nada de
busca ou avaliação antes do perft bater.

---

## 11. Referências para estas etapas

- **Chess Programming Wiki** — *Make Move*, *Unmake Move*, *Castling Rights*, *En passant*,
  *Encoding Moves*, *Knight Pattern*, *Pawn Push*, *Perft*, *Perft Results*
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas, ler inteira antes da Etapa E
- **TSCP** — `makemove`/`takeback` em mailbox legível. Note que ele usa o desenho *oposto* ao
  D1 (pilha global `hist_dat`); vale ler justamente para ver o contraste
- `man gcc`, *Options to Request or Suppress Warnings* — em especial a nota sob
  `-Wmaybe-uninitialized` sobre a dependência do nível de otimização
