# Guia da suíte de testes de aceitação — Xadrez ES2

Este guia descreve a implementação existente em `acceptance-tests/`: como os
testes iniciam o jogo e interagem com ele, como verificam os resultados e quais
arquivos e relatórios são produzidos. Os cenários usam a interface gráfica; as
informações do motor são consultadas somente como evidência complementar.

## 1. Visão geral

A suíte usa Python e pytest para automatizar fluxos na aplicação desktop do
Xadrez ES2. Em cada cenário, ela inicia o executável do jogo, ativa sua janela,
realiza cliques e, quando necessário, pressiona teclas ou redimensiona a janela.
As verificações combinam evidências visuais com respostas do motor que a própria
interface já registrou em seu log.

O objetivo é verificar resultados observáveis em fluxos de uso, por exemplo, se
um lance realizado pela interface atualiza as peças e o turno como esperado ou
se o jogo identifica xeque-mate. Os testes não alteram a aplicação nem enviam
lances diretamente ao motor. Como todos usam a mesma área de trabalho, mouse e
teclado, devem ser executados em série.

## 2. Estrutura dos arquivos

| Arquivo ou pasta | Função |
|---|---|
| `run_tests.py` | Ponto de entrada recomendado. Cria o diretório da execução, define `ACCEPTANCE_RUN_DIR`, inicia o pytest a partir da pasta da suíte e configura a saída JUnit. |
| `tests/test_acceptance_flows.py` | Contém os dez cenários e funções auxiliares para interpretar estados FEN, aplicar um lance a uma posição esperada e verificar casas e peças. |
| `tests/conftest.py` | Integra o pytest à automação: cria e encerra uma sessão por cenário, guarda os resultados de cada fase e gera `report.json` e `report.md` ao final. |
| `desktop_automation.py` | Infraestrutura de desktop: inicia o processo, encontra e ativa a janela, converte coordenadas, captura imagens, espera condições e lê as respostas UCI já registradas no log. |
| `pytest.ini` | Configura a descoberta dos testes, o diretório `tests/` e o marcador `acceptance`. |
| `requirements.txt` | Lista as dependências Python da suíte. |
| `poc_window.py` | Prova de conceito preservada. Abre o jogo, procura a janela pelo título, captura a tela e clica em Jogar. Não é o executor dos dez testes do pytest. |
| `screenshots/` | Guarda as capturas associadas à POC. As evidências dos cenários pytest são guardadas separadamente em `reports/`. |
| `reports/` | Recebe um subdiretório por execução, com relatórios, logs e capturas dos cenários. |

Durante a execução, o fluxo principal é:

1. `run_tests.py` prepara um diretório exclusivo em `reports/<timestamp>/` e
   chama o pytest.
2. O pytest descobre os cenários em `tests/test_acceptance_flows.py` conforme
   `pytest.ini`.
3. A fixture `game`, em `tests/conftest.py`, cria uma `GameSession` por cenário
   e chama `launch()`.
4. `desktop_automation.py` inicia o jogo e fornece as operações de janela,
   captura, clique, teclado e leitura do log.
5. O cenário executa o fluxo pela interface e faz suas asserções.
6. A fixture fecha o processo. Se o teste falhou e ainda há processo ativo, ela
   tenta salvar uma captura do estado final.
7. Ao terminar a sessão pytest, `conftest.py` compila os resultados em Markdown
   e JSON. O pytest grava o JUnit solicitado pelo executor.

## 3. Inicialização e interação com a interface

`GameSession.launch()` verifica a disponibilidade do ambiente Windows e das
dependências de automação, confere os executáveis e inicia o jogo com o
diretório de trabalho apontando para `interface/`. Isso permite que a aplicação
encontre seus assets e o motor por caminhos relativos. A saída padrão e a saída
de erro do jogo são redirecionadas para um arquivo de log exclusivo do cenário.

A janela é identificada pelo título `Xadrez - ES2` e associada ao processo
iniciado pelo teste. A automação traz a janela para primeiro plano, aguarda a
tela inicial e salva uma captura. O encerramento normal é solicitado pela
mensagem de fechamento da janela; se o processo não terminar dentro do prazo,
o harness tenta encerrá-lo e reporta o timeout.

Os pontos dos cenários usam um espaço lógico de 1120 × 800. `ScreenCapture` e
`GameSession` convertem esses pontos para pixels usando as dimensões atuais da
área cliente da janela, com centralização quando necessário. Assim, os cliques
acompanham o redimensionamento em vez de depender da posição da janela na tela.
O harness também configura a conscientização de DPI no Windows e ajusta o
tamanho inicial da janela à área de trabalho; áreas muito pequenas são
consideradas indisponíveis.

Os cenários usam `click()` e `press()` para interagir com menus, controles e
casas do tabuleiro. Para executar um lance, o teste seleciona a peça de origem
com o mouse, espera a condição de seleção e clica na casa de destino. Ele não
envia comandos UCI para executar jogadas.

## 4. Como os resultados são verificados

### Capturas e verificações visuais

`capture()` captura a área cliente da janela. `wait_for()` repete a captura até
que uma condição visual seja satisfeita ou que o tempo limite termine; também
detecta se o processo encerrou durante a espera. As imagens relevantes são
salvas por `save_capture()`.

Algumas verificações visuais usam a cor de um ponto específico para reconhecer
telas ou realces de controles. Outras comparam os pixels de uma região antes e
depois de uma interação, por exemplo, para detectar a mudança do painel de
histórico ou o aparecimento de um alerta.

Para reconhecer as peças, `piece_at()` compara cada casa com as 12 texturas PNG
presentes em `interface/assets/pieces/`. As texturas são redimensionadas para a
casa observada e comparadas usando os pixels opacos da imagem. O matcher
classifica uma peça quando a melhor correspondência tem erro até 35 e distância
mínima de 8 para a segunda melhor; considera a casa vazia quando o menor erro é
65 ou mais. Valores intermediários são tratados como incertos, não como uma
peça ou casa vazia. Essa incerteza pode ocorrer, por exemplo, se uma animação ou
sobreposição encobrir a casa.

Quando um cenário compara a posição inicial completa, ele verifica as 64 casas.
Em movimentos, costuma conferir a origem e o destino; cenários especiais
inspecionam também as casas afetadas pela captura, roque ou promoção.

### Leitura passiva das respostas do motor

O jogo já registra no seu stdout determinadas respostas recebidas do motor.
Como o stdout do processo é direcionado ao log do cenário, o harness pode
procurar linhas como:

```text
[UCI] Comando recebido: state check - result * reason none fen ...
[UCI] Comando recebido: bestmove b8c6
```

`logged_uci_responses()` filtra essas linhas do arquivo. O parser dos testes
interpreta os campos `state` e o FEN, incluindo a posição das peças e o turno.
Para conferir um lance, o teste calcula a posição esperada para aquele
movimento e compara o tabuleiro e o turno com o próximo estado registrado.
Essa comparação contempla os casos usados pela suíte, como captura en passant,
roque e promoção para dama.

Na partida contra IA, o teste espera uma resposta `bestmove`, confere o formato
do lance e compara a transição com o estado subsequente. Também verifica que a
peça de origem pertence ao lado que deveria jogar e que o tabuleiro visível
reflete a resposta.

Essa observação é somente leitura: a automação não abre nem consome os pipes
entre a interface e o motor, não envia comandos adicionais e não inicia o motor
com opções especiais de trace. Se uma resposta necessária não aparecer no log
normal da interface dentro do timeout, a verificação falha em vez de consultar
o motor por outro canal.

## 5. Os dez cenários

Os nomes abaixo correspondem às funções existentes em
`tests/test_acceptance_flows.py`.

| Cenário | Fluxo e resultado verificado |
|---|---|
| `test_menu_opens_and_exit_button_closes_game` | Confirma a tela inicial, clica em Sair e verifica que o processo termina com código 0. |
| `test_ai_configuration_and_escape_navigation` | Abre a configuração, seleciona visualmente dificuldade Fácil, cor preta e relógio de 5 minutos e confirma que Escape retorna ao menu. Não inicia essa partida. |
| `test_local_game_moves_highlights_and_history` | Inicia uma partida local, confere a posição inicial e o destaque de destino de e2; executa e2-e4 e e7-e5 e compara os dois estados FEN. Também verifica uma alteração visual na região do histórico. |
| `test_game_against_ai_produces_a_visible_reply` | Inicia uma partida contra a IA, joga e2-e4 pela interface, espera `bestmove` e confere o estado posterior e a resposta visível do motor. |
| `test_invalid_move_resign_and_return_to_menu` | Tenta e2-e5, verifica que e2 ainda contém o peão e e5 permanece vazia, e que não surgiu um novo `state`. Em seguida realiza e2-e4 e confere a transição; depois desiste e retorna ao menu. |
| `test_resized_window_keeps_menu_interactive` | Redimensiona a janela, verifica as dimensões da captura e confirma que o clique abre a tela de configuração. |
| `test_fools_mate_and_restart_from_postgame_controls` | Executa o Mate do Louco pela interface, confirma no estado registrado `check=e1`, resultado `0-1` e motivo `checkmate`; então inicia outra partida pelo controle pós-jogo e verifica a posição inicial. |
| `test_en_passant_through_mouse_interaction` | Executa a sequência que habilita en passant e verifica no FEN e nas texturas que o peão branco está em d6 e que d5 e e5 estão vazias. |
| `test_castling_through_mouse_interaction` | Libera e executa o roque curto; confere rei em g1, torre em f1 e casas e1 e h1 vazias, no estado do motor e visualmente. |
| `test_promotion_through_mouse_interaction` | Executa a sequência até a7xb8 e verifica no estado e na imagem que o peão branco foi substituído por uma dama em b8 e a7 ficou vazia. O teste corresponde à promoção automática para dama observada na aplicação. |

## 6. O que fica armazenado

Cada execução feita por `run_tests.py` cria uma pasta com data e hora, por
exemplo:

```text
reports/20261122-143000-123456/
├── junit.xml
├── report.json
├── report.md
├── evidence/
│   └── <nome-do-cenario>/
│       └── <etapa>.png
└── logs/
    └── <nome-do-cenario>.log
```

- **`evidence/<cenário>/*.png`**: capturas da janela em etapas importantes do
  fluxo. Se um cenário falhar com o processo ainda ativo, também pode incluir
  `falha-estado-final.png`.
- **`logs/<cenário>.log`**: saída padrão e erros do jogo, incluindo, quando
  registrados pela interface, os dados UCI observados pelo teste.
- **`report.json`** e **`report.md`**: resultados agregados pelo plugin pytest
  local, com status, comportamento esperado, duração, observações, evidências
  do protocolo, limitações, caminhos das capturas e log, e motivo da falha.
- **`junit.xml`**: resultado pytest em formato JUnit, útil para ferramentas que
  consomem esse formato.

Os dados do relatório incluem apenas evidências efetivamente coletadas. Por
exemplo, cenários de navegação sem consultas ao motor não terão evidência UCI.

## 7. Relatórios e interpretação

O `report.md` é voltado à leitura rápida: apresenta o resumo da execução e uma
seção por cenário, com comportamento esperado e observado, limitações,
eventuais motivos de falha e links relativos para capturas e log.

O `report.json` contém os mesmos dados em formato estruturado. No nível da
execução, registra data/hora, código de saída do pytest e contagens de PASS,
FAIL, SKIP e total. Para cada cenário, inclui seu nome e `nodeid`, resultado,
expectativa, motivo da falha, duração, observações, evidências de protocolo,
limitações, capturas e caminho do log.

O `junit.xml` é escrito pelo pytest quando executado pelo comando recomendado;
é compatível com leitores de JUnit. Ele registra os resultados dos testes e
suas falhas ou skips, mas não substitui as evidências detalhadas dos relatórios
Markdown/JSON.

- **PASS**: o cenário executou e todas as asserções passaram.
- **FAIL**: ocorreu uma falha de asserção, de automação ou de execução. Consulte
  o motivo no relatório e examine as capturas e o log do cenário.
- **SKIP**: o cenário não foi executado, por exemplo, porque o ambiente não
  permite a automação Win32 em uma plataforma diferente do Windows. SKIP não
  significa aprovação.

O código de saída do pytest também deve ser considerado: uma execução com
falhas terá código diferente de zero, ainda que outros cenários tenham passado.

## 8. Instalação e execução

### Requisitos

- Windows com sessão gráfica interativa disponível.
- Python 3.10 ou superior.
- Executável do jogo em `../interface/build/xadrez.exe`, ou caminho alternativo
  configurado em `CHESS_GAME_EXE`.
- Motor em `../Backend/build/main.exe`.
- Dependências listadas em `requirements.txt`: pytest, PyAutoGUI, PyGetWindow e
  Pillow.

No PowerShell, entre em `acceptance-tests/` e crie o ambiente:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

Para executar todos os cenários, em série:

```powershell
.\.venv\Scripts\python.exe .\run_tests.py
```

Para executar apenas um cenário, informe seu `nodeid` ao mesmo comando. Por
exemplo:

```powershell
.\.venv\Scripts\python.exe .\run_tests.py tests\test_acceptance_flows.py::test_local_game_moves_highlights_and_history
```

O diretório da execução e os caminhos dos relatórios são impressos no terminal.
Também é possível fornecer outro executável do jogo antes de iniciar:

```powershell
$env:CHESS_GAME_EXE = "C:\caminho\para\xadrez.exe"
.\.venv\Scripts\python.exe .\run_tests.py
```

O diretório de trabalho do jogo continua sendo `interface/`, pois a aplicação
usa caminhos relativos para carregar assets e localizar o motor. A automação
precisa de uma sessão gráfica visível; uma execução em serviço ou CI sem desktop
interativo pode ser marcada como SKIP ou não conseguir realizar as interações.

## 9. Limitações conhecidas

- As telas e os realces de controles são reconhecidos por cores em coordenadas
  lógicas. Alterações de layout, cores ou comportamento de renderização podem
  exigir ajustes.
- O reconhecimento das peças compara as texturas atuais e pode ficar incerto
  com animações, sobreposições ou diferenças visuais significativas.
- O cenário de histórico não usa OCR. Ele confere a ordem das posições FEN e
  detecta uma alteração na região do painel, mas não confirma que o texto
  exibido no histórico corresponde aos lances.
- O `bestmove` é confrontado com o estado posterior, com o turno e com a peça
  de origem; a suíte não obtém do log a lista completa de movimentos legais.
- O cenário de configuração verifica os destaques visuais das opções, mas não
  inicia a partida para confirmar que cada escolha foi aplicada ao jogo.
- O cenário de redimensionamento exercita uma dimensão por execução; não cobre
  uma matriz de resoluções e escalas de DPI.
- Os cenários compartilham mouse, teclado e janela. Evite executá-los em
  paralelo.
- A POC em `poc_window.py` é separada da suíte pytest e utiliza um caminho de
  executável definido no próprio script.
- A cobertura se limita às funcionalidades incluídas nos dez cenários; não
  inclui, por exemplo, escolha de diferentes peças de promoção, leitura do
  histórico por OCR ou recursos que não estejam implementados na aplicação.
