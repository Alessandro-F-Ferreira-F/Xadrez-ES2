# Xadrez-ES2

Jogo de xadrez em que o usuário joga contra o computador, desenvolvido como trabalho da
disciplina de Engenharia de Software 2 (2026.2). Licença [MIT](LICENSE).
Membros do projeto:
- Caio Veiga
- Eduardo Rottschaefer
- Alessandro Ferreira
- Felipe Cabral
- João Vitor Baiana
- José Viana
- Alllaan Vignolia

## Módulos

| Módulo | Linguagem | O que faz |
|---|---|---|
| [`Backend/engine/`](Backend/engine) | C11 | Máquina de regras: tabuleiro, FEN, geração e validação de lances, fim de partida (mate, afogamento, 50 lances, material insuficiente, repetição) e o protocolo de texto no estilo UCI |
| [`Backend/IA/`](Backend/IA) | C11 | Busca alfa-beta com aprofundamento iterativo e quiescência, avaliação por material e tabelas peça-casa, ordenação MVV-LVA e três níveis de dificuldade. Compilada dentro do binário do motor |
| [`interface/`](interface) | C++17 + SFML 2 | Cliente gráfico: tabuleiro, seleção por mouse com destaque dos lances legais, relógio, histórico e alertas de xeque e fim de jogo |
| [`tools/ctest/`](tools/ctest) | C | Mini-framework de testes do projeto (`TEST`, `EXPECT_*`, `ASSERT_*`) |
| [`agent/`](agent) | Python | Opcional, fora do produto: agente "Scrum Master" (FastAPI) integrado ao GitHub e ao WhatsApp |
| [`docs/`](docs) | — | Gestão do projeto e apresentações |

Como as partes conversam:

```
interface/build/xadrez ── stdin/stdout, uma linha por mensagem ──► Backend/build/main.out
                                                                    ├── engine: regras e protocolo
                                                                    └── IA: chamada pelo comando go
```

A interface executa o motor como subprocesso e fala o protocolo descrito em
[`uci-comandos.md`](Backend/engine/docs/uci-comandos.md). A posição vai sempre completa
(`position startpos moves ...` ou `position fen ...`). A interface não valida lances: pergunta
ao motor os lances legais e o estado da partida.

## Como compilar

Requisitos:

- **Backend**: `gcc` e `make`. No Windows, MinGW com `mingw32-make`.
- **Interface**: `g++` com C++17 e SFML 2 (no Ubuntu, `sudo apt install libsfml-dev`). Roda em
  Linux/POSIX; o lado Windows do `EngineBridge` ainda não foi implementado.
- **Testes de protocolo**: Python 3.

### Backend (motor + IA)

```sh
make -C Backend          # gera Backend/build/main.out (main.exe no Windows)
make -C Backend run      # abre o motor no terminal, para digitar comandos do protocolo
make -C Backend clean
```

### Interface

```sh
make -C Backend          # a interface precisa do motor compilado
make -C interface run    # compila interface/build/xadrez e abre o jogo
```

A interface procura o motor em `../Backend/build/main.out` e as imagens em `./assets/`, os dois
relativos à pasta `interface/`. Por isso, rode o jogo pelo `make -C interface run` ou de dentro
de `interface/`.

### Testes

| Comando | O que roda |
|---|---|
| `make -C Backend test` | Suíte C do motor: perft das seis posições da Chess Programming Wiki (até a profundidade 5, cerca de 1 minuto), aplicar/desfazer lances, empates e a IA ligada ao motor. `PERFT_MAX_DEPTH=3` encurta; `TEST_FILTER=texto` filtra pelo nome |
| `make -C Backend protocol` | Testes de protocolo (Python): transcrições `.in`/`.out`, transporte por pipe, legalidade do `go` e tempo de resposta por nível |
| `make -C interface test` | Fumaça, unidade, integração e sistema da interface, contra o motor real |

As suítes isoladas da IA (`Backend/IA/test/`) usam um stub das regras de empate e ainda não
têm alvo no Makefile:

```sh
cd Backend
for t in unit_test integration_test system_test; do
  gcc -std=c11 -Og -g -Iengine/include -IIA/header -IIA/test \
      IA/test/$t.c IA/src/{ia,avaliar,ordenar,niveis,stubs}.c ../tools/ctest/test_api.c \
      $(find engine/src -name '*.c' ! -name main.c) -o build/ia_$t.out && ./build/ia_$t.out
done
cd ..
```

## Nível da IA

| Nível | Profundidade | Prazo por lance | Quiescência |
|---|---|---|---|
| 1 — Fácil | 2 | 180 ms | não |
| 2 — Médio (padrão) | 3 | 180 ms | sim |
| 3 — Difícil | até 64, enquanto houver tempo | 950 ms | sim |

- **Escolher o nível**: envie `setoption name Nivel value N` (N de 1 a 3) antes do `go`. O
  nível vale até o próximo `setoption`, inclusive depois de `ucinewgame`.
- **Limitar um único lance**: `go depth N` ou `go movetime T` (em ms) substituem a
  profundidade ou o prazo do nível só naquele `go`; a quiescência continua a do nível.
- **Mudar o que cada nível faz**: edite a tabela em
  [`Backend/IA/src/niveis.c`](Backend/IA/src/niveis.c) (`PROF_FACIL`, `PROF_MEDIO`,
  `TEMPO_BASICO_MS`, `TEMPO_DIFICIL_MS`). Os `_Static_assert` do arquivo impedem prazos acima
  do RNF01 (200 ms e 1 s). Depois, rode `make -C Backend protocol`, que mede o tempo de cada
  nível.
- A interface ainda não oferece a escolha do nível: joga sempre no padrão (Médio).

## Documentação

**Técnica**

- [`Backend/engine/docs/project_context.md`](Backend/engine/docs/project_context.md) — contexto
  do motor: estado atual, decisões arquiteturais, contrato com a IA, mapa dos módulos e
  possíveis melhorias. Comece por aqui.
- [`Backend/engine/docs/uci-comandos.md`](Backend/engine/docs/uci-comandos.md) — protocolo do
  motor: comandos, respostas, erros e uma sessão de exemplo.
- [`Backend/IA/docs/`](Backend/IA/docs) — contexto da IA em cada etapa do desenvolvimento.
- [`Backend/IA/Pseudocode/`](Backend/IA/Pseudocode) — pseudocódigo dos algoritmos da IA.
- [`interface/README.md`](interface/README.md) — README da interface. Desatualizado: cita
  SFML 3 e CMake 3.28, mas o build usa SFML 2 e o `interface/Makefile`.
- [`AGENTS.md`](AGENTS.md) — instruções para agentes de código.

**Testes**

- [`docs/estrategia_testes.md`](docs/estrategia_testes.md) — estratégia de verificação,
  validação e testes dos três módulos.
- [`interface/docs/relatorio_testes.md`](interface/docs/relatorio_testes.md) — relatório de
  testes da interface.

**Gestão do projeto**

- [`docs/descricao.md`](docs/descricao.md) — enunciado do trabalho.
- [`docs/requisitos.md`](docs/requisitos.md) — requisitos funcionais e não funcionais.
- [`docs/EAP.md`](docs/EAP.md) e [`docs/EAP.jpeg`](docs/EAP.jpeg) — estrutura analítica do
  projeto.
- [`docs/estimativas_apf.md`](docs/estimativas_apf.md) e
  [`docs/estimativas_poker.md`](docs/estimativas_poker.md) — estimativas por pontos de função e
  Planning Poker.
- [`docs/levantamento_de_riscos .md`](docs/levantamento_de_riscos%20.md) — riscos, exposição e
  planos de contenção e contingência.
- [`docs/cronograma_gantt.pdf`](docs/cronograma_gantt.pdf) — cronograma (fontes editáveis:
  `cronograma_gantt.pod` e `cronograma_gantt_Atualizado.pod`).
- [`docs/sprints.md`](docs/sprints.md) — sprints, tarefas e horas.
- [`docs/monitoramento_controle.md`](docs/monitoramento_controle.md) — burndown e análise de
  valor agregado.

**Apresentações**

- [`docs/apresentacao.md`](docs/apresentacao.md) (fonte Marp) e
  [`docs/apresentacao.pdf`](docs/apresentacao.pdf) — slides da primeira apresentação.
- [`docs/roteiro_apresentacao.md`](docs/roteiro_apresentacao.md) — roteiro da primeira
  apresentação.
- [`docs/Xadrez-ES2_Primeira_Apresentacao.pptx`](docs/Xadrez-ES2_Primeira_Apresentacao.pptx) —
  versão em PowerPoint.
