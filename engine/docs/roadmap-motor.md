# Roadmap do Motor de Xadrez

> Plano de execução em fases incrementais, com critérios de saída verificáveis.
>
> Estado base: **5 de setembro de 2026**, commit `0c4a88a`.
> Leia antes: `onboarding-motor.md` (contexto, sem exigir C).
> Detalhe técnico completo: `project_context.md`.

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

| Fase | Nome | Esforço | Portão | Destrava |
|---|---|---|---|---|
| 0 | Fundação de qualidade | 1 sessão | 🚧 | Tudo. Item de maior retorno do roadmap |
| 1 | Vocabulário completo | 1–2 sessões | 🚧 | Roque, en passant, `Undo` |
| 2 | Geometria pré-computada | 1 sessão | | Cavalo e rei |
| 3 | Geração pseudo-legal | 2–3 sessões | | Perft(1) |
| 4 | Aplicar/desfazer lance | 2–3 sessões | 🚧 | Legalidade e busca |
| 5 | Filtro de legalidade | 1–2 sessões | 🚧 | Xeque, mate, afogamento, perft |
| 6a | Protocolo + IA aleatória | 1 sessão | | **A equipe do cliente** |
| 6b | Perft | 2–4 sessões | 🚧 | Autorização para escrever IA |
| 7 | IA incremental (v1→v5) | 3–5 sessões | | Força de jogo |
| 8 | Robustez e empacotamento | 2 sessões | | Entrega |

**Total estimado:** 14–22 sessões de trabalho.

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

### 2.1 Campos faltantes na `Board` → **fechar agora, junto com o `Undo`**

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

### 2.2 Lance em 16 vs 32 bits → **ficar com 32 e encerrar a discussão**

O argumento decisivo não é sobre bits — é que **a abstração que torna isso
reversível já foi construída.** `encode_move` e `decode_move` são os únicos pontos
que tocam o formato, e `MoveDescription` é a forma desempacotada que o resto do
código consome. Enquanto esse encapsulamento se mantiver, trocar o layout é uma
tarde, em qualquer momento do projeto.

O que tornaria a decisão cara seria alguém escrever `(move >> 16) & 0xFF` fora do
`decode_move`. **A disciplina de encapsulamento é o que importa; o número de bits
não.** Vale um comentário no header dizendo isso explicitamente.

**Nota para quando revisitar:** o layout padrão de 16 bits (6+6+4) funciona porque
o campo de flags é uma **enumeração de 16 combinações mutuamente exclusivas**
(quieto, avanço duplo, roque curto, roque longo, captura, captura EP, 4 promoções,
4 promoções-com-captura) — não quatro booleanos independentes. Se os flags
permanecerem como bits independentes, 4 bits não bastam. A migração exige repensar
os flags, não só apertá-los.

**Momento natural de revisitar:** quando a `MoveList` passar a viver na pilha de
recursão da busca (Fase 7). Aí o tamanho vira mensurável. Antes disso, é otimização
especulativa.

### 2.3 Makefile vs. CMake → **migrar na Fase 0**

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

### 2.4 Onde vive `side_to_move` na geração → **laço externo, uma checagem**

É a opção que o documento já identificou como limpa, e está certa. Mas há uma
armadilha específica da codificação de peça do projeto que precisa ser blindada:

```c
/* ERRADO — COLOR_OF(EMPTY) == BLACK nesta codificação.
   Casas vazias entram na geração das pretas. */
if (COLOR_OF(b->array[sq]) != b->side_to_move) continue;

/* CERTO */
if (!is_own(b->array[sq], b->side_to_move)) continue;
```

```c
static inline bool is_own(Piece p, Color c)   { return p != EMPTY && COLOR_OF(p) == c; }
static inline bool is_enemy(Piece p, Color c) { return p != EMPTY && COLOR_OF(p) != c; }
```

**Escreva esses dois helpers antes de escrever qualquer peça nova.** O
`project_context.md` já registra `COLOR_OF(EMPTY) == BLACK` como armadilha
conhecida — o passo que falta é torná-la **inalcançável** em vez de lembrada. Uma
armadilha documentada ainda é uma armadilha; uma armadilha encapsulada não é.

---

## 3. Decisões arquiteturais a somar

### 3.1 Estender a filosofia do `SQ_TO_EDGE` para as saltadoras

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

### 3.2 Duas funções de geração, com `const` diferente

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

### 3.3 Separar `board.c` — mas apenas em dois

```
types.h     →  vocabulário. Só <stdbool.h> e <stdint.h>.
square.h/c  →  sq_from_coord, coord_from_sq, SQ_TO_EDGE, tabelas pré-computadas
fen.h/c     →  parse_fen, board_to_fen
board.h/c   →  Board, print_board, board_check_invariants
movegen.h/c →  (como está)
```

Coordenada não é detalhe de FEN — o `project_context.md` já concluiu isso. O que
falta notar é que **`square.c` é também onde as tabelas de §3.1 pertencem**: são
todas "geometria do tabuleiro", independentes de peça e de posição. Isso dá ao
módulo uma responsabilidade coerente em vez de ser um saco de utilitários.

**Faça na Fase 1**, enquanto já se está mexendo em `board.c` para completar a FEN.
Depois que `movegen.c` triplicar de tamanho, o custo do split sobe.

Sobre `types.h` arrastando seis headers da libc: corte para `stdbool` e `stdint`, e
deixe cada `.c` incluir o que usa. O ganho é o já identificado — o compilador volta
a avisar quando surge acoplamento novo. É um **mecanismo de detecção**, não estética.

### 3.4 `add_move` deve falhar alto, não silenciar

O bug do `>` vs `>=` tem correção óbvia. Mas há uma decisão embutida: **o que fazer
quando a lista enche?**

Hoje o comportamento seria descartar silenciosamente. Numa posição legal isso
**nunca** acontece — o máximo teórico é 218 lances e `GEN_MOVES_MAX` é 256. Logo, se
acontecer, é bug em outro lugar: geração duplicada, laço que não termina,
corrupção. Silenciar transforma um bug detectável num perft levemente errado.

```c
void add_move(u32 move, MoveList *list) {
    assert(list->count < GEN_MOVES_MAX);   /* 218 é o máximo legal — estourar é bug */
    list->moves[list->count++].move = move;
}
```

### 3.5 Hash Zobrist — opcional, com um motivo não-óbvio

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

Build limpo com `-Wall -Wextra -Wpedantic -Wstrict-prototypes -Og`, **zero avisos**,
e o binário roda sob ASan/UBSan sem disparar nada.

---

### Fase 1 — Vocabulário completo 🚧

**Esforço:** 1–2 sessões.

#### O quê

1. `Board` e `Undo` completos (§2.1)
2. `parse_fen` lendo os 6 campos; `board_to_fen` emitindo os 6
3. Split de módulos (§3.3) e corte dos includes de `types.h`
4. `board_check_invariants()` implementada
5. Helpers `is_own` / `is_enemy` (§2.4)
6. Teste de round-trip de FEN sobre um corpus

#### Como

**O corpus de FEN é o entregável real desta fase.** Um arquivo texto com ~30
posições, uma por linha: inicial, Kiwipete, posições com en passant ativo, com
roque parcial (`Kq`, `-`), com contadores não-triviais, com promoção iminente. O
teste lê o arquivo, faz `parse_fen → board_to_fen`, compara string por string.

> Enquanto `board_to_fen` só emitia a posição, o round-trip não provava nada sobre
> roque e en passant. Este é o momento em que ele passa a provar.

**Este item pode ser delegado** a um colega sem C — ver `onboarding-motor.md` §9.1.

**Sobre `board_check_invariants()`** — o que verificar:

- exatamente um rei de cada cor
- `array[king_square[c]]` é de fato o rei da cor `c` — *pega o cache dessincronizado*
- nenhum peão na 1ª ou 8ª fileira
- `ep_square`, se definido, na fileira coerente com `side_to_move`
- direitos de roque consistentes com rei/torre nas casas iniciais
- nenhum código de peça inválido (7, 8, 15)

Chamar no início e no fim de `make_move`/`unmake_move` e depois de `parse_fen`. Em
release ela desaparece. O retorno é desproporcional: praticamente todo bug de
`make/unmake` se manifesta como quebra de invariante **uma ou duas jogadas antes**
de causar sintoma visível — e a diferença entre depurar na causa e depurar no
sintoma, dentro de uma árvore de profundidade 6, é de horas.

**Confira também** se `parse_fen` monta num `Board` local e só copia para o
chamador em caso de sucesso. Uma FEN inválida não pode deixar o motor num estado
meio-inicializado, porque o comando seguinte vai operar sobre lixo.

#### Critério de saída 🚧

Round-trip idêntico nas 30 posições do corpus; `board_check_invariants` passa em
todas.

---

### Fase 2 — Geometria pré-computada

**Esforço:** 1 sessão.

#### O quê

`KNIGHT_TARGETS`, `KING_TARGETS`, `PAWN_ATTACKS` (§3.1), geradas em
`precompute_move_data(void)`.

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
| Soma de `KNIGHT_COUNT` sobre as 64 casas | 336 |
| Rei em a1 | 3 destinos |
| Rei em e4 | 8 destinos |
| Soma de `KING_COUNT` | 420 |

Essas somas são a forma mais barata de verificar uma tabela inteira de uma vez.

#### Critério de saída

As somas batem; `SQ_OFFBOARD` foi deletado.

---

### Fase 3 — Geração pseudo-legal completa

**Esforço:** 2–3 sessões.

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

#### O quê

`make_move` completo: inverter `side_to_move`, atualizar `king_square`, tratar
captura, roque movendo duas peças, en passant limpando a casa correta, promoção
trocando o tipo, `castle_mask`, `halfmove_clock`, `fullmove_number`.
`unmake_move` restaurando via `Undo`.

#### Como

**O teste que define esta fase:**

```
para cada posição do corpus:
    fen_antes = board_to_fen(b)
    para cada lance pseudo-legal m:
        Undo u;
        make_move(b, m, &u)
        unmake_move(b, m, &u)
        assert(board_to_fen(b) == fen_antes)
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

#### Critério de saída 🚧

Round-trip de make/unmake verde nas 30 posições, com invariantes passando.

---

### Fase 5 — Filtro de legalidade 🚧

**Esforço:** 1–2 sessões.

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

#### O quê

Traduzir "robusto" em invariantes verificáveis — que viram critério de aceite dos
requisitos não funcionais:

| Propriedade | Como garantir | Como verificar |
|---|---|---|
| Nunca crasha | ASan/UBSan em todo build de teste | perft profundo sob sanitizers |
| Nunca devolve lance ilegal | assert `bestmove ∈ generate_legal` | o próprio assert, **também em release** |
| Nunca trava | `setvbuf` no `main`; `go` sempre responde | testar via pipe, não só no terminal |
| Entrada malformada não corrompe | `parse_fen` monta em local e só copia no sucesso | fuzz: 10k strings aleatórias |
| Comando desconhecido é ignorado | dispatch com fallback silencioso | mandar lixo pelo stdin |
| Reprodutível | stateless entre comandos; sem RNG não-semeado | mesma FEN + profundidade → mesmo lance |

**O fuzz do parser de FEN** é ~15 linhas e desproporcionalmente valioso: gere
strings aleatórias e mutações do corpus (trocar um caractere, cortar no meio,
duplicar uma `/`), chame `parse_fen`, verifique que ou retorna `false` ou produz um
`Board` que passa nos invariantes. Sob ASan, encontra estouros que teste manual não
encontra. **Delegável a um colega sem C** — ver `onboarding-motor.md` §9.4.

---

## 5. Ordem de sacrifício se o cronograma apertar

Do menos doloroso ao mais:

1. **Zobrist / transposition table** — puro desempenho, e repetição tripla é regra rara
2. **v6+ da IA** (PSQT, tapered eval) — força de jogo, não correção
3. **Split de módulos** — dívida técnica, não bloqueia nada
4. **v4 (ordenação de lances)** — perde profundidade, não correção
5. **CMake** — se o Makefile com as flags certas estiver de pé, ele serve

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

*Documento vivo. Ao concluir uma fase, marque o critério de saída como atingido e
atualize o `project_context.md` §2 (estado atual) e §5 (bugs abertos).*
