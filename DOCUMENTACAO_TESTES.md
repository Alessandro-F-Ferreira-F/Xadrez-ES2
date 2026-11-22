# Documentação das estratégias e da cobertura de testes — Xadrez ES2

## 1. Escopo e resumo

O repositório contém testes automatizados para o motor de regras em C, para a
busca da IA em C, para a interface em C++ e para o protocolo UCI que conecta os
componentes. Há ainda uma suíte Python que opera a aplicação desktop pela
interface gráfica.

As suítes automatizadas usam uma API de testes própria, compartilhada entre C
e C++, e pytest para a automação desktop. GoogleTest, Catch2 e CTest do CMake
não são usados. O nome `tools/ctest/` refere-se à biblioteca mínima de teste do
projeto, não ao runner CTest da distribuição do CMake.

O projeto também documenta testes manuais de aceitação Alfa/Beta. Eles
descrevem práticas realizadas ou planejadas, mas não são cenários executáveis
pela suíte automatizada e não devem ser somados aos resultados dos testes.

## 2. Inventário das estratégias existentes

### 2.1 Motor de regras — testes C

**Arquivos principais:** `Backend/engine/test/test.c`, `rules.c` e `search.c`.

**Objetivo e organização:** verificar as regras e operações fundamentais do
tabuleiro, a geração de lances e a integração do adaptador usado pela busca.
Os casos são registrados com a macro `TEST` da API própria e executados pelo
runner `test_run_all()`, compilado junto com os módulos do motor e da IA.

**Técnicas e comportamentos verificados:**

- `perft` compara a quantidade de posições alcançáveis com contagens de
  referência para seis posições de xadrez. O padrão do código é até
  profundidade 5; a CI limita a execução a 3 por meio de `PERFT_MAX_DEPTH=3`.
  `make test-full` solicita profundidade 5.
- `make_unmake` percorre lances legais em 18 posições FEN, incluindo posições
  com roque, en passant, promoção, xeque e mate, e verifica que aplicar e
  desfazer o lance restaura a FEN.
- Os testes em `rules.c` cobrem alvos do rei, rejeição de FENs inválidas sem
  corromper o tabuleiro e regras de empate por material e pela regra dos 50
  lances.
- Os testes em `search.c` exercitam a ligação da busca às regras de empate,
  verificam que a busca devolve um lance legal sem alterar a posição de entrada
  e testam mate no limite de 100 meios-lances.

São testes predominantemente **caixa-branca**: compilam e chamam diretamente
funções e estruturas internas do motor. Usam dados de entrada explícitos (FEN,
lances e profundidades) e oráculos como contagens de referência, igualdade de
FEN antes/depois e resultados esperados.

### 2.2 Busca e níveis da IA — testes C

**Arquivos principais:** `Backend/IA/test/unit_test.c`,
`integration_test.c`, `system_test.c` e `ia_test_common.h`.

O Makefile compila três executáveis: testes de unidade, integração e sistema.
O alvo `make test-ia` executa os três. A compilação de teste usa o stub
`Backend/IA/src/stubs.c` no lugar do adaptador de regras da IA, para controlar
resultados de empate em alguns casos.

- **Unidade:** valida parâmetros inválidos, avaliação material/posicional,
  simetria de cores, conversão de scores centipawn/mate, ordenação de capturas
  MVV-LVA e seleção aleatória de candidatos com sementes reproduzíveis.
- **Integração da busca:** compara Minimax e Alfa-Beta em posições selecionadas,
  confere scores, candidatos, determinismo, contagem de nós e restauração do
  tabuleiro. Também testa quiescência, detecção de mate/afogamento e delta
  pruning. Como o build usa `stubs.c`, essa suíte não equivale a uma integração
  de todos os componentes de produção da IA.
- **Sistema/desempenho:** verifica limites de tempo, níveis Fácil/Médio/Difícil,
  profundidade atingida e legalidade dos lances produzidos.

Esses testes validam regras internas da busca, casos-limite e requisitos de
tempo. A maioria é caixa-branca, com alguns oráculos de comportamento externo,
como lance legal e limite de resposta.

### 2.3 Protocolo UCI — testes Python de integração

**Arquivos:** `Backend/engine/test/protocol/run.py` e 13 pares de arquivos
`.in`/`.out`.

O script inicia o executável real do motor como subprocesso e compara sua saída
com transcrições esperadas. As linhas de `legalmoves` e os blocos de contagem
`go perft` são comparados como conjuntos ordenados pelo runner, pois a ordem
não é parte do contrato. O teste não fixa uma resposta `bestmove` sorteada:
confere que ela pertence à lista de lances legais.

Além das transcrições, o runner testa:

- handshake e comandos de posição, lances legais, busca e término;
- promoção, en passant, xeque-mate, afogamento, empate de 50 lances e erros de
  comando/FEN/lance;
- entrega da resposta com stdin ainda aberta, para detectar saída presa em
  buffer, e término do motor ao receber EOF;
- legalidade de `bestmove`, opções e limites de busca, determinismo, estados de
  empate e tempos de resposta por nível.

É principalmente **caixa-preta** e **baseado em casos de entrada/saída**: o
runner observa o processo pelo protocolo, sem chamar funções internas do
motor. Também é um teste de comunicação entre processos e validação de
entradas/saídas. O comando de referência é `make test-protocol` na raiz.

### 2.4 Interface — smoke, unidade, integração e sistema em C++17

**Arquivos:** `interface/test/smoke_test.cpp`, `unit_test.cpp`,
`integration_test.cpp` e `system_test.cpp`.

Os executáveis usam `tools/ctest/test_api.h`, compilado em C e ligado às suítes
C++. A macro `TEST` registra casos e `test_run_all()` executa a seleção; a
variável `TEST_FILTER` permite filtrar casos pelo nome. A opção CMake
`BUILD_TESTING=ON` compila os executáveis de teste, e o Makefile da interface
oferece `make test` para compilar e executar as quatro suítes.

- **Smoke:** inicia o motor como subprocesso, verifica `uci`/`uciok` e
  `isready`/`readyok`, encerra o bridge e instancia o tabuleiro. Não é uma
  automação de janela gráfica.
- **Unidade:** testa classes e lógica da interface, incluindo conversão entre
  casas e pixels, estado inicial, aplicação de lances UCI, comandos UCI
  processados pelo tabuleiro, seleção por coordenadas, realces, lances
  inválidos, alertas, relógios, histórico, configuração do menu e controles
  pós-jogo. Há casos específicos de fronteira e caminhos básicos.
- **Stub do bridge:** um teste de unidade verifica pipes, leitura de linhas e
  encerramento do `EngineBridge` com um processo de eco de teste. No Windows, o
  próprio executável de teste atua como stub; em POSIX, é usado `/bin/cat`.
- **Integração:** inicia o motor real e combina o bridge com `ChessBoard`.
  Verifica handshake, consulta de lances legais, turnos acionados por
  coordenadas, estados de xeque-mate, controles do HUD, partida local, IA com
  tabuleiro invertido, níveis de dificuldade, cancelamento de busca e uma
  partida completa até o estado terminal. As interações são chamadas aos
  objetos C++ com coordenadas, não cliques enviados ao sistema operacional.
- **Sistema:** verifica recuperação após falha ao iniciar processo ou após
  comandos inválidos e executa uma sequência de até 40 meios-lances medindo
  latência. O alvo `profile` está configurado para executar o teste de sistema
  com Valgrind.

Assim, os testes de unidade são principalmente caixa-branca. A integração
verifica comunicação real entre interface e motor, e os testes de sistema
exercitam sequências e recuperação entre componentes. Embora alguns testes
usem coordenadas de clique, eles não substituem a automação de desktop: o código
chama os métodos da interface diretamente, sem abrir uma janela, clicar com
mouse real ou validar pixels.

### 2.5 Aceitação automatizada — desktop Python

**Arquivos:** `acceptance-tests/tests/test_acceptance_flows.py`,
`tests/conftest.py`, `desktop_automation.py` e `run_tests.py`.

Essa suíte usa pytest, PyAutoGUI, PyGetWindow e Pillow. `run_tests.py` cria o
diretório de execução e configura a saída JUnit. A fixture `game` inicia uma
instância do jogo por cenário, captura sua janela, fornece operações de mouse e
teclado e encerra o processo. Os dez cenários simulam ações pela interface
gráfica real.

As verificações combinam cores/regiões de tela, diferenças entre capturas e
comparação das casas do tabuleiro com as texturas PNG das peças. Como evidência
complementar, o harness lê do log do jogo as respostas UCI que a interface já
registrou, como `state` com FEN e `bestmove`. Não envia comandos ao motor nem
intercepta os pipes entre aplicação e motor.

Essa camada é **caixa-preta orientada a cenários**: testa fluxos do ponto de
vista do usuário e valida resultados visuais e estados observáveis. A cobertura
atual detalhada está na seção 4. A suíte exige Windows com desktop interativo;
os dez cenários compartilham mouse, teclado e janela, devendo rodar em série.

Os cenários presentes são:

| Cenário | Fluxo e verificação principal |
|---|---|
| `test_menu_opens_and_exit_button_closes_game` | Confirma o menu inicial, aciona Sair e verifica encerramento do processo com código 0. |
| `test_ai_configuration_and_escape_navigation` | Seleciona dificuldade Fácil, cor preta e relógio de 5 minutos pelos controles; verifica os realces e o retorno ao menu com Escape. |
| `test_local_game_moves_highlights_and_history` | Inicia jogo local, confirma posição inicial e destino realçado, executa e2-e4/e7-e5 e compara os FENs subsequentes. Verifica também alteração na área do histórico, não o texto renderizado. |
| `test_game_against_ai_produces_a_visible_reply` | Faz e2-e4 pela interface, espera `bestmove` do motor, confere o FEN após a resposta e as casas visíveis. |
| `test_invalid_move_resign_and_return_to_menu` | Tenta e2-e5, confere que a origem/destino permanecem e que não surgiu novo `state`, executa e2-e4 e testa desistência/retorno ao menu. |
| `test_resized_window_keeps_menu_interactive` | Redimensiona a janela, confere a dimensão capturada e abre a configuração pelo clique calculado. |
| `test_fools_mate_and_restart_from_postgame_controls` | Executa o Mate do Louco, verifica `check=e1`, resultado `0-1` e `reason=checkmate`, depois reinicia e confere a posição inicial. |
| `test_en_passant_through_mouse_interaction` | Executa a sequência de en passant e verifica peão branco em d6, com d5 e e5 vazias, no FEN e pela textura. |
| `test_castling_through_mouse_interaction` | Executa roque curto e confere rei em g1, torre em f1 e e1/h1 vazias, no FEN e visualmente. |
| `test_promotion_through_mouse_interaction` | Executa a7xb8 e verifica a dama branca em b8 e a7 vazia, no estado do motor e na captura. |

### 2.6 Testes manuais documentados

`docs/estrategia_testes.md` descreve testes Alfa em ambiente controlado e
testes Beta com usuários externos, com avaliação de usabilidade e registro de
solicitações. Também apresenta critérios de homologação. Não há, nesses trechos,
um runner ou casos automatizados que possam ser contabilizados como parte das
suítes do repositório. Eles são práticas manuais documentadas, não resultados
automatizados reproduzíveis.

## 3. Ferramentas e execução

| Ferramenta | Uso observado no projeto |
|---|---|
| API própria `tools/ctest/` | Macros `TEST`, `EXPECT_*`, `ASSERT_*` e runner `test_run_all()` para as suítes C/C++. Não é GoogleTest, Catch2 nem CTest do CMake. |
| pytest | Descoberta, fixture e execução serial dos cenários de aceitação Python. |
| PyAutoGUI | Cliques, teclado e capturas de tela na automação desktop. |
| PyGetWindow e Win32 | Localização/ativação da janela e consulta de sua área cliente no Windows. |
| Pillow | Manipulação/comparação de imagens, amostragem de cores e comparação com texturas. |
| Python `subprocess` | Execução do motor nos testes do protocolo e captura da saída; também usado pelo harness desktop para iniciar o jogo. |
| Make | Compilação e execução das suítes do backend e da interface, além dos alvos agregados da raiz. |
| CMake | Configuração/compilação da interface e opção `BUILD_TESTING` para criar os executáveis de teste. |
| GCC/G++ | Compilação C11/C++17; a suíte do backend usa avisos estritos. |
| `gcov` e `--coverage` | Instrumentação configurada para medir cobertura de código C++ da interface. |
| Valgrind | Comando `profile` configurado no Makefile da interface para verificar memória no teste de sistema. |
| GitHub Actions | Pipeline com jobs separados para backend, protocolo e interface, em Linux e Windows. A interface Linux roda sob Xvfb. |

Comandos agregados documentados na raiz:

```sh
make                 # compila backend e interface
make run             # compila e abre o jogo
make test            # backend, protocolo e interface
make test-full       # make test com PERFT_MAX_DEPTH=5
make test-engine     # testes do motor
make test-ia         # testes da IA
make test-protocol   # testes do protocolo UCI
make test-interface  # smoke, unidade, integração e sistema da interface
```

Para a interface, os alvos individuais são `make -C interface test-smoke`,
`test-unit`, `test-integration` e `test-system`. Para a suíte desktop, no
PowerShell e a partir de `acceptance-tests/`:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe .\run_tests.py
.\.venv\Scripts\python.exe .\run_tests.py tests\test_acceptance_flows.py::test_local_game_moves_highlights_and_history
```

O workflow `.github/workflows/ci.yml` executa automaticamente testes do
backend, protocolo e interface em Linux/Windows. Ele não executa
`acceptance-tests/`, que depende de uma sessão desktop Windows interativa.

## 4. Cobertura funcional existente

A tabela resume os comportamentos efetivamente representados nos testes. A
coluna “Cobertura/limitação” descreve escopo qualitativo; não representa
percentual de linhas ou branches.

| Módulo ou funcionalidade | Tipo de teste | O que é verificado | Cobertura/limitação |
|---|---|---|---|
| Geração de lances e estado do motor | Unidade/algoritmo C | Contagens perft de seis posições; restauração da FEN em `make_move`/`unmake_move`; alvos do rei e integridade após FEN inválida. | Boa cobertura de cenários canônicos e regras selecionadas. Perft padrão local pode ir a profundidade 5; CI usa profundidade 3. Não é medição de cobertura de código. |
| Regras de empate e busca terminal | Unidade/integração C | Empate por material e 50 lances, mate/afogamento na raiz e mate próximo ao limite do relógio. | Casos explícitos; não demonstra cobertura de todos os estados ou sequências possíveis. |
| Busca e avaliação da IA | Unidade, integração e sistema C | Validação de parâmetros, avaliação, score, ordenação, aleatoriedade, Minimax vs. Alfa-Beta, quiescência, poda e restauração da posição. | Vários oráculos internos e posições FEN; os testes de IA usam stub para algumas regras de empate. |
| Níveis e tempo da IA | Sistema/desempenho C e protocolo | Profundidade, candidatos legais e limites de resposta por nível; opções de busca e `bestmove` legal. | Limites medidos no ambiente de execução, sujeitos a variação de máquina; não constituem benchmark universal. |
| Protocolo UCI do motor | Integração Python e C++ | Handshake, posições, movimentos legais, busca, promoção, en passant, mate, afogamento, empates, erros, EOF e transporte com stdin aberta. | Testa o contrato do processo e várias entradas inválidas; logs em stderr são intencionalmente ignorados pelo teste de transcrição. |
| API lógica do tabuleiro da interface | Unidade C++ | Conversão de coordenadas, posições, aplicação de movimentos e interpretação de comandos/estados UCI. | Testa métodos do modelo diretamente; não cobre por si a renderização real da janela. |
| Interação lógica, turnos e destaques | Unidade/integração C++ | Seleção, destinos, lance legal/ilegal, turnos, coordenação com o motor e tabuleiro invertido. | A integração chama métodos C++ com coordenadas, não envia eventos reais de mouse para o sistema. |
| HUD, histórico, relógios, menus e alertas | Unidade C++ | Formatação e contagem do relógio, timeout, histórico formatado, desistência, nova partida, opções de configuração e estados de alerta. | Verifica o estado e algumas coordenadas de componentes; não equivale a validar todos os elementos desenhados em uma janela. |
| EngineBridge e processo do motor | Smoke/unidade/integração/sistema C++ | Pipes com stub, handshake com motor real, resposta legal, recuperação de erros, busca assíncrona e encerramento/cancelamento. | Integração exercita o subprocesso real em fluxos selecionados; diferenças ambientais de processo ainda podem escapar. |
| Interface visual desktop | Aceitação Python | Abrir/sair, configuração, redimensionamento, início de jogo, movimentos, alertas, desistência e retorno ao menu. | Dez fluxos na sessão Windows; pontos/cores e reconhecimento de sprites dependem do layout e das texturas atuais. |
| Regras especiais pela GUI | Aceitação Python | En passant, roque, promoção automática para dama e Mate do Louco. | Estados FEN e peças visíveis conferem as casas críticas; promoção verifica dama, não a escolha entre diferentes peças. |
| Resposta da IA na GUI | Aceitação Python | Executa o lance humano pela interface, lê `bestmove` já registrado e confere o FEN resultante e as casas visíveis. | Não consulta uma lista legal completa no log; a posição resultante e a origem/turno são verificadas. |
| Histórico visível na GUI | Aceitação Python | Confere sequência de estados FEN após dois lances e uma alteração na região do painel. | Sem OCR: o texto e a ordem renderizada do histórico não são comparados diretamente. |
| Testes Alfa/Beta e usabilidade | Manual/documentado | Execução observada e avaliação com usuários são descritas na estratégia do projeto. | Sem automação ou resultados reproduzíveis nos scripts inventariados. |

## 5. Limitações e áreas sem cobertura adequada

- **Renderização e apresentação:** as suítes C++ verificam principalmente estado
  e métodos; a aceitação desktop verifica uma seleção de telas e pontos. Não há
  validação automatizada abrangente de todas as combinações de resolução,
  escala DPI, animações ou qualidade visual.
- **Texto do histórico na GUI:** os estados sucessivos do motor permitem
  confirmar a ordem dos lances aplicados, mas o conteúdo textual renderizado no
  painel não é lido por OCR.
- **Configuração contra IA:** o cenário desktop seleciona dificuldade, cor e
  tempo e confirma os realces, mas não inicia essa partida para confirmar que
  todas as opções selecionadas foram aplicadas. Os testes C++ cobrem estados de
  configuração e alguns níveis no motor, não todas as combinações via janela.
- **Promoção:** o cenário desktop cobre a promoção automática para dama. A
  interface C++ também tem testes para os quatro tipos via `applyUCIMove`, mas
  não há fluxo desktop para escolher torre, bispo ou cavalo.
- **Cobertura completa de regras:** posições perft, FENs e casos especiais
  cobrem um conjunto amplo, mas não todas as posições, sequências, entradas
  malformadas ou combinações de estado possíveis.
- **Persistência, rede e dados:** as suítes não testam servidor, multiplayer
  online ou persistência de partidas; esses componentes não fazem parte dos
  fluxos testados aqui.
- **Execução desktop:** requer Windows interativo e é serial. O workflow de CI
  atual não executa os dez cenários desktop.
- **Relatórios:** os testes C/C++ reportam o resultado no runner próprio. Os
  relatórios detalhados Markdown, JSON e JUnit são produzidos pela suíte
  Python; não há evidência de exportação unificada de cobertura funcional das
  suítes nativas.

## 6. Cobertura de código

A cobertura de código foi avaliada por meio de instrumentação com GCC e gcov, abrangendo módulos de produção do Backend (motor e IA) e da Interface.

A medição foi realizada separadamente para cada componente, considerando cobertura de linhas, funções e branches. Os resultados, a metodologia utilizada, os arquivos incluídos e as limitações da análise estão documentados em `RELATORIO_COBERTURA.md`.

Os percentuais apresentados representam a execução dos testes automatizados sobre os arquivos de produção instrumentados. Portanto, não devem ser interpretados como cobertura integral de todo o projeto.

## 7. Síntese

O projeto combina testes de unidade caixa-branca do motor, IA e lógica da
interface; testes de integração entre subprocessos e componentes; testes de
sistema de recuperação e latência; testes de protocolo baseados em
transcrições; e aceitação desktop com interação real pela janela.

Essa combinação cobre regras de xadrez selecionadas, busca, contrato UCI,
coordenação entre interface e motor, estado do HUD e fluxos visuais relevantes.
Os resultados funcionais não devem ser confundidos com cobertura percentual
de código: a medição configurada é somente para alguns módulos da interface e
não foi executável neste ambiente. Os valores publicados anteriormente são
históricos e precisam ser remensurados para a árvore atual.
