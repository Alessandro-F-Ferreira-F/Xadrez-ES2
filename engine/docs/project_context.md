# Contexto do Projeto — Chess Engine em C

> Documento de contexto para retomar o projeto ou apresentá-lo a quem for trabalhar nele,
> sem precisar ler a codebase inteira.
> Estado em **4 de outubro de 2026**: commit `1582162` ("geração de movimentos legais
> corrigida"). A única alteração não commitada na árvore é uma linha de cabeçalho em
> `docs/perft_results.txt`.
>
> **Esta revisão é a primeira em que o motor tem filtro de legalidade e perft próprios — e
> os dois batem nas seis posições de referência da Chess Programming Wiki**, inclusive a
> posição inicial na profundidade 7 (3 195 901 860 nós). Verificado com evidência (§2), não
> por leitura. As regras do xadrez, do ponto de vista de "quais lances existem", estão
> prontas. O que falta para a demo deixou de ser regras e passou a ser **integração**: o
> binário ainda é um menu interativo, não fala o protocolo, e a IA (em `IA/`) ainda não está
> ligada a ele.
>
> Atualizados junto com este documento, nesta mesma data: `next_steps.md` (com as etapas de
> otimização depois do UCI), `roadmap-motor.md` (marcas de status), `onboarding-motor.md`
> (§6–§9, §11), `repl-comandos.md` (estado), `perft_results.txt` e o `README.md` da raiz.

---

## 1. O que é este repositório

Um **motor de xadrez em C**, escrito do zero como projeto de aprendizado — e o repositório
oficial do grupo na disciplina de Engenharia de Software 2. O objetivo declarado não é força
de jogo: é entender como uma engine funciona por dentro, e fazê-la funcionar em equipe.

A fronteira do motor é estreita e deliberada: **um binário que fala um protocolo texto por
stdin/stdout.** O repositório tem três partes:

| Diretório | O quê | Como fala com o motor |
|---|---|---|
| `engine/` | Regras (este documento) | — |
| `IA/` | Busca minimax / alfa-beta e avaliação, em C (outro membro da equipe) | **No mesmo processo**: inclui `board.h`, `move.h`, `makemove.h` e chama `make_move`/`unmake_move` direto. Ainda não ligada ao binário do motor (ver §6, contrato) |
| `interface/` | Cliente gráfico em C++/SFML (outro membro da equipe) | **Subprocesso**: `EngineBridge` faz `fork` + `execlp` do binário e escreve comandos no stdin dele. A leitura da resposta (`readCommand`) ainda é `TODO`, e o ramo Windows também |

Hoje o executável de `engine/` é um **menu interativo** (`ui()` em `main.c`, 13 opções,
incluindo perft, perft divide e o teste de make/unmake) — não ainda o protocolo.

A fase atual continua sendo **protótipo/demo**: um binário que joga xadrez legalmente do
início ao fim, com uma IA qualquer. Correção das regras vem primeiro (feito, §2), protocolo
em segundo, IA em terceiro, força de jogo e desempenho por último. As etapas de otimização
estão planejadas em `next_steps.md` Parte II — **depois** do UCI, não antes.

---

## 2. Estado atual, honestamente

O motor tem **~3 000 linhas** de C: 11 headers em `include/` (432 linhas), 11 fontes em
`src/` (2 470) e `test/` (115). Eram ~2 550 em 01/10. O crescimento veio do filtro de
legalidade (`movegen.c` 267 → 378), do perft e do menu ampliado (`main.c` 210 → 393) e da
volta de `test/test.c` (108 linhas).

**O resumo desta revisão: geração, aplicação e legalidade estão completas e provadas por
perft; o build tem 18 avisos e zero erros; `make test` não linka; e o caminho de crash que
restou só é alcançável por FEN ilegal (Bug #1) ou pela edição manual de casa no menu
(Bug #2).**

### Funciona e está verificado com evidência

Tudo nesta tabela foi reproduzido em 04/10, com um harness descartável no diretório
temporário (fora do repositório) que chama **o `perft()` do próprio `main.c`** — ou seja,
`generate_legal_moves` + `make_move` + `unmake_move` do motor, sem filtro do harness no
meio. Na revisão de 01/10 o filtro de legalidade era do harness; agora é o do motor.

| Área | Estado |
|---|---|
| `fen_parse` / `fen_write` | Seis campos, validação completa, round-trip idêntico. Sem mudança de comportamento desde 07/09. Ver `docs/fen.md`. Falta rejeitar posição com o lado que não joga em xeque (Bug #1) |
| Geração pseudo-legal | Peão (push, duplo aninhado, captura, promoção nas 4 peças com e sem captura, en passant), cavalo, bispo/torre/dama por `SQ_TO_EDGE`, rei, roque (direito + casas vazias + torre na casa) |
| **`is_square_attacked`** | **Implementada** (`movegen.c:308`). Busca reversa: peão por `PAWN_ATTACKS` invertido, cavalo por `KNIGHT_ATTACKS`, raios diagonais e ortogonais parando na primeira peça, rei. O bug de *wraparound* do rei (h4 + 1 = a5), achado por bissecção de perft em 04/10, está corrigido — o laço testa `SQ_TO_EDGE[sq][dir] == 0` (`movegen.c:363`) |
| **`generate_legal_moves`** | **Implementada** (`movegen.c:273`). Aplica-e-testa: para cada lance pseudo-legal, `make_move` → o rei de quem jogou ficou atacado? → `unmake_move`. O roque tem as duas checagens extras que o filtro sozinho não cobre: **não rocar estando em xeque** e **não atravessar casa atacada** (`movegen.c:289-292`). Esse era o segundo bug do perft de 04/10 |
| **Perft no motor** | `perft()` e `perft_divide()` em `main.c:43` e `main.c:66`, acessíveis pelas opções 10 e 13 do menu. Batem nas seis posições — tabela abaixo |
| `make_move` / `unmake_move` | Completos. `king_square` agora é atualizado **incrementalmente** (só quando a peça movida ou capturada é rei, `makemove.c:72-77`), e `unmake_move` restaura também o rei capturado (`makemove.c:184-186`) — o antigo Bug #2 fechou |
| **Round-trip make/unmake em todo nó** | Sob ASan + UBSan, profundidade 4 nas seis posições: `fen_write` antes do `make` == `fen_write` depois do `unmake`, e `board_check_invariants` verde depois de cada um. **11 024 485 checagens, zero falhas** |
| `run_make_unmake_tests()` (`test/test.c`) | 18 FENs, 424 round-trips, verde. Roda pela opção 12 do menu — `make test` está quebrado (Bug #4) |
| REPL só aceita lance legal | A opção 2 do menu procura o lance digitado na lista de `generate_legal_moves`; a captura de rei em partida normal ficou inalcançável |

**A bateria de perft.** Valores de referência da
[Chess Programming Wiki — Perft Results](https://www.chessprogramming.org/Perft_Results).
Tempos com `-O2 -DNDEBUG`, numa máquina de 20 núcleos (o perft é *single-thread*):

| Posição | Profundidade | Nós | Esperado | Tempo |
|---|---|---|---|---|
| Inicial | 6 | 119 060 324 | ✅ 119 060 324 | 9,2 s |
| Inicial | 7 | 3 195 901 860 | ✅ 3 195 901 860 | 243,5 s |
| Kiwipete | 5 | 193 690 690 | ✅ 193 690 690 | 13,8 s |
| Posição 3 | 7 | 178 633 661 | ✅ 178 633 661 | 15,8 s |
| Posição 4 | 5 | 15 833 292 | ✅ 15 833 292 | 1,2 s |
| Posição 5 | 5 | 89 941 194 | ✅ 89 941 194 | 6,9 s |
| Posição 6 | 5 | 164 075 551 | ✅ 164 075 551 | 11,7 s |

O que isso prova: geração, make/unmake **e o filtro de legalidade do motor** estão corretos
em roque (inclusive através de casa atacada e saindo de xeque), en passant (inclusive o caso
de cravada horizontal da posição 3), promoção, cravadas e direitos de roque. A posição 3 é a
que pegaria o *wraparound* do rei na profundidade 1; a posição 5, o roque saindo de xeque.


**Desempenho medido (linha de base para a Parte II do `next_steps.md`).** Perft sem *bulk
counting* — o filtro de legalidade gera e testa até a última folha:

| Build | Mnós/s |
|---|---|
| `-Og -g` (o `make` padrão) | 10,5 |
| `-O2 -DNDEBUG` | 12,2 |
| `-O3` (o `make release` atual; com ou sem `NDEBUG` dá o mesmo — os `assert` não pesam) | 14,4 |
| `-O2 -DNDEBUG -flto` | **19,0** |

O salto do `-flto` não é acaso: o perfil (`gprof`, posição inicial prof. 6 + Kiwipete
prof. 5) mostra que os acessores de lance de uma linha em `move.c` — `move_to`, `move_from`,
`move_is_castle`, `move_is_promotion`, `move_type`, `move_is_ep_capture` — somam **~28% do
tempo**, com ~9,5 bilhões de chamadas fora de linha (~30 por folha), porque moram em outro
arquivo e o compilador não consegue expandi-los sem LTO. O resto: `is_square_attacked` ~26%,
`make_move` ~14%, `unmake_move` ~9%, geração ~12%. Por folha do perft, o motor faz ~2,07
pares make/unmake (um no filtro, outro no laço do perft) e ~1,09 chamadas de
`is_square_attacked`. Detalhe e plano em `next_steps.md`, etapa O2 e O3.

### Incompleto ou ausente

| Área | Estado |
|---|---|
| **Protocolo UCI** | **Não existe.** `main()` sempre abre o menu. `io.h/c` é a base prevista, mas tem dois bugs (Bugs #3, #5) |
| Subcomandos `test` / `perft` | Não existem — perft e teste só pelo menu. `main()` não olha `argv` |
| Portão de teste no repositório | `make test` **não linka** (Bug #4). O corpus de `test/test.c` não inclui as seis posições de referência, e o teste não chama `board_check_invariants`. Os números da tabela acima só existem neste documento e no harness descartável |
| Fim de partida | Mate e afogamento são deriváveis (lista legal vazia + `is_square_attacked` no rei), mas **não há função** que diga o estado da partida. Não há `in_check` |
| Empates por regra | Nenhum: nem 50 lances (`halfmove_clock` já existe), nem material insuficiente, nem repetição (exige histórico — ver §3, "motor stateless"). São requisito RF-M4 do `arquitetura-xadrez.md` e o contrato da IA os pede |
| `board_check_invariants` | 4 das 6 checagens de `docs/guides.md` (Bug #7) |
| Ligação com a IA | A IA espera `gerarLancesLegais`, `emXeque` e `testeEmpateRegra` (`IA/header/contrato.h`); o motor tem só a primeira, com outro nome (`generate_legal_moves`). Ver §6 |

---

## 3. Decisões arquiteturais firmadas

Escolhas conscientes com trade-off avaliado. Mudá-las agora custa caro. As marcadas
**(04/10)** mudaram ou foram confirmadas nesta revisão.

### Representação: mailbox de 64 casas

`Piece array[64]`, não bitboards. `array[64]` = 64 bytes = uma linha de cache; varredura
completa medida em ~9,5 ns contra ~12,3 ns de uma piece list. Sem mudança. Bitboards
continuam fora de escopo — inclusive na fase de otimização: `next_steps.md` §O6 registra a
recomendação de **não** migrar dentro do prazo do projeto.

### Indexação: `a1 = 0` (Little-Endian Rank-File)

`sq = rank * 8 + file`; `h8 = 63`. `PAWN_PUSH[WHITE] = +8`, `PAWN_PUSH[BLACK] = -8`.

### Codificação da peça

`piece.h`: `PIECE_MAKE(color, type) = (color << 3) | type`, com `WHITE = 1`, `BLACK = 0`,
`EMPTY = 0`. As duas armadilhas continuam valendo: `PIECE_COLOR(EMPTY) == BLACK`, e comparar
o valor cru contra um `PieceType` funciona por acidente para as pretas e falha para as
brancas.

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
comparam por igualdade. Quem for adicionar um predicado novo sobre `MoveType`: **teste por
`&` só nos dois bits que são bits.** `MOVE_NONE == 0` é seguro como sentinela porque nenhum
lance gerado tem origem igual ao destino.

### Helpers de casa com cor implícita; ataque com cor explícita — **(04/10) fechada**

`board.h` tem `is_empty`/`is_own`/`is_enemy(const Board *, int sq)`, com `assert` de casa
dentro do tabuleiro e a cor de referência implícita (`b->side_to_move`). A revisão de 01/10
deixou em aberto como `is_square_attacked` lidaria com cor explícita. A resposta que o código
deu: **ela não usa esses helpers** — compara a peça crua contra `PIECE_MAKE(opposite, TIPO)`.
Os helpers ficam para a geração (onde a cor é sempre a do lado a jogar), e o ataque compara
peças com cor explícita. Duas ferramentas para duas perguntas, sem variante nova.

### `is_square_attacked(b, sq, side)`: `side` é o **dono** da casa — **(04/10) armadilha nova**

A assinatura planejada (CPW, `roadmap-motor.md` Fase 5) era `is_square_attacked(b, sq, by)`,
com a cor do **atacante**. A implementada recebe a cor do **defensor**: "a casa `sq`, que é
de `side`, está atacada pelo adversário de `side`?". O filtro de legalidade a chama certo
(`is_square_attacked(b, king_sq, side)` com `side` = quem acabou de jogar), e o perft prova
isso. O risco é o próximo chamador — o `fen_parse` do Bug #1, o `in_check` do UCI, a IA —
passar o atacante por hábito e inverter o resultado sem nenhum aviso. A opção 11 do menu
também pergunta só "Insert side". **Recomendação: renomear o parâmetro para `defender` (ou
`us`) e dizer isso num comentário no header.** É a armadilha `PIECE_COLOR(EMPTY) == BLACK` de
novo: documentada ainda é armadilha; no nome do parâmetro, deixa de ser.

### Direções: tabela de distância até a borda

`SQ_TO_EDGE[64][8]`, ordem do enum como carga estrutural (ortogonais 0–3, diagonais 4–7),
para que torre, bispo e dama usem uma única função parametrizada por intervalo. O rei usa a
mesma tabela com o laço parando em 1 — na geração **e** no ataque, e foi justamente no ataque
que faltava a checagem de borda (corrigido em `1582162`).

### Cavalo por tabela pré-computada

`KNIGHT_ATTACKS[64][8]`, preenchida no init a partir de `KNIGHT_VECTORS` por
`offset_square` — a mesma função que constrói `PAWN_ATTACKS`. Saltos para fora ficam
`SQ_NONE`. Serve duas vezes: geração e `is_square_attacked`. `KING_TARGETS[64][8]` continua
alocada, zerada e sem leitor — apagar.

### Pseudo-legal primeiro, legalidade depois — **(04/10) implementado**

Decisão de setembro, agora em código e provada por perft. A divisão de trabalho do roque
ficou exatamente como decidida: o **gerador** verifica direito, casas vazias e torre na casa;
o **filtro** verifica não estar em xeque, não atravessar casa atacada e (pelo aplica-e-testa
geral) não chegar em casa atacada. A casa atravessada é calculada como `(from + to) / 2`, que
dá f1/d1/f8/d8 nos quatro roques. A casa b1/b8 do roque grande precisa estar vazia mas **não**
precisa estar desatacada — e não é testada, corretamente.

### `Undo` é do chamador

```c
typedef struct {
    Piece captured;          /* no en passant: o peao realmente capturado */
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);
void unmake_move(Board *b, Move move, const Undo *u);
```

A pilha da recursão é a pilha de undo; não existe pilha global. O filtro de legalidade e o
perft usam `Undo u;` local, como planejado.

O histórico de uma partida, quando necessário fora da busca, é uma estrutura explícita
`BidHistory`: mantém listas alinhadas de `Move` e `Undo`, sem snapshots do `Board`. Seu
`bidhistory_add` reserva capacidade antes de chamar `make_move`, captura o `Undo` produzido
por esse lance e armazena ambos no mesmo índice. `bidhistory_undo_last` usa o par mais recente
com `unmake_move` e então reduz a contagem. O `Board` continua pertencendo ao chamador.
Isso não muda o contrato UCI, no qual o cliente é dono do histórico da partida.

### `king_square` é cache incremental — **(04/10) voltou a ser cache**

Em 01/10 `make_move`/`unmake_move` refaziam a varredura de 64 casas a cada lance. Agora só
atualizam quando a peça movida (ou capturada) é rei. `board_check_invariants` confere o cache
contra a varredura, e passou nos 11 milhões de nós do §2. Sobra um lugar que ainda varre:
`check_allowed_castles` chama `board_find_king` em vez de ler o cache (§4).

### Contrato de validação: fronteira valida, núcleo confia

`make_move` é primitiva, não serviço: não valida, e é por isso que o filtro pode aplicar um
lance que talvez deixe o rei em xeque. Quem valida é a fronteira — hoje o menu (só aceita
lance da lista legal); amanhã o comando `position ... moves`.

### Motor stateless entre comandos, processo stateful

A posição chega como FEN completa (+ sufixo `moves`, que o motor reaplica). Limite conhecido:
repetição tripla não é detectável a partir de uma FEN — é o sufixo `moves` que dá o
histórico. Isso agora deixou de ser teórico: o contrato da IA pede `testeEmpateRegra`, que
inclui repetição (ver §6).

### Integração com o cliente: subprocesso direto — **(04/10) registrada como fechada**

Fechada no `arquitetura-xadrez.md` §8 em 13/09, e este documento ainda a listava como aberta:
**o cliente C++ roda o motor como subprocesso, sem backend no meio**, e o protocolo inclui um
comando `legalmoves` que responde `moves e2e4 g1f3 ...` para a interface destacar lances
(`arquitetura-xadrez.md` §3). O `EngineBridge` de `interface/` já implementa o lado POSIX
do spawn.

### `board_check_invariants` sem out-param

`bool board_check_invariants(const Board *b)`, com as falhas num log global em `utils.c`
(`fail_msg` / `print_fail_log`). Sem mudança; as consequências continuam abertas (Bug #7).

### Testes como subcomando do binário

**Ainda não implementado**, e a decisão de 10/09 (`./main.out test`, `./main.out perft N`)
continua de pé. O caso a favor ficou mais forte: o perft e o teste já existem, só estão
presos atrás do menu. Plano em `next_steps.md` Etapa 8.

### Sistema de build: Makefile

O `CMakeLists.txt` quebrado da revisão anterior foi apagado — decisão encerrada. O Makefile
tem `all` (`-Og -g`), `debug` (sanitizers), `release` (`-O3`, **sem avisos ligados e sem
`-DNDEBUG`**), `test` (quebrado) e um ramo para Windows (`build/main.exe`) — ou seja, parte do
grupo compila no Windows, o que torna o Bug #6 real.

---

## 4. Mapa dos módulos

Headers em `include/`, implementação em `src/`. Todo `.c` inclui o próprio `.h` primeiro (por
caminho relativo, `"../include/foo.h"`), e todo header compila sozinho.

| Arquivo | Papel | Linhas (.h/.c) | Nota |
|---|---|---|---|
| `types.h` | Typedefs de largura fixa, `MAX_*`, `MIN`/`MAX`, `MemoryZero*`, `Array_Size` | 59 / — | Ainda arrasta `ctype.h stdio.h stdlib.h string.h`. `PrintSize`, `MemoryZero*`, `Array_Size` e `MAX_SEARCH_PLY` sem uso |
| `log.h/c` | `LOG_ERROR` (sempre) / `LOG_DEBUG` (só com `-DDEBUG`) | 20 / 20 | Correto |
| `piece.h/c` | `Color`, `PieceType`, `Piece`, macros, char ↔ peça | 42 / 27 | `PIECE_CHAR` global sem declaração em header |
| `square.h/c` | Coordenadas, `SQ_TO_EDGE`, `DIR_OFFSET`, `PAWN_PUSH`, `PAWN_ATTACKS`, `KNIGHT_ATTACKS` | 50 / 141 | `KING_TARGETS` órfã; `KNIGHT_OFFSETS` (header) e `DIR_CHARMAP` sem uso; `KNIGHT_VECTORS` global sem declaração |
| `board.h/c` | `Board`, `CastleRights`, `CASTLE_POSITIONS`, `BOARD_START_POS`, `is_empty`/`is_own`/`is_enemy`, `board_find_king`, `board_check_invariants`, `board_clear`, `board_new`, `board_print`, `fill_sq` | 80 / 156 | Bug #7 |
| `fen.h/c` | `fen_parse`, `fen_write`, `START_FEN` | 12 / 612 | Maduro. Cabeçalho do `.c` ainda diz `board.c` / `parse_fen`. Bug #1 |
| `move.h/c` | `Move`, `MoveType`, `MoveList`, encode/decode, predicados, `movelist_*`, str ↔ lance, `print_moves` | 58 / 176 | Acessores fora de linha: ~28% do tempo (§2). Bugs #8, #13 |
| `makemove.h/c` | `Undo`, `make_move`, `unmake_move` | 18 / 197 | Completos e provados. Bug #12 (higiene) |
| `movegen.h/c` | Geração pseudo-legal por peça, `generate_pseudo_legal_moves`, `generate_legal_moves`, `is_square_attacked` | 16 / 378 | Bugs #9, #10. `generate_knight_moves` fora do header |
| `io.h/c` | `String` (fatia sem posse), trim/split/parse, `read_line`/`read_word`/`read_int`, `join_args` | 52 / 227 | Sem mudança desde 30/09. Bugs #3, #5, #11 |
| `utils.h/c` | `fail_msg`/`print_fail_log`, `COLOR_CHAR`, `strslc`, `clear_screen`, `get_fen`, `get_int`, `read_move`, `read_coord`, `wait_enter`, `copy_board` | 25 / 143 | Depende de `io.c` (por isso o Bug #4). `read_move` e `print_piece_chart` sem uso |
| `main.c` | `perft`, `perft_divide`, menu `ui()` (13 opções), `main()`, e uma IA aleatória morta | — / 393 | Bugs #2, #6, #14 |
| `test/test.h/c` | `run_make_unmake_tests()` sobre 18 FENs; `main` próprio sob `-DMAKE_UNMAKE_TEST_STANDALONE` | 8 / 107 | Verde. `test_api.c` existe e está vazio |

**Problemas estruturais que atravessam módulos:**

- **`perft` e `perft_divide` moram em `main.c`.** O comando `perft N` do UCI e o subcomando
  `./main.out perft` vão precisar deles, e `main.c` não é módulo — não tem header. Um
  `perft.h/c` próprio resolve, e é onde o corpus de referência com os números esperados
  deveria morar (`next_steps.md` Etapa 8).
- **Oito funções com linkage externo e sem protótipo** (os avisos de
  `-Wmissing-prototypes`): `genenare_moves_from_direction`, `check_allowed_castles`
  (`movegen.c`), `print_move` (`move.c`), `string_to_cstr_static`, `string_split_next`
  (`io.c`), `perft`, `perft_divide` (`main.c`) — e `generate_knight_moves`, que é pública mas
  `movegen.h` não declara. Cada uma é **ou** helper que devia ser `static`, **ou** API que
  devia estar no header.
- **Dependência na direção errada, prestes a nascer:** `is_square_attacked` mora em
  `movegen.c`, e o Bug #1 pede que `fen_parse` a use. Fazer `fen.c` incluir `movegen.h` liga o
  parser ao gerador inteiro. Recomendação: um módulo `attack.h/c` que só depende de
  `board.h`/`square.h`, usado por `movegen.c`, `fen.c` e pela IA (`next_steps.md` Etapa 9).
- **`const` regrediu e não voltou:** `generate_pawn_moves` e `generate_pseudo_legal_moves`
  recebem `Board *`, mas não escrevem no tabuleiro.
- **IA aleatória morta em `main.c:315-393`** (`ai_game`, `make_random_move`, `draw_frame`,
  `sleep_ms`, `get_random_move`): nunca chamada, usa o gerador pseudo-legal e um contorno de
  captura de rei que o filtro tornou desnecessário. Ou vira o `go` provisório do UCI (sorteio
  sobre `generate_legal_moves`), ou sai.
- O typo `genenare_moves_from_direction` continua.

---

## 5. Bugs abertos

Numeração nova nesta revisão. Entre parênteses, o número de 01/10 quando o bug já existia.

**Fechados desde 01/10:**

| Era | Como fechou |
|---|---|
| #1 — o processo cai quando um rei é capturado em partida normal | Pela raiz: o filtro de legalidade torna a captura de rei inalcançável a partir de posição legal. E `generate_king_moves` e `check_allowed_castles` agora tratam `SQ_NONE` em vez de confiar na pré-condição. **Resta um caminho, por FEN ilegal — é o Bug #1 abaixo** |
| #2 — `unmake_move` não restaurava o `king_square` do rei capturado | `makemove.c:184-186` |
| #6 — `#include "../include/assert.h"` apontando para arquivo inexistente | Agora `<assert.h>` |
| #8 (metade) — `CMakeLists.txt` sem fontes | Apagado. A outra metade (`make test`) continua quebrada, por outro motivo — Bug #4 |
| #15 — `b = (Board){0}` em FEN inicial inválida | `main()` agora faz `board_new` e sai com erro se a FEN falhar |
| Perft de 04/10: roque saindo de xeque e através de casa atacada | `movegen.c:289-292`. Achado por bissecção com Stockfish (`perft divide` + `compare_perft.py`) |
| Perft de 04/10: *wraparound* do rei em `is_square_attacked` | `movegen.c:363`. A posição 5 sozinha escondia este; a 3 o mostra na profundidade 1 — é por isso que o portão tem de ser as seis posições, não uma |

### Graves

| # | Onde | Problema |
|---|---|---|
| 1 | `fen.c` (aceita), `movegen.c:309` (cai) | (antigo #17, mais o que sobrou do antigo #1) **FEN com o lado que não joga em xeque é aceita, e ela derruba o processo.** Reproduzido pelo menu: opção 1 com `R6k/8/8/8/8/8/8/K7 w - - 0 1`, opção 2 com `a8h8` (o filtro aceita: capturar o rei não deixa o rei **branco** em xeque), opção 6 → `movegen.c:309: is_square_attacked: Assertion '!SQ_OFFBOARD(sq)' failed`, porque `king_square[BLACK]` virou `SQ_NONE`. Com `-DNDEBUG` seria leitura fora do tabuleiro. **Agora tem correção barata**: `is_square_attacked` existe, e `fen_parse` pode rejeitar a posição — ver a nota de dependência no §4 |
| 2 | `main.c:209-216` | (antigo #3) **A opção 7 do menu ("Edit square") escreve fora do tabuleiro.** Reproduzido de novo: entrada `7`, `z9`, `.` → `main.c:215: index -1 out of bounds` + `AddressSanitizer: stack-buffer-underflow` (WRITE). E a edição não atualiza `king_square`, direitos de roque nem `ep_square`, nem zera `last_move`. `-Wconversion` aponta as duas linhas exatas (`main.c:213`, `main.c:215`) |
| 3 | `io.c:181` | (antigo #4) **`read_word` usa buffer não inicializado em EOF.** `read_line` devolve `0` em EOF e `read_word` só testa `-1`. Reproduzido de novo: com entrada `e2e4\n`, a 1ª chamada devolve `e2e4`; a 2ª e a 3ª, já em EOF, devolvem `e2e4` **com sucesso**. Nenhum sanitizer pega. Importa porque é a base prevista do laço UCI: um cliente que fecha o pipe faria o motor repetir o último comando para sempre |
| 4 | `Makefile:51-55` | **`make test` não linka.** O alvo filtra `src/io.c` para fora, mas `utils.c` passou a depender dele (`read_move`, `read_coord`, `wait_enter` chamam `read_word`/`read_line`): `undefined reference to 'read_word'`. O teste em si está verde (opção 12); o alvo do Makefile é que não roda. Junto: `test.c` não chama `board_check_invariants`, e o corpus não tem as seis posições de referência |
| 5 | `io.c:205-228` | (antigo #5) **`join_args` não coloca separador entre os argumentos.** Reproduzido: os seis campos da FEN inicial viram `rnbqkbnr/.../RNBQKBNRwKQkq-01`, que `fen_parse` rejeita. Sem chamador ainda — mas é a função feita para montar `position fen ...` |
| 6 | `main.c:90`, `main.c:93`, `main.c:227`, `main.c:223` | **Portabilidade, e o Makefile tem ramo Windows.** O perft imprime `u64` com `%zu` e `%lu`. Em Linux x86-64 os dois têm 64 bits e funciona; no Windows (MinGW, modelo LLP64) `unsigned long` tem **32 bits** e a contagem sai truncada — a profundidade 7 (3,2 bilhões) já não cabe. O certo é `PRIu64` de `<inttypes.h>`. E `main.c:223` declara `int d` logo depois do rótulo `case 10:`, o que C11 não permite (`-Wpedantic` avisa; GCC anterior ao 11 e outros compiladores dão **erro**) |
| 7 | `board.c:41`, `utils.c:16` | (antigo #7) `board_check_invariants` cobre 4 das 6 checagens de `docs/guides.md`: faltam `ep_square` coerente com `side_to_move` e direitos de roque coerentes com rei/torres (esta **já está escrita**: `castling_matches_board`, `static` em `fen.c:385`). E `fail_msg` acumula num log global que nunca é zerado (teto 128) e recebe `char *` — os 6 avisos de `-Wwrite-strings` |

### Menores / higiene

| # | Onde | Problema |
|---|---|---|
| 8 | `move.c:112` | (antigo #9) `promotion_type = EMPTY;` atribui `PieceType` a `MoveType` (`-Wenum-conversion`). Promoção em maiúscula (`e7e8Q`) ou letra inválida vira lance sem promoção em silêncio, e o REPL responde "invalid move" sem dizer por quê |
| 9 | `movegen.c:129-158` | (antigo #10) No cavalo: `piece_to` atribuída e nunca lida; filtro de cor por `PIECE_COLOR` cru em vez de `is_own`; o último `else if (is_enemy(...))` é sempre verdadeiro. Nenhum produz lance errado — o perft prova |
| 10 | `movegen.c:57`, `movegen.c:192`, `movegen.c:188` | (antigo #11) `piece = b->array[to];` morto; `static const enum {LEFT, RIGHT};` ("useless storage class"); `check_allowed_castles` acha o rei por varredura (`board_find_king`, ~1,7% do tempo) em vez do cache que agora é confiável |
| 11 | `io.c:112`, `io.c:131`, `io.c:121` | (antigo #12) `char *p = line;` descarta o `const`; `if (*p \|\| *p == '\0')` é sempre verdadeiro; `string_to_cstr` não confere o `malloc`; `string_parse_int` não detecta estouro |
| 12 | `makemove.c:8`, `makemove.c:91-103` | (antigo #13) `check_rook_squares` também trata `E1`/`E8` (o nome mente). A torre do roque é posta com `fill_sq(b, "d1", ...)` — parsing de string no caminho quente, com o retorno descartado. `CASTLE_POSITIONS` já tem as casas como números. Os seis `&= ~(CASTLE_*)` em `u8` são 6 dos 11 avisos de `-Wconversion` |
| 13 | `move.c:125` | (antigo #14) `movelist_add` loga e descarta quando a lista enche, em vez de `assert` |
| 14 | vários | Sobras: IA aleatória morta (`main.c:315-393`, `-Wunused-function`); `int i` não usado em `perft` (sombreado pelo `i` do laço — o único aviso de `-Wshadow`) e `result_list[MAX_MOVES]` não usado em `perft_divide`; `KING_TARGETS`, `KNIGHT_OFFSETS`, `DIR_CHARMAP`, `CASTLE_TYPE_MASK`, `read_move`, `print_piece_chart`, `PrintSize`/`FILE_DIST`/`MemoryZero*`/`Array_Size` sem uso; macros `TEST_FEN_*` de `test.c` sem uso (e `TEST_FEN_EN_PASSANT` com `-` no campo de en passant). Comentários desatualizados: `main.c:133-137` e `Makefile:42` dizem que `LOG_ERROR` some fora do debug (não é verdade desde 10/09); o cabeçalho de `fen.c` ainda diz `board.c`/`parse_fen`. O menu abre na posição 5 da CPW (`main.c:289`), não na inicial — sobra da bissecção |

### Sobre o Makefile e as flags

`make` e `make debug` hoje: **18 avisos**, zero erros (eram 14 em 01/10):

- 8 de `-Wmissing-prototypes` (§4);
- 2 de `-Wdiscarded-qualifiers` em `io.c` (Bug #11);
- 2 de variável não usada em `main.c` (`i`, `result_list`), 1 `-Wpedantic` (declaração depois
  de rótulo, Bug #6), 1 `-Wunused-function` (`ai_game`);
- 1 `-Wenum-conversion` (Bug #8), 1 `-Wunused-but-set-variable` (Bug #9), 1 "useless storage
  class" (Bug #10), 1 `-Wunused-const-variable` (`DIR_CHARMAP`).

As quatro flags que o `CLAUDE.md` §7 pede e o Makefile não liga, medidas contra esta árvore:

| Flag | Custo | O que pega |
|---|---|---|
| `-Wshadow` | +1 | O `int i` morto de `perft`, sombreado pelo do laço |
| `-Wcast-qual` | **0** | De graça |
| `-Wwrite-strings` | +6 | Todos `fail_msg(char *)` recebendo literal (Bug #7). Uma palavra fecha os seis |
| `-Wconversion` | +11 | **Dois em `main.c:213/215`, exatamente o Bug #2.** Seis são os `&= ~(CASTLE_*)` de `makemove.c` (Bug #12 — uma tabela `CASTLE_MASK[64]` resolve os seis de uma vez); os outros três em `io.c:122`, `move.c:16` e `utils.c:40`. O GCC 13 rotula esses nove como `-Wsign-conversion`, que em C vem junto com `-Wconversion` |

O alvo `release` compila com `-O3` e **nenhum aviso ligado** — se alguém só usar `release`,
volta ao cenário de setembro em que os avisos não disparavam.

---

## 6. Decisões em aberto

| Decisão | Situação |
|---|---|
| ~~Sistema de build~~ | **Fechada:** Makefile; o `CMakeLists.txt` foi apagado |
| ~~Cor implícita vs. explícita nos helpers~~ | **Fechada pelo código** (§3) |
| ~~Camada de integração cliente ↔ motor~~ | **Fechada em 13/09** no `arquitetura-xadrez.md` §8: subprocesso direto, com `legalmoves` no protocolo (§3) |
| ~~Onde mora o corpus de FEN~~ | **Fechada pelo código:** `const char *TEST_FENS[]` em `test/test.c`. Falta acrescentar as seis posições de referência com os números esperados |
| **Contrato regras ↔ IA** | **Nova, e bloqueia o `go` do UCI.** `IA/header/contrato.h` declara `gerarLancesLegais`, `emXeque` e `testeEmpateRegra`, e diz ele mesmo que deve ser substituído pelos headers das regras quando elas existirem. Recomendação: a IA chama os nomes do motor diretamente (`generate_legal_moves` já tem a mesma forma) e o motor fornece `in_check` e a detecção de empate — dois nomes para uma função é a mesma duplicação que este projeto já decidiu evitar. Decidir com o responsável pela IA. Junto: `escolherJogada` não recebe profundidade nem tempo (o limite é a constante `PROF_MAX = 9000` em `ia.h`), e o `go depth N` / `go movetime N` do UCI precisa de um dos dois |
| **Quem guarda o histórico para repetição** | Nova. Repetição exige as posições desde o último lance irreversível. Recomendação: a camada de protocolo (`Game`, como decidido em 10/09) guarda chaves de posição e passa para quem decide empate; Zobrist entra só na Parte II do `next_steps.md` (O5), quando a busca precisar de repetição dentro da árvore |
| **Onde mora `is_square_attacked`** | Nova. Ver §4: recomendação de um `attack.h/c` antes de `fen_parse` passar a usá-la |
| **Zobrist antes ou depois do perft** | Resolvida pelo tempo: o perft bateu sem ele. Volta como etapa O5 da Parte II |
| **`Move` sem score** | Confirmado. Ordenação de lances é a etapa O4 da Parte II; a IA tem sua própria lista de candidatos |

---

## 7. Próximos passos, em ordem

Detalhe, critérios de saída e as etapas de otimização em `next_steps.md`. Resumo:

1. **Portão dentro do repositório** (Etapa 8): consertar `make test` (Bug #4), tirar `perft`
   de `main.c` para um módulo, `main()` olhar `argv` (`test`, `perft N`), e pôr as seis
   posições com os números do §2 no corpus. Os números de hoje só existem neste documento —
   o próximo commit que quebrar a geração não vai avisar ninguém.
2. **Fim de partida e contrato com a IA** (Etapa 9): `in_check`, estado da partida (mate,
   afogamento, 50 lances, material insuficiente, repetição), `fen_parse` rejeitando o lado que
   não joga em xeque (Bug #1), e o acordo de nomes com `IA/`.
3. **UCI mínimo** (Etapa 10): laço em cima de `io.c` — depois de corrigir o EOF (Bug #3) e o
   `join_args` (Bug #5) —, `setvbuf`, `position ... moves`, `legalmoves`, `go` chamando a IA.
4. **Fechar o build** (Etapa 11): os 18 avisos, as quatro flags, Bugs #2 e #6. Pode entrar a
   qualquer momento, mas **antes** da otimização, que vai mexer no código quente.
5. **Completar `board_check_invariants`** (Etapa 12, Bug #7).
6. Só então a **Parte II — otimização**, etapas O1 a O6, cada uma com perft como portão.

---

## 8. Referências ativas

- **Chess Programming Wiki**: [Perft Results](https://www.chessprogramming.org/Perft_Results)
  (as seis posições do §2), [Square Attacked By](https://www.chessprogramming.org/Square_Attacked_By),
  [Castling](https://www.chessprogramming.org/Castling), [Checkmate](https://www.chessprogramming.org/Checkmate),
  [Stalemate](https://www.chessprogramming.org/Stalemate),
  [Fifty-move Rule](https://www.chessprogramming.org/Fifty-move_Rule),
  [Repetitions](https://www.chessprogramming.org/Repetitions),
  [Encoding Moves](https://www.chessprogramming.org/Encoding_Moves).
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas, ler inteira antes da Etapa 10.
  Em especial o que acontece quando a entrada acaba (Bug #3) e o formato de `bestmove`
  quando não há lance (`bestmove (none)` ou `0000`).
- **Stockfish como oráculo de perft**: `position fen <FEN> [moves ...]`, `go perft N`; cuidado
  com FEN ilegal (o Stockfish não imprime lance nenhum, e isso significa "corrija a FEN", não
  "zero lances").
- `man gcc`, seção *Options to Request or Suppress Warnings*; `man 3 printf` e `<inttypes.h>`
  para `PRIu64` (Bug #6); `gprof` para o perfil do §2.

---

## Apêndice — referência rápida dos módulos

Typedefs, enums, macros e assinaturas, na ordem de dependência. Comentários `/* … */` ao lado
de uma linha apontam para o bug correspondente no §5. Includes, guards e linhas em branco
omitidos. Reflete o commit `1582162`.

### `types.h` — vocabulário mínimo

```c
typedef uint64_t u64;  typedef uint32_t u32;  typedef uint16_t u16;  typedef uint8_t u8;
typedef int16_t i16;   typedef int64_t i64;

#define INPUT_STR_SIZE 128
#define MAX_FEN_STRING 256
#define BOARD_SIZE     64
#define BOARD_WIDTH    8
#define MAX_MOVES      256
#define MAX_SEARCH_PLY 64      /* sem uso ainda */
#define MAX_GAME_PLY   1024    /* limite do historico de lances */
#define NUM_COLORS     2

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
/* MemoryZero, MemoryZeroStruct, PrintSize, Array_Size: sem uso */
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

char  piece_to_char(Piece p);     /* indexa PIECE_CHAR = ".pnbrqk..PNBRQK." */
Piece piece_from_char(char c);
```

### `square.h` — geometria do tabuleiro

```c
#define SQ_NONE (-1)
#define RANK_OF(sq)       ((sq) / BOARD_WIDTH)
#define FILE_OF(sq)       ((sq) % BOARD_WIDTH)
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
extern int KING_TARGETS[BOARD_SIZE][8];             /* orfa -- apagar */
extern int PAWN_ATTACKS[NUM_COLORS][BOARD_SIZE][2]; /* SQ_NONE nas bordas */
static const int KNIGHT_OFFSETS[8];                 /* sem uso -- apagar */

void init_square_tables(void);   /* SQ_TO_EDGE + PAWN_ATTACKS + KNIGHT_ATTACKS; chamar 1x */
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
    int   king_square[2];    /* cache incremental, conferido por board_check_invariants */
    u8    castling_rights;   /* bitmask CASTLE_* */
    int   ep_square;         /* SQ_NONE se nao houver */
    int   halfmove_clock;
    int   fullmove_number;
} Board;

int  board_find_king(const Board *b, Color c);   /* varredura; -1 se nao achar */
bool board_check_invariants(const Board *b);     /* 4 das 6 checagens -- Bug #7 */
void board_clear(Board *b);                      /* side_to_move fica 0 = BLACK */
void board_new(Board *new);                      /* posicao inicial */
void board_print(const Board *board);
bool fill_sq(Board *b, const char *sq_str, Piece p);

/* todos com assert(!SQ_OFFBOARD(sq)); a cor de referencia e b->side_to_move */
static inline bool is_empty(const Board *b, int sq);
static inline bool is_own  (const Board *b, int sq);
static inline bool is_enemy(const Board *b, int sq);
```

### `fen.h`

```c
#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
bool fen_parse(const char *fen_string, Board *out);   /* so escreve *out se tudo validar; Bug #1 */
void fen_write(const Board *board, char fen_out[MAX_FEN_STRING]);
```

### `move.h` — o lance e a lista de lances

```c
typedef u16 Move;
#define MOVE_NONE     ((Move)0)
#define MOVE_STR_SIZE 6

typedef enum {
    MV_QUIET       =  0, MV_DOUBLE_PUSH =  1, MV_CASTLE_KING =  2, MV_CASTLE_QUEEN =  3,
    MV_CAPTURE     =  4, MV_EP_CAPTURE  =  5, /* 6 e 7 nao existem */
    MV_PROMO_N     =  8, MV_PROMO_B     =  9, MV_PROMO_R     = 10, MV_PROMO_Q      = 11,
    MV_PROMO_CAP_N = 12, MV_PROMO_CAP_B = 13, MV_PROMO_CAP_R = 14, MV_PROMO_CAP_Q  = 15
} MoveType;

typedef struct { Move moves[MAX_MOVES]; int count; } MoveList;

/* os nove abaixo moram em move.c, fora de linha: ~28% do tempo do perft (§2) */
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
void movelist_add(MoveList *l, Move m);           /* loga e descarta se cheia -- Bug #13 */
int  movelist_find(MoveList *l, const char *uci); /* casa origem, destino e promocao; -1 */
void move_to_str(Move m, char out[6]);            /* promocao sempre minuscula */
Move move_from_str(const char *in);               /* 4 ou 5 chars; MOVE_NONE se invalido -- Bug #8 */
void print_moves(MoveList *list);
```

### `makemove.h` — aplicar e desfazer

```c
typedef struct {
    Piece captured;          /* no en passant: o peao capturado (nao EMPTY) */
    u8    castling_rights;   /* estado ANTES do lance */
    int   ep_square;
    int   halfmove_clock;
} Undo;

void make_move  (Board *b, Move m, Undo *u);
void unmake_move(Board *b, Move move, const Undo *u);
```

Ordem interna de `make_move`: decodifica → **salva o `Undo` antes de tocar no tabuleiro** →
move a peça e inverte o lado → `ep_square` → `king_square` (só se rei moveu ou foi capturado)
→ direitos de roque (origem e destino) → torre do roque → peão do en passant → promoção →
`halfmove_clock` → `fullmove_number`.

Ordem interna de `unmake_move`: inverte o lado → se promoção, volta a peça em `to` para peão
→ se roque, devolve a torre por `CASTLE_POSITIONS` → devolve a peça para `from` → repõe a
capturada (em `to`, ou atrás de `to` no en passant; se era rei, restaura o cache dele) →
restaura `ep_square`, `castling_rights`, `halfmove_clock`, decrementa `fullmove_number` se as
pretas jogaram → se a peça movida era rei, `king_square` volta para `from`.

### `movegen.h` — geração de lances e ataque

```c
void generate_pawn_moves        (Board *board, MoveList *list);   /* const perdido -- §4 */
void generate_sliding_moves     (const Board *board, MoveList *list);
void generate_king_moves        (const Board *b, MoveList *list); /* inclui roque (pseudo) */
void generate_pseudo_legal_moves(Board *b, MoveList *list);       /* limpa a lista; peao,
                                                                     deslizantes, rei, cavalo */
void generate_legal_moves       (Board *b, MoveList *list);       /* limpa a lista; aplica-e-
                                                                     testa + regras do roque */

/* side = DONO da casa (defensor); testa ataque do adversario de side -- §3 */
bool is_square_attacked(const Board *b, const int sq, const Color side);

/* generate_knight_moves(const Board *, MoveList *) existe em movegen.c mas NAO esta
   declarada aqui -- §4 */
```

### `io.h` — strings e leitura

```c
#define LINE_CAP 4096
#define WORD_CAP 256

typedef struct { const char *data; size_t len; } String;   /* fatia, nao dona */

String string_make(const char *data, size_t len);
String string_from_cstr(const char *cstr);
char  *string_to_cstr(String s);            /* malloc -- o chamador libera */
void   string_trim(String *s);              /* + _left, _right, chop_left, chop_right */
bool   string_eq(String a, String b);
bool   string_starts_with(String s, String prefix);
bool   string_parse_int(String s, int *out);
int    string_split(const char *line, String argv[], int max_split);

int  read_line(char *buf, size_t cap);      /* 1 ok, 0 EOF, -1 linha longa; trata \r */
bool read_word(char *buf, size_t cap);      /* Bug #3 */
bool read_int(int *out);
bool join_args(int argc, char **argv, int start, char *out_buf, size_t buf_cap);  /* Bug #5 */
```

### `log.h` / `utils.h` / `test.h`

```c
void log_emit(const char *level, const char *file, int line,
              const char *func, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));
#define LOG_ERROR(...) /* sempre ativo */
#define LOG_DEBUG(...) /* so com -DDEBUG */

extern const char *COLOR_CHAR[2];   /* {"BLACK", "WHITE"} */
void print_piece_chart(void);       /* sem uso */
void get_fen(char fen[MAX_FEN_STRING]);
int  get_int(const char *msg);
void clear_screen(void);
void strslc(const char *src, char *dest, int start, int end);
void fail_msg(char *msg);           /* deveria ser const char * -- Bug #7 */
void print_fail_log(void);          /* e falta um reset -- Bug #7 */
Move read_move(void);               /* sem uso */
int  read_coord(void);              /* SQ_NONE se invalida -- o menu nao confere: Bug #2 */
void wait_enter(void);
Board copy_board(Board *b);

bool run_make_unmake_tests(void);   /* test/test.h -- 18 FENs, round-trip por FEN */
```

E em `main.c`, sem header: `u64 perft(Board *, int depth)` e
`u64 perft_divide(Board *, int depth, FILE *out)` — a segunda imprime `lance: nós` e
`Nodes searched: N`, no mesmo formato do Stockfish, que é o que o `compare_perft.py` local
espera.

---

*Este documento é escrito à mão. A próxima revisão deveria ser disparada por um evento
concreto — o motor jogando uma partida inteira pelo protocolo contra a interface ou contra a
IA — não por passagem de tempo.*
