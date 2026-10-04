# Xadrez-ES2

Xadrez desenvolvido como trabalho da disciplina de Engenharia de Software 2 — 2026.2.

# AVISO!!
O arquivo [`estado-do-projeto`](engine/docs/onboarding-motor.md) contém o estado atual do motor, escrito para quem não programa em C (§6).<br>
Leiam para entender o que já foi feito até o momento.

O estado técnico detalhado está em [`project_context`](engine/docs/project_context.md), e os próximos passos — até o protocolo UCI e, depois dele, a otimização — em [`next_steps`](engine/docs/next_steps.md).<br>
O [`roadmap`](engine/docs/roadmap-motor.md) mostra as fases do motor e o status de cada uma.

## O projeto

O sistema é dividido em duas partes que conversam por um protocolo de texto:

- **Motor de regras e IA**, escrito em C, neste repositório sob `engine/`.
- **Cliente gráfico**, que desenha o tabuleiro e recebe os lances do jogador.

O motor é um executável que lê comandos por `stdin` e responde por `stdout`, uma linha por
mensagem, em protocolo no estilo UCI. O cliente o executa como subprocesso. Essa fronteira
é deliberada: o motor não sabe nada sobre interface, e o cliente não sabe nada sobre as
regras do xadrez.

A posição é sempre trafegada como **FEN completa**, e não como histórico incremental — o
que torna qualquer posição reproduzível isoladamente em teste.

## Estrutura

```
engine/          Motor de regras, em C
  include/       Headers
  src/           Código-fonte
  test/          Teste de aplicar/desfazer lances
  docs/          Documentação técnica
IA/              Busca e avaliação, em C (usa os headers de engine/)
interface/       Cliente gráfico, em C++ com SFML
README.md
LICENSE
```

## Compilando o motor

Requer `gcc` e `make`.

```sh
cd engine
make            # build de desenvolvimento
make run        # executa
make debug      # build com sanitizers (ASan/UBSan) — use este para testar
make release    # build otimizado (-O3)
make test       # teste de ida e volta make/unmake — QUEBRADO em 04/10, ver abaixo
make clean
```

O binário sai em `engine/build/main.exe` no Windows e `engine/build/main.out`
nos demais sistemas. No Windows, compile e execute com `mingw32-make all` e
`mingw32-make run`; nos demais sistemas, use `make all` e `make run`.

Hoje o binário abre um menu de teste. A opção 12 roda o teste de ida e volta make/unmake
(o `make test` não linka no momento — `project_context.md` §5, Bug #4), e as opções 10 e 13
rodam o perft e o perft *divide*. A lista de FENs do teste fica em `engine/test/test.c` e
pode ser ampliada adicionando posições ao vetor.

## Estado atual (4 de outubro de 2026)

**As regras do xadrez estão completas e provadas.** O que já funciona:

- Leitura e escrita de FEN, com os seis campos e validação completa.
- Geração de todos os lances — todas as peças, roque, en passant, promoção nas quatro peças.
- Aplicar e desfazer lances, testado em mais de 11 milhões de posições sem falha.
- Filtro de legalidade: o motor descarta lances que deixam o próprio rei em xeque, incluindo
  as regras especiais do roque.
- **Perft exato** nas seis posições de referência da Chess Programming Wiki (a posição
  inicial até a profundidade 7: 3 195 901 860 nós). Os números estão em
  [`engine/docs/perft_results.txt`](engine/docs/perft_results.txt).

Em desenvolvimento:

- Protocolo `stdin`/`stdout` no estilo UCI — o motor ainda não responde aos comandos da
  interface. É o próximo passo.
- Detecção de fim de partida (mate, afogamento) e empates por regra.
- Ligação da IA (`IA/`) com o motor.
- Depois do protocolo: otimização, com etapas medidas ([`next_steps.md`](engine/docs/next_steps.md), Parte II).

## Documentação

- [`engine/docs/project_context.md`](engine/docs/project_context.md) — visão geral do motor:
  estado atual verificado, decisões arquiteturais e o raciocínio por trás delas, mapa dos
  módulos, bugs abertos. **Comece por aqui** se for mexer no motor.
- [`engine/docs/next_steps.md`](engine/docs/next_steps.md) — plano de execução: do estado
  atual até o protocolo UCI, e depois as etapas de otimização.
- [`engine/docs/roadmap-motor.md`](engine/docs/roadmap-motor.md) — as fases do motor e o
  status de cada uma.
- [`engine/docs/onboarding-motor.md`](engine/docs/onboarding-motor.md) — guia de contexto
  para quem está chegando, sem exigir C.
- [`engine/docs/arquitetura-xadrez.md`](engine/docs/arquitetura-xadrez.md) — desenho do
  sistema completo (motor, IA, interface).
- [`engine/docs/fen.md`](engine/docs/fen.md) — notas sobre o formato FEN e a indexação
  do tabuleiro.
- [`engine/docs/biblioteca-referencias-chess-engine.md`](engine/docs/biblioteca-referencias-chess-engine.md)
  — referências usadas.

## Convenções

- **C11**, compilado sempre com warnings ligados: `-Wall -Wextra -Wpedantic`.
- Builds de depuração rodam sob sanitizers (`-fsanitize=address,undefined`).
- Mensagens de commit em português.
- O histórico de `engine/` anterior a este repositório foi importado de um repositório de
  desenvolvimento separado, o que explica as mensagens em inglês nos commits mais antigos.
