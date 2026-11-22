# RelatÃ³rio de cobertura de cÃ³digo â€” Xadrez ES2

**Data da anÃ¡lise:** 22/11/2026
**Escopo:** dados `.gcno`/`.gcda` jÃ¡ existentes nos builds de cobertura do Backend e da Interface.
**Regra de execuÃ§Ã£o:** nenhum teste foi reexecutado e nenhum build foi limpo ou recompilado.

## 1. Resumo dos resultados

| Componente | Linhas executadas | Cobertura de linhas | FunÃ§Ãµes executadas | Cobertura de funÃ§Ãµes | Branches executados | Cobertura de branches |
|---|---:|---:|---:|---:|---:|---:|
| Backend C | 1.080 / 1.819 | **59,37%** | 76 / 141 | **53,90%** | 832 / 1.362 | **61,09%** |
| Interface C++ | 976 / 1.485 | **65,72%** | 134 / 152 | **88,16%** | 839 / 2.276 | **36,86%** |

As linhas nÃ£o executadas sÃ£o 739 no Backend e 509 na Interface. O HTML discrimina
as linhas instrumentÃ¡veis executadas e nÃ£o executadas de cada arquivo.

Os resultados sÃ£o separados: nenhum contador de Backend foi somado aos da
Interface. Os percentuais consideram somente cÃ³digo de produÃ§Ã£o pertencente ao
projeto e que possui dados de execuÃ§Ã£o nos builds analisados.

## 2. Metodologia

1. Conferi a configuraÃ§Ã£o de compilaÃ§Ã£o e os objetos instrumentados nos diretÃ³rios
   de cobertura.
2. Usei o `gcov` 16.2.0 do MSYS2 UCRT64 para ler os `.gcda` existentes e produzir
   JSON intermediÃ¡rio com `--json-format`, `--branch-probabilities` e
   `--function-summaries`.
3. Agreguei as execuÃ§Ãµes repetidas do mesmo arquivo/funÃ§Ã£o/linha entre os
   executÃ¡veis de teste do respectivo componente. A linha ou branch conta como
   coberta se foi executada em ao menos um deles.
4. Filtrei arquivos de testes, stubs, ferramentas auxiliares, cÃ³digo gerado e
   dependÃªncias externas. As declaraÃ§Ãµes inline de headers do prÃ³prio projeto
   que possuem contadores foram mantidas.
5. Gerei HTML por arquivo, XML compatÃ­vel com o formato Cobertura e JSON de
   resumo, em diretÃ³rios separados.

**DefiniÃ§Ãµes usadas:**

- **Linha:** linha instrumentÃ¡vel reportada pelo gcov. Linhas em branco,
  comentÃ¡rios e outras linhas nÃ£o executÃ¡veis nÃ£o entram no denominador.
- **FunÃ§Ã£o:** funÃ§Ã£o Ãºnica por arquivo, nome e intervalo de linhas; Ã© coberta
  quando seu contador de execuÃ§Ã£o Ã© maior que zero.
- **Branch:** saÃ­da de decisÃ£o registrada pelo gcov, identificada pelo arquivo,
  linha e blocos de origem/destino. A contagem Ã© consolidada entre os executÃ¡veis
  de teste. O nÃºmero de branches Ã© especÃ­fico do compilador e pode incluir
  saÃ­das de condiÃ§Ãµes compostas.

Os relatÃ³rios detalhados foram gerados diretamente dos dados JSON do gcov porque
`gcovr` nÃ£o estÃ¡ instalado. O XML foi validado como XML e seus totais foram
comparados aos da agregaÃ§Ã£o; nÃ£o foi validado por um validador externo de schema
Cobertura.

## 3. ConfiguraÃ§Ã£o e dados inspecionados

### Backend

- Build analisado: `Backend/build-coverage`.
- HÃ¡ **90 arquivos `.gcno` e 90 `.gcda`**, todos os `.gcda` com seu `.gcno`
  correspondente.
- Os dados estÃ£o distribuÃ­dos em 24 objetos da suÃ­te do motor, 22 da suÃ­te
  unitÃ¡ria da IA, 22 da suÃ­te de integraÃ§Ã£o da IA e 22 da suÃ­te de sistema da IA.
- O `gcov` leu os dados compilados com GCC 16.2.0.
- As mÃ©tricas incluem 14 fontes de `Backend/engine/src`, cinco fontes de
  `Backend/IA/src` e `Backend/engine/include/board.h`.
- `Backend/IA/src/stubs.c` aparece instrumentado nos objetos da IA, mas foi
  excluÃ­do por ser stub. Fontes de teste, `tools/ctest` e o cÃ³digo de teste
  tambÃ©m foram excluÃ­dos. `engine/src/main.c` nÃ£o integra esses executÃ¡veis de
  teste, portanto nÃ£o faz parte dos percentuais.

O Makefile atual declara flags padrÃ£o `-Og -g` e nÃ£o contÃ©m um alvo ou registro de
configuraÃ§Ã£o de cobertura para `build-coverage`. A presenÃ§a dos pares `.gcno` /
`.gcda` e a leitura bem-sucedida pelo gcov confirmam que os objetos analisados
foram instrumentados. Contudo, nÃ£o hÃ¡ registro persistido no repositÃ³rio que
permita confirmar independentemente que a compilaÃ§Ã£o especÃ­fica usou
`--coverage -O0 -g`; essa configuraÃ§Ã£o foi informada no pedido.

### Interface

- Build analisado: `interface/build-coverage`.
- `CMakeCache.txt` confirma `BUILD_TESTING=ON`, gerador `MinGW Makefiles`,
  compiladores em `C:/msys64/ucrt64/bin/gcc.exe` e `g++.exe`, e flags C/C++
  `--coverage -O0 -g`. O arquivo `compile_commands.json` confirma essas flags
  nos comandos de compilaÃ§Ã£o.
- HÃ¡ **112 arquivos `.gcno` e 87 `.gcda`**; todos os `.gcda` possuem `.gcno`
  correspondente.
- As quatro suÃ­tes â€” smoke, unitÃ¡ria, integraÃ§Ã£o e sistema â€” tÃªm oito objetos
  com dados cada uma. Esses objetos incluem os cinco fontes de produÃ§Ã£o
  `interface/src` e foram agregados entre os alvos.
- Outros **55 `.gcda`** pertencem Ã  SFML compilada por FetchContent e foram
  excluÃ­dos. Arquivos de testes, `tools/ctest`, arquivos de identificaÃ§Ã£o do
  compilador e cÃ³digo gerado tambÃ©m nÃ£o entram nos percentuais.
- As mÃ©tricas incluem os cinco fontes de produÃ§Ã£o (`board.cpp`,
  `engine_bridge.cpp`, `hud_panel.cpp`, `menu_screen.cpp` e `piece.cpp`) e seis
  headers do projeto com linhas instrumentadas.
- `main.cpp` estÃ¡ no alvo `xadrez` compilado com as flags de cobertura, mas nÃ£o
  hÃ¡ `.gcda` para `CMakeFiles/xadrez.dir`. Portanto, nÃ£o foi executado nesse
  build e nÃ£o foi incluÃ­do nos percentuais.

O inventÃ¡rio foi feito sobre os dados jÃ¡ existentes; os 25 `.gcno` sem `.gcda`
da Interface nÃ£o foram tratados como execuÃ§Ã£o coberta.

## 4. Resultados por arquivo

`Linhas` e `branches` mostram contagem coberta/total. As pÃ¡ginas individuais do
HTML contÃªm os nÃºmeros das linhas nÃ£o executadas.

### Backend C

| Arquivo de produÃ§Ã£o | Linhas | FunÃ§Ãµes | Branches |
|---|---:|---:|---:|
| `Backend/IA/src/avaliar.c` | 19/19 (100,00%) | 1/1 | 12/12 |
| `Backend/IA/src/ia.c` | 359/368 (97,55%) | 16/16 | 354/374 |
| `Backend/IA/src/niveis.c` | 4/4 (100,00%) | 1/1 | 4/4 |
| `Backend/IA/src/ordenar.c` | 16/16 (100,00%) | 2/2 | 12/12 |
| `Backend/IA/src/regras.c` | 5/7 (71,43%) | 1/1 | 3/5 |
| `Backend/engine/include/board.h` | 11/11 (100,00%) | 3/3 | 14/20 |
| `Backend/engine/src/attack.c` | 28/41 (68,29%) | 2/2 | 40/54 |
| `Backend/engine/src/board.c` | 10/91 (10,99%) | 2/7 | 8/62 |
| `Backend/engine/src/draw.c` | 19/19 (100,00%) | 1/1 | 22/22 |
| `Backend/engine/src/fen.c` | 174/245 (71,02%) | 10/10 | 117/177 |
| `Backend/engine/src/io.c` | 0/119 (0,00%) | 0/21 | 0/88 |
| `Backend/engine/src/log.c` | 8/8 (100,00%) | 1/1 | 0/0 |
| `Backend/engine/src/makemove.c` | 121/127 (95,28%) | 4/4 | 51/65 |
| `Backend/engine/src/move.c` | 57/93 (61,29%) | 11/16 | 18/35 |
| `Backend/engine/src/movegen.c` | 164/164 (100,00%) | 10/10 | 136/144 |
| `Backend/engine/src/perft.c` | 11/11 (100,00%) | 1/1 | 4/4 |
| `Backend/engine/src/piece.c` | 7/9 (77,78%) | 2/2 | 2/4 |
| `Backend/engine/src/square.c` | 62/67 (92,54%) | 7/7 | 35/42 |
| `Backend/engine/src/uci.c` | 0/333 (0,00%) | 0/24 | 0/222 |
| `Backend/engine/src/utils.c` | 5/67 (7,46%) | 1/11 | 0/16 |
| **Total** | **1.080/1.819 (59,37%)** | **76/141 (53,90%)** | **832/1.362 (61,09%)** |

Os menores Ã­ndices de linhas estÃ£o em `io.c`, `uci.c`, `utils.c` e `board.c`.
`io.c` e `uci.c` nÃ£o registraram execuÃ§Ã£o nesses dados. A cobertura de linhas
alta em alguns mÃ³dulos nÃ£o significa que todas as decisÃµes estejam cobertas;
por exemplo, `board.h` tem 100% das linhas, mas 14 de 20 branches cobertos.

### Interface C++

| Arquivo de produÃ§Ã£o | Linhas | FunÃ§Ãµes | Branches |
|---|---:|---:|---:|
| `interface/include/board.hpp` | 35/35 (100,00%) | 35/35 | 9/12 |
| `interface/include/game_config.hpp` | 13/14 (92,86%) | 5/5 | 10/16 |
| `interface/include/hud_panel.hpp` | 9/9 (100,00%) | 9/9 | 0/0 |
| `interface/include/menu_screen.hpp` | 13/13 (100,00%) | 11/11 | 0/0 |
| `interface/include/piece.hpp` | 4/4 (100,00%) | 4/4 | 0/0 |
| `interface/include/ui_helpers.hpp` | 22/57 (38,60%) | 2/6 | 13/50 |
| `interface/src/board.cpp` | 603/786 (76,72%) | 44/50 | 679/1.414 |
| `interface/src/engine_bridge.cpp` | 102/124 (82,26%) | 6/6 | 59/122 |
| `interface/src/hud_panel.cpp` | 87/217 (40,09%) | 11/13 | 31/332 |
| `interface/src/menu_screen.cpp` | 69/204 (33,82%) | 3/8 | 33/320 |
| `interface/src/piece.cpp` | 19/22 (86,36%) | 4/5 | 5/10 |
| **Total** | **976/1.485 (65,72%)** | **134/152 (88,16%)** | **839/2.276 (36,86%)** |

Os menores Ã­ndices da Interface estÃ£o em `menu_screen.cpp`, `ui_helpers.hpp` e
`hud_panel.cpp`. O percentual de funÃ§Ãµes Ã© significativamente maior que o de
branches; a suÃ­te chega a executar a maioria das funÃ§Ãµes, mas percorre apenas
parte das saÃ­das de decisÃ£o.

## 5. RelatÃ³rios gerados

| Componente | HTML | XML | Resumo JSON |
|---|---|---|---|
| Backend | [coverage-reports/backend/index.html](./coverage-reports/backend/index.html) | [coverage-reports/backend/coverage.xml](./coverage-reports/backend/coverage.xml) | [coverage-reports/backend/coverage-summary.json](./coverage-reports/backend/coverage-summary.json) |
| Interface | [coverage-reports/interface/index.html](./coverage-reports/interface/index.html) | [coverage-reports/interface/coverage.xml](./coverage-reports/interface/coverage.xml) | [coverage-reports/interface/coverage-summary.json](./coverage-reports/interface/coverage-summary.json) |

O HTML de cada componente apresenta a visÃ£o geral e uma pÃ¡gina por arquivo,
destacando linhas executadas e nÃ£o executadas. O XML traz contadores por linha,
funÃ§Ã£o e branch. Os JSONs incluem totais e nÃºmeros das linhas executÃ¡veis nÃ£o
cobertas. Os intermediÃ¡rios `.gcov.json.gz` e logs do gcov estÃ£o em
`coverage-reports/backend/gcov-json/` e `coverage-reports/interface/gcov-json/`.

## 6. Testes executados e preservaÃ§Ã£o dos dados

NÃ£o executei testes, `clean`, CMake configure, Make ou recompilaÃ§Ã£o. Isso evita
alterar os contadores existentes ou sobrescrever executÃ¡veis. Os resultados de
execuÃ§Ã£o informados no pedido â€” Backend 22/22 e Interface 31/31 (smoke 1,
unitÃ¡rios 18, integraÃ§Ã£o 10 e sistema 2) â€” nÃ£o foram revalidados nesta anÃ¡lise.

O gcov leu 90 arquivos `.gcda` do Backend e 32 dos alvos de teste da Interface;
os 55 arquivos `.gcda` da SFML foram excluÃ­dos. Registrei hashes SHA-256 dos
contadores antes e depois da extraÃ§Ã£o e comparei-os novamente apÃ³s gerar os
relatÃ³rios: **os dados `.gcda` permaneceram inalterados**. O registro estÃ¡ em
[coverage-reports/gcov-collection-metadata.json](./coverage-reports/gcov-collection-metadata.json).

## 7. gcovr, aceitaÃ§Ã£o desktop e limitaÃ§Ãµes

`gcovr` nÃ£o foi encontrado no PATH e o Python UCRT64 nÃ£o possui o mÃ³dulo
`gcovr` nem `pip`. NÃ£o instalei ferramentas. Para uma instalaÃ§Ã£o futura, uma
opÃ§Ã£o Ã© usar um Python com `pip` (`python -m pip install gcovr`) e apontar o
gcovr para `C:\msys64\ucrt64\bin\gcov.exe`; a instalaÃ§Ã£o nÃ£o foi feita.

Os testes de aceitaÃ§Ã£o em Python nÃ£o contribuÃ­ram para estes percentuais. Por
padrÃ£o, [desktop_automation.py](./acceptance-tests/desktop_automation.py) abre
`interface/build/xadrez.exe` e usa `Backend/build/main.exe`, nÃ£o os executÃ¡veis
dos diretÃ³rios `build-coverage`. As capturas visuais e a leitura passiva dos
logs do protocolo validam comportamento observÃ¡vel, mas nÃ£o criam contadores
`.gcda` para estes builds.

LimitaÃ§Ãµes desta mediÃ§Ã£o:

- Os percentuais descrevem os executÃ¡veis de teste instrumentados e os
  contadores atualmente disponÃ­veis; nÃ£o abrangem todo o ciclo de execuÃ§Ã£o dos
  executÃ¡veis finais.
- `main.c` do Backend nÃ£o foi incluÃ­do nos alvos de teste; `main.cpp` da
  Interface foi compilado, mas nÃ£o gerou `.gcda`. Esses pontos de entrada nÃ£o
  entram nos denominadores.
- Os testes de protocolo e a aceitaÃ§Ã£o desktop nÃ£o foram executados com os
  binÃ¡rios deste build instrumentado.
- A cobertura de branches depende da instrumentaÃ§Ã£o do GCC 16.2.0 e nÃ£o mede a
  qualidade das asserÃ§Ãµes nem garante que um comportamento esteja correto.
- A configuraÃ§Ã£o exata de flags do build de cobertura do Backend nÃ£o estÃ¡
  registrada no Makefile nem em um cache de build; a instrumentaÃ§Ã£o dos objetos
  Ã© comprovada pelos arquivos de dados, mas os argumentos especÃ­ficos
  `-O0 -g` nÃ£o puderam ser confirmados independentemente.
- O percentual antigo de 88,16% nÃ£o foi reutilizado como cobertura de linhas.
  O valor de 88,16% apresentado neste relatÃ³rio Ã© a cobertura atual de funÃ§Ãµes
  da Interface, calculada a partir dos contadores desta anÃ¡lise.

## 8. ConclusÃ£o

Os dados coletados indicam cobertura de linhas moderada no Backend (59,37%) e na
Interface (65,72%). O Backend tem funÃ§Ãµes e caminhos inteiros sem execuÃ§Ã£o,
especialmente nos mÃ³dulos de I/O, UCI e utilitÃ¡rios. Na Interface, a maioria das
funÃ§Ãµes foi alcanÃ§ada (88,16%), mas a cobertura de branches Ã© baixa (36,86%),
com lacunas maiores nos menus e no painel HUD.

Os relatÃ³rios detalhados permitem localizar as linhas especÃ­ficas nÃ£o
executadas. Para ampliar a mediÃ§Ã£o funcionalmente, seria necessÃ¡rio incluir
execuÃ§Ã£o instrumentada dos caminhos de protocolo e da aplicaÃ§Ã£o desktop; isso
nÃ£o foi feito nesta tarefa para preservar os builds e dados existentes.
