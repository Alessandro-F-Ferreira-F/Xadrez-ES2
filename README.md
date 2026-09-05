# Xadrez-ES2

Xadrez desenvolvido como trabalho da disciplina de Engenharia de Software 2 — 2026.2.

# AVISO!!
O arquivo [`estado-do-projeto`](engine/docs/onboarding-motor.md) contém o estado atual do projeto.<br>
Leiam para entender o que já foi feito até o momento.

Juntamente a este arquivo, há o documento [`roadmap`](engine/docs/roadmap-motor.md) que explica quais são os próximos passos.<br>
*(Tem uma explicação mais detalhada do código)*

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
engine/          Motor em C
  src/           Código-fonte
  docs/          Documentação técnica
README.md
LICENSE
```

## Compilando o motor

Requer `gcc` e `make`.

```sh
cd engine
make            # build de desenvolvimento
make run        # executa
make debug      # build com sanitizers (ASan) e warnings extras
make clean
```

O binário sai em `engine/build/main.out`.

## Estado atual

O motor está em fase de protótipo. O que já funciona:

- Leitura e validação de FEN, com detecção dos erros mais comuns (rei ausente ou duplicado,
  peão em fileira inválida, fileira incompleta, excesso de peças).
- Serialização de volta para FEN.
- Representação do tabuleiro em *mailbox* de 64 casas, com `a1 = 0`.
- Tabela de distância até a borda para cada casa e direção, base da geração de lances das
  peças deslizantes.
- Geração de lances de peão (avanço simples).
- Aplicação de um lance sobre o tabuleiro.

Em desenvolvimento:

- Geração completa de lances: torre, bispo, rainha, cavalo, rei; captura, roque, en passant
  e promoção.
- Filtro de legalidade e detecção de xeque.
- Validação por `perft` contra os valores de referência publicados.
- Protocolo `stdin`/`stdout` ponta a ponta.
- Avaliação e busca.

## Documentação

- [`engine/docs/project_context.md`](engine/docs/project_context.md) — visão geral do motor:
  decisões arquiteturais e o raciocínio por trás delas, mapa dos módulos, convenções de
  codificação e próximos passos. **Comece por aqui** se for mexer no motor.
- [`engine/docs/arquitetura-xadrez.md`](engine/docs/arquitetura-xadrez.md) — desenho do
  sistema completo.
- [`engine/docs/fen.md`](engine/docs/fen.md) — notas sobre o formato FEN e a indexação
  do tabuleiro.
- [`engine/docs/refs.md`](engine/docs/refs.md) e
  [`engine/docs/biblioteca-referencias-chess-engine.md`](engine/docs/biblioteca-referencias-chess-engine.md)
  — referências usadas.

## Convenções

- **C11**, compilado sempre com warnings ligados: `-Wall -Wextra -Wpedantic`.
- Builds de depuração rodam sob sanitizers (`-fsanitize=address,undefined`).
- Mensagens de commit em português.
- O histórico de `engine/` anterior a este repositório foi importado de um repositório de
  desenvolvimento separado, o que explica as mensagens em inglês nos commits mais antigos.
