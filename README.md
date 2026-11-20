# Xadrez-ES2

Jogo de xadrez desenvolvido para a disciplina de Engenharia de Software 2 — 2026.2.

## Visão Geral

O projeto é dividido em módulos independentes que se comunicam por protocolo de texto no estilo UCI (`stdin`/`stdout`):

- **`Backend/`** — motor de regras (`Backend/engine/`) e busca minimax/alfa-beta (`Backend/IA/`) em C11, compilados em um único executável (`Backend/build/main.out`).
- **`interface/`** — cliente gráfico em C++17 com SFML 2 (`interface/build/xadrez`), que executa o backend como subprocesso via `EngineBridge`.
- **`tools/ctest/`** — biblioteca leve de testes em C/C++ compartilhada entre `Backend/` e `interface/`.
- **`agent/`** — API FastAPI opcional (Scrum Master via OpenRouter / GitHub MCP / WhatsApp).

## Estrutura

```text
Backend/
  Makefile             Compila engine + IA e suas suítes de teste
  engine/              Regras, FEN, perft, protocolo UCI e testes de protocolo (Python)
  IA/                  Busca minimax/alfa-beta, avaliação, níveis e suítes de teste
interface/
  Makefile             Compila o cliente gráfico SFML 2 e os testes de interface
  src/                 Tabuleiro, peças, HUD e ponte de comunicação (EngineBridge)
  test/                Testes de smoke, unidade, integração e sistema
tools/
  ctest/               Runner e macros de asserção compartilhados (test_api.h)
agent/                 Serviço FastAPI opcional de apoio ao time
Makefile               Makefile global na raiz do repositório
```

## Pré-requisitos

- `gcc`, `g++` (C11 / C++17), `make` e `python3`
- **SFML 2** (headers de desenvolvimento: `libsfml-dev` no Debian/Ubuntu)

## Como compilar, rodar e testar

Todos os comandos abaixo são executados a partir da **raiz do repositório**:

```sh
make                 # compila Backend (Backend/build/main.out) e Interface (interface/build/xadrez)
make run             # compila e abre o jogo gráfico conectado à IA
make test            # roda toda a suíte (Backend/engine com perft d3 + IA + protocolo UCI + interface)
make test-full       # roda toda a suíte com perft completo (profundidade 5)
make clean           # remove os binários de Backend/build e interface/build
```

Alvos individuais disponíveis:

```sh
make test-backend    # testes C da engine + IA
make test-engine     # apenas a suíte de regras/busca da engine
make test-ia         # apenas os testes de unidade, integração e sistema da IA
make test-protocol   # suíte Python do protocolo UCI (Backend/engine/test/protocol/run.py)
make test-interface  # testes de smoke, unidade, integração e sistema da interface
```

Também é possível compilar cada módulo diretamente com `make -C Backend` ou `make -C interface`.

## Documentação

- [`Backend/engine/docs/project_context.md`](Backend/engine/docs/project_context.md) — arquitetura do motor, decisões técnicas e contrato com a IA.
- [`Backend/engine/docs/uci-comandos.md`](Backend/engine/docs/uci-comandos.md) — especificação do protocolo UCI implementado.
- [`Backend/engine/docs/onboarding-motor.md`](Backend/engine/docs/onboarding-motor.md) — guia introdutório sobre o funcionamento do motor.

## Convenções

- **Backend:** C11 com `-Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes`.
- **Interface:** C++17 com `-Wall -Wextra` e SFML 2.
- **Testes:** padronizados com `tools/ctest/test_api.h` (`TEST`, `EXPECT_*`, `ASSERT_*`).
- **Commits:** mensagens claras e em português.
