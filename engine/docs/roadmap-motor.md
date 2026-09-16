# Roadmap do Motor de Xadrez

> Plano de execução em fases incrementais, com critérios de saída verificáveis.
>
> Escrito em **5 de setembro de 2026**, commit `0c4a88a`. Revisado em **14 de setembro de
> 2026** e de novo em **16 de setembro de 2026** para registrar o que de fato aconteceu —
> commit `f79d50a` mais a árvore de trabalho, depois da refatoração de módulos (`8f8a52d`),
> da rodada de correção de bugs (`04fdba4`) e da sessão que completou `make_move`.
> Leia antes: `onboarding-motor.md` (contexto, sem exigir C).
> Detalhe técnico completo, e a fonte de verdade sobre o estado atual: `project_context.md`.

> **Como ler esta revisão:** cada fase e cada decisão ganhou uma marca de status —
> ✅ feito, 🔶 parcial, ⬜ não iniciado — e uma nota curta dizendo o que mudou. O raciocínio
> original (o "porquê" de cada escolha) continua valendo na maior parte dos casos e **não**
> foi reescrito; onde a equipe decidiu diferente do que este documento recomendava, isso está
> marcado explicitamente — o caso mais importante é §2.2 (lance em 16 bits, não 32: a equipe
> foi na direção oposta à recomendação daqui, e é a decisão certa). Números de linha e nomes
> de função foram atualizados para bater com os módulos atuais (o split de `board.c` foi mais
> fundo do que a §3.3 original previa — ver `project_context.md` §3).
>
> **Mudança de plano registrada em 16/09:** o teste de round-trip de FEN sobre o corpus saiu
> do critério de saída da **Fase 1** e passou para a **Fase 4**, junto com o round-trip de
> make/unmake. Motivo: o módulo `fen` é o mais maduro do projeto — seis campos, validação
> completa, round-trip verificado em todas as posições em que foi exercitado — e gastar uma
> sessão testando-o isoladamente agora não compra informação nova. Testado dentro da Fase 4
> ele compra as duas coisas de uma vez: é o **oráculo** do teste de make/unmake, então cada
> uma das ~1000 comparações de FEN daquele teste é também um round-trip de FEN. O corpus
> continua sendo entregável — mudou a fase em que ele nasce, não a decisão de que ele existe.

---

## Como ler este roadmap

Cada fase tem quatro partes:

- **O quê** — os entregáveis concretos
- **Como** — sugestão de implementação e as armadilhas conhecidas
- **Por quê agora** — a justificativa da posição na ordem
- **Critério de saída** — a condição objetiva para considerar a fase concluída

Fases marcadas com 🚧 são **portões**: não avance sem cumprir o critério de saída.
Não é rigor cerimonial — em cada um desses casos, avançar sem o portão fechado
significa depurar o problema muitas horas depois, num contexto onde ele é
irreconhecível.

---

## 0. Resumo executivo

| Fase | Nome | Esforço | Portão | Destrava | Status em 16/09 |
|---|---|---|---|---|---|
| 0 | Fundação de qualidade | 1 sessão | 🚧 | Tudo. Item de maior retorno do roadmap | 🔶 parcial — **8 avisos faltando fechar** |
| 1 | Vocabulário completo | 1–2 sessões | 🚧 | Roque, en passant, `Undo` | ✅ **fechada em 16/09** (critério revisado) |
| 2 | Geometria pré-computada | 1 sessão | | Cavalo e rei | 🔶 parcial — rei resolvido sem tabela; cavalo não |
| 3 | Geração pseudo-legal | 2–3 sessões | | Perft(1) | 🔶 parcial — só falta o cavalo |
| 4 | Aplicar/desfazer lance | 2–3 sessões | 🚧 | Legalidade e busca | 🔶 **metade**: `make_move` completo e verificado, `unmake_move` sem corpo |
| 5 | Filtro de legalidade | 1–2 sessões | 🚧 | Xeque, mate, afogamento, perft | ⬜ não iniciada |
| 6a | Protocolo + IA aleatória | 1 sessão | | **A equipe do cliente** | ⬜ não iniciada — só o menu interativo `ui()` existe |
| 6b | Perft | 2–4 sessões | 🚧 | Autorização para escrever IA | ⬜ não iniciada |
| 7 | IA incremental (v1→v5) | 3–5 sessões | | Força de jogo | ⬜ não iniciada |
| 8 | Robustez e empacotamento | 2 sessões | | Entrega | ⬜ não iniciada |

**Total estimado:** 14–22 sessões de trabalho. Passaram-se aproximadamente 6-7 sessões desde
a escrita original (5/09 → 16/09). **A Fase 1 é a primeira a fechar seu critério de saída
🚧**, com o critério revisado desta data. A Fase 0 continua aberta por oito avisos de
compilação — é meia sessão de trabalho, e três dos oito são bugs reais. A Fase 4 está pela
metade, e é a metade difícil que já foi: `make_move` trata roque, en passant, promoção,
relógios e direitos de roque **corretamente**, verificado com evidência; falta o espelho.
Ver `project_context.md` §2 e §5 para o detalhe de cada item; o resumo por fase está abaixo.

A variabilidade concentra-se na Fase 6b, que não é uma fase de escrever código — é
a fase de *encontrar bugs escritos nas fases 3, 4 e 5*. Quanto melhor o trabalho
anterior, mais curta ela é. Essa é a razão de existirem portões antes dela.

---

## 1. Diagrama de dependências

```
   Fase 0 ──┬── Fase 1 ──┬── Fase 2 ── Fase 3 ──┬── Fase 4 ── Fase 5 ──┬── Fase 6b ── Fase 7 ── Fase 8
  qualidade │  vocabul.  │   geometria  geração │   make/     legalid. │    perft       IA
            │            │                      │   unmake             │
            │            │                      │                      │
            └────────────┴──────────────────────┴─── Fase 6a ──────────┘
                                                     protocolo
                                                   (pode ser feita
                                                  em paralelo, e deve)
```

**A Fase 6a é deliberadamente fora de ordem.** Tecnicamente ela deveria vir depois
do perft. Mas ela é o que **desbloqueia a equipe do cliente**, que hoje não tem
alvo de integração. Um motor que fala o protocolo corretamente e joga mal libera
semanas de trabalho paralelo — e integração descobre cedo problemas de protocolo
que você não quer descobrir na véspera da apresentação.

---

## 2. Decisões fechadas nesta revisão

As quatro decisões que estavam em aberto no `project_context.md` §6, agora com
resposta e justificativa.

### 2.1 Campos faltantes na `Board` → **fechar agora, junto com o `Undo`** — ✅ feito

**Status em 16/09:** implementado exatamente como proposto, e **agora efetivamente usado**.
`Board` (`board.h`) tem os quatro campos, e `Undo` (`makemove.h`) tem a mesma forma — `Piece
captured`, `u8 castling_rights`, `int ep_square`, `int halfmove_clock`, sem `Move` nem
`fullmove_number`. `make_move` preenche `*u` **antes** de tocar no tabuleiro, que era o ponto
crítico da ordem, e restaura/atualiza todos os campos da `Board`. Falta `unmake_move`, que é
quem lê o `Undo` — ver Fase 4.

Uma assimetria que só aparece na hora de escrever o `unmake` e vale registrar aqui: **no en
passant `u->captured` fica `EMPTY`**, porque a casa de destino estava vazia. O peão capturado
tem de ser deduzido do `MoveType` + `side`, não lido do `Undo`. Isso não é defeito do desenho
— é o motivo pelo qual o lance volta como parâmetro em `unmake_move`.

O `project_context.md` já identificou que são os mesmos campos. A conclusão que
faltava tirar: isso os torna **uma decisão só**, não duas.

```c
typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;

    u8    castling_rights;   /* bitmask KQkq */
    int   ep_square;         /* SQ_NONE se não houver */
    int   halfmove_clock;
    int   fullmove_number;

    int   king_square[2];    /* cache derivado */
} Board;

typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;
```

**Por que o `Undo` não guarda `king_square`:** se a peça movida era o rei,
`unmake_move` o restaura para a origem do lance. Guardar seria redundante — e o
invariante que recalcula o rei do zero pega o erro caso você escorregue.

**Por que não guarda `fullmove_number`:** é sempre reconstruível (decrementa se as
pretas jogaram).

**Por que bitmask e não quatro `bool`:** além do tamanho, ele habilita a tabela
`castle_mask[64]`, que resolve numa linha três situações que um `if` manual quase
sempre trata em duas:

```c
b->castling_rights &= castle_mask[origin] & castle_mask[target];
```

Isso cobre: o rei se moveu (via `origin`), a torre se moveu (via `origin`), e — o
esquecido — **a torre foi capturada no canto** (via `target`). Se as pretas capturam
a torre em h1, as brancas perdem o roque curto sem que nenhuma peça branca tenha
se movido.

### 2.2 Lance em 16 vs 32 bits → **a equipe foi na direção oposta a esta recomendação, e fez bem**

**Status em 14/09: revertido.** A recomendação original era ficar em 32 bits e não mexer
mais nisso; o commit `04fdba4` ("lance de u32 para u16") trocou `typedef u32 Move` por
`typedef u16 Move` em `move.h`. Registro aqui, para não perder o porquê:

A previsão de que "trocar o layout é uma tarde, em qualquer momento do projeto" **se
confirmou** — foi literalmente uma linha (`typedef`) mais o ajuste das duas ou três funções
que já encapsulavam o formato (`encode_move`, `move_from`, `move_to`, `move_type`). A
migração não exigiu tocar em `movegen.c` nem em `main.c`. Isso valida o argumento original
sobre encapsulamento — só a conclusão prática ("não vale a pena agora") é que não se
sustentou: não havia motivo para *esperar* a Fase 7 se a troca é essencialmente grátis assim
que o encapsulamento existe.

**O layout adotado já é o de 16 bits (6+6+4) que a nota de rodapé original previa:**
`MoveType` (`move.h`) é uma enumeração de 16 valores mutuamente exclusivos — exatamente a
pré-condição que este documento apontava como necessária antes de apertar para 4 bits de
flag (não quatro booleanos independentes). `MoveDescription` (a struct desempacotada citada
acima) não existe mais; no lugar, `move_from`/`move_to`/`move_type` decodificam sob demanda a
partir do `u16`, e `move_is_capture`/`move_is_promotion`/`move_promo_type` derivam da
aritmética de bits do `MoveType` — o mesmo espírito de "decodifique quando precisar" só que
sem struct intermediária.


### 2.3 Makefile vs. CMake → **migrar na Fase 0** — ⬜ não adotado

**Status em 14/09:** não há `CMakeLists.txt` no repositório — o build continua só
Makefile. O argumento sobre o `-Og`/sanitizers continua válido e o risco que ele descreve
continua real: o Makefile atual (`engine/Makefile`) só liga uma das cinco flags novas
acordadas no `CLAUDE.md` §7 (`-Wmissing-prototypes`; faltam `-Wconversion`, `-Wshadow`,
`-Wwrite-strings`, `-Wcast-qual`), o que é exatamente o tipo de "alvo ad-hoc com flags
inconsistentes" que a migração pretendia evitar. Continua sendo uma decisão em aberto — ver
`project_context.md` §6.

Mas não pelo motivo que o documento de arquitetura dá (integração com a equipe do
cliente). O motivo real é que **o bug do `-Og` é estrutural**: um Makefile artesanal
convida a alvos ad-hoc com flags inconsistentes, que foi exatamente o que
aconteceu. O CMake dá `CMAKE_BUILD_TYPE` com Debug e Release configurados
separadamente e difíceis de errar.

```cmake
cmake_minimum_required(VERSION 3.16)
project(chess_engine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # clangd / editores

add_executable(engine
    src/main.c src/board.c src/fen.c src/square.c
    src/movegen.c src/utils.c src/log.c)

target_compile_options(engine PRIVATE
    -Wall -Wextra -Wpedantic -Wstrict-prototypes
    -Wshadow -Wconversion -Wcast-qual)

target_compile_options(engine PRIVATE
    $<$<CONFIG:Debug>:-Og -g -fsanitize=address,undefined>)
target_link_options(engine PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>)

target_compile_definitions(engine PRIVATE
    $<$<CONFIG:Debug>:DEBUG>)
```

**Mantenha o Makefile como *task runner*, não como sistema de build.** `make context`,
`make perft`, `make test` viram wrappers de uma linha em cima do CMake. Nada do
fluxo atual se perde.

### 2.4 Onde vive `side_to_move` na geração → **laço externo, uma checagem** — ✅ feito

**Status em 16/09:** fechada. Os dois helpers existem, literalmente com estes nomes, em `piece.h`
(o `COLOR_OF` da época virou `PIECE_COLOR` no split de módulos — mesma macro, nome novo):

```c
static inline bool is_own(Piece p, Color c)   { return p != EMPTY && PIECE_COLOR(p) == c; }
static inline bool is_enemy(Piece p, Color c) { return p != EMPTY && PIECE_COLOR(p) != c; }
```

**Usados em `generate_sliding_moves`** (`movegen.c`): `if (!is_own(piece,
b->side_to_move)) continue;` no laço externo, exatamente como recomendado. E a segunda metade
fechou em 16/09: **`generate_pawn_moves` perdeu o parâmetro `side`** e lê `b->side_to_move`,
então `main.c` não a chama mais duas vezes. `generate_king_moves` nasceu já com o padrão
certo. O que restou é cosmético: o peão ainda testa
`PIECE_COLOR(piece) != side` em vez de `is_own` (`movegen.c:24`) — funciona porque a linha
anterior já descartou casas vazias via `PIECE_TYPE(piece) != PAWN`, mas é a armadilha
sobrevivendo por ordem de linhas em vez de por encapsulamento.

É a opção que o documento já identificou como limpa, e está certa. Mas há uma
armadilha específica da codificação de peça do projeto que precisa ser blindada:

```c
/* ERRADO — PIECE_COLOR(EMPTY) == BLACK nesta codificação.
   Casas vazias entram na geração das pretas. */
if (PIECE_COLOR(b->array[sq]) != b->side_to_move) continue;

/* CERTO */
if (!is_own(b->array[sq], b->side_to_move)) continue;
```

**Escreva esses dois helpers antes de escrever qualquer peça nova.** O
`project_context.md` já registra `PIECE_COLOR(EMPTY) == BLACK` como armadilha
conhecida — o passo que falta é torná-la **inalcançável** em vez de lembrada. Uma
armadilha documentada ainda é uma armadilha; uma armadilha encapsulada não é.

---

## 3. Decisões arquiteturais a somar

### 3.1 Estender a filosofia do `SQ_TO_EDGE` para as saltadoras — 🔶 parcial

**Status em 16/09:** metade fechada, e a outra metade mudou de forma.

O **rei** foi resolvido **sem tabela**, e essa é a decisão certa: `generate_king_moves`
(`movegen.c:180`) usa `SQ_TO_EDGE[from][dir] == 0` como checagem de borda, com o laço parando
em 1. Isso é o *mesmo* mecanismo dos deslizantes, não um segundo — que era exatamente a
preocupação desta seção. `KING_TARGETS[64][8]` continua declarada, alocada e zerada em
`square.c:16`, **sem nenhum leitor**: é alocação órfã e deve ser apagada, antes que alguém a
preencha e crie o segundo mecanismo que a decisão evitou de propósito.

O **cavalo** continua sendo o caso real desta seção, e continua não feito.
`init_square_tables()` (o nome real da função, não `precompute_move_data`) chama
`init_sq_to_edge()` e `init_pawn_attacks()` — `KNIGHT_TARGETS` fica zerada, e não há
`generate_knight_moves`. Medido em 16/09: a posição inicial gera **16** lances, não 20, e a
Kiwipete gera **37**, não 48 — a diferença é exatamente o cavalo. `KNIGHT_COUNT` também não
existe; `offset_square` (`square.c:23`), que `init_pawn_attacks` já usa, é a função certa
para construir a tabela sem escrever um segundo mecanismo de borda.

`SQ_TO_EDGE` resolve o wraparound para deslizantes e rei elegantemente — a checagem
de borda vira o limite do laço, e código de runtime não testa nada. Mas **ela não
cobre o cavalo**, porque os saltos do cavalo não estão entre as 8 direções.

| Opção | Runtime | Consistência |
|---|---|---|
| Checagem de distância de coluna | `abs(FILE_OF(to) - FILE_OF(from)) > 2` | **mistura dois mecanismos** de borda no mesmo arquivo |
| Tabela de destinos pré-computada | `for (i = 0; i < KNIGHT_COUNT[sq]; i++)` | mesmo padrão do `SQ_TO_EDGE` |

**Recomendação: tabela pré-computada**, e o motivo não é desempenho — é consistência
de mecanismo. Um `movegen.c` onde deslizantes usam `SQ_TO_EDGE` e cavalos usam
aritmética de coluna tem **duas formas de estar errado**, e a segunda é a que
ninguém revisa.

```c
static int KNIGHT_TARGETS[64][8];   static int KNIGHT_COUNT[64];
static int KING_TARGETS[64][8];     static int KING_COUNT[64];
static int PAWN_ATTACKS[2][64][2];  /* [cor][casa][esq/dir], SQ_NONE se não existe */
```

`PAWN_ATTACKS` merece destaque: ela serve **duas vezes**. Na geração de capturas de
peão e, invertida, em `is_square_attacked` — que é justamente onde a inversão de
peão mais confunde. Ter a tabela torna a inversão explícita em vez de mental.

Efeito colateral: `SQ_OFFBOARD` deixa de ser necessário na geração, e seu bug de
parênteses (`types.h:33`) se resolve por deleção — a melhor forma de resolver um bug.

### 3.2 Duas funções de geração, com `const` diferente — ⬜ não iniciada

**Status em 16/09:** meio caminho. `generate_all_moves(Board *b, MoveList *list)`
(`movegen.c:230`) é a entrada única pedida — limpa a lista e chama os três geradores. Mas ela
recebe `Board *` **não-const** enquanto `generate_pawn_moves`, `generate_sliding_moves` e
`generate_king_moves` recebem `const Board *`. O `const` certo é o dos três: esta é a metade
const do par, e perdê-lo aqui apaga justamente a informação que a seção existe para
registrar. `generate_legal` não existe (Fase 5).

```c
int generate_pseudo_legal(const Board *b, MoveList *out);   /* const: não toca nada */
int generate_legal(Board *b, MoveList *out);                /* NÃO const: filtra com make/unmake */
```

O filtro de legalidade **precisa mutar** o tabuleiro — é aplicando que se descobre.
Então `generate_legal` não pode ser `const`, e isso está correto, não é um defeito.
A assinatura proposta no `project_context.md` §7.3 (`const Board *`) vale para a
versão pseudo-legal.

Documente no header que `generate_legal` restaura o tabuleiro exatamente — e prove:
um `assert` de invariantes na saída, comparando com um snapshot da FEN na entrada,
em build de debug.

**API pública:** `generate_legal` é o que o protocolo e a busca chamam.
`generate_pseudo_legal` fica exposta porque o perft e a depuração precisam dela.

### 3.3 Separar `board.c` — mas apenas em dois — ✅ feito, e foi além do proposto

**Status em 14/09:** o split aconteceu na Fase 1, como recomendado, mas com um módulo a
mais do que o previsto — a codificação de peça (`MAKE_PIECE`/`TYPE_OF`/`COLOR_OF` de então)
também saiu de `types.h` para seu próprio `piece.h/c`. O resultado real:

```
types.h      →  vocabulário mínimo: typedefs de largura fixa, MAX_*, MIN/MAX
piece.h/c    →  Color, PieceType, Piece, PIECE_MAKE/TYPE/COLOR, piece_to_char, is_own/is_enemy
square.h/c   →  sq_from_coord, sq_to_coord, SQ_TO_EDGE, Direction, PAWN_ATTACKS
fen.h/c      →  fen_parse (era parse_fen), fen_write (era board_to_fen)
board.h/c    →  Board, CastleRights, board_print, board_clear
move.h/c     →  Move, MoveType, MoveList, encode/decode, movelist_*   (não previsto aqui)
makemove.h/c →  Undo, make_move, unmake_move                          (não previsto aqui)
movegen.h/c  →  generate_pawn_moves, generate_sliding_moves
```

`move.h/c` e `makemove.h/c` saindo de `movegen.h/c` não estava nesta lista — foi decidido
depois, junto com a assinatura `Undo`-do-chamador de §2.1. `board_check_invariants` **ainda
não está implementada** (só declarada, em `board.h` e de novo, sem motivo, no topo de
`board.c`) — é o item que mais falta desta fase. Ver `project_context.md` §3 e §4 para o
mapa completo, com linhas por arquivo.

**O corte de `types.h` para `stdbool`/`stdint` (parágrafo abaixo) não foi feito** — o header
ainda arrasta `ctype.h stdio.h stdlib.h string.h`.

Coordenada não é detalhe de FEN — o `project_context.md` já concluiu isso. O que
falta notar é que **`square.c` é também onde as tabelas de §3.1 pertencem**: são
todas "geometria do tabuleiro", independentes de peça e de posição. Isso dá ao
módulo uma responsabilidade coerente em vez de ser um saco de utilitários.

**Faça na Fase 1**, enquanto já se está mexendo em `board.c` para completar a FEN.
Depois que `movegen.c` triplicar de tamanho, o custo do split sobe.

Sobre `types.h` arrastando seis headers da libc: corte para `stdbool` e `stdint`, e
deixe cada `.c` incluir o que usa. O ganho é o já identificado — o compilador volta
a avisar quando surge acoplamento novo. É um **mecanismo de detecção**, não estética.

### 3.4 `add_move` deve falhar alto, não silenciar — 🔶 parcial, caminho diferente do proposto (sem mudança em 16/09)

**Status em 14/09:** o `>` virou `>=` (hoje `movelist_add`, em `move.c`). Mas a resposta ao
"o que fazer quando a lista enche" não foi o `assert` recomendado abaixo — foi
`LOG_ERROR(...); return;`, isto é, descarta o lance e segue em frente, logando o evento:

```c
void movelist_add(MoveList *l, Move m) {
    if (l->count >= MAX_MOVES) {
        LOG_ERROR("MoveList cheia (%d lances) -- lance descartado", l->count);
        return;
    }
    l->moves[l->count++] = m;
}
```

Isso é exatamente o padrão que o `project_context.md` chama de problemático em outro lugar
("`LOG_ERROR` não é tratamento de erro... logar e seguir é pior do que não logar, porque dá
a impressão de que o caso foi tratado"). Vale revisitar para `assert` quando o gerador
estiver mais maduro — hoje o teto é `MAX_MOVES = 256`, contra um máximo teórico de 218
lances legais, então o argumento original ainda se aplica.

O bug do `>` vs `>=` tem correção óbvia. Mas há uma decisão embutida: **o que fazer
quando a lista enche?**

Hoje o comportamento seria descartar silenciosamente. Numa posição legal isso
**nunca** acontece — o máximo teórico é 218 lances e `MAX_MOVES` (nome atual; era
`GEN_MOVES_MAX`) é 256. Logo, se
acontecer, é bug em outro lugar: geração duplicada, laço que não termina,
corrupção. Silenciar transforma um bug detectável num perft levemente errado.

```c
void add_move(u32 move, MoveList *list) {
    assert(list->count < MAX_MOVES);   /* 218 é o máximo legal — estourar é bug */
    list->moves[list->count++].move = move;
}
```

### 3.5 Hash Zobrist — opcional, com um motivo não-óbvio — ⬜ não iniciada, decisão ainda aberta

**Status em 16/09:** não há hash — mas **a condição que faltava mudou**: `make_move` está
completo, então já existe onde pendurar a atualização incremental. A recomendação de
sequenciamento continua a mesma e agora tem uma resposta mais clara: **não agora.** A Fase 4
está pela metade, e adicionar Zobrist antes de `unmake_move` existir significa depurar duas
coisas novas ao mesmo tempo, sem o teste-portão para separá-las. Depois do round-trip verde,
o `assert(hash_incremental == hash_recalculado)` no fim de make/unmake vira o detector mais
sensível que existe para esta fase — mas ele só vale se o round-trip já estiver verde,
porque é ele que diz qual das duas metades está errada.

O uso conhecido é a transposition table e a detecção de repetição tripla. Há um
terceiro, que importa **antes** desses dois:

```c
assert(b->hash == compute_hash_from_scratch(b));   /* no fim de make/unmake */
```

O hash incremental é um resumo de 64 bits de **tudo** que o `make_move` deveria ter
alterado: peça na origem, peça no destino, peça capturada, direitos de roque, casa
de en passant, lado a mover. Se o incremental divergir do recalculado, algum desses
foi esquecido — e o assert dispara **no lance exato**, não três níveis de busca
depois. É o detector de bugs de `make/unmake` mais sensível que existe, em ~80
linhas.

**Trade-off honesto:** adicioná-lo na Fase 4 significa duas coisas novas ao mesmo
tempo, e o `make_move` já é a parte mais delicada. Se o cronograma apertar, adie
para depois do perft verde — você perde o uso como depurador, mas mantém o
original. É decisão de sequenciamento, não de arquitetura.

---

## 4. As fases

---

### Fase 0 — Fundação de qualidade 🚧

**Esforço:** 1 sessão. **Prioridade máxima, sem exceção.**

**Status em 16/09: 🔶 parcial, e agora a meia sessão de distância.**

| Item | Estado |
|---|---|
| 1. `CMakeLists.txt` | **Descartado por ora, não pendente.** A equipe decidiu manter só o Makefile até o cliente precisar integrar (`CLAUDE.md` §4). Este item sai da conta da fase |
| 2. `LOG_ERROR` fora do `#ifdef DEBUG` | ✅ feito — `log.h` distingue `LOG_ERROR` (sempre) de `LOG_DEBUG` (condicional) |
| 3. Fechar todos os avisos | ❌ **8 avisos** (eram 22 em 14/09). Cinco são `-Wmissing-prototypes`; os outros três são bugs reais. Ver `next_steps.md` §5 |
| 4. Fechar os bugs conhecidos | 🔶 A lista virou outra: cinco dos seis críticos de `docs/bugs.txt` foram fechados na árvore de trabalho; a lista atual é `project_context.md` §5 |
| 5. Esqueleto de testes | ❌ `test/test.c` existe e o Makefile tem alvo `test`, mas é um **segundo REPL manual sem assertions**. Um alvo chamado `test` que não testa é pior que nenhum, porque parece cobertura |

Medido em 16/09, contra esta árvore, as quatro flags que faltam: `-Wshadow` **0 avisos**,
`-Wcast-qual` **0**, `-Wwrite-strings` **+6** (todos o mesmo defeito: `fail_msg(char *)`
recebendo literal — uma palavra fecha os seis), `-Wconversion` **+8** (todos benignos desta
vez: `&= ~(CASTLE_*)` em `u8`). Ligar as três primeiras custa uma palavra.

Continua sendo o item de maior retorno pendente, e agora é o mais barato também.

#### O quê

1. `CMakeLists.txt` conforme §2.3, com Debug/Release separados e sanitizers no Debug
2. **Tirar `LOG_ERROR` de baixo do `#ifdef DEBUG`.** Hoje todo erro é engolido em
   release — inclusive FEN inválida e lance ilegal. Se quiser níveis, use
   `LOG_DEBUG` (condicional) e `LOG_ERROR` (sempre)
3. Compilar com as flags novas e **fechar todos os avisos**
4. Fechar os 10 bugs conhecidos do `project_context.md` §5
5. Esqueleto de testes: `tests/test_main.c`, sem framework — só assertions e
   contagem de passa/falha

#### Como

Não tente ser esperto. Rode `cmake --build build`, leia cada aviso, corrija,
repita. Os bugs de `main.c` (o `flags` no lugar do `&out`, o `move` digitado
passado ao `make_move`, o buffer de 5 bytes que não comporta `e7e8q`) são de cinco
minutos cada.

Sobre os sanitizers: **nenhum deles pega leitura de memória não inicializada.**
ASan pega fora-dos-limites e use-after-free; UBSan pega comportamento indefinido
aritmético. Quem pegaria memória não inicializada é o MemorySanitizer, que é só do
clang e exige recompilar a libc. Na prática, **o aviso do compilador é a única
defesa viável** — o que reforça o item 3.

#### Por quê agora

> Das últimas seis sessões, todo bug encontrado era detectável em tempo de
> compilação.

Seis de seis, com causa única: o GCC só faz análise de fluxo de dados — a que
alimenta `-Wuninitialized` e família — quando há otimização ligada. Sem `-O`, os
avisos que teriam apontado os bugs simplesmente não disparam. `-Og` é o nível feito
para depuração: liga as análises sem tornar o código irrastreável no gdb.

Todo trabalho posterior fica mais barato. **É o único item do roadmap com retorno
composto.**

#### Critério de saída 🚧

Build limpo com `-Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Og`,
**zero avisos**, e o binário roda sob ASan/UBSan sem disparar nada.

**Onde isso está em 16/09:** 8 avisos, e a metade ASan/UBSan **já passa** — 98 chamadas de
`make_move` em 4 posições, zero disparos. Falta só fechar os avisos, e um deles
(`generate_king_moves` lendo `SQ_TO_EDGE[-1]`, ver Fase 5) é um global-buffer-overflow que o
ASan pega assim que o caminho é exercitado. O item 5 (esqueleto de teste) migrou para a
Fase 4, onde agora é o critério de saída — ver a nota do cabeçalho.

---

### Fase 1 — Vocabulário completo 🚧

**Esforço:** 1–2 sessões.

**Status em 16/09: ✅ FECHADA**, com o critério revisado nesta data.

| Item | Estado |
|---|---|
| 1. `Board` e `Undo` completos | ✅ |
| 2. `fen_parse`/`fen_write` nos 6 campos | ✅ Módulo mais maduro do projeto. Ver `docs/fen.md` |
| 3. Split de módulos | ✅ Mais fundo do que o previsto — 11 módulos |
| 4. `board_check_invariants()` | ✅ Implementada (`board.c:42`). Cobre 4 das 6 checagens de `docs/guides.md` — ver a ressalva abaixo |
| 5. `is_own` / `is_enemy` | ✅ Cópia única em `piece.h`, usada por `movegen.c` e `board.c` |
| 6. Teste de round-trip de FEN sobre um corpus | ➡️ **movido para a Fase 4** (ver nota do cabeçalho) |

**Ressalva registrada, não bloqueante:** `board_check_invariants` verifica código de peça
válido, exatamente um rei de cada cor, o cache `king_square` contra uma varredura do zero, e
nenhum peão na 1ª/8ª fileira. **Faltam duas** das seis de `docs/guides.md`: coerência de
`ep_square` com `side_to_move`, e direitos de roque contra rei/torres nas casas de origem.
A segunda **já está escrita** — `castling_matches_board`, hoje `static` em `fen.c:385`; expor
é a maior parte do trabalho. As duas entram antes de a Fase 4 rodar, porque é lá que a função
vai ser chamada ~1000 vezes e é lá que a cobertura dela se paga. Fechar a Fase 1 com isso
anotado é honesto; fechá-la e esquecer não seria.

A função também mudou de assinatura em relação ao proposto: `board_check_invariants(const
Board *b)`, sem o out-param `const char **fail_msgs`, com as falhas acumuladas num log global
em `utils.c`. Defensável, e com duas consequências já pagas — ver `project_context.md` §5,
Bug #7.

#### O quê

1. `Board` e `Undo` completos (§2.1)
2. `fen_parse` lendo os 6 campos; `fen_write` emitindo os 6 (nomes atuais, em `fen.h`;
   era `parse_fen`/`board_to_fen` quando este roadmap foi escrito)
3. Split de módulos (§3.3) e corte dos includes de `types.h`
4. `board_check_invariants()` implementada
5. Helpers `is_own` / `is_enemy` (§2.4)
6. ~~Teste de round-trip de FEN sobre um corpus~~ → **movido para a Fase 4 em 16/09**

#### Como

**O corpus de FEN era o entregável real desta fase, e continua sendo entregável — só que da
Fase 4.** ~30 posições: inicial, Kiwipete, posições com en passant ativo, com roque parcial
(`Kq`, `-`), com contadores não-triviais, com promoção iminente. Num `const char *[]` num
`.c` próprio, não num arquivo lido em runtime — sem I/O, sem caminho relativo, sem caso de
erro no caminho do teste.

> Enquanto `fen_write` só emitia a posição, o round-trip não provava nada sobre
> roque e en passant. Esse momento já passou: `fen_write` emite os seis campos desde 07/09,
> e o round-trip foi conferido em todas as posições em que o módulo foi exercitado. É por
> isso que testá-lo isoladamente agora não compra informação nova — e por isso ele foi para
> a Fase 4, onde a mesma comparação também prova o make/unmake.

**Este item pode ser delegado** a um colega sem C — ver `onboarding-motor.md` §9.1. Montar o
corpus é trabalho de xadrez, não de C, e é o caminho crítico da Fase 4.

**Sobre `board_check_invariants()`** — o que verificar:

- exatamente um rei de cada cor
- `array[king_square[c]]` é de fato o rei da cor `c` — *pega o cache dessincronizado*
- nenhum peão na 1ª ou 8ª fileira
- `ep_square`, se definido, na fileira coerente com `side_to_move`
- direitos de roque consistentes com rei/torre nas casas iniciais
- nenhum código de peça inválido (7, 8, 15)

Chamar no início e no fim de `make_move`/`unmake_move` e depois de `fen_parse`. Em
release ela desaparece. O retorno é desproporcional: praticamente todo bug de
`make/unmake` se manifesta como quebra de invariante **uma ou duas jogadas antes**
de causar sintoma visível — e a diferença entre depurar na causa e depurar no
sintoma, dentro de uma árvore de profundidade 6, é de horas.

**Confira também** se `fen_parse` monta num `Board` local e só copia para o
chamador em caso de sucesso. Uma FEN inválida não pode deixar o motor num estado
meio-inicializado, porque o comando seguinte vai operar sobre lixo.

#### Critério de saída 🚧 — revisado em 16/09

**Antes:** "Round-trip idêntico nas 30 posições do corpus; `board_check_invariants` passa em
todas."

**Agora:** `board_check_invariants` implementada e passando nas posições exercitadas;
`fen_parse`/`fen_write` completos nos seis campos.

O round-trip de FEN sobre o corpus **saiu daqui e entrou na Fase 4**. O raciocínio está no
cabeçalho deste documento: o módulo `fen` já está maduro, e o teste dele não desaparece —
ele vira o **oráculo** do teste de make/unmake, onde cada uma das ~1000 comparações é
também um round-trip de FEN. Testar duas vezes a mesma coisa em duas fases diferentes é
tempo gasto sem informação nova; testar uma vez, no lugar onde ela também prova outra coisa,
é o mesmo teste rendendo o dobro.

**Atingido em 16/09.**

---

### Fase 2 — Geometria pré-computada

**Esforço:** 1 sessão.

**Status em 16/09: 🔶 parcial — o rei saiu da fase, o cavalo continua nela.**

A metade do **rei** foi resolvida de outro jeito, e o jeito certo: `generate_king_moves` usa
`SQ_TO_EDGE` com o laço parando em 1, que é o mesmo mecanismo dos deslizantes. `KING_TARGETS`
e `KING_COUNT` **não são mais entregáveis desta fase** — `KING_TARGETS[64][8]` está alocada e
zerada em `square.c:16` sem nenhum leitor, e o trabalho aqui é **apagá-la**, não preenchê-la
(ver §3.1).

A metade do **cavalo** continua exatamente onde estava. `KNIGHT_TARGETS[64][8]` alocada e
zerada; `init_square_tables()` só popula `SQ_TO_EDGE` e `PAWN_ATTACKS`; `KNIGHT_COUNT` não
existe; não há `generate_knight_moves`. Medido em 16/09: posição inicial gera **16** lances
(correto: 20), Kiwipete gera **37** (correto: 48).

#### O quê

`KNIGHT_TARGETS` e `PAWN_ATTACKS` (§3.1), geradas em `precompute_move_data(void)` — hoje
chama-se `init_square_tables(void)`, e o corpo ainda não faz a parte do cavalo. `PAWN_ATTACKS`
já está pronta. **`KING_TARGETS` saiu do escopo em 16/09** e deve ser apagada.

`offset_square` (`square.c:23`) já faz a aritmética `(rank, file)` com checagem de limites e
já é usada por `init_pawn_attacks` — reaproveite-a para o cavalo em vez de escrever uma
segunda.

Note o `void`: em C, `()` declara "argumentos não especificados", não "nenhum
argumento" — é o aviso de `-Wstrict-prototypes` que já apareceu na Fase 0.

#### Como

Aritmética `(file, rank)` com checagem de limites explícita, **no init**, onde é
fácil de verificar e onde um erro não é silencioso.

Teste por contagens conhecidas:

| Verificação | Valor esperado |
|---|---|
| Cavalo em a1 | 2 destinos |
| Cavalo em b1 | 3 destinos |
| Cavalo em c3 | 8 destinos |
| Cavalo em h8 | 2 destinos |
| Soma de `KNIGHT_COUNT` sobre as 64 casas | **336** |

A soma é a forma mais barata de verificar uma tabela inteira de uma vez. As linhas de rei
saíram: ele não tem tabela, e conferir `SQ_TO_EDGE` é outra verificação.

#### Critério de saída

A soma bate (336); `KING_TARGETS` foi apagada; `generate_all_moves` na posição inicial
devolve **20** e na Kiwipete devolve **48**.

Nota sobre `SQ_OFFBOARD`: o critério original pedia a deleção dele. Ele continua vivo e
**continua sendo usado de propósito** em `make_move` (`assert`) e em `generate_pawn_moves` —
lugares que não são geração por tabela. A deleção era consequência esperada de tirar a
checagem de borda do runtime na geração; como ela virou defesa em outra camada, o item cai.

---

### Fase 3 — Geração pseudo-legal completa

**Esforço:** 2–3 sessões.

**Status em 16/09: 🔶 parcial — falta uma peça, literalmente.**

| Item | Estado |
|---|---|
| 1. `side_to_move` no laço externo | ✅ nos três geradores (§2.4) |
| 2. Assinatura única `generate_pseudo_legal` | 🔶 existe `generate_all_moves(Board *, MoveList *)` (`movegen.c:230`), que limpa a lista e chama os três geradores. É a entrada única pedida, mas em cima de três laços de 64, não de um só — e recebe `Board *` não-const enquanto os três que ela chama recebem `const Board *` |
| 3. Deslizantes com `SQ_TO_EDGE` | ✅ correto |
| 4. Cavalo e rei | 🔶 **rei ✅** (8 direções, `SQ_TO_EDGE`); **cavalo ❌** (Fase 2) |
| 5. Peão | ✅ **completo**: push, duplo aninhado dentro do simples, captura, promoção nas **quatro** peças (com e sem captura), en passant |
| 6. Roque | ✅ gerado — direito presente **e** casas entre rei e torre vazias (`check_castle_side`). Xeque/casa atacada ficam para a Fase 5, de propósito |

Verificado em 16/09: a Kiwipete gera 37 lances pseudo-legais contra 48 legais — a diferença é
exatamente o cavalo. Fechada a Fase 2, esta fecha junto.

#### O quê e em que ordem

A ordem interna importa:

1. **`side_to_move` no laço externo** (§2.4) — antes de escrever qualquer peça
   nova, senão a mesma checagem é escrita seis vezes
2. **Fixar a assinatura** `generate_pseudo_legal(const Board *, MoveList *)`
3. **Deslizantes** — uma função só, parametrizada por intervalo de direção (torre
   0–3, bispo 4–7, dama 0–7), dirigida por `SQ_TO_EDGE`
4. **Cavalo e rei** por tabela — trivial depois da Fase 2
5. **Peões por último** — é o que tem mais casos

#### Como

**Deslizantes.** Laço interno de três saídas: casa vazia → adiciona e continua;
peça inimiga → adiciona e **para**; peça amiga → **para**. O `break` da captura fica
*fora* do `if` de cor, senão sua torre atravessa seu próprio peão.

Lembrete do `project_context.md`: a ordem do `enum Direction` é **carga estrutural,
não estética**. Torre usa 0–3, bispo 4–7, dama 0–7. Reordenar quebra isso em
silêncio.

**Peões — os cinco casos, e por que são cinco:**

| Comportamento | O que quebra a simetria |
|---|---|
| Avanço reto | Destino tem que estar **vazio** — oposto de toda outra peça |
| Captura diagonal | Destino tem que estar **ocupado por inimigo** |
| Avanço duplo | Só da fileira inicial, e **aninhado dentro** do avanço simples |
| Promoção | Um lance de origem gera **quatro** `Move` distintos |
| En passant | Captura numa casa onde **não há peça** |

Os dois erros que mais custam aqui:

- **Avanço duplo em paralelo com o simples**, em vez de aninhado. O peão salta
  sobre uma peça. Sintoma: perft(1) da posição inicial dá 20 (correto, porque não
  há bloqueios lá) e quebra na profundidade 2 ou 3.
- **Promoção só no caso de captura.** Um peão que avança reto para a 8ª fileira
  também promove, e também gera 4 lances.

**Sobre o `ep_square`:** há duas convenções — defini-lo a todo avanço duplo, ou só
quando existe peão inimigo capaz de capturar. A segunda é otimização que afeta o
hash Zobrist, **não** as contagens de perft. As posições de referência publicadas
usam a primeira. Use a primeira agora.

#### Critério de saída

`perft(1)` da posição inicial = **20**; da Kiwipete = **48**. Nessas duas posições
as contagens pseudo-legais coincidem com as legais, então servem como checagem
antecipada antes da Fase 5 existir.

---

### Fase 4 — `make_move` / `unmake_move` 🚧

**Esforço:** 2–3 sessões. **A fase mais delicada do projeto.**

**Status em 16/09: 🔶 metade — e é a metade difícil que já foi.**

**`make_move` está completo e verificado.** Ele faz, na ordem certa: decodifica, **salva o
`Undo` antes de tocar no tabuleiro** (que era o ponto crítico), move a peça, inverte o lado,
define `ep_square` no push duplo, atualiza `king_square`, derruba direitos de roque por
origem **e** por destino, move a torre do roque, limpa a casa correta no en passant, troca a
peça na promoção, e ajusta `halfmove_clock` e `fullmove_number`.

Verificado nesta data com a FEN de saída conferida à mão em cada caso:

```
roque O-O            e1g1  -> r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1
roque O-O-O          e1c1  -> r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1
en passant           e5f6  -> rnbqkbnr/ppp1p1pp/5P2/3p4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3
promoção a dama      e7e8q -> rn2Q2k/1P6/8/8/8/8/8/7K b - - 0 1
promoção com captura b7a8q -> Qn5k/4P3/8/8/8/8/8/7K b - - 0 1
push duplo           e2e4  -> rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1
torre captura torre  h1h8  -> r3k2R/8/8/8/8/8/8/R3K3 b Qq - 0 1
```

A última linha é a que mais vale: `h1h8` derruba `K` (a torre branca saiu de h1) **e** `k`
(a torre preta foi capturada em h8 sem se mover). É a Armadilha 2 desta seção, e ela passou.
Mais 98 chamadas de `make_move` em 4 posições sob ASan/UBSan, zero disparos.

**`unmake_move` não tem corpo.** `makemove.c:130` é a declaração repetida, terminada em `;`.
Reproduzido: `undefined reference to 'unmake_move'` no link. O build passa porque ninguém
chama. É o item que bloqueia as Fases 5 e 6b inteiras, e o teste-portão desta própria fase.

Detalhe de implementação que só aparece agora: **no en passant `u->captured` vale `EMPTY`**,
porque a casa de destino estava vazia. O `unmake` tem de deduzir o peão do `MoveType` +
`side`, não lê-lo do `Undo`. Plano passo a passo em `next_steps.md` §2.

#### O quê

`make_move` completo: inverter `side_to_move`, atualizar `king_square`, tratar
captura, roque movendo duas peças, en passant limpando a casa correta, promoção
trocando o tipo, `castle_mask`, `halfmove_clock`, `fullmove_number`.
`unmake_move` restaurando via `Undo`.

#### Como

**O teste que define esta fase:**

```
para cada posição do corpus:
    fen_antes = fen_write(b)
    para cada lance pseudo-legal m:
        Undo u;
        make_move(b, m, &u)
        unmake_move(b, m, &u)
        assert(fen_write(b) == fen_antes)
        assert(board_check_invariants(b))
```

Trinta posições × ~35 lances = **~1000 verificações**, e roda em milissegundos. Ele
pega praticamente todo bug de `make/unmake` — incluindo os dois abaixo — **antes**
que o perft precise entrar em cena.

**Armadilha 1 — en passant.** A peça capturada **não está** na casa de destino. Está
na casa adjacente, na mesma fileira de onde o peão capturador partiu:

```
Peão branco em e5, peão preto acabou de jogar d7-d5.  ep_square = d6.

  Lance:            e5xd6 en passant
  Peça capturada:   o peão preto em D5, não em d6
  Casa a limpar:    ep_square - PUSH[side]  =  d6 - 8  =  d5
```

Restaurar a peça capturada em `target` no `unmake` faz sumir um peão e aparecer
outro.

**Armadilha 2 — `king_square`.** O `assert` de debug que recalcula o rei do zero e
compara com o cache. Campo denormalizado que dessincroniza é dos bugs mais chatos
de rastrear — o `project_context.md` já registra isso; aqui é onde se paga.

**Zobrist (opcional, §3.5):** se entrar, entra aqui, com o assert de
incremental-vs-recalculado.

#### Por quê é portão

Sem `unmake_move` correto não há filtro de legalidade, não há perft e não há busca.
Um bug aqui contamina todas as três, e se manifesta em cada uma de forma diferente
— o que torna o diagnóstico muito mais difícil do que o teste acima.

#### Critério de saída 🚧 — ampliado em 16/09

Round-trip de make/unmake verde nas ~30 posições do corpus, com `board_check_invariants`
passando — **e, na mesma passagem, o round-trip de FEN sobre esse mesmo corpus**, que era o
critério de saída da Fase 1 até esta data.

Os dois cabem no mesmo laço porque são a mesma comparação:

```
para cada FEN do corpus:
    fen_parse(fen, &b)
    fen_antes = fen_write(&b)
    assert(fen_antes == fen)                 <- round-trip de FEN (ex-Fase 1)
    assert(board_check_invariants(&b))

    generate_all_moves(&b, &lista)
    para cada lance m:
        Undo u;
        make_move(&b, m, &u)
        unmake_move(&b, m, &u)
        assert(fen_write(&b) == fen_antes)    <- round-trip de make/unmake
        assert(board_check_invariants(&b))
```

Uma linha a mais no teste, e a Fase 1 ganha seu portão de verdade em vez de um teste
isolado. O corpus continua sendo entregável — ~30 posições num `const char *[]` num `.c`
próprio, começando pelas quatro posições de referência dos *Perft Results*, uma com en
passant disponível, uma com promoção a um lance, uma com os quatro roques vivos e uma com
roque já perdido.

**Antes de rodar 1000 iterações:** `fail_msg` (`utils.c:14`) acumula num log global que nunca
é zerado e tem teto de 128. Ou ele ganha um reset, ou o teste para na primeira falha — que é
a opção melhor, porque numa suíte de round-trip a primeira falha é a informativa e as 999
seguintes são consequência.

---

### Fase 5 — Filtro de legalidade 🚧

**Esforço:** 1–2 sessões.

**Status em 16/09: ⬜ não iniciada.** Bloqueada na Fase 2 (precisa de `KNIGHT_TARGETS`
populada para a busca reversa) e na Fase 4 (precisa de `unmake_move` para aplicar-e-desfazer).
Não existe `is_square_attacked` no repositório.

**Um bug que esta fase vai acordar, e que já está reproduzido em 16/09.**
`generate_king_moves` (`movegen.c:191`) lê `SQ_TO_EDGE[b->king_square[side]][dir]` **sem
checar `SQ_NONE`**:

```
FEN "R6k/8/8/8/8/8/8/K7 w - - 0 1"
  o gerador emite a8h8 — captura do rei, pseudo-legal e legítima nesta camada
  make_move grava king_square[BLACK] = -1
  o generate_all_moves seguinte:
    movegen.c:191 runtime error: index -1 out of bounds for type 'int [64][8]'
    AddressSanitizer: global-buffer-overflow
```

Hoje o sintoma é silencioso, porque o vizinho na memória é `KNIGHT_TARGETS`, que está zerada
— o `continue` da linha seguinte salva por acidente. **Isso deixa de valer no instante em que
a Fase 2 popular `KNIGHT_TARGETS`**, e o filtro de legalidade é exatamente o código que
aplica lances de captura de rei aos milhares. Duas linhas de checagem em
`generate_king_moves` tornam o bug inexprimível; confiar em pré-condição documentada seria a
mesma "armadilha lembrada" que §2.4 já decidiu não aceitar.

#### O quê

`is_square_attacked(const Board *, int sq, Color by)` por busca reversa;
`generate_legal` como filtro sobre a pseudo-legal; `in_check`, e daí mate e
afogamento de graça.

#### Como

**A busca reversa** aproveita a simetria dos ataques: em vez de gerar todos os
lances do adversário e ver se algum chega em `sq`, você **se posiciona em `sq` e
olha para fora**:

| Família | A partir de `sq`... | Custo |
|---|---|---|
| Peão | diagonais **invertidas** pela cor do atacante | 2 lookups |
| Cavalo | offsets de cavalo, procurando cavalo inimigo | 8 lookups |
| Rei | offsets de rei, procurando rei inimigo | 8 lookups |
| Diagonais | caminhar até a **primeira** peça: é bispo ou dama? | 4 raios |
| Retas | caminhar até a **primeira** peça: é torre ou dama? | 4 raios |

Ordene do mais barato ao mais caro e retorne na primeira confirmação.

**Os raios param na primeira peça.** Se ela for do tipo errado — ou sua própria —
você **para**, não continua. Bloqueantes bloqueiam. Continuar depois da primeira
peça é o bug número um dessa função, e o sintoma é o motor achar que está em xeque
quando não está.

**A inversão de peão** é a armadilha específica. Peões são a única peça cuja relação
de ataque não é simétrica — eles atacam só para frente. Para achar peões **brancos**
que atacam uma casa, você olha nas diagonais **para baixo**. É onde a tabela
`PAWN_ATTACKS` da Fase 2 se paga: ela torna a inversão explícita.

**O bug de sinal no filtro.** Depois de `make_move`, `side_to_move` **já virou**. O
rei a testar é o de `!b->side_to_move`. Escrever `b->side_to_move` produz um motor
que verifica se está *dando* xeque em vez de *tomando* — comportamento bizarro o
bastante para custar horas.

**Roque.** As condições "rei não está em xeque" e "rei não passa por casa atacada"
ficam na **geração**, não no filtro — o filtro só cobre a casa final. E no roque
grande, b1/b8 precisa estar **vazia** mas **não** desatacada: a torre passa por ela,
o rei não.

#### O que o filtro captura de graça

Vale saber, porque justifica a escolha da abordagem mais lenta:

- **peça cravada** — você nunca escreve a palavra "cravada"
- **não sair de um xeque existente**
- **rei fugindo na mesma linha do ataque** — rei em e1 sob xeque de torre em a1 não
  pode ir para d1 nem f1. Abordagens que calculam casas atacadas com o rei ainda no
  tabuleiro erram isso, porque o próprio rei bloqueia o raio
- **en passant com xeque descoberto** — dois peões saem da mesma fileira ao mesmo
  tempo, expondo o rei a uma torre. É o caso que gerações "só legais" mais erram, e
  o filtro acerta porque não raciocina sobre a regra: ele aplica e olha

#### Critério de saída 🚧

Numa posição com peça cravada, o lance dela **não** aparece na lista legal; posição
de mate conhecida retorna lista vazia com `in_check` verdadeiro.

---

### Fase 6a — Protocolo mínimo + IA aleatória

**Esforço:** 1 sessão. **Faça antes do perft estar verde.**

**Status em 14/09: ⬜ não iniciada.** A interação hoje é só o menu interativo `ui()` em
`main.c` (opções numeradas, `fgets` por linha) — nenhum comando UCI, nenhum `setvbuf`, nenhum
dispatch de linha. §7 (a divergência entre `arquitetura-xadrez.md` e `project_context.md`
sobre quem fala com o motor) segue sem resposta registrada e continua bloqueando esta fase.

#### O quê

| Comando | Resposta |
|---|---|
| `uci` | `id name`, `id author`, `uciok` |
| `isready` | `readyok` |
| `position startpos [moves ...]` | — |
| `position fen <FEN> [moves ...]` | — |
| `go [movetime N \| depth N]` | `bestmove <lance>` (aleatório, por ora) |
| `quit` | encerra |
| `d` | imprime o tabuleiro *(não-UCI, depuração)* |
| `perft N` | contagem *(não-UCI, depuração)* |
| desconhecido | ignora em silêncio (a spec manda) |

#### Como

**As duas linhas obrigatórias no começo do `main`:**

```c
setvbuf(stdout, NULL, _IOLBF, 0);
setvbuf(stdin,  NULL, _IONBF, 0);
```

Quando a stdout é um terminal, a libc usa buffer de linha e cada `printf` sai na
hora. Quando é um **pipe** — exatamente o caso do cliente rodando o motor como
subprocesso — ela troca para buffer de bloco de 4 KB. O `bestmove` fica preso, o
cliente espera para sempre, e **não há sintoma para depurar**. Este é o bug que, se
aparecer na demo, não tem depuração possível ao vivo.

**Fim de linha:** `line[strcspn(line, "\r\n")] = '\0';` trata `\n` e `\r\n` numa
linha só. A própria spec do UCI avisa que o terminador varia por sistema
operacional.

**Buffer de entrada generoso:** `position startpos moves ...` numa partida longa
passa de 1 KB. Use 8192.

**Um invariante barato e valioso:** antes de emitir `bestmove`, verifique que o
lance está na lista legal. Custa uma varredura e garante que o motor **nunca**
devolve lance ilegal — que é um dos requisitos funcionais do projeto.

**Suporte o sufixo `moves` desde já.** Ele depende só do `make_move`, que a Fase 4
entregou, e é o que permite plugar o motor no Cute Chess ou Arena.

**Limitação aceita conscientemente:** a spec exige que o motor processe stdin
enquanto pensa (para atender `stop`). Um laço bloqueante não faz isso. Para busca
rasa é irrelevante. Resolve-se depois com uma thread de input — **anote como dívida
técnica consciente**, não deixe implícito.

#### Por quê agora, fora de ordem

A equipe do cliente está bloqueada esperando um alvo de integração. Um motor que
fala o protocolo corretamente e joga mal desbloqueia semanas de trabalho paralelo.
E integração descobre cedo problemas de protocolo que você não quer descobrir na
véspera da apresentação.

#### Critério de saída

O cliente (ou Cute Chess/Arena) joga uma partida inteira contra o motor, do início
ao mate ou afogamento, sem travar.

---

### Fase 6b — Perft 🚧

**Esforço:** 2–4 sessões. Altamente variável — **é a fase de encontrar bugs, não de
escrever código.**

**Status em 14/09: ⬜ não iniciada.** Não há `perft` nem `perft divide` no código. Sem
Fases 2, 4 e 5, não há o que contar de forma confiável ainda.

#### O quê

`perft(depth)` e `perft divide`.

#### Como

**`divide` é a técnica; `perft` é só o alarme.** `perft` diz *que* há bug. `divide`
diz *onde*: para cada lance da raiz, imprime o lance e `perft(N-1)` dele. Compare
com o Stockfish (`go perft N`, mesmo formato). Ache o primeiro lance divergente,
aplique, `divide N-1`, repita. É uma busca binária dentro da árvore.

> Isso transforma "meu perft(4) dá 197.283 em vez de 197.281" — insolúvel por
> inspeção — em "nesta posição específica, meu gerador produz dois lances a mais".
> É a diferença entre uma tarde perdida e dez minutos.

**Referências:**

| Posição | d1 | d2 | d3 | d4 | d5 |
|---|---|---|---|---|---|
| Inicial | 20 | 400 | 8.902 | 197.281 | 4.865.609 |
| Kiwipete | 48 | 2.039 | 97.862 | 4.085.603 | 193.690.690 |

Kiwipete: `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`

A posição inicial **não basta** — ela não exercita roque, en passant nem promoção.
Use pelo menos 4 posições da página *Perft Results*. As colunas de detalhe
(capturas, EP, roques, promoções) permitem **localizar a categoria** do bug antes de
rodar `divide`.

**Duas armadilhas do perft em si:**

- **Bulk counting inválido.** Otimizar retornando `list.count` na profundidade 1 só
  vale se a lista for **legal**, não pseudo-legal.
- **Promoções contam 4.** Uma promoção com escolha de 4 peças conta como 4 nós,
  porque o gerador cria 4 lances distintos.

#### Por quê é portão

**Nada de busca ou avaliação antes do perft bater.** Um bug de geração se manifesta
na busca como comportamento inexplicável seis níveis de profundidade abaixo, e você
procura no lugar errado por dias.

**Registre no repositório cada divergência encontrada e sua causa.** Vira material
de apresentação **e** rede de segurança contra o gargalo de conhecimento (§6).

#### Critério de saída 🚧

4 posições de referência, todas até profundidade 5, exatas.

---

### Fase 7 — IA incremental

**Esforço:** 3–5 sessões até v2; o resto conforme sobrar tempo.

**Status em 14/09: ⬜ não iniciada.** Não há avaliação, busca, nem seleção de lance de
nenhum tipo — nem o v0 (aleatório) da Fase 6a. Depende de tudo antes dela.

#### O quê

Cada versão é jogável e demonstrável isoladamente:

| Versão | O quê | Ganho esperado |
|---|---|---|
| v0 | lance aleatório | *(feito na 6a)* |
| v1 | avaliação só material + minimax profundidade fixa 3 | joga xadrez reconhecível |
| v2 | poda alfa-beta | ~10× menos nós → profundidade 5–6 |
| v3 | aprofundamento iterativo + `go movetime` | controle de tempo real |
| v4 | ordenação de lances (MVV-LVA) | alfa-beta fica muito mais eficaz |
| v5 | busca de quiescência | **maior salto de força por linha escrita** |
| v6+ | piece-square tables, avaliação tapered, TT com Zobrist | refinamento |

#### Como

**O teste de regressão que ninguém faz e que economiza mais dor:** v2 deve devolver
**exatamente o mesmo lance** que v1 na mesma profundidade. Alfa-beta poda sem
alterar o resultado do minimax. Se divergir, a poda está errada.

**Proteja o v5.** Sem quiescência o motor sofre efeito de horizonte severo: ele para
a busca no meio de uma sequência de trocas e avalia uma posição que qualquer humano
vê como perdida. É a diferença mais visível numa demo.

**Este é o momento de quebrar o item 4.2 da EAP.** "Busca minimax + alfa-beta +
aprofundamento iterativo" como item único fica 0% por três semanas e depois pula
para 100% — o burndown não mostra nada. Quebrado em v1…v5, cada versão é
entregável, demonstrável e verificável.

#### Critério de saída

v2 rodando, devolvendo o mesmo lance que v1 e alcançando profundidade ≥5 em 2
segundos.

---

### Fase 8 — Robustez e empacotamento

**Esforço:** 2 sessões.

**Status em 14/09: ⬜ não iniciada.** Depende de todas as anteriores.

#### O quê

Traduzir "robusto" em invariantes verificáveis — que viram critério de aceite dos
requisitos não funcionais:

| Propriedade | Como garantir | Como verificar |
|---|---|---|
| Nunca crasha | ASan/UBSan em todo build de teste | perft profundo sob sanitizers |
| Nunca devolve lance ilegal | assert `bestmove ∈ generate_legal` | o próprio assert, **também em release** |
| Nunca trava | `setvbuf` no `main`; `go` sempre responde | testar via pipe, não só no terminal |
| Entrada malformada não corrompe | `fen_parse` monta em local e só copia no sucesso | fuzz: 10k strings aleatórias |
| Comando desconhecido é ignorado | dispatch com fallback silencioso | mandar lixo pelo stdin |
| Reprodutível | stateless entre comandos; sem RNG não-semeado | mesma FEN + profundidade → mesmo lance |

**O fuzz do parser de FEN** é ~15 linhas e desproporcionalmente valioso: gere
strings aleatórias e mutações do corpus (trocar um caractere, cortar no meio,
duplicar uma `/`), chame `fen_parse`, verifique que ou retorna `false` ou produz um
`Board` que passa nos invariantes. Sob ASan, encontra estouros que teste manual não
encontra. **Delegável a um colega sem C** — ver `onboarding-motor.md` §9.4.

---

## 5. Ordem de sacrifício se o cronograma apertar

Do menos doloroso ao mais:

1. **Zobrist / transposition table** — puro desempenho, e repetição tripla é regra rara
2. **v6+ da IA** (PSQT, tapered eval) — força de jogo, não correção
3. ~~Split de módulos~~ — **já feito** (Fase 1, e mais fundo do que o item previa). Removido
   desta lista em 14/09
4. **v4 (ordenação de lances)** — perde profundidade, não correção
5. ~~**CMake**~~ — **cortado por decisão, não por cronograma** (`CLAUDE.md` §4: Makefile
   agora, CMake quando o cliente precisar integrar). O que **não** foi cortado junto são as
   quatro flags de `CLAUDE.md` §7 que o Makefile ainda não liga: medidas em 16/09, três delas
   custam **0, 0 e 6 avisos** (`-Wshadow`, `-Wcast-qual`, `-Wwrite-strings`), e os 6 são o
   mesmo defeito de uma palavra. Cortar a migração nunca foi motivo para adiar essas

### O que **não** se corta, em nenhuma hipótese

| Item | Por quê |
|---|---|
| **Fase 0** | Cortar aqui torna todo o resto mais caro. Retorno composto |
| **Perft** | Sem ele não há evidência de que o motor joga xadrez. Nenhuma quantidade de teste manual substitui |
| **`setvbuf`** | Duas linhas entre demo funcionando e demo travada sem sintoma |
| **Quiescência (v5)** | É o que separa "joga xadrez" de "faz lances legais" |

---

## 6. Riscos

| Risco | Impacto | Mitigação |
|---|---|---|
| Fase 6b (perft) se estender muito | Atraso em cascata na IA | Portões 1, 4 e 5 existem exatamente para reduzir a superfície de bugs que chega lá |
| **Gargalo de conhecimento** | Se o perft travar, trava com uma pessoa só | Registrar cada divergência e sua causa no repositório; `onboarding-motor.md` §9 lista trabalho delegável |
| Camada de integração indefinida (§7) | Protocolo implementado errado | Resolver **antes** da Fase 6a |
| Bugs de memória em C | Crash em demo | Sanitizers ligados em todo build de teste desde a Fase 0 |
| Escopo da IA crescer | Perft não fecha, IA fica pela metade | Ordem de sacrifício (§5) decidida **antes** da pressão, não durante |

Sobre o gargalo de conhecimento: ele merece entrar no documento de riscos da
disciplina, não só aqui. É um risco de projeto clássico (*bus factor* = 1) e a
mitigação já está parcialmente implementada — o `project_context.md` existe
justamente para isso. O que ele **não** documenta é o estado mental de depuração,
e é por isso que o registro de divergências de perft importa.

---

## 7. Decisão pendente que bloqueia a Fase 6a

Os documentos do projeto divergem:

- `arquitetura-xadrez.md`: o cliente **nunca** fala diretamente com o motor —
  sempre via backend Node/TypeScript.
- `project_context.md`: o **cliente C++ roda o motor como subprocesso direto**, sem
  backend no caminho.

Consequência concreta para o motor:

| Se... | O motor precisa... |
|---|---|
| Cliente fala direto | Expor mais comandos (`legalmoves` para a UI destacar lances); requisito de partidas simultâneas some |
| Há backend no meio | Ficar mais fino; o backend monta as respostas para a UI |

**Resolver antes da Fase 6a**, porque é ela que define a superfície do protocolo.
Conversa de 15 minutos que evita implementar comandos que ninguém vai chamar — ou
descobrir na integração que falta um.

---

## 8. Mapeamento para a EAP

| Fase | Item da EAP | Observação |
|---|---|---|
| 0 | — | Infraestrutura. Vale registrar como tarefa própria: é 1 sessão com retorno em todas as outras |
| 1 | 3.1 Representação + FEN | |
| 2, 3 | 3.2 Geração pseudo-legal | |
| 4 | 3.5 Apply/undo com pilha | |
| 5 | 3.3 Filtro de legalidade + 3.6 Detecção de fim de jogo | 3.6 sai de graça a partir de 3.3 |
| 6a | 4.4 Interface de comandos | Antecipada em relação à EAP, por dependência externa |
| 6b | 3.7 Testes perft | |
| 7 | 4.1, 4.2, 4.3 | **Quebrar 4.2 em v1…v5** para visibilidade no burndown |
| 8 | 4.5 Testes de validação | |

Três notas para a apresentação:

**A APF encaixa mal aqui, e isso deve ser dito.** O motor tem essencialmente duas
transações (recebe posição, devolve lance) e milhares de linhas de lógica atrás
disso. A complexidade é **algorítmica**, não transacional. Reconhecer a limitação
explicitamente demonstra compreensão da técnica; forçar uma contagem artificial
demonstra o oposto.

**O perft é o melhor material de teste do projeto.** Teste determinístico, oráculo
externo (Stockfish), cobertura comprovada de casos-limite, e uma técnica de
depuração associada que é literalmente busca binária no espaço de estados. Vale
slide próprio.

**A Fase 0 é um argumento de qualidade de processo.** "Identificamos que seis de
seis bugs recentes eram detectáveis em compilação, diagnosticamos a causa raiz na
configuração de build, e corrigimos" é exatamente o tipo de raciocínio que a
disciplina quer ver — e é verdade.

---

## 9. Nota sobre esta revisão (16/09)

**A Fase 1 fechou** — a primeira a fechar um critério de saída 🚧, e ela fechou de duas
formas ao mesmo tempo: `board_check_invariants` ganhou corpo (era o item de maior retorno
isolado apontado na revisão de 14/09), e o teste de round-trip de FEN saiu do critério por
decisão, não por omissão. Essa segunda metade merece ser lida como decisão de engenharia e
não como corte: o módulo `fen` é o mais maduro do projeto, e o teste dele não desapareceu —
ele mudou de fase para onde ele prova duas coisas de uma vez, como oráculo do round-trip de
make/unmake. Testar duas vezes a mesma coisa em duas fases é tempo gasto sem informação nova.

**A Fase 4 está pela metade, e é a metade difícil que já foi.** `make_move` trata roque, en
passant, promoção nas quatro peças, relógios e direitos de roque por origem **e** por destino
— inclusive a Armadilha 2 desta seção, que é o bug clássico de perft na profundidade 4 — e
tudo isso foi verificado com FEN de saída conferida à mão, não por leitura. O que falta é o
espelho: `unmake_move` não tem corpo, e é erro de link.

**A lição de processo desta revisão é a mesma de sempre, e desta vez em positivo.** Toda
verificação acima foi feita com harness descartável em `/tmp`. Funcionou: provou que
`make_move` está certo **hoje**. Não prova nada sobre amanhã, e não roda no `make` de
ninguém. O mesmo código, dentro do repositório e no alvo `test`, é regressão permanente —
e é literalmente o critério de saída da Fase 4. É o item de maior retorno pendente do
roadmap inteiro, e já era na revisão passada.

Para o estado bug-a-bug, use sempre `project_context.md` §5 — este documento rastreia fases
e decisões, não a lista de bugs, que muda rápido demais para viver em dois lugares.
`docs/bugs.txt` é a revisão de 14/09 e está obsoleto: cinco dos seis itens críticos de lá
foram corrigidos na árvore de trabalho.

---

*Documento vivo. Ao concluir uma fase, marque o critério de saída como atingido e
atualize o `project_context.md` §2 (estado atual) e §5 (bugs abertos). Marcações de status
(✅/🔶/⬜) devem ser revisadas na mesma ocasião — não deixe uma fase "parcial" congelada
depois que ela fechar.*
