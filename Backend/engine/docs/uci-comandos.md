# Protocolo do motor

Como conversar com `engine/build/main.out`: um subconjunto do UCI mais extensões do projeto.
Implementação em `engine/src/uci.c`; contrato testado em `engine/test/protocol/`. Nos
exemplos, `>` é o que o cliente envia e `<` o que o motor responde (o formato do `--trace`).

## Transporte

- Uma mensagem por linha: comandos pela stdin, respostas pela stdout. `\r\n` é aceito.
  Tokens separados por espaço ou tab; comandos diferenciam maiúsculas.
- Linha vazia e comando desconhecido são ignorados em silêncio, como no UCI.
- A stderr é diagnóstico livre (por exemplo, o motivo de uma FEN recusada); o cliente não deve
  lê-la. Cada linha da stdout sai com `fflush`, então o pipe não segura respostas.
- Linha de até 8193 bytes, sem contar o `\n`. Mais que isso gera `error line-too-long` e a
  linha inteira é descartada.
- `quit` ou EOF na stdin encerram o motor com código 0. Um comando por vez: durante um `go`
  a stdin não é lida, e o que chegar é processado depois do `bestmove`, em ordem.
- `main.out --trace arquivo` grava o diálogo (`> ` recebido, `< ` enviado). Sem o nome do
  arquivo o motor sai com código 2; argumento desconhecido gera aviso na stderr e é ignorado.

## Comandos

| Comando | Resposta | Semântica |
|---|---|---|
| `uci` | `id name`, `id author`, `option`, `uciok` | Identificação e opções (ver a sessão típica) |
| `isready` | `readyok` | Sincronização |
| `ucinewgame` | nenhuma | Volta à posição inicial, limpa o histórico e reinicia a semente do sorteio. Mantém o nível |
| `setoption name Nivel value N` | nenhuma | Nível da IA, N de 1 a 3 (padrão 2). Outras opções são ignoradas |
| `position startpos [moves L ...]` | nenhuma, ou `error` | Define a posição a partir da inicial |
| `position fen <FEN> [moves L ...]` | nenhuma, ou `error` | Define a posição a partir de uma FEN de seis campos |
| `legalmoves` | `legalmoves L L ...` | Lances legais, em ordem livre. Só `legalmoves` (sem lances) não indica fim de partida: quem diz é o `state` |
| `state` | `state check ... fen ...` | Situação da partida |
| `go [depth N] [movetime T]` | `info ...` e `bestmove L` | Lance escolhido pela IA. Não aplica o lance |
| `go perft N` | `L: n` por lance e `Nodes searched: total` | Contagem de nós, N de 1 a 8 |
| `d` | desenho e `fen <FEN>` | Só depuração: o desenho não é contrato |
| `quit` | nenhuma | Encerra |

### `position`

- **Atômico**: ou a posição inteira é aceita, ou nada muda. Se um lance da lista é ilegal, o
  motor fica na posição que tinha antes do comando. Quando aceito, não há resposta.
- A FEN vem direto nos tokens, sem aspas, sempre com os seis campos. FEN inválida ou
  impossível (inclusive com o lado que não joga em xeque) é recusada. No máximo 1024 lances.
- Lances em coordenadas, minúsculas: `e2e4`. Roque é o lance do rei (`e1g1`, `e1c1`); en
  passant, o do peão (`e5d6`); promoção acrescenta `n`, `b`, `r` ou `q` (`e7e8q`).
  `e7e8` e `e7e8Q` são recusados.

### `state`

```
state check <casa|-> result <resultado> reason <motivo> fen <FEN>
```

| Campo | Valores |
|---|---|
| `check` | casa do rei do lado a jogar se está em xeque (ex.: `e1`); senão `-` |
| `result` | `*` (em andamento), `1-0`, `0-1`, `1/2-1/2` |
| `reason` | `none`, `checkmate`, `stalemate`, `fifty-moves`, `insufficient-material`, `repetition` |
| `fen` | sempre o último campo; ocupa o resto da linha |

Mate e afogamento têm precedência; depois vêm 50 lances (relógio ≥ 100, sem reclamação),
material insuficiente e repetição tripla, que usa o histórico da última
`position ... moves ...` (uma FEN isolada não tem passado). A FEN do motor marca a casa de en
passant após todo avanço duplo, mesmo sem captura possível.

```
> position startpos moves f2f3 e7e5 g2g4 d8h4
> state
< state check e1 result 0-1 reason checkmate fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3
> legalmoves
< legalmoves
```

### `go`

```
< info depth <D> score cp|mate <V> nodes <K>
< bestmove <lance>
```

- Sem limites, valem os parâmetros do nível. `depth N` (1 a 64) substitui a profundidade e
  desliga o prazo. `movetime T` (ms, 0 a 2147483647) substitui o prazo e libera a
  profundidade até 64; `0` desliga o relógio. Com os dois, para no primeiro limite. Os
  limites valem só para este `go`; quiescência e sorteio seguem o nível.
- `info`: `depth` é a profundidade da iteração aproveitada (0 se só a passada de segurança
  terminou no prazo); `score` é da perspectiva de quem joga, `cp` em centipeões ou `mate` em
  lances completos (positivo: quem joga dá mate); `nodes` soma todas as iterações. A busca
  para antes do limite quando acha mate.
- Partida terminada (`result` diferente de `*`, inclusive repetição) ou falha interna da IA:
  só `bestmove 0000`, sem `info`.
- **`go` não aplica o lance**: o cliente reenvia `position ... moves ... <lance>`.
- Entre lances empatados, sorteio com semente fixa: sem relógio, a mesma sequência de
  comandos dá as mesmas respostas; com prazo, a velocidade da máquina pode mudar o resultado.

```
> position fen 7k/8/5KQ1/8/8/8/8/8 w - - 0 1
> go depth 3
< info depth 1 score mate 1 nodes 52
< bestmove g6g7
> position fen k7/8/1K6/8/8/8/8/7R b - - 0 1
> go depth 3
< info depth 2 score mate -1 nodes 43
< bestmove a8b8
> position startpos moves f2f3 e7e5 g2g4 d8h4
> go
< bestmove 0000
```

### Níveis da IA

Todos usam alfa-beta e sorteio entre lances empatados (`IA/src/niveis.c`). Os prazos ficam
abaixo do requisito de resposta (200 ms nos níveis 1 e 2, 1 s no 3), que o `run.py` mede. O
nível vale até outro `setoption`; `setoption` inválido não o altera.

| Nível | Profundidade | Prazo | Quiescência |
|---|---|---|---|
| 1 (fácil) | 2 | 180 ms | não |
| 2 (médio, padrão) | 3 | 180 ms | sim |
| 3 (difícil) | 64 | 950 ms | sim |

### `go perft N`

Formato do Stockfish, sem a linha em branco antes do total. Cada linha sai quando o lance da
raiz termina, em ordem que não faz parte do contrato. Não altera a posição. Sem lances
legais, só `Nodes searched: 0`. Profundidades altas ocupam o motor por minutos.

```
> position fen k7/8/1K6/8/8/8/8/7R b - - 0 1
> go perft 3
< a8b8: 45
< Nodes searched: 45
```

## Erros

Formato `error <código> [detalhe]`. O erro é a única resposta do comando que falhou; posição
e nível não mudam, e o motor continua respondendo.

| Mensagem | Quando |
|---|---|
| `error bad-command position` | `position` sem `startpos`/`fen`; FEN com menos de seis campos ou com `moves` no meio; palavra diferente de `moves` depois da posição |
| `error bad-fen` | FEN de seis campos inválida ou impossível; o motivo vai para a stderr |
| `error illegal-move <lance> <i>` | o lance de índice `i` (a partir de 0) não é legal ou não tem 4 ou 5 caracteres; o lance aparece truncado em 19 caracteres |
| `error too-many-moves` | mais de 1024 lances em `position`, ou 1040 tokens ou mais numa linha |
| `error bad-command setoption` | `setoption` sem `name`, ou `Nivel` sem exatamente `value N` com N de 1 a 3 |
| `error bad-command go` | `depth`/`movetime` ausente, inválido, fora da faixa ou repetido; `go perft` sem N, com N fora de 1 a 8 ou com argumento extra |
| `error line-too-long` | linha com mais de 8193 bytes |

## Sessão típica

Como a interface joga: reenvia a partida inteira a cada lance, consulta o `state`, pede o
`go` e reenvia a partida com o lance do motor.

```
> uci
< id name Xadrez-ES2 0.1
< id author Equipe Xadrez-ES2
< option name Nivel type spin default 2 min 1 max 3
< uciok
> isready
< readyok
> ucinewgame
> position startpos moves e2e4
> state
< state check - result * reason none fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1
> go
< info depth 3 score cp 10 nodes 14414
< bestmove b8c6
> position startpos moves e2e4 b8c6
> state
< state check - result * reason none fen r1bqkbnr/pppppppp/2n5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2
> legalmoves
< legalmoves a2a3 a2a4 b2b3 b2b4 c2c3 c2c4 d2d3 d2d4 f2f3 f2f4 g2g3 g2g4 h2h3 h2h4 e4e5 d1e2 d1f3 d1g4 d1h5 f1e2 f1d3 f1c4 f1b5 f1a6 e1e2 b1c3 b1a3 g1h3 g1f3 g1e2
> position startpos moves e2e4 b8c6 e1e3
< error illegal-move e1e3 2
> quit
```

## Limitações

- Sem `stop` nem `ponderhit`; a stdin não é lida durante `go`.
- `wtime`, `btime`, `winc`, `binc`, `movestogo` e `infinite` são ignorados: o prazo vem do
  nível ou de `movetime`.
- `go movetime 0` sem `depth` busca sem prazo até a profundidade 64: na prática não termina.
- A repetição só é vista na partida, não dentro da busca da IA.
- Quinto caractere em lance que não é promoção é ignorado: `e2e4x` vale `e2e4`.
- Uma única posição por processo; o histórico da partida é do cliente.
- Única opção: `Nivel`. `position fen` exige os seis campos.
