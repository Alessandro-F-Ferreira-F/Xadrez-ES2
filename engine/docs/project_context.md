# Contexto do Projeto — Chess Engine em C

> Documento de contexto para retomar o projeto ou apresentá-lo a quem for trabalhar nele,
> sem precisar ler a codebase inteira.
> Estado em **16 de setembro de 2026**, revisão pós-`make_move` (commit `f79d50a`,
> "board_check_invariants(); geração dos movimentos do rei; aprimoramento da função
> make_move()", **mais as correções ainda não commitadas na árvore de trabalho**).
>
> **Esta revisão é a primeira em que o núcleo do motor faz algo verificável.** As duas
> anteriores descreviam estrutura pronta e comportamento ausente. Agora `make_move` aplica
> roque, en passant, promoção, relógios e direitos de roque **corretamente** — verificado
> com evidência (§2), não por leitura. O que ficou para trás é o espelho: `unmake_move`
> não tem corpo nenhum.
>
> `docs/next_steps.md` foi reescrito junto com este documento e está sincronizado.
> `docs/bugs.txt` é a revisão anterior (contra o commit `f79d50a` puro) e está **obsoleto**:
> cinco dos seis bugs que ele lista foram corrigidos na árvore de trabalho — ver §5.

---

## 1. O que é este repositório

Um **motor de xadrez em C**, escrito do zero como projeto de aprendizado — e o repositório
oficial do grupo na disciplina de Engenharia de Software 2. O objetivo declarado não é força
de jogo: é entender como uma engine funciona por dentro, e fazê-la funcionar em equipe.

A fronteira do repositório é estreita e deliberada: **um binário que fala um protocolo texto
por stdin/stdout.** Nada além disso vive aqui.

- O **cliente** é de outra equipe, em `interface/` (C++/SFML).
- Este repo entrega um executável que recebe comandos por linha e responde por linha —
  hoje, na prática, um menu interativo (`ui()`), não ainda o protocolo real.

A fase atual é **protótipo/demo**: um binário que joga xadrez legalmente do início ao fim,
com uma IA qualquer. Correção das regras vem primeiro; força de jogo vem por último, e só se
sobrar tempo. Bitboards, transposition table e magic bitboards estão explicitamente fora de
escopo.

---

## 2. Estado atual, honestamente

O projeto tem **~1990 linhas** em **11 módulos** (era ~1640 em 10 na revisão de 13/09). O
crescimento é quase todo em `makemove.c` (21 → 129 linhas), `movegen.c` (119 → 235) e
`board.c` (56 → 138) — ou seja, comportamento, não estrutura. A estrutura já estava pronta.

**O resumo desta revisão: `make_move` está completo e verificado; `unmake_move` não existe;
o cavalo não é gerado.** O build compila com **8 avisos** e zero erros.

### Funciona e está verificado com evidência

Tudo nesta tabela foi reproduzido nesta revisão — compilando, rodando sob ASan/UBSan, e
comparando a FEN de saída contra o resultado esperado calculado à mão.

| Área | Estado |
|---|---|
| `fen_parse` / `fen_write` | Os seis campos, validação completa, round-trip idêntico. Módulo maduro, sem mudança de comportamento desde 07/09. Ver `docs/fen.md` |
| `fen_parse` rejeita posição sem rei | Verificado: `"rnbq1bnr/…/RNBQ1BNR w - - 0 1"` é rejeitada com `board must have exactly one king of each color` (`fen.c:291`). É a fronteira protegendo o núcleo, exatamente como a arquitetura prevê |
| **`make_move`** | **Completo.** Undo preenchido, captura, roque movendo as duas peças, en passant limpando a casa certa, promoção nas quatro peças, `castling_rights` por origem **e** destino, `ep_square`, `halfmove_clock`, `fullmove_number`, `king_square`. Ver a bateria de verificação abaixo |
| **`board_check_invariants`** | **Implementada** (`board.c:42`). Código de peça válido, exatamente um rei de cada cor, cache `king_square` batendo com varredura do zero, nenhum peão na 1ª/8ª fileira. Passa nas posições testadas. Cobre 4 das 6 checagens de `docs/guides.md` — ver Bug #6 |
| `board_find_king` | Implementada (`board.c:18`), varredura simples, `SQ_NONE` se não achar |
| Geração do rei | 8 direções via `SQ_TO_EDGE`, como decidido em 10/09 — sem tabela `KING_TARGETS` |
| Geração de roque | `check_allowed_castles` / `check_castle_side` — direito presente **e** casas entre rei e torre vazias. Verificado nos dois lados |
| Peão completo | Push, push duplo aninhado corretamente, captura, promoção nas 4 peças (com e sem captura), en passant. O parâmetro `side` saiu — lê `b->side_to_move` |
| `generate_all_moves` | Entrada única (`movegen.c:230`): limpa a lista e chama os três geradores |
| REPL de `main.c` | Consertado: usa o índice devolvido por `movelist_find` e aplica o lance real. Os Bugs #1/#2 da revisão anterior estão fechados |
| `move_is_castle` / `move_is_ep_capture` | Comparam por igualdade, não por máscara de bit. O bug crítico de `bugs.txt` está fechado |
| Binário sob ASan/UBSan | 98 chamadas de `make_move` em 4 posições, **zero disparos** |

**A bateria que prova o `make_move`** — cada linha é a FEN devolvida por `fen_write` depois
do lance, conferida contra o resultado correto:

```
roque O-O branco     e1g1  -> r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1
roque O-O-O branco   e1c1  -> r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1
en passant           e5f6  -> rnbqkbnr/ppp1p1pp/5P2/3p4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3
promoção a dama      e7e8q -> rn2Q2k/1P6/8/8/8/8/8/7K b - - 0 1
promoção com captura b7a8q -> Qn5k/4P3/8/8/8/8/8/7K b - - 0 1
push duplo           e2e4  -> rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1
torre captura torre  h1h8  -> r3k2R/8/8/8/8/8/8/R3K3 b Qq - 0 1
```

A última linha é a que mais vale: `h1h8` derruba `K` (a torre branca saiu de h1) **e** `k`
(a torre preta em h8 foi capturada sem se mover). É a metade sutil da armadilha de roque do
`next_steps.md` §B4, e ela está certa.

### Incompleto ou ausente

| Área | Estado |
|---|---|
| **`unmake_move`** | **Sem corpo.** `makemove.c:130` é a declaração repetida, terminada em `;`. Chamar a função é **erro de link** — reproduzido: `undefined reference to 'unmake_move'`. É o item que bloqueia tudo o que vem depois |
| **Cavalo** | Não gerado. `KNIGHT_TARGETS`/`KING_TARGETS` continuam alocadas e **zeradas**; `init_square_tables()` só preenche `SQ_TO_EDGE` e `PAWN_ATTACKS`. Medido: a posição inicial gera **16** lances, não 20 — os 4 que faltam são os do cavalo. Kiwipete gera **37**, não 48 |
| `is_square_attacked`, filtro de legalidade | Não existem. Todo lance é pseudo-legal |
| Perft | Não existe |
| Protocolo UCI | Não existe — só o menu `ui()` |
| Testes automatizados | **Não existem.** `test/test.c` é um segundo REPL manual: lê lances do stdin, imprime o tabuleiro, **nenhuma assertion**. `make test` compila, mas não é portão de nada |
| Corpus de FEN | Não existe como arquivo. Há cinco `#define TEST_FEN_*` em `main.c` e quatro em `test/test.c`, espalhados |

---

## 3. Decisões arquiteturais firmadas

Escolhas conscientes com trade-off avaliado. Mudá-las agora custa caro.

### Representação: mailbox de 64 casas

`Piece array[64]`, não bitboards. `array[64]` = 64 bytes = uma linha de cache; varredura
completa medida em ~9,5 ns contra ~12,3 ns de uma piece list. Sem mudança.

### Indexação: `a1 = 0` (Little-Endian Rank-File)

`sq = rank * 8 + file`; `h8 = 63`. `PAWN_PUSH[WHITE] = +8`, `PAWN_PUSH[BLACK] = -8`, com os
índices na ordem certa desde `04fdba4`. Sem mudança.

### Codificação da peça

`piece.h`: `PIECE_MAKE(color, type) = (color << 3) | type`, com `WHITE = 1`, `BLACK = 0`,
`EMPTY = 0`. `is_own`/`is_enemy` moram aqui como cópia única e são usados por `movegen.c` e
`board.c`. As duas armadilhas continuam valendo: `PIECE_COLOR(EMPTY) == BLACK`, e comparar o
valor cru contra um `PieceType` funciona por acidente para as pretas e falha para as brancas.

### Codificação do lance: `u16` de 6+6+4 bits

```
       tipo (4)   destino (6)   origem (6)
u16 = [0000]      [000000]      [000000]
       <<12          <<6           <<0
```

Esquema de 4 bits da Chess Programming Wiki ([Encoding Moves](https://www.chessprogramming.org/Encoding_Moves)):
bit 3 (`& 8`) é promoção, bit 2 (`& 4`) é captura, e os dois bits baixos, quando há promoção,
indexam a peça (`KNIGHT + (type & 3)`).

**Lição desta revisão, já paga uma vez:** `MV_CASTLE_KING = 2`, `MV_CASTLE_QUEEN = 3` e
`MV_EP_CAPTURE = 5` **não são bits isolados** — são códigos. Só captura e promoção são bits.
A versão de `move_is_castle`/`move_is_ep_capture` que testava por `&` classificava toda
promoção a dama como roque e toda captura comum como en passant, e `make_move` agia sobre
isso. Hoje as duas comparam por igualdade (`move.c:32-44`). Quem for adicionar um predicado
novo sobre `MoveType`: **teste por `&` só nos dois bits que são bits.**

### Direções: tabela de distância até a borda

`SQ_TO_EDGE[64][8]`, ordem do enum como carga estrutural (ortogonais 0–3, diagonais 4–7),
para que torre, bispo e rainha usem uma única função parametrizada por intervalo. Vive em
`square.h/c`. O rei usa a mesma tabela com o laço parando em 1 — é o mesmo mecanismo, não um
segundo, que era a preocupação legítima do `roadmap-motor.md` §3.1.

### Pseudo-legal primeiro, legalidade depois

Sem mudança de decisão. O gerador de roque materializa isso de propósito: `check_castle_side`
verifica direito e casas vazias, e **não** verifica se o rei está em xeque, atravessa casa
atacada ou chega em casa atacada. Os três dependem de `is_square_attacked` e pertencem ao
filtro. Isso está documentado aqui porque é exatamente o tipo de divisão que alguém
reimplementa metade nos dois lugares.

### `Undo` é do chamador — assinatura pronta, metade implementada

```c
typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);     /* implementado e verificado */
void unmake_move(Board *b, Move move, const Undo *u);  /* SEM CORPO */
```

A decisão de 10/09 continua valendo inteira (a pilha da recursão é a pilha de undo; não
existe pilha global; o compilador verifica o pareamento). `make_move` a cumpre: preenche
`*u` **antes** de tocar no tabuleiro, que era o ponto crítico da ordem. Falta o espelho.

**Uma assimetria que o `Undo` atual não resolve, e que quem escrever `unmake_move` vai
encontrar:** no en passant, `u->captured` fica `EMPTY`, porque a casa de destino estava
vazia. O peão capturado não está em `to`, está em `to - PAWN_PUSH[side]`. O `unmake` não
pode restaurá-lo a partir do `Undo`; tem de **deduzir** que é um peão da cor adversária,
porque `MV_EP_CAPTURE` já diz isso. Não é defeito do `Undo` — é o motivo pelo qual o campo
`captured` sozinho não basta e o lance precisa voltar como parâmetro.

### Contrato de validação: fronteira valida, núcleo confia

Sem mudança. `make_move` é primitiva, não serviço: não valida o lance, porque o filtro de
legalidade precisa aplicar lances possivelmente ilegais. Isso agora tem consequência
concreta e reproduzida — ver Bug #1.

### `board_check_invariants` mudou de assinatura — decidido nesta janela

Planejado: `bool board_check_invariants(const Board *b, const char **fail_msgs)`.
Implementado: `bool board_check_invariants(const Board *b)`, com as falhas acumuladas num log
global em `utils.c` (`fail_msg` / `print_fail_log`).

A troca é defensável — o chamador quase sempre só quer o `bool`, e um out-param que quase
ninguém lê é peso morto na assinatura. Mas ela **importou um global mutável** para dentro de
um módulo que era puro, e o `next_steps.md` §9.1 tem uma regra sobre isso. As duas
consequências práticas estão no Bug #7.

### Testes como subcomando do binário

**Ainda não implementado**, e agora é a lacuna mais cara do projeto. A decisão de 10/09
(`./main.out test`, `./main.out perft N`) continua de pé. O que existe é `test/test.c` +
alvo `make test`, mas é um segundo REPL manual sem assertions — não é portão.

Todo item verificado nesta revisão foi verificado por harness descartável em `/tmp`. Isso
funciona uma vez e some. A mesma verificação, dentro do repositório e rodando no `make`,
seria regressão permanente — e é literalmente o mesmo código.

### Sistema de build: Makefile

CMake fica para quando o cliente precisar integrar. O item 1 da Fase 0 do roadmap
(`CMakeLists.txt`) está **conscientemente descartado por ora**, não pendente.

---

## 4. Mapa dos módulos

| Arquivo | Papel | Linhas (.h/.c) | Nota |
|---|---|---|---|
| `types.h` | Typedefs de largura fixa, `MAX_*`, `MIN`/`MAX` | 57 / — | Ainda arrasta `ctype.h stdio.h stdlib.h string.h`. `PrintSize` e `FILE_DIST` nunca são usados; `MemoryZeroStruct` tem um uso só (`main.c:74`) |
| `log.h/c` | `LOG_ERROR` (sempre) / `LOG_DEBUG` (sob `DEBUG`) via `log_emit` | 20 / 20 | Sem mudança, continua correto |
| `piece.h/c` | `Color`, `PieceType`, `Piece`, macros, `is_own`/`is_enemy`, char ↔ peça | 45 / 27 | Sem mudança |
| `square.h/c` | Coordenadas, `SQ_TO_EDGE`, `PAWN_ATTACKS`, `DIR_OFFSET`, `PAWN_PUSH` | 47 / 101 | `KNIGHT_TARGETS`/`KING_TARGETS` alocadas e zeradas (Bug #2). `DIR_CHARMAP` não usado (Bug #9). Ganhou `SQ_C1/G1/C8/G8` para o roque |
| `board.h/c` | `Board`, `CastleRights`, `board_find_king`, `board_check_invariants`, `fill_sq`, `is_empty`, impressão | 41 / 138 | **Onde está o maior ganho desta revisão.** Bugs #6 e #7 |
| `fen.h/c` | `fen_parse`, `fen_write` | 12 / 612 | Módulo maduro, sem mudança. Maior arquivo do projeto. `castling_matches_board` (`fen.c:385`) é `static` e é exatamente a checagem que falta em `board_check_invariants` |
| `move.h/c` | `Move`, `MoveType`, `MoveList`, encode/decode, predicados, `movelist_*`, str ↔ lance | 58 / 176 | Predicados de roque/EP corrigidos. Bugs #5, #8 |
| `makemove.h/c` | `Undo`, `make_move`, `unmake_move` | 18 / 129 | `make_move` completo e verificado; `unmake_move` sem corpo (Bug #1). Bugs #3, #4 |
| `movegen.h/c` | Peão, deslizantes, rei, roque, `generate_all_moves` | 12 / 235 | Falta o cavalo (Bug #2). Bugs #10, #11 |
| `utils.h/c` | Impressão de debug, leitura de stdin, `strslc`, `fail_msg`/`print_fail_log` | 19 / 107 | Bug #7 |
| `main.c` | Menu interativo `ui()` + `main()` | — / 113 | REPL consertado. FEN de partida é um `#define` hardcoded |
| `test/test.c` | Segundo REPL manual | — / 75 | Sem assertions. Não é teste |

**Problemas estruturais que atravessam módulos:**

- **Cinco funções com linkage externo que só são usadas dentro do próprio arquivo e
  deveriam ser `static`** — as cinco são exatamente os avisos de `-Wmissing-prototypes`:
  `ui` (`main.c:17`), `print_move` (`move.c:162`), `check_pawn_promotion` (`movegen.c:7`),
  `genenare_moves_from_direction` (`movegen.c:79`), `check_allowed_castles` (`movegen.c:153`).
- **Duas formas de achar o rei no mesmo arquivo:** `generate_king_moves` lê
  `b->king_square[side]` (o cache) e `check_allowed_castles` chama `board_find_king` (a
  varredura) — a 50 linhas de distância, para a mesma pergunta. Mecanismo duplicado tem duas
  formas de estar errado.
- O typo `genenare_moves_from_direction` (por `generate_`) continua, documentado desde 10/09.
- `generate_all_moves(Board *b, …)` recebe `Board *` não-const enquanto os três geradores que
  ela chama recebem `const Board *`. O `const` certo é o dos três.

---

## 5. Bugs abertos

Cada item foi reproduzido nesta revisão — aviso de compilação, ASan/UBSan, erro de link, ou
saída medida — não inferido por leitura. `docs/bugs.txt` é a lista da revisão anterior e
está **obsoleto**: dos seis itens críticos de lá, cinco foram corrigidos na árvore de
trabalho (predicados de roque/EP por igualdade; `SQ_H8` derrubando `CASTLE_BK`; o `return
true` que faltava; `piece_code_is_valid` com `&&`; `update_castling_rights` cobrindo `to`).
O sexto — `unmake_move` sem corpo — é o Bug #1 abaixo.

### Bloqueantes

| # | Onde | Problema |
|---|---|---|
| 1 | `makemove.c:130` | **`unmake_move` não tem corpo.** A linha é a declaração repetida, terminada em `;`. Reproduzido: `undefined reference to 'unmake_move'` no link. Enquanto isso durar não há filtro de legalidade, não há perft, não há busca — e **não há o teste de round-trip que é o critério de saída da Fase 4** |
| 2 | `square.c:71`, `movegen.c:230` | **Cavalo não é gerado.** `init_square_tables()` não popula `KNIGHT_TARGETS`, e `generate_all_moves` não tem um `generate_knight_moves`. Medido: posição inicial gera **16** lances (correto: 20); Kiwipete gera **37** (correto: 48) |

### Graves

| # | Onde | Problema |
|---|---|---|
| 3 | `movegen.c:191` | **Leitura fora dos limites, confirmada pelo ASan.** Se `b->king_square[side] == SQ_NONE`, `SQ_TO_EDGE[-1][dir]` é global-buffer-overflow. Reproduzido: em `R6k/8/8/8/8/8/8/K7 w - - 0 1` o gerador emite `a8h8` (captura do rei — pseudo-legal, e é *o* caso que o filtro de legalidade vai produzir aos milhares), `make_move` grava `king_square[BLACK] = -1`, e o `generate_all_moves` seguinte estoura. Hoje o sintoma é silencioso porque `KNIGHT_TARGETS` (o vizinho na memória) está zerada — o `continue` da linha 191 salva por acidente. Deixa de salvar assim que o Bug #2 for fechado |
| 4 | `makemove.c:72-73` | **`make_move` recalcula os dois reis por varredura completa a cada lance** — 128 leituras por nó de busca, e o campo `king_square` existe justamente para não fazer isso. Correto é atualizar só quando a peça movida é rei (incluindo o roque). Não é bug de correção; é bug de propósito — o cache deixou de ser cache |
| 5 | `move.c:112` | `promotion_type = EMPTY;` atribui um `PieceType` a uma variável `MoveType`. Funciona por acidente (`EMPTY == 0 == MV_QUIET`), e o compilador acusa: `-Wenum-conversion`. Dois enums distintos cujo zero coincide é exatamente a classe de confusão que o `enum Promotion` antigo causou |
| 6 | `board.c:42` | `board_check_invariants` cobre **4 das 6** checagens de `docs/guides.md`. Faltam: (a) `ep_square`, se definido, na fileira coerente com `side_to_move`; (b) direitos de roque coerentes com rei e torres nas casas de origem. A (b) **já está escrita**: `castling_matches_board` em `fen.c:385`, hoje `static`. Expor é a maior parte do trabalho |
| 7 | `utils.c:14-28` | O `FailLog` de `fail_msg` **nunca é zerado** — não há função de reset. Num round-trip de ~1000 pares make/unmake ele bate no teto de 128 e passa a emitir `LOG_ERROR("too many fail messages")` por cima do teste. E a assinatura é `fail_msg(char *)`, não `const char *`: são **6 avisos de `-Wwrite-strings`**, todos em `board.c`, todos de passar literal. Correção de uma palavra |

### Menores / higiene

| # | Onde | Problema |
|---|---|---|
| 8 | `move.c:13` | `CASTLE_TYPE_MASK` definido e nunca usado — sobra da versão dos predicados por máscara de bit, que era o bug crítico. Apagar fecha a porta de volta |
| 9 | `square.c:8` | `DIR_CHARMAP` definido e nunca usado (`-Wunused-const-variable`) |
| 10 | `movegen.c:159` | `static const enum {LEFT, RIGHT};` — declaração vazia com especificador de classe de armazenamento. O GCC acusa (`useless storage class specifier in empty declaration`). Funciona, mas o que se quis dizer é `enum { LEFT, RIGHT };` |
| 11 | `movegen.c:56` | Dentro do laço de captura, `piece` (a variável que guarda o **peão de origem**) é reaproveitada para guardar a peça do **destino**. Correto por sorte de ordem de uso; ilegível por construção |
| 12 | `makemove.c:8` | `check_rook_squares` também trata `SQ_E1`/`SQ_E8`, que são casas de **rei**. O nome mente. E aplicá-la ao destino faz o caso `E1`/`E8` derrubar os direitos de um lado quando algo se move *para* a casa do rei — hoje inalcançável (com direito vivo, o rei está lá), mas é um raciocínio que precisa ser refeito toda vez que alguém lê a função |
| 13 | `makemove.c:87` | `fill_sq(b, "d1", …)` coloca a torre do roque **por parsing de string**, dentro do caminho mais quente do motor, e descarta o `bool` de retorno — contra a convenção do `CLAUDE.md` §7 ("funções que podem falhar devolvem status"). `b->array[SQ_D1] = PIECE_MAKE(WHITE, ROOK)` é mais barato e não tem caminho de falha |
| 14 | `move.c:125` | `movelist_add` loga e descarta quando a lista enche. O acordo (`next_steps.md` A5) era `assert`: 218 é o máximo legal numa posição real, contra teto de 256 — se encheu, **é bug em outro lugar**, e descartar em silêncio o converte num perft levemente errado |

### Sobre o Makefile e as flags

Build atual: **8 avisos**, zero erros. Cinco são `-Wmissing-prototypes` (§4), e os outros
três são os Bugs #5, #9 e #10. Ou seja: **fechar os oito avisos é fechar três bugs reais e
tomar cinco decisões de `static` que hoje estão sendo tomadas por omissão.**

Medido nesta revisão, contra a árvore atual, as quatro flags que o `CLAUDE.md` §7 pede e o
Makefile ainda não liga:

| Flag | Custo | O que pega |
|---|---|---|
| `-Wshadow` | **0** | De graça |
| `-Wcast-qual` | **0** | De graça |
| `-Wwrite-strings` | +6 | **Todos** o Bug #7 — `fail_msg(char *)` recebendo literal. Uma palavra fecha os seis |
| `-Wconversion` | +8 | **Todos** benignos desta vez: `*castling_rights &= ~(CASTLE_*)` em `u8` (`makemove.c:12-27`). Adotar exige um cast ou um helper `u8` |

Recomendação: ligar `-Wshadow`, `-Wcast-qual` e `-Wwrite-strings` **já** (custo real: uma
palavra). `-Wconversion` só depois de decidir como escrever o `&= ~` sem ruído — ela é a
flag que mais pagou neste projeto historicamente, e não vale adotá-la para depois
silenciá-la.

---

## 6. Decisões em aberto

| Decisão | Situação |
|---|---|
| **Sistema de build** | Makefile por enquanto; CMake quando o cliente precisar integrar. Item 1 da Fase 0 do roadmap: **descartado por ora**, não pendente |
| **`Move` sem score** | Confirmado. Entra quando começar a IA, em outra estrutura |
| **Camada de integração cliente ↔ motor** | **Ainda em aberto, e bloqueia a Fase 6a.** `arquitetura-xadrez.md` diz que o cliente nunca fala direto com o motor; o cliente C++ em `interface/` roda o motor como subprocesso. Decide se o protocolo precisa de um comando `legalmoves`. Conversa de 15 minutos com a equipe do cliente, e ela tem de acontecer antes de escrever o protocolo |
| **Onde mora o corpus de FEN** | Arquivo texto lido em runtime, ou `const char *[]` num `.c`? O `next_steps.md` §B6 recomendou o `.c` (sem I/O, sem caminho relativo, sem caso de erro). Hoje as FENs estão como `#define` espalhados por `main.c` e `test/test.c` |

---

## 7. Próximos passos, em ordem

Detalhe completo, com armadilhas e critérios, em `docs/next_steps.md`. O resumo:

1. **`unmake_move` (Bug #1).** Espelho de `make_move`, em ordem reversa, invertendo
   `side_to_move` primeiro. É pré-requisito de tudo.
2. **O teste de round-trip make/unmake.** `./main.out test`, corpus de ~30 FENs, ~1000
   verificações, milissegundos. É o critério de saída da Fase 4 do roadmap, e o portão que
   teria pego sozinho quase todo bug desta revisão.
3. **Cavalo (Bug #2)** — popular `KNIGHT_TARGETS` no init e escrever `generate_knight_moves`.
   Depois disso, `perft(1)` da posição inicial deve dar 20.
4. **Fechar os 8 avisos do build** (três bugs reais) e ligar as três flags de custo zero-ou-um.
5. **`board_check_invariants` completa (Bug #6)** — expor `castling_matches_board`, somar a
   checagem de `ep_square`.
6. Só então: `is_square_attacked` e o filtro de legalidade; perft; protocolo.

O item 2 é o de maior retorno da lista, e a razão é medível: **toda verificação desta revisão
foi feita por harness descartável em `/tmp`.** Foi o suficiente para provar que `make_move`
está certo hoje. Não prova nada sobre amanhã, e não roda no `make` de ninguém.

---

## 8. Referências ativas

- **Chess Programming Wiki**: [Make Move](https://www.chessprogramming.org/Make_Move) e
  [Unmake Move](https://www.chessprogramming.org/Unmake_Move) (a etapa imediata),
  [Knight Pattern](https://www.chessprogramming.org/Knight_Pattern),
  [Encoding Moves](https://www.chessprogramming.org/Encoding_Moves),
  [Castling Rights](https://www.chessprogramming.org/Castling_Rights),
  [Square Attacked By](https://www.chessprogramming.org/Square_Attacked_By),
  [Perft](https://www.chessprogramming.org/Perft) e
  [Perft Results](https://www.chessprogramming.org/Perft_Results).
- **TSCP** — `makemove`/`takeback` em mailbox legível. Vale ler agora que `unmake_move` é a
  tarefa; note que ele usa o desenho **oposto** ao nosso (pilha global `hist_dat`).
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas, ler inteira antes do protocolo.
- `man gcc`, seção *Options to Request or Suppress Warnings*.

**Pendência de documentação:** o cabeçalho de `docs/fen.md` ainda diz "Documento de
referência do módulo `src/board.c`" — o módulo se chama `fen.c` desde o split de 12/09.
Correção de uma linha. E `docs/bugs.txt` deveria ser apagado ou marcado como obsoleto: cinco
dos seis itens dele já foram corrigidos, e uma lista de bugs desatualizada é pior que
nenhuma.

---

## Apêndice — referência rápida dos módulos

Typedefs, enums, macros e assinaturas, na ordem de dependência. Comentários `/* … */` ao lado
de uma linha apontam para o bug correspondente no §5. Includes, guards e linhas em branco
omitidos. Reflete a **árvore de trabalho em 16/09** (commit `f79d50a` + alterações não
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
#define MAX_SEARCH_PLY 64      /* profundidade de busca */
#define MAX_GAME_PLY   1024    /* plies de uma partida */
#define NUM_COLORS     2

#define MemoryZero(addr, size)   memset((addr), 0x0, (size))
#define MemoryZeroStruct(a, st)  MemoryZero((a), sizeof(st))
#define PrintSize(type)          /* nunca usado */
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
```

Ainda inclui `ctype.h stdio.h stdlib.h string.h`.

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

static inline bool is_own(Piece p, Color c)   { return p != EMPTY && PIECE_COLOR(p) == c; }
static inline bool is_enemy(Piece p, Color c) { return p != EMPTY && PIECE_COLOR(p) != c; }

char  piece_to_char(Piece p);
Piece piece_from_char(char c);
```

### `square.h` — geometria do tabuleiro

```c
#define SQ_NONE (-1)
#define RANK_OF(sq)       ((sq) / BOARD_WIDTH)
#define FILE_OF(sq)       ((sq) % BOARD_WIDTH)
#define SQ_AT(rank, file) ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq)   (((sq) < 0) || ((sq) >= BOARD_SIZE))

enum { SQ_A1 = 0, SQ_E1 = 4, SQ_H1 = 7, SQ_A8 = 56, SQ_E8 = 60, SQ_H8 = 63,
       SQ_C1 = 2, SQ_G1 = 6, SQ_C8 = 58, SQ_G8 = 62 };   /* D1/F1/D8/F8 faltam — Bug #13 */

typedef enum { DIR_N, DIR_S, DIR_E, DIR_W,
               DIR_NE, DIR_SW, DIR_SE, DIR_NW, NUM_DIRS } Direction;

extern const int DIR_OFFSET[NUM_DIRS];              /* {+8,-8,+1,-1,+9,-9,-7,+7} */
extern const int PAWN_PUSH[NUM_COLORS];             /* [BLACK]=-8 [WHITE]=+8 */
extern int SQ_TO_EDGE[BOARD_SIZE][NUM_DIRS];
extern int KNIGHT_TARGETS[BOARD_SIZE][8];           /* alocada, ZERADA — Bug #2 */
extern int KING_TARGETS[BOARD_SIZE][8];             /* alocada, ZERADA e sem uso previsto */
extern int PAWN_ATTACKS[NUM_COLORS][BOARD_SIZE][2];

void init_square_tables(void);                      /* so SQ_TO_EDGE + PAWN_ATTACKS */
int  sq_from_coord(const char *coord);
void sq_to_coord(int sq, char out[3]);
```

`KING_TARGETS` está alocada mas a decisão de 10/09 foi **não usá-la** (o rei usa
`SQ_TO_EDGE`), e `generate_king_moves` de fato não a usa. É alocação órfã: apagar.

### `board.h` — o tabuleiro

```c
enum CastleRights {
    CASTLE_NONE = 0,
    CASTLE_WK = 1, CASTLE_WQ = 2, CASTLE_BK = 4, CASTLE_BQ = 8,
    CASTLE_WHITE = CASTLE_WK | CASTLE_WQ,
    CASTLE_BLACK = CASTLE_BK | CASTLE_BQ,
    CASTLE_ALL   = CASTLE_WHITE | CASTLE_BLACK
};

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int   king_square[2];    /* cache derivado */
    u8    castling_rights;   /* bitmask CASTLE_* */
    int   ep_square;         /* SQ_NONE se nao houver */
    int   halfmove_clock;
    int   fullmove_number;
} Board;

int  board_find_king(const Board *b, Color c);
bool board_check_invariants(const Board *b);     /* 4 das 6 checagens — Bug #6 */
void board_clear(Board *b);
void board_print(const Board *board);
bool fill_sq(Board *b, const char *sq_str, Piece p);
bool is_empty(const Board *b, int sq);
```

O typo `enum ClastleRights` foi corrigido. `board_clear` faz `memset` e depois restaura
`ep_square`, `king_square[2]` e `fullmove_number` — note que `side_to_move` fica `0`, que é
`BLACK`; quem usa `board_clear` sem `fen_parse` em seguida precisa saber disso.

### `fen.h` — a interface inteira de um módulo de 612 linhas

```c
bool fen_parse(const char *fen_string, Board *out);
void fen_write(const Board *board, char fen_out[MAX_FEN_STRING]);
```

Todo o resto de `fen.c` é `static`. **`castling_matches_board` (`fen.c:385`) é a função que
o Bug #6 precisa** — a mesma verificação que `fen_parse` faz na entrada, aplicada também na
saída de `make_move`, é o que fecha o ciclo que a decisão "FEN inconsistente é rejeitada,
não corrigida" abriu de propósito. Ver `docs/fen.md`.

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

Move     encode_move(int from, int to, MoveType type);
int      move_from(Move m);
int      move_to(Move m);
MoveType move_type(Move m);

bool      move_is_capture(Move m);      /* & 4  — bit de verdade  */
bool      move_is_promotion(Move m);    /* & 8  — bit de verdade  */
bool      move_is_castle(Move m);       /* IGUALDADE, nao mascara */
bool      move_is_ep_capture(Move m);   /* IGUALDADE, nao mascara */
PieceType move_promo_type(Move m);      /* KNIGHT + (type & 3)    */

void movelist_clear(MoveList *l);
void movelist_add(MoveList *l, Move m);          /* loga e descarta — Bug #14 */
int  movelist_find(MoveList *l, const char *uci); /* devolve INDICE, -1 se nao achar */

void move_to_str(Move m, char out[6]);
Move move_from_str(const char *in);              /* promocao em in[4] */
void print_moves(MoveList *list);
```

`movelist_find` casa por origem, destino **e** `move_promo_type` — é o que faz `e7e8q` achar
a promoção certa entre as quatro, e o que faz `e7d8q` (promoção com captura) casar mesmo
tendo `MoveType` diferente do que `move_from_str` produziu.

### `makemove.h` — aplicar e desfazer

```c
typedef struct {
    Piece captured;
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);            /* completo, verificado */
void unmake_move(Board *b, Move move, const Undo *u);   /* SEM CORPO — Bug #1 */
```

Ordem interna de `make_move`, como implementada e verificada: decodifica → **salva o `Undo`
antes de tocar no tabuleiro** → move a peça e inverte o lado → `ep_square` → `king_square` →
direitos de roque (origem e destino) → torre do roque → peão do en passant → promoção →
`halfmove_clock` → `fullmove_number`.

### `movegen.h` — geração de lances

```c
void generate_pawn_moves   (const Board *board, MoveList *list);
void generate_sliding_moves(const Board *board, MoveList *list);
void generate_king_moves   (const Board *b, MoveList *list);
void generate_all_moves    (Board *b, MoveList *list);   /* const errado — §4 */
```

Falta `generate_knight_moves`. Não há `generate_legal` — todo lance aqui é pseudo-legal.

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
void strslc(const char *src, char *dest, int start, int end);
void clear_screen(void);
int  get_int(const char *msg);
void fail_msg(char *msg);           /* deveria ser const char * — Bug #7 */
void print_fail_log(void);          /* e falta um reset — Bug #7 */
```

---

*Este documento é escrito à mão. A próxima revisão deveria ser disparada por um evento
concreto — `unmake_move` fechado com o round-trip verde, ou `perft(1)` batendo 20 — não por
passagem de tempo.*
