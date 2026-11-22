# Estratégia de Verificação, Validação e Testes — Xadrez-ES2

Documento consolidado dos três módulos do projeto (`Backend/engine/`, `Backend/IA/` e `interface/`).

---

## 1. Abordagem Geral e Oráculos

- **Desenvolvimento guiado por testes (TDD):** aplicado na `interface/` para entrega das funcionalidades de interação (`#4`, `#5`, `#7`, `#8`, `#9`) e evolução contínua das suítes automatizadas.
- **Infraestrutura padronizada e multiplataforma (Linux + Windows):**
  - **API compartilhada (`tools/ctest/`):** biblioteca leve própria em C11/C++17 (`tools/ctest/test_api.h`, `test_api.c`, `timer.c`) usada por todas as suítes de `Backend/engine/`, `Backend/IA/` e `interface/`.
  - **Protocolo UCI (`Backend/engine/test/protocol/run.py`):** runner Python que exercita o motor real via pipe com `stdin` aberta.
  - **Aceitação desktop (`acceptance-tests/`):** automação em Python (`pytest`) que interage com a janela real da aplicação e gera evidências (`screenshots`, `junit.xml`, `report.md`).
  - **Orquestração global (`Makefile` na raiz):** centraliza compilação, execução, testes (`make test`, `make test-full`) e cobertura consolidada por módulo (`make coverage`).
- **Oráculos utilizados:**
  - `Backend/engine/`: contagens oficiais de **Perft** (Chess Programming Wiki / Stockfish) nas 6 posições padrão e round-trip exato de FEN em `make_move`/`unmake_move`.
  - `Backend/IA/`: paridade diferencial **Minimax vs. Alfa-Beta (`v1 = v2`)**, contagem exata de nós frente ao `perft` e invariantes de score/tempo (`RNF01`).
  - `interface/`: contrato do protocolo UCI (`Backend/engine/docs/uci-comandos.md`), estado esperado do tabuleiro (`ChessBoard`) e requisitos do produto (`docs/requisitos.md`).

---

## 2. Teste de Fumaça (*Smoke Test*)

- **Arquivo:** `interface/test/smoke_test.cpp` (1 caso padronizado em `tools/ctest`: `smoke_engine_handshake_and_board`).
- **Objetivo:** validar rapidamente se o build combinado (`interface/` + `Backend/build/main.out` no Linux ou `main.exe` no Windows) inicializa o subprocesso via pipe, completa o handshake (`uci` $\rightarrow$ `uciok`, `isready` $\rightarrow$ `readyok`), encerra limpo (`stop()`) e instancia `ChessBoard` processando lances iniciais sem erros.
- **Comando:** `make -C interface test-smoke`

---

## 3. Testes de Unidade (Caixa Branca, Caixa Preta, Fronteiras, Drivers e Stubs)

Todos os módulos compartilham a mesma API leve de testes em `tools/ctest/test_api.h` (`TEST`, `EXPECT_*`, `ASSERT_*`, `test_run_all`), combinando **caixa-branca** (grafo de fluxo/$V(G)$, invariantes internos e cobertura `gcov`), **caixa-preta** (classes de equivalência, valores limite e oráculos externos) e **isolamento por *stubs***:

- **Motor (`Backend/engine/test/` — 17 casos):**
  - `test.c` *(invariantes e oráculo externo)*: `make_unmake` aplica e desfaz todos os lances legais em 18 FENs críticas (roques, *en passant*, promoções, xeque e mate), verificando restauração exata da FEN; `perft_*` valida geração de lances nó a nó nas 6 posições oficiais da CPW (até profundidade 5 por padrão).
  - `rules.c` e `search.c` *(regras e adaptador)*: alvos de rei (`king_targets`), rejeição atômica de FEN com xeque ilegal, regras de empate (`draw_rules`: 50 lances e material insuficiente) e adaptador de busca com regras reais (`search_real_draw_adapter`, preservação do tabuleiro e mate no limite de 100 meios-lances).
  - `unit_core.c` *(caixa-branca, classes de equivalência e fronteiras)*: `board_invariants_and_lifecycle` (`board_check_invariants` sob corrupções de peça, reis e peões nas fileiras 1/8, `board_clear`, `board_new`, `board_print`), `io_strings_and_join_args` (utilidades de `String` e fronteiras de buffer em `join_args`), `fen_validation_error_paths` (caminhos de erro de `fen_parse`, FEN opcional de 4 campos e round-trip `fen_write`) e `move_and_square_utilities` (promoções `n`/`b`/`r`/`q`, `SQ_NONE` e limite `MAX_MOVES` em `movelist_add`).
  - **Comando:** `make test-engine` (usa `PERFT_MAX_DEPTH=3` por padrão; `PERFT_MAX_DEPTH=5 make test-engine` para profundidade completa).
- **IA (`Backend/IA/test/unit_test.c` — 5 casos):**
  - **Caixa-branca, fronteiras e propriedades:** validação defensiva de parâmetros (`ia_verificar_chamada`), avaliação material/posicional e simetria entre cores (`ia_avaliacao`), conversão de score centipeão/mate (`ia_conversao_score`), ordenação de capturas MVV-LVA (`ia_ordenacao_mvv_lva`) e sorteio determinístico com semente fixa (`ia_sorteio`).
  - **Uso de Stub:** compilado com `Backend/IA/src/stubs.c` (`varTesteEmpate`) no lugar de `regras.c`, isolando a IA para simular estados de empate por regra de forma controlada.
  - **Comando:** `make -C Backend test-ia-unit` (ou `make test-ia` para rodar também integração e sistema da IA).
- **Interface (`interface/test/unit_test.cpp` — 21 casos):**
  - **Caixa-branca / Conjunto básico ($V(G)$) e análise de fronteiras:** `parseSquare` ($V(G) = 6$, limites `'a'`–`'h'` / `'1'`–`'8'`), `pixelToSquare`, `applyUCIMove` (normal, captura, roques, *en passant*, 4 promoções), `processUCICommand` (`position`, `moves`, `bestmove`, `state` e motivos genéricos), `handleMouseClick` (seleção, troca, desseleção, lance ilegal, bloqueio pós-jogo), alertas/pop-ups (`[X]`, `updateAlert`), HUD e relógios (`HudPanel`: `MM:SS`, `advanceClock`, `tickClock` em tempo real, timeout, modo sem tempo, histórico com scroll e controles *in-game* vs. pós-jogo), `MenuScreen` (navegação, `updateHover` e alternância de modo/dificuldade/cor/tempo), inversão de tabuleiro (`boardFlipped`), animações de peça (`updateAnimations`, `isPieceAnimated`) e viewport *letterbox* (`16:9`, `4:3`, `0×0`).
  - **Renderização headless (Linux + Windows):** `render_headless_board_hud_and_menu` usa uma `sf::RenderWindow` oculta (`setVisible(false)`) exercitando `draw()` de `MenuScreen`, `ChessBoard`, `HudPanel`, `Piece` e `ui_helpers.hpp` (portável em Linux/Xvfb e Windows).
  - **Uso de Driver e Stub multiplataforma:** `engine_bridge_with_stub` testa os pipes do `EngineBridge` (múltiplas linhas, CRLF, reinício e `stop()`) isolados do motor usando `/bin/cat` no Linux e modo de eco próprio (`XADREZ_ECHO_STUB=1`) no Windows.
  - **Comando:** `make -C interface test-unit`

---

## 4. Cobertura de Testes (`gcov`)

O projeto possui coleta automatizada de cobertura por módulo a partir da raiz (`make coverage`), instrumentando `Backend/engine/`, `Backend/IA/` e `interface/` com `--coverage` + `gcov -r` e consolidando o resultado em [`tools/coverage.md`](../tools/coverage.md) (via `tools/coverage_report.sh`), além dos arquivos `.gcov` detalhados em `Backend/build/cov/` e `interface/build/cov/`:

| Módulo | Arquivos | Linhas Cobertas | Linhas Executáveis | Cobertura (`gcov`) |
| :--- | :---: | :---: | :---: | :---: |
| `Backend/engine` | 16 | 1315 | 1450 | **90,69%** |
| `Backend/IA` | 5 | 403 | 412 | **97,82%** |
| `interface` | 10 | 1357 | 1452 | **93,46%** |
| **TOTAL PROJETO** | **31** | **3075** | **3314** | **92,79%** |

- **Comandos:** `make coverage` (projeto inteiro), `make coverage-backend` e `make coverage-interface`.
- **Justificativa dos 7,21% não cobertos (239 linhas):**
  - **Código legado do antigo REPL interativo (109 linhas):** helpers de terminal não chamados no modo UCI (`Backend/engine/src/utils.c`, `main.c:perft_divide` e `read_word`/`read_int` em `io.c`).
  - **Limitações mecânicas de instrumentação (72 linhas):** ramo do processo filho entre `fork()` e `execlp()` em `engine_bridge.cpp` (o `exec` substitui o processo antes de gravar `.gcda`) e instâncias `inline` não invocadas em `ui_helpers.hpp`.
  - **Guardas defensivas inalcançáveis (58 linhas):** falhas de syscalls do SO (`pipe() < 0`, `fork() < 0`, `clock() == -1`), truncamento de buffer em `fen_write`, captura de rei (impossível em lances legais) e `default` de enums já exauridos.

---

## 5. Testes de Integração (Incremental *Bottom-Up*)

- **Camada 1 — Protocolo UCI (`Backend/engine/test/protocol/run.py` + 13 pares `.in`/`.out`):**
  - Sobe `Backend/build/main.out` (ou `main.exe`) como subprocesso real com `stdin` aberta em pipe, validando handshake, `position`, `legalmoves`, promoções, *en passant*, xeque-mate, afogamento, empates (50 lances, material, repetição), precedência de mate, `go perft`, encerramento por EOF, `--trace <arquivo>`, `d`, `line-too-long` (> 8192 bytes), `too-many-moves`, argumentos CLI inválidos e tempos de resposta dos níveis 1, 2 e 3.
  - **Comando:** `make test-protocol`
- **Camada 2 — IA + Motor (`Backend/IA/test/integration_test.c` — 2 casos):**
  - Compila a busca da IA diretamente com os módulos de `Backend/engine/src/` (`generate_legal_moves`, `make_move`/`unmake_move`), validando paridade de score e contagem de nós entre Minimax e Alfa-Beta (`ia_v1_v2_e_nos`) e funcionamento da busca de quiescência com *stand-pat* e *delta pruning* (`ia_quiescencia`).
  - **Comando:** `make -C Backend test-ia-integration`
- **Camada 3 — Interface + Motor Real + IA (`interface/test/integration_test.cpp` — 10 casos):**
  - Substitui o *stub* de eco pelo binário real `Backend/build/main.out` (`main.exe` no Windows): handshake + 20 lances iniciais, múltiplos turnos via clique (`playTurnWithEngine`), Mate do Louco com `state`, xeque-mate disparado pelo jogador (`1. e4 g5 2. d4 f6 3. Qh5#`), partida completa da `startpos` até estado terminal, botões do HUD (*Desistir* / *Nova Partida* / fechar alerta `[X]`), modo local PvP sem IA, partida contra IA jogando de Pretas (tabuleiro invertido), troca dos 3 níveis de dificuldade (`setoption name Nivel`) e fluxo assíncrono da IA com delay natural, bloqueio de cliques durante pensamento e cancelamento limpo ao desistir.
  - **Comando:** `make -C interface test-integration`

---

## 6. Testes de Sistema, Profiling e Regressão Contínua (CI)

- **Sistema da IA (`Backend/IA/test/system_test.c` — 2 casos):**
  - Valida respeito estrito ao limite de tempo (`ia_limite_tempo`) e comportamento/profundidade dos três níveis de dificuldade Fácil, Médio e Difícil (`ia_niveis`), garantindo conformidade com o `RNF01` (`< 200 ms` nos níveis básicos, `< 1 s` no difícil).
  - **Comando:** `make -C Backend test-ia-system`
- **Sistema da Interface (`interface/test/system_test.cpp` — 2 casos):**
  - **Recuperação (`system_recovery`):** força binário inexistente, comandos malformados (`error bad-command position`) e lances ilegais (`error illegal-move`), garantindo integridade atômica do estado e recuperação do pipe (detectou e corrigiu bug real de `SIGPIPE` em `EngineBridge::stop()`).
  - **Estresse e Latência (`system_stress_and_latency_rnf01`):** executa 40 meios-lances consecutivos integrando `ChessBoard` + pipe + motor com IA real (nível padrão 2), medindo média de **~78–120 ms** por turno (abaixo do limite médio de **200 ms** do `RNF01`).
  - **Comando:** `make -C interface test-system`
- **Profiling de Memória (`valgrind`):**
  - `make -C interface profile` executa `valgrind --leak-check=full --error-exitcode=1 ./build/system_test` (zero `definitely lost` e zero erros de acesso inválido de memória).
- **Regressão Contínua no GitHub Actions (`.github/workflows/ci.yml`):**
  - Disparado automaticamente em todo `pull_request` para `main`, `push` na `main` e `workflow_dispatch`, com **6 jobs em paralelo** cobrindo **Linux** (`ubuntu-latest` / `ubuntu-22.04` com `xvfb-run`) e **Windows** (`windows-latest` com MinGW UCRT64 + CMake + SFML 2.6.2):
    - `backend-tests` e `backend-tests-windows` (`make test-engine` com `PERFT_MAX_DEPTH=3` + `make test-ia`);
    - `protocol-tests` e `protocol-tests-windows` (`Backend/engine/test/protocol/run.py`);
    - `interface-tests` (Linux/Xvfb: smoke, unidade, integração e sistema) e `interface-tests-windows` (Windows: smoke, unidade, integração, sistema + 10 testes de aceitação desktop com publicação de artefatos).
  - Qualquer falha em qualquer suíte interrompe o job e bloqueia o PR. Localmente, toda a regressão é rodada com `make test` (ou `make test-full` para Perft profundidade 5).

---

## 7. Testes de Aceitação (Automatizados Desktop, Alfa e Beta)

- **Critérios de Homologação (ligados a `docs/requisitos.md`):**
  - Funcionais: seleção de modo, cor, dificuldade e tempo (`RF01`–`RF03`), tabuleiro 8×8 e peças (`RF04`), clique e movimentação (`RF05`), destaque de lances válidos (`RF06`), cronômetros (`RF07`), alertas visuais de xeque/captura/inválido/fim (`RF08`), regras especiais — roque, *en passant* e promoção (`RF09`/`RF11`), detecção de fim de jogo (`RF12`), turnos (`RF13`) e histórico (`RF20`).
  - Não funcionais: latência `< 200 ms` / `< 1 s` (`RNF01`) e usabilidade/responsividade em redimensionamento (`RNF06`).
- **Suíte Automatizada de Aceitação Desktop (`acceptance-tests/` — 10 cenários `pytest`):**
  - Opera a janela gráfica real do executável validando ponta a ponta (executada automaticamente no CI Windows com validação de `10 PASS / 0 FAIL / 0 SKIP` e captura de screenshots):
    1. Abertura do menu e encerramento pelo botão *Sair*;
    2. Configuração de partida contra IA (dificuldade, cor, tempo) e navegação com `ESC`;
    3. Partida local PvP com destaques de lances válidos e atualização do histórico;
    4. Partida contra IA com resposta visível no tabuleiro;
    5. Lance ilegal com alerta visual, desistência e retorno ao menu;
    6. Redimensionamento de janela mantendo interatividade (*letterbox*);
    7. Mate do Louco e reinício pelos controles de pós-jogo;
    8. Captura *en passant* via clique do mouse;
    9. Roque curto via clique do mouse;
    10. Promoção de peão via clique do mouse.
- **Teste Alfa (manual — ambiente controlado da equipe):**
  - Execução observada de `make run` validando fluxo completo de partida, pop-ups, relógio, animações, histórico e controles; defeitos encontrados corrigidos antes do merge (ex.: sincronização das linhas `info`/`bestmove` do `go` e ordem de inicialização X11/`fork`).
- **Teste Beta (manual — usuários externos):**
  - Sessões livres com usuários fora da equipe avaliando clareza visual, usabilidade e fluidez, utilizando **GitHub Issues** como registro formal de solicitações e melhorias (ex.: controles de *Desistir* / reinício no HUD da Issue `#8` e fluxo de menu da Issue `#9`).
