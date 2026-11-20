# Contexto do motor de xadrez

Ponto de partida para quem vai mexer no motor (`engine/`): o que existe, as decisões que
valem hoje e onde cada coisa mora. O protocolo está em [`uci-comandos.md`](uci-comandos.md).
Se este documento e o código divergirem, vale o código.

## 1. O que é

Motor de xadrez em C11: regras completas e um protocolo de texto no estilo UCI. O binário é
`engine/build/main.out` (`main.exe` no Windows). O repositório tem três partes:

| Parte | O que é | Como fala com o motor |
|---|---|---|
| `engine/` | Regras, FEN, perft e protocolo | — |
| `IA/` | Busca minimax/alfa-beta e avaliação, em C | No mesmo processo: compilada pelo `engine/Makefile` e chamada pelo `go` |
| `interface/` | Cliente gráfico em C++/SFML | Por subprocesso: escreve comandos na stdin do motor e lê a stdout |

A posição trafega como FEN completa (`position fen ...`) ou como `startpos` mais a lista de
lances. O motor não sabe nada da interface, e a interface não implementa regras.

## 2. Estado atual

- **Regras completas**: todas as peças, roque, en passant, promoção nas quatro peças e
  filtro de legalidade. Provadas por perft exato nas seis posições de referência da Chess
  Programming Wiki (inicial, Kiwipete e posições 3 a 6), com os valores esperados em
  `engine/test/test.c`.
- **FEN**: leitura com validação completa e escrita dos seis campos.
- **Protocolo**: `uci`, `isready`, `ucinewgame`, `setoption`, `position`, `legalmoves`,
  `state`, `go`, `go perft`, `d`, `quit` e a opção `--trace`.
- **Fim de partida**: xeque, mate, afogamento, 50 lances, material insuficiente e repetição
  tripla, informados por `state`.
- **IA no `go`**: alfa-beta com aprofundamento iterativo, três níveis
  (`setoption name Nivel`) e conferência do lance escolhido contra a lista legal.

Como verificar (a partir da raiz):

| Comando | O que cobre |
|---|---|
| `make -C engine` | Build do motor com a IA (`engine/build/main.out`) |
| `make -C engine test` | Suíte C: perft das seis posições até a profundidade 5, ida e volta make/unmake, empates, FEN com xeque ilegal e a IA ligada ao motor. Cerca de 1 minuto |
| `PERFT_MAX_DEPTH=3 make -C engine test` | A mesma suíte com perft mais raso |
| `TEST_FILTER=perft_pos3 make -C engine test` | Só os testes cujo nome contém o texto |
| `python3 engine/test/protocol/run.py` | Transcrições do protocolo, transporte com a stdin aberta, legalidade do `go`, limites de busca, empates e tempo de resposta por nível. Exige o motor compilado |

A suíte isolada da IA (`IA/src/teste.c`, com `main` próprio) se compila à parte: `teste.c`,
`ia.c`, `avaliar.c`, `ordenar.c`, `niveis.c` e `stubs.c`, mais os fontes de `engine/src`
menos `main.c` e `uci.c`, com `-Iengine/include -IIA/header`.

## 3. Decisões arquiteturais

Decisões firmes e vigentes. Mudar qualquer uma afeta o perft, o protocolo ou a IA.

### Representação

- **Mailbox de 64 casas** (`Piece array[64]` em `Board`), não bitboards: simples, e os 64
  bytes cabem numa linha de cache.
- **`a1 = 0`**: `sq = rank * 8 + file`, `h8 = 63`. Peão branco avança `+8`, preto `-8`. A FEN
  vem da fileira 8 para a 1, então sua primeira fileira ocupa os índices 56 a 63.
- **Peça em um byte**: `Piece = (color << 3) | type`, com `WHITE = 1`, `BLACK = 0`,
  `EMPTY = 0` e tipos de `PAWN = 1` a `KING = 6`. Use sempre `PIECE_TYPE()` e
  `PIECE_COLOR()`. Como `PIECE_COLOR(EMPTY) == BLACK`, teste "casa vazia" antes da cor.
- **`Board` completa**: peças, lado a jogar, `king_square[2]`, direitos de roque (bitmask
  `CASTLE_*`), `ep_square` (`SQ_NONE` sem en passant), relógio de meio-lances e número do
  lance. É tudo o que a FEN descreve, e `fen_write` devolve os seis campos.
- **`Move` é `u16`**: bits 0–5 origem, 6–11 destino, 12–15 tipo (`MoveType`, esquema
  *Encoding Moves* da CPW). Pequeno, e dois lances se comparam por igualdade.

  | `MoveType` | Código |
  |---|---|
  | quieto, avanço duplo | 0, 1 |
  | roque curto, roque longo | 2, 3 |
  | captura, captura en passant | 4, 5 |
  | promoção a N, B, R, Q | 8 a 11 |
  | promoção com captura a N, B, R, Q | 12 a 15 |

  Só dois bits são bits de verdade: `& 4` é captura e `& 8` é promoção, e só
  `move_is_capture`/`move_is_promotion` usam máscara. Roque e en passant são **códigos**
  comparados por igualdade. `MOVE_NONE == 0` (a1a1) nunca é gerado.

### Geração e aplicação de lances

- **Tabelas pré-calculadas** em `init_square_tables()`: `SQ_TO_EDGE[64][8]` (casas até a
  borda em cada direção), `KNIGHT_ATTACKS`, `PAWN_ATTACKS` e `KING_TARGETS`, com `SQ_NONE`
  para destino fora do tabuleiro. Evitam testar borda durante a geração. `main()` e
  `fen_parse` as inicializam; quem usar o núcleo sem eles precisa chamar antes.
- **Ordem de `Direction` é estrutural**: ortogonais 0–3, diagonais 4–7. Torre, bispo e dama
  usam a mesma função, variando só o intervalo de direções.
- **Pseudo-legal primeiro, filtro depois**: `generate_legal_moves` aplica cada lance
  pseudo-legal, testa se o próprio rei ficou atacado e desfaz. É simples e o perft prova. No
  roque, o gerador confere direito, casas vazias e torre; o filtro confere que o rei não está
  em xeque e que a casa atravessada (`(from + to) / 2`) não está atacada.
- **`Undo` é do chamador**: `make_move(b, m, &u)` e `unmake_move(b, m, &u)` com `Undo u;`
  local. A pilha da recursão é a pilha de undo, sem estado global. `Undo` guarda só o que não
  se recupera: peça capturada, roque, en passant e relógio de meio-lances.
- **`king_square` é cache**: `make_move`/`unmake_move` só o atualizam quando o rei se move ou
  é capturado, e o teste de xeque não precisa varrer o tabuleiro.
- **A fronteira valida, o núcleo confia**: `make_move` não confere legalidade, porque o
  filtro precisa aplicar lances possivelmente ilegais. Quem recebe texto (`position`) procura
  o lance na lista de `generate_legal_moves` e aplica o `Move` gerado, que já traz o tipo
  certo (por exemplo, en passant em vez de captura comum).

### Ataque e fim de partida

- **`is_square_attacked(b, sq, side)`: `side` é o DEFENSOR**, o dono da casa, não o atacante.
  `in_check(b)` faz a pergunta para o rei do lado a jogar. Inverter o argumento compila sem
  aviso e inverte o resultado. Os dois ficam em `attack.h`, sem depender do gerador, para o
  `fen_parse` e a IA poderem usá-los.
- **Empate em duas camadas**: `draw_reason(b)` (`draw.c`) olha só o tabuleiro e responde 50
  lances (relógio ≥ 100, encerra sem reclamação) e material insuficiente. Repetição precisa
  do histórico, por isso fica no protocolo (`uci.c`).
- **Mate e afogamento vêm antes dos empates**: quem chama verifica a lista legal primeiro;
  mate no centésimo meio-lance é mate.
- **Material insuficiente**: rei contra rei, uma única peça menor, ou só bispos, todos na
  mesma cor de casa. K+NN×K e bispos em cores opostas não são empate.
- **Repetição tripla**: o protocolo guarda a chave de cada posição da última `position`
  (até 1025). A chave tem peças, lado, roque e a casa de en passant apenas se há captura en
  passant legal; os relógios não entram. Só se comparam posições dentro da janela do relógio
  de meio-lances.

### FEN

- **FEN inconsistente é rejeitada, não corrigida**: direito de roque sem rei e torre nas casas
  de origem, en passant impossível, peão na primeira ou última fileira, número de reis
  diferente de um por cor, mais de 8 peões ou 16 peças por cor, relógios fora da faixa
  (meio-lances 0–1000, lance 1–9999) e o lado que não joga em xeque. Corrigir em silêncio
  esconderia bugs de `make_move`.
- **`fen_parse` é atômico**: monta num `Board` local e só escreve na saída se tudo validar.
  Aceita de 4 a 6 campos (relógios ausentes valem 0 e 1); o protocolo exige os 6. O motivo da
  recusa vai para a stderr por `LOG_ERROR`.

### Protocolo e processo

- **Processo com estado, cliente dono da partida**: o motor guarda uma posição (a última
  aceita por `position`) e o histórico dela; `state`, `legalmoves` e `go` se referem a ela. O
  cliente guarda a partida e reenvia `position ... moves ...` inteira a cada lance, o que dá ao
  motor o histórico para a repetição.
- **`position` é atômico**: monta posição e histórico em rascunho e só troca a sessão no fim.
  Um lance ilegal no meio da lista deixa a posição anterior intacta.
- **`go` não muda a sessão**: a busca roda numa cópia do `Board`, e o lance não é aplicado.
- **Uma saída só**: toda linha passa por `uci_send`, que dá `fflush` por linha (num pipe a libc
  usa buffer de bloco, e no Windows `_IOLBF` não basta) e grava o `--trace`.
- **Reprodutível**: o sorteio entre lances empatados usa um xorshift próprio com semente fixa,
  reiniciada por `ucinewgame`, e não o `rand()` da libc. A mesma sequência de comandos sem
  relógio gera as mesmas respostas em qualquer plataforma.
- **Uma thread**: o motor não lê a stdin durante `go`. Por isso não há `stop`, e `go perft`
  tem teto de profundidade 8.
- **Integração por subprocesso**: a interface executa o binário e conversa por pipe, sem
  servidor no meio. Comando desconhecido é ignorado, como manda o UCI.

### Contrato com a IA (cabeçalhos congelados)

- **Build**: `engine/Makefile` compila `IA/src/ia.c`, `avaliar.c`, `ordenar.c`, `niveis.c` e
  `regras.c` com `-I../IA/header`. `stubs.c` e `teste.c` ficam fora; `regras.c` e `stubs.c`
  nunca entram juntos.
- **Fluxo do `go`**: `uci.c` lê os parâmetros do nível (`parametrosNivel`, `niveis.h`),
  aplica `depth`/`movetime`, chama `escolherJogada` (`ia.h`) com o sorteio da IA desligado e
  confere `JOGADA.resultado`. Se o nível pede sorteio, escolhe entre `CANDIDATOS` com o
  xorshift do protocolo. O lance final é conferido contra a lista legal; qualquer falha
  responde `bestmove 0000`.
- **O que a IA usa do motor**: `generate_legal_moves`, `make_move`/`unmake_move`,
  `in_check`, `is_square_attacked`, `movelist_*`, os predicados de `Move`, as macros
  `PIECE_*`, `RANK_OF`/`FILE_OF` e as tabelas `SQ_TO_EDGE`, `DIR_OFFSET`, `KNIGHT_ATTACKS`,
  `PAWN_ATTACKS` e `PAWN_PUSH`.
- **Empates na busca**: a IA chama `testeEmpateRegra` (`pendenciasRegras.h`), que
  `IA/src/regras.c` traduz de `draw_reason`. A repetição não entra na árvore de busca; só o
  protocolo a detecta.
- **Congelado** (mudança incompatível exige aviso e acordo prévio com a equipe da IA até a
  apresentação final): assinaturas, codificação e semântica de `board.h`, `piece.h`, `move.h`,
  `makemove.h`, `movegen.h`, `square.h` e `perft.h`; das funções `generate_legal_moves`,
  `make_move`, `unmake_move`, `move_is_capture`, `move_is_ep_capture`, `move_from`, `move_to`,
  `move_to_str`, `movelist_find`, `is_square_attacked` e `perft`; e das tabelas `SQ_TO_EDGE`,
  `DIR_OFFSET`, `KNIGHT_ATTACKS`, `PAWN_ATTACKS`, `PAWN_PUSH`, `RANK_OF` e `FILE_OF`,
  incluindo índices e o sentinela `SQ_NONE`. `KING_TARGETS[sq][dir]` guarda o vizinho na
  direção `dir` ou `SQ_NONE`.

### Build e convenções

- **Makefile**: `make -C engine` (alvo `all`) e `make -C engine test`. No Windows, com
  `mingw32-make`, o binário é `build/main.exe`. Os alvos `debug` e `release` estão quebrados:
  usam `$(SRC)`, que não existe.
- **Warnings fazem parte do acordo**: C11 com `-Wall -Wextra -Wpedantic -Wstrict-prototypes
  -Wmissing-prototypes`, em `-Og -g` (sem otimização o GCC não faz a análise de fluxo que
  acusa variável não inicializada). O build ainda tem avisos antigos; não acrescente novos nem
  os silencie.
- **Headers como contrato**: todo `.c` inclui o próprio `.h` primeiro, e cada header compila
  sozinho.

## 4. Mapa dos módulos

`engine/include` (headers) e `engine/src` (implementação). "Legado" marca código que compila
mas não está no caminho do protocolo.

| Arquivo | Responsabilidade |
|---|---|
| `types.h` | Inteiros de largura fixa (`u8` a `u64`) e limites (`BOARD_SIZE`, `MAX_MOVES`, `MAX_FEN_STRING`) |
| `piece.h/c` | `Color`, `PieceType`, `Piece`, macros `PIECE_*` e conversão peça ↔ caractere da FEN |
| `square.h/c` | Geometria: `SQ_*`, `RANK_OF`/`FILE_OF`, direções, tabelas pré-calculadas e casa ↔ coordenada |
| `board.h/c` | `Board`, direitos de roque, `is_empty`/`is_own`/`is_enemy`, `board_print` (do `d`), `fill_sq`, `board_find_king`. `board_new` e `board_check_invariants`: legado |
| `fen.h/c` | `fen_parse` (valida, atômico), `fen_write` e `START_FEN` |
| `move.h/c` | `Move`, `MoveType`, `MoveList`, codificação, predicados, lance ↔ texto UCI e `movelist_find` |
| `movegen.h/c` | Geração pseudo-legal por peça e `generate_legal_moves` |
| `makemove.h/c` | `Undo`, `make_move` e `unmake_move` |
| `attack.h/c` | `is_square_attacked` (por defensor) e `in_check` |
| `draw.h/c` | `draw_reason`: 50 lances e material insuficiente |
| `perft.h/c` | `perft(board, depth)` sobre lances legais |
| `uci.h/c` | Protocolo: laço de leitura, comandos, sessão, histórico, repetição e chamada da IA |
| `io.h/c` | `String` sem posse, `string_split` e `read_line` (usados pelo protocolo). `read_word`, `read_int` e `join_args`: legado |
| `utils.h/c` | `strslc` (usado por `move_from_str`) e `fail_msg`. O resto (leituras do menu, `clear_screen`, `copy_board`...): legado |
| `log.h/c` | `LOG_ERROR` (sempre, na stderr) e `LOG_DEBUG` (só com `-DDEBUG`) |
| `main.c` | `main()`: inicializa as tabelas e chama `uci_main`. Menu `ui()`, `perft_divide` e IA aleatória: legado, sem chamada |

`IA/src` (compilados com o motor, exceto os dois últimos):

| Arquivo | Responsabilidade |
|---|---|
| `ia.c` | `escolherJogada`: minimax e alfa-beta com aprofundamento iterativo, relógio e quiescência |
| `avaliar.c` | Avaliação: material e tabelas peça-casa |
| `ordenar.c` | Ordenação MVV-LVA, com o melhor lance da iteração anterior primeiro |
| `niveis.c` | Tabela nível → parâmetros da busca |
| `regras.c` | Adaptador `draw_reason` → `testeEmpateRegra` |
| `stubs.c` | `testeEmpateRegra` controlável, só para a suíte isolada da IA |
| `teste.c` | Suíte isolada da IA, com `main` próprio |

`engine/test`:

| Arquivo | Responsabilidade |
|---|---|
| `test.c` | Perft das seis posições da CPW, ida e volta make/unmake e o `main` da suíte |
| `rules.c` | `KING_TARGETS`, recusa de FEN com xeque ilegal e `draw_reason` |
| `search.c` | IA ligada ao motor: empates na raiz, lance legal, mate no centésimo meio-lance |
| `test.h` | Declara `run_make_unmake_tests`, que não existe mais: legado |
| `ctest/` | Mini-framework (`TEST`, `EXPECT_*`, `ASSERT_*`, `TEST_FILTER`); `timer.c` sem uso |
| `protocol/` | `run.py` e as transcrições `NN-nome.in` com a saída esperada `NN-nome.out` |

## 5. Possíveis melhorias

Qualquer otimização mantém o perft das seis posições exato e mede o ganho. O tempo varia
entre execuções (até ~20% em CPU híbrida sob WSL2): compare medianas de 10 execuções
intercaladas. `gprof` (`-pg`) e `perf` funcionam; no WSL2 o `perf` não tem contadores de
hardware.

- Acessores de `Move` como `static inline` em `move.h`: são as funções mais chamadas do perft
  e hoje só são expandidas com `-flto`.
- `CASTLE_MASK[64]` no lugar dos dois `switch` de `check_rook_squares`: uma operação por lance.
- Torre do roque por índice (`CASTLE_POSITIONS`), não por `fill_sq(b, "d1", ...)`: tira
  conversão de texto do `make_move`.
- `check_allowed_castles` ler `king_square` em vez de `board_find_king`: o cache já é
  confiável.
- Uma varredura de geração em vez de três: peões, deslizantes e cavalos varrem as 64 casas
  cada um.
- Contagem em massa no perft (na profundidade 1, devolver `list.count`): vale porque a lista
  já é legal, e acelera a suíte.
- Filtro de legalidade com peças cravadas: só lances do rei, de peças cravadas, en passant e
  lances em xeque precisariam do aplica-e-testa, que hoje custa um make/unmake por lance.
- `KING_TARGETS` na geração e no ataque do rei: a tabela já existe e está testada.
- Hash Zobrist incremental: chave de repetição em 64 bits, repetição dentro da busca e base
  para uma tabela de transposição.
- Na IA, killer moves/history e tabela de transposição: menos nós para a mesma profundidade.
- `stop` com uma thread leitora: hoje um `go` longo (por exemplo `go movetime 0` sem `depth`)
  deixa o motor inalcançável.
- Relógio de partida (`wtime`, `btime`, `winc`, `binc`): hoje ignorado; o prazo vem só do
  nível ou de `movetime`.
- Recusar o quinto caractere em lance que não é promoção: hoje `e2e4x` vale `e2e4`.
- Consertar `make debug` e `make release`: devolvem os sanitizers e o build otimizado.
- Zerar os avisos atuais e ligar `-Wshadow`, `-Wwrite-strings`, `-Wcast-qual` e
  `-Wconversion`: mais bugs pegos na compilação.
- Remover o legado sem chamada (menu, `perft_divide`, IA aleatória, helpers de REPL,
  `test.h`) e usar `board_check_invariants` no teste de make/unmake ou apagá-la.
- Corrigir os comentários que citam `protocolo.md` (inexistente) e as "dívidas" de `uci.h`
  que já foram resolvidas.
- Um alvo `bench` com posições fixas, para medir Mnós/s e nós da busca de forma reproduzível.
