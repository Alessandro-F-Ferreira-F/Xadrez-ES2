# Protocolo do motor: comandos e formatos

Referência rápida do que o motor (`build/main.out`) aceita pela **stdin** e responde pela
**stdout**. É um subconjunto do UCI mais algumas extensões do projeto. Implementação em
`src/uci.c`; testes em `test/protocol/` (`python3 test/protocol/run.py`).

## Regras gerais

- **Uma mensagem por linha**, terminada em `\n` (`\r\n` também é aceito). Os campos de uma
  linha são separados por espaços ou tabs.
- O motor responde **só na stdout**. A stderr é diagnóstico para humanos, em formato livre,
  e não deve ser lida pelo cliente.
- Linha vazia é ignorada. **Comando desconhecido é ignorado em silêncio** (regra do UCI).
  Os comandos diferenciam maiúsculas de minúsculas: `Position` não é `position`.
- Uma linha pode ter até **8192 bytes** de conteúdo. Mais que isso gera
  `error line-too-long`, e o resto da linha é descartado.
- `quit`, ou o fim da entrada (EOF), encerra o motor com código de saída 0.
- O motor guarda **uma posição**, a última aceita por `position`. `state`, `legalmoves`, `go`
  e `d` sempre se referem a ela.
- Ao iniciar, a posição é a inicial.

```sh
./build/main.out                  # fala o protocolo
./build/main.out --trace log.txt  # idem, e grava o diálogo em log.txt
```

No arquivo de trace, as linhas recebidas começam com `> ` e as enviadas com `< `.

## Comandos

| Comando | Resposta |
|---|---|
| `uci` | `id name ...`, `id author ...`, `uciok` |
| `isready` | `readyok` |
| `ucinewgame` | nenhuma (volta à posição inicial) |
| `position ...` | nenhuma se aceita; `error ...` se recusada |
| `legalmoves` | `legalmoves <lance> <lance> ...` |
| `state` | `state check ... result ... reason ... fen ...` |
| `go [parâmetros]` | `bestmove <lance>` |
| `go perft <n>` | uma linha por lance da raiz, mais `Nodes searched: <total>` |
| `d` | desenho do tabuleiro e `fen <fen>` (só para depuração) |
| `quit` | nenhuma (o motor termina) |

### `uci`

```
> uci
< id name Xadrez-ES2 0.1
< id author Equipe Xadrez-ES2
< uciok
```

### `isready`

```
> isready
< readyok
```

### `position`

Define a posição. Duas formas, ambas com a lista opcional de lances no fim:

```
position startpos [moves <lance> <lance> ...]
position fen <peças> <lado> <roque> <en-passant> <meio-lances> <lance-número> [moves <lance> ...]
```

- A FEN tem **sempre os seis campos**, separados por espaço (isto é, vem direto nos tokens
  da linha, sem aspas).
- **Atômico:** ou a posição inteira é aceita, ou nada muda. Se o quinto lance de uma lista é
  ilegal, o motor continua na posição que tinha antes do comando.
- A FEN não pode ter o lado que **não** joga em xeque, nem faltar um rei.
- No máximo **1024 lances** por comando.
- Sem erro, o motor não responde nada. Para conferir, mande `state`.

```
> position startpos moves e2e4 e7e5
> position fen 7k/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8q
```

**Formato dos lances** (notação de coordenadas, tudo em minúsculas):

| Lance | Formato | Exemplo |
|---|---|---|
| normal e captura | `<origem><destino>` | `e2e4`, `d5e6` |
| roque | lance do rei, duas casas | `e1g1` (curto), `e1c1` (longo) |
| en passant | lance do peão, como captura comum | `e5d6` |
| promoção | acrescenta a peça: `n`, `b`, `r` ou `q` | `e7e8q` |

Promoção **sem** a letra (`e7e8`) e promoção em maiúscula (`e7e8Q`) são recusadas.

### `legalmoves`

Lista os lances legais da posição atual, separados por espaço. **A ordem não é garantida.**
Sem nenhum lance legal, a linha é só a palavra `legalmoves`.

```
> position startpos moves e2e4 e7e5
> legalmoves
< legalmoves a2a3 a2a4 b2b3 b2b4 c2c3 c2c4 d2d3 d2d4 f2f3 f2f4 g2g3 g2g4 h2h3 h2h4 d1e2 ...
```

### `state`

Uma linha com a situação da partida:

```
state check <casa|-> result <resultado> reason <motivo> fen <fen>
```

| Campo | Valores |
|---|---|
| `check` | casa do rei do lado a jogar, se está em xeque (ex.: `e1`); senão `-` |
| `result` | `*` (em andamento), `1-0`, `0-1`, `1/2-1/2` |
| `reason` | `none`, `checkmate`, `stalemate`, `fifty-moves` |
| `fen` | **sempre o último campo**, e ocupa o resto da linha (a FEN tem espaços) |

Mate e afogamento têm precedência sobre a regra dos 50 lances. Repetição tripla e material
insuficiente **ainda não** são detectados.

```
> position startpos moves f2f3 e7e5 g2g4 d8h4
> state
< state check e1 result 0-1 reason checkmate fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3
```

Quem decide o fim da partida é o `result` do `state`, **não** o `legalmoves` vazio: na regra
dos 50 lances ainda existem lances, e a partida acabou.

### `go`

```
go [qualquer coisa]
→ bestmove <lance>
```

Devolve um lance **legal**, sorteado por uma sequência pseudoaleatória de semente fixa (a
mesma sequência de comandos dá sempre a mesma resposta). Os parâmetros (`movetime`, `depth`,
`wtime`, ...) são aceitos e **ignorados**. Sem lance legal, responde `bestmove 0000`.

### `go perft <n>`

Conta as posições alcançáveis em `n` meios-lances, lance da raiz por lance da raiz. O formato
é o do Stockfish (`go perft`), o que permite comparar as duas saídas.

```
go perft <n>      (n de 1 a 8)
→ <lance>: <contagem>      (uma linha por lance legal da raiz, na ordem de geração)
→ ...
→ Nodes searched: <total>
```

```
> position startpos
> go perft 2
< a2a3: 20
< a2a4: 20
< ...
< h2h4: 20
< Nodes searched: 400
```

- Cada linha sai assim que o lance da raiz termina, então nas profundidades altas dá para ver o
  progresso. A ordem das linhas não é garantida; o que vale é o conjunto e o total.
- Diferente do Stockfish, **não há linha em branco** antes de `Nodes searched`.
- Não altera a posição do motor.
- Posição sem lance legal (mate, afogamento): só `Nodes searched: 0`.
- `n` ausente, `0`, negativo, não numérico, maior que 8 ou com argumentos a mais gera
  `error bad-command go`.
- **Tempo:** em `-O3` o motor faz uns 27 milhões de nós por segundo. A profundidade 6 da
  posição inicial leva ~5 s, a 7 leva ~2 min e a 8 leva ~50 min. Não existe `stop`: enquanto
  um `go` roda, o motor não lê a entrada.

### `d`

Imprime o tabuleiro desenhado para humanos e, na última linha, `fen <fen>`. O desenho é de
formato livre e **o cliente não deve interpretá-lo**.

### `quit`

Encerra o motor.

## Erros

Formato: `error <código> [detalhe]`. Um erro aparece **no lugar** da resposta do comando
que falhou; os demais comandos continuam funcionando.

| Mensagem | Quando |
|---|---|
| `error bad-command position` | `position` sem argumento, com palavra desconhecida depois, FEN com menos de seis campos ou com `moves` no meio da FEN |
| `error bad-command go` | `go perft` com profundidade inválida |
| `error bad-fen` | a FEN tem seis campos mas é inválida, ou descreve uma posição impossível |
| `error illegal-move <token> <índice>` | o lance na posição `<índice>` da lista (começa em 0) não é legal; o `<token>` aparece truncado em 19 caracteres |
| `error too-many-moves` | mais de 1024 lances em `position`, ou tokens demais na linha |
| `error line-too-long` | linha com mais de 8193 bytes |

Os detalhes de uma FEN inválida (qual campo, por quê) vão para a **stderr**, não para a stdout.

## Exemplo de sessão

```
> uci
< id name Xadrez-ES2 0.1
< id author Equipe Xadrez-ES2
< uciok
> isready
< readyok
> position startpos moves e2e4 e7e5
> state
< state check - result * reason none fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2
> go
< bestmove f1c4
> position startpos moves e2e4 e2e4
< error illegal-move e2e4 1
> quit
```

## Limitações conhecidas

- **Não há `stop`** nem leitura da entrada durante um `go`. Comandos enviados nesse intervalo
  esperam e são processados depois da resposta.
- `go` não usa a IA do projeto; sorteia um lance legal.
- Repetição tripla e material insuficiente não são reportados em `state`.
- **Um quinto caractere inválido num lance que não é promoção é ignorado**: `e2e4x` é
  aceito como `e2e4`. Mande apenas lances de 4 caracteres (5 só em promoção).
- O motor guarda uma única posição, e quem guarda a partida (o histórico de lances) é o
  cliente, que reenvia `position ... moves ...` inteiro a cada mudança.
