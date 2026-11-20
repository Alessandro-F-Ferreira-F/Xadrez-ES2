# Estratégia de Verificação, Validação e Testes — Xadrez-ES2

Documento consolidado dos três módulos do projeto (`engine/`, `IA/` e `interface/`).

---

## 1. Abordagem Geral e Oráculos

- **Desenvolvimento guiado por testes (TDD):** aplicado na `interface/` para entrega das funcionalidades de interação (`#4`, `#5`, `#7`, `#8`) e suíte de testes (`#41`).
- **Infraestrutura própria (sem frameworks externos):**
  - `engine/`: API própria em C11 (`engine/test/ctest/`) + runner Python via pipe (`engine/test/protocol/run.py`).
  - `IA/`: suíte própria em C11 (`IA/src/teste.c`).
  - `interface/`: suíte em C++17 padrão (`interface/test/`).
- **Oráculos utilizados:**
  - `engine/`: contagens oficiais de **Perft** (Chess Programming Wiki / Stockfish) nas 6 posições padrão e round-trip de FEN em `make_move`/`unmake_move`.
  - `IA/`: paridade entre variantes **Minimax vs. Alfa-Beta (`v1 = v2`)**, contagem exata de nós pelo `perft` e invariantes de score/tempo (`RNF01`).
  - `interface/`: protocolo UCI documentado (`engine/docs/uci-comandos.md`), estado esperado do tabuleiro (`ChessBoard`) e requisitos do produto (`docs/requisitos.md`).

---

## 2. Teste de Fumaça (*Smoke Test*)

- **Arquivo:** `interface/test/smoke_test.cpp`
- **Objetivo:** validar rapidamente se o build combinado (`interface/` + `engine/build/main.out`) inicializa o subprocesso via pipe, completa o handshake (`uci` $\rightarrow$ `uciok`, `isready` $\rightarrow$ `readyok`), encerra limpo (`stop()`) e instancia `ChessBoard` sem erros.
- **Comando:** `make -C interface test-smoke`

---

## 3. Testes de Unidade (Caixa Branca, Fronteiras, Drivers e Stubs)

- **Motor (`engine/test/test.c`):**
  - `make_unmake`: testa aplicação e reversão completa de todos os lances legais em 18 FENs críticas (roques, *en passant*, promoções, xeque e mate), verificando invariância da FEN antes/depois.
  - `perft_*` (6 posições da CPW até profundidade 5 por padrão): valida geração de lances legais, pinos, xeque e desfazer lances nó a nó.
  - **Comando:** `make -C engine test` (ou `PERFT_MAX_DEPTH=3 make -C engine test`).
- **IA (`IA/src/teste.c` — 9 blocos):**
  - Validação defensiva de parâmetros (`verificarChamada`), conversão de score (`converterScore`), avaliação material/posicional e simetria de cores (~122 mil nós), paridade `v1 = v2` + determinismo, ordenação MVV-LVA, quiescência (stand-pat antecipado + delta pruning), sorteio de candidatos, níveis de dificuldade (`niveis.c`) e controle de tempo (`RNF01`).
  - **Uso de Stub:** `IA/src/stubs.c` (`varTesteEmpate`) simula empates por regra isolando a IA das regras ainda pendentes no motor.
- **Interface (`interface/test/unit_test.cpp` — 12 casos):**
  - **Caixa branca / Conjunto básico ($V(G)$) e fronteiras:** `parseSquare` ($V(G) = 6$, incluindo limites `'a'`–`'h'` / `'1'`–`'8'` e strings inválidas), `pixelToSquare`, `applyUCIMove` (normal, captura, roques, *en passant*, 4 promoções), `processUCICommand` (`position`, `moves`, `bestmove`, `state`), `handleMouseClick` (seleção, troca, desseleção, lance ilegal, bloqueio pós-fim de jogo), alertas/pop-ups (`[X]`, captura, xeque, mate, afogamento, 50 lances) e HUD (`HudPanel`: formatação `MM:SS`, relógio regressivo, timeout, histórico e botões *Desistir* / *Nova Partida*).
  - **Uso de Driver e Stub:** `test_engine_bridge_with_stub` usa `/bin/cat` como *stub* de subprocesso para testar os pipes do `EngineBridge` (múltiplas linhas, CRLF e `stop()`) isolados da `engine/`.
  - **Comando:** `make -C interface test-unit`

---

## 4. Cobertura de Testes (`gcov`)

- **Interface (`interface/`):**
  - Alvo automatizado `make -C interface coverage` (compila com `--coverage` e gera arquivos `.gcov` em `build/cov/`).
  - Medição registrada em `16/11` (pré-extração de `hud_panel.cpp`, detalhada em `interface/docs/relatorio_testes.md`): **88,16%** total (`335/380` linhas executáveis — `board.cpp`: 91,67%, `piece.cpp`: 81,25%, `engine_bridge.cpp`: 77,91%, `piece.hpp`: 100%).
  - **Pendência:** incluir `src/hud_panel.cpp` no alvo `coverage` do `interface/Makefile` e remedir.
- **Motor (`engine/`) e IA (`IA/`):**
  - Suítes funcionais completas; medição formal de `%` de linhas via `gcov` ainda pendente.

---

## 5. Testes de Integração (Incremental *Bottom-Up*)

- **Camada 1 — Protocolo UCI (`engine/test/protocol/run.py` + 13 cenários `.in`/`.out`):**
  - Sobe `engine/build/main.out` como subprocesso real com `stdin` aberta em pipe, validando handshake, `position`, `legalmoves`, promoções, *en passant*, xeque-mate, afogamento, 50 lances, tratamento de erros (`illegal-move`, `bad-fen`, `bad-command`), `go perft` e encerramento por EOF.
  - **Comando:** `python3 engine/test/protocol/run.py`
- **Camada 2 — IA + Motor (`IA/src/teste.c`):**
  - Compila a IA diretamente com os módulos de `engine/src/` (exceto `main.c`), validando `generate_legal_moves`, `make_move`/`unmake_move` e igualdade na contagem de nós frente ao `perft`.
- **Camada 3 — Interface + Motor Real (`interface/test/integration_test.cpp` — 6 casos):**
  - Substitui o *stub* (`/bin/cat`) pelo binário real `engine/build/main.out`: handshake + 20 lances iniciais, múltiplos turnos via `playTurnWithEngine`, Mate do Louco sincronizado com `state`, xeque-mate disparado por clique do jogador (`1. e4 g5 2. d4 f6 3. Qh5#`), botões do HUD (*Desistir* / *Nova Partida*) ressincronizando o motor e simulação de **partida completa** da `startpos` até o fim (`421` meios-lances, `fifty-moves`).
  - **Comando:** `make -C interface test-integration`

---

## 6. Testes de Sistema, Profiling e Regressão

- **Recuperação e Estresse/Carga (`interface/test/system_test.cpp` — 2 casos):**
  - **Recuperação:** testa falha com binário inexistente, comandos malformados e lances ilegais, garantindo integridade do estado anterior e recuperação do pipe (revelou e corrigiu bug real de `SIGPIPE` em `EngineBridge::stop()`).
  - **Estresse e Latência (`RNF01`):** executa 40 meios-lances contínuos (`position` + `state` + `legalmoves` + `go`) medindo média de **0,15–0,21 ms** e máximo de **1,18–1,35 ms** por turno (limite do `RNF01`: `< 200 ms`).
  - **Comando:** `make -C interface test-system`
- **Profiling de Memória (`valgrind`):**
  - `make -C interface profile` executa `valgrind --leak-check=full --error-exitcode=1 ./build/system_test` (medido em `16/11`: **0 bytes `definitely lost`** e **0 erros** em 58.900 alocações).
- **Medição de Desempenho (`engine/` e `IA/`):**
  - `IA/` instrumentada por contagem de nós (`NOS_VISITADOS`, `NOS_QUIESCENCIA`, `PROF_ALCANCADA`) e teto de tempo por nível (`< 200 ms` no Fácil/Médio, `< 1 s` no Difícil).
- **Regressão:**
  - Executada localmente via `make -C engine test`, `python3 engine/test/protocol/run.py` e `make -C interface test` antes de cada merge; configuração de pipeline no GitHub Actions prevista para automação contínua.

---

## 7. Testes de Aceitação (Alfa e Beta)

- **Critérios de Homologação (ligados a `docs/requisitos.md`):**
  - Funcionais: tabuleiro 8×8 e peças (`RF04`), clique e movimentação (`RF05`), destaque de lances válidos (`RF06`), cronômetros (`RF07`), alertas visuais de xeque/captura/inválido/fim (`RF08`), regras especiais (`RF11`), detecção de fim de jogo (`RF12`), turnos (`RF13`), histórico (`RF20`), modos de jogo e níveis da IA (`RF01`–`RF03`, pendentes de ligação final na demo).
  - Não funcionais: latência `< 200 ms` / `< 1 s` (`RNF01`) e usabilidade (`RNF06`).
- **Teste Alfa (ambiente controlado — equipe de desenvolvimento):**
  - Execução observada de `make -C interface run` validando fluxo completo de partida, pop-ups, relógio, histórico e controles; defeitos encontrados corrigidos antes do merge (ex.: dessincronização do 2º turno após `bestmove`).
- **Teste Beta (ambiente real — usuários externos):**
  - Sessão livre com usuários fora da equipe para avaliar clareza visual, usabilidade e fluidez, utilizando **GitHub Issues** como registro formal de solicitações e melhorias (ex.: solicitação de botão *Desistir* / reinício incorporada no HUD da Issue `#8`).
