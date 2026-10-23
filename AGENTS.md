# AGENTS.md

Instruções para agentes de código (OpenCode, Claude Code, Codex, etc.) neste repositório.
O projeto é um xadrez de ES2 com
três partes independentes que se comunicam por texto (FEN + protocolo estilo UCI):

- `engine/` — regras e protocolo, em C11 (executável `build/main.out`).
- `interface/` — cliente gráfico C++17/SFML 2 (executável `build/xadrez`).
- `IA/` — busca minimax/alfa-beta em C, **sem build próprio e ainda não ligada ao motor**.
- `agent/` — API FastAPI opcional (Scrum Master via OpenRouter/GitHub MCP/WhatsApp).

Não há CI. Verificação é local; `make test` e o runner de protocolo são as fontes de verdade.

## Comandos verificados

Tudo a partir da raiz do repositório:

```sh
make -C engine                 # build padrão; gera engine/build/main.out
make -C engine test            # suíte C (~65 s no padrão); gera engine/build/test.out
PERFT_MAX_DEPTH=3 make -C engine test   # reduz perft (padrão é 5)
TEST_FILTER=perft_pos3 make -C engine test   # filtra testes pelo nome
python3 engine/test/protocol/run.py     # testes de protocolo; exige engine compilado
python3 engine/test/protocol/run.py /caminho/do/motor   # alvo alternativo
make -C interface              # gera interface/build/xadrez (requer SFML 2 dev)
cmake -S interface -B interface/build && cmake --build interface/build
```

`make -C engine debug` e `make -C engine release` estão **quebrados** (usam `$(SRC)`, que
não existe no Makefile e resulta em "no input files"). Para debug funcional, edite as flags
de `CFLAGS`/`DBGFLAGS` ou compile manualmente; não presuma que esses alvos funcionam.

Ordem útil antes de finalizar mudanças no motor:

1. `make -C engine`
2. `python3 engine/test/protocol/run.py`
3. `make -C engine test` (ou `PERFT_MAX_DEPTH=3` durante o desenvolvimento)

O runner de protocolo testa resposta por pipe **com stdin aberta** e encerramento por EOF.
Não substitua por `printf ... | motor`: isso esconde buffer preso.

## Motor: arquitetura e armadilhas

- Entrypoint único: `main()` em `engine/src/main.c` sempre chama `uci_main()`. O menu
  interativo (`ui()`) ainda compila, mas o caminho `./build/main.out repl` está comentado;
  não conte com REPL.
- A camada de protocolo é `engine/src/uci.c`. Comandos: `uci`, `isready`, `ucinewgame`,
  `position`, `legalmoves`, `state`, `go`, `go perft N`, `d`, `quit`. `--trace arquivo` grava
  o diálogo. Documentação correta: `engine/docs/uci-comandos.md`.
- `go` **sorteia** um lance legal com semente fixa; a IA não está conectada. `state` cobre
  xeque/mate/afogamento/50 lances, mas não repetição nem material insuficiente.
- `position` é atômico: monta um `Board` de rascunho e só troca a sessão no fim. Mantenha
  assim.
- `is_square_attacked(b, sq, side)`: `side` é o **dono/defensor** da casa, não o atacante.
  Inverter esse argumento não gera aviso do compilador.
- `make_move` não valida legalidade; a fronteira (`position`, REPL) é quem valida. Sempre
  aplique lances vindos de `generate_legal_moves`.
- Codificação: `a1 = 0`, `Piece = (color << 3) | type`, `Move` é `u16` (6 bits origem,
  6 destino, 4 tipo). `PIECE_COLOR(EMPTY) == BLACK`.
- `perft()` mora em `engine/src/perft.c`; `perft_divide` ainda está em `main.c`.
- Warnings fazem parte do acordo: C11 com `-Wall -Wextra -Wpedantic -Wstrict-prototypes
  -Wmissing-prototypes`. Não silencie warnings novos.
- Em `MoveType`, promoção e roque são **códigos comparados por igualdade**, não bits; use
  máscara só em `move_is_capture`/`move_is_promotion`.

## Build e git

- `build/`, `interface/build/`, `*.out`, `*.o` e `.venv/` são ignorados. Binários e caches
  existentes são artefatos locais; não os versione.
- `.gitignore` da raiz ignora `*.py`, `*.out` e o próprio `.gitignore`. `engine/test/protocol/.gitignore`
  re-inclui `*.out`, `*.py` e `.gitignore` **de propósito**: os `.out` ali são saídas
  esperadas, não binários. Não remova essas exceções.
- `CLAUDE.md` é ignorado pelo git; não é fonte de verdade disponível no clone.
- Commits pequenos, em português, e uma mudança comportamental por commit. Refatoração que
  move código não deve alterar comportamento no mesmo commit.
- CODEOWNERS: `engine/` @caioveigap, `interface/` @Rottschaefer/@Alvidosa, `IA/`
  @Alessandro-F-Ferreira-F.

## Documentação: cuidado com desatualização

`README.md`, `engine/docs/project_context.md` e `engine/docs/next_steps.md` são de
**2026-10-04**, anteriores à implementação do UCI. Eles ainda dizem que o protocolo "não
existe" e que `make test` "não linka" — ambas as coisas já estão resolvidas. Para o estado
real, confie em `engine/src/uci.c`, `engine/test/test.c`, `engine/docs/uci-comandos.md` e nos
testes. Vários comentários citam `protocolo.md`, que **não existe no repositório**.

## Interface

- `interface/README.md` está errado para o código atual: fala SFML 3.0, CMake 3.28 e
  `build/bin/main`; o build real é SFML 2, CMake mínimo 3.20 e binário `build/xadrez`.
  Prefira `interface/Makefile` / `interface/CMakeLists.txt`.
- `EngineBridge` só implementa o spawn POSIX (`fork`/`execlp`) e `sendCommand`;
  `readCommand`, `stop` e o ramo Windows são `TODO`. `main.cpp` hoje só processa um comando
  UCI fixo localmente — a interface ainda não conversa de fato com o motor.

## IA e agente

- `IA/` inclui headers de `engine/` e chama `generate_legal_moves`/`make_move` diretamente,
  mas não tem Makefile nem target no build do motor. `IA/src/teste.c` tem `main` próprio para
  testes manuais; compile-o explicitamente se for mexer nessa pasta.
- `agent/` exige `.env` (copie de `agent/.env.example`) com `OPENROUTER_API_KEY`;
  `GITHUB_PERSONAL_ACCESS_TOKEN` é opcional e o orquestrador usa `npx` + MCP oficial do
  GitHub. Rodar: `cd agent && uvicorn main:app --port 8000`. `whatsapp_client.py` roda na
  porta 3001 e usa sessão SQLite local (`whatsapp_session.sqlite3`, ignorada).
- `agent/requirements.txt` **não lista `segno`**, importado por `whatsapp_client.py`.
  Instale-o à parte se for subir o serviço de WhatsApp.
