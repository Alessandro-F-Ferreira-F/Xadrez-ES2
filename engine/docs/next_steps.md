# Próximos Passos — do filtro de legalidade ao UCI, e depois a otimização

> Guia de implementação e arquitetura. **Documento vivo**: cada etapa tem critério de saída
> objetivo; marque conforme avança e registre aqui o que mudou de opinião no caminho.
>
> Estado base: **4 de outubro de 2026** (commit `1582162`).
> Contexto e estado bug-a-bug: `project_context.md` (os números de bug citados aqui são os
> de lá). Fases de longo prazo: `roadmap-motor.md`.
>
> **Esta é a quarta versão deste documento.** A terceira (16/09) planejava sete etapas, de
> `unmake_move` até o UCI. Quatro estão cumpridas e provadas — inclusive a mais delicada, o
> filtro de legalidade, que bate o perft nas seis posições de referência. As outras três
> estão abertas, e uma delas (o teste-portão) está "quase": o código existe, só não está
> ligado ao build. A §2 é a prestação de contas; as Etapas 8 a 12 são o que falta até o
> motor falar o protocolo; a **Parte II** (novidade desta versão) são as etapas de
> otimização, que só começam **depois** do UCI.

---

## 1. Estado em 2026-10-04

| Área | Estado |
|---|---|
| Geração pseudo-legal | ✅ Completa: peão, cavalo, bispo/torre/dama, rei, roque |
| `make_move` / `unmake_move` | ✅ Completos. `king_square` incremental |
| `is_square_attacked` | ✅ Busca reversa, com o *wraparound* do rei corrigido |
| `generate_legal_moves` | ✅ Aplica-e-testa + as duas regras extras do roque |
| Perft | ✅ No motor (`main.c`), menu opções 10 e 13. **Seis posições exatas**, posição inicial até a profundidade 7 |
| Teste de make/unmake | 🔶 `test/test.c` verde (18 FENs), mas só pelo menu: `make test` não linka (Bug #4) |
| `board_check_invariants` | 🔶 4 das 6 checagens (Bug #7) |
| Build | 🔶 18 avisos, zero erros. As quatro flags do `CLAUDE.md` §7 continuam fora |
| Fim de partida, empates | ⬜ Não há `in_check` nem função de estado da partida |
| UCI | ⬜ Só o menu interativo |
| IA | ⬜ Existe em `IA/` (minimax + alfa-beta + avaliação), mas não está ligada ao binário |

Linha de base de desempenho (perft sem *bulk counting*, `-O2 -DNDEBUG`): **12,2 Mnós/s**;
com `-flto`, **19,0**. Perfil e números em `project_context.md` §2 — eles são o ponto de
partida da Parte II.

---

## 2. Prestação de contas — as sete etapas de 16/09

| Etapa de 16/09 | Critério de saída de então | Resultado |
|---|---|---|
| 1 — `unmake_move` | Linka e desfaz um lance simples | ✅ **Muito além:** desfaz todos os tipos de lance, provado em 11 milhões de round-trips sob ASan/UBSan (`project_context.md` §2) |
| 2 — Teste-portão 🚧 | `./main.out test` verde em ~30 posições, invariantes passando | 🔶 `run_make_unmake_tests()` existe e está verde em 18 posições — mas não é subcomando, não checa invariantes, e `make test` não linka. Continua como **Etapa 8** |
| 3 — Cavalo | Soma = 336; inicial gera 20; Kiwipete gera 48 | ✅ Medido em 01/10 (soma 336), e o perft de hoje o cobre por completo |
| 4 — Build | `make` com zero avisos e três flags novas | ⬜ Foi na direção contrária: 8 → 18 avisos. Continua como **Etapa 11** |
| 5 — Invariantes | As 6 checagens de `guides.md` | ⬜ Continua em 4. Vira **Etapa 12** |
| 6 — Legalidade 🚧 | `perft(1)` = 20, `perft(2)` = 400, Kiwipete `perft(1)` = 48 | ✅ **Muito além:** as seis posições da CPW exatas até a profundidade 5–7 |
| 7 — UCI | Partida inteira contra Cute Chess/Arena sem travar | ⬜ Vira **Etapa 10**, depois do fim de partida (Etapa 9), que o `go` precisa |

### O que mudou de opinião no caminho

- **`perft divide` + Stockfish funcionou exatamente como previsto**, e vale registrar como:
  a posição 5 divergia na profundidade 5 (89 949 152 contra 89 941 194); a bissecção
  (`perft_divide` no formato do Stockfish + um script local de comparação) desceu até duas
  folhas e achou **dois** bugs numa sessão — roque saindo de xeque / através de casa atacada,
  e o *wraparound* do rei em `is_square_attacked`. A lição que fica: **o portão é as seis
  posições, não uma.** A posição 5 sozinha escondia o segundo bug; a posição 3 o mostra na
  profundidade 1.
- **A divisão do roque ficou como decidida**: o gerador verifica direito e casas vazias; o
  filtro verifica xeque e casa atravessada. Ninguém reimplementou metade nos dois lugares.
- **`is_square_attacked` saiu com a cor do defensor, não do atacante** (`(b, sq, side)` com
  `side` = dono da casa). Diverge do plano (`by`), funciona — o perft prova —, mas é uma
  armadilha para o próximo chamador. Ver Etapa 9 e `project_context.md` §3.
- **O cache de `king_square` voltou a ser cache**, como §2.5 de 16/09 pedia. Os outros dois
  itens baratos daquela seção (`fill_sq` com string no `make_move`, `check_rook_squares` que
  mente no nome) continuam — agora têm custo medido e entram na etapa O2.

---

# Parte I — até o motor falar o protocolo

## 3. Etapa 8 — O portão dentro do repositório 🚧

**Esforço:** 1 sessão. **É o item de maior retorno da lista, pela terceira versão seguida.**
A diferença é que agora o código já existe: o perft está em `main.c`, o teste em
`test/test.c`, e os números esperados estão no `project_context.md` §2. Falta ligar.

### 8.1 Por que agora, e não depois do UCI

Os números que provam que o motor está certo existem hoje em dois lugares: um documento e um
harness descartável. Nenhum dos dois roda quando alguém faz um commit. A Etapa 10 (UCI) vai
mexer em `main.c` inteiro, a Etapa 11 em vários arquivos, e a Parte II inteira mexe no código
mais quente — cada uma delas é uma chance de quebrar a geração em silêncio. O portão precisa
existir **antes** delas.

### 8.2 `make test` voltar a linkar (Bug #4)

O alvo filtra `src/io.c` para fora, mas `utils.c` passou a depender dele (`read_move`,
`read_coord` e `wait_enter` chamam `read_word`/`read_line`). Duas saídas:

1. Parar de filtrar `io.c`. Uma linha.
2. Reconhecer a causa: essas três funções **são código do REPL**, não utilitários. Elas e o
   menu vão para um `repl.c` (que a Etapa 10 já pede, ver 10.5), e `utils.c` volta a não
   depender de entrada.

**Recomendação: a 2**, junto com a Etapa 10. Até lá, a 1 destrava o portão hoje.

### 8.3 `perft` sai de `main.c`

```c
/* perft.h */
u64  perft(Board *b, int depth);
u64  perft_divide(Board *b, int depth, FILE *out);   /* formato do Stockfish */
bool perft_suite(int max_depth);                     /* as seis posicoes, valores esperados */
```

Três consumidores vão precisar disto, e `main.c` não tem header: o subcomando
`./main.out perft`, o comando de depuração `perft N` do UCI (Etapa 10), e o `bench` da etapa
O1. Junto: os `printf` de `u64` passam a usar `PRIu64` (Bug #6) — o grupo compila no Windows,
onde `%lu` tem 32 bits e a profundidade 7 (3,2 bilhões) não cabe.

### 8.4 O corpus de referência, com os números

As seis posições da [CPW — Perft Results](https://www.chessprogramming.org/Perft_Results),
num `const` dentro de `perft.c` (não arquivo lido em runtime — a decisão de 16/09 vale):

| Posição | FEN | d1 | d2 | d3 | d4 | d5 |
|---|---|---|---|---|---|---|
| Inicial | `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1` | 20 | 400 | 8 902 | 197 281 | 4 865 609 |
| Kiwipete | `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1` | 48 | 2 039 | 97 862 | 4 085 603 | 193 690 690 |
| Posição 3 | `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1` | 14 | 191 | 2 812 | 43 238 | 674 624 |
| Posição 4 | `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1` | 6 | 264 | 9 467 | 422 333 | 15 833 292 |
| Posição 5 | `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8` | 44 | 1 486 | 62 379 | 2 103 487 | 89 941 194 |
| Posição 6 | `r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10` | 46 | 2 079 | 89 890 | 3 894 594 | 164 075 551 |

**Profundidade 4 nas seis** são ~10,7 milhões de nós: ~1 s, tanto em `-O2` quanto no `make`
padrão. É o tamanho certo para um portão que todo mundo roda. A profundidade 5 (~470 milhões,
~40 s) fica para antes de PR que toque `movegen.c` ou `makemove.c`.

### 8.5 O teste de round-trip ganha os invariantes

`test.c` compara FEN antes/depois, e isso pega quase tudo — mas não pega cache
dessincronizado que o `fen_write` não serializa (`king_square`). Chamar
`board_check_invariants` depois do `make` e depois do `unmake` custa uma linha cada, e foi o
que o harness de 04/10 fez em 11 milhões de nós. Parar na **primeira** falha e chamar
`print_fail_log()` — numa suíte de round-trip, a primeira falha é a informativa; e o log
global não tem reset (Bug #7).

### 8.6 `main()` olha `argv`

```
./main.out              menu (hoje)  →  laço UCI (a partir da Etapa 10)
./main.out test         round-trip + invariantes + perft_suite(4)
./main.out perft N      perft da posição inicial (ou de uma FEN dada)
./main.out divide N     perft_divide, para comparar com o Stockfish
```

E `make test` passa a ser `./build/main.out test` — um alvo, um binário, sem segundo `main`
sob `#ifdef`.

### Critério de saída 🚧

`make test` verde em menos de 5 s, cobrindo round-trip + invariantes nas 18 FENs atuais e
`perft_suite(4)` nas seis posições. E a regra que isso impõe, daqui para frente: **todo PR
que toca `movegen.c`, `makemove.c`, `board.c` ou `square.c` roda `make test` antes**.

---

## 4. Etapa 9 — Fim de partida, e o contrato com a IA

**Esforço:** 1 sessão. Precede o UCI porque o `go` precisa saber se há partida para jogar, e
porque a IA (`IA/header/contrato.h`) já pede três funções que o motor não tem.

### 9.1 O que a IA espera, e o que o motor tem

| `IA/header/contrato.h` | No motor hoje | Falta |
|---|---|---|
| `void gerarLancesLegais(Board *b, MoveList *lista)` | `generate_legal_moves` — **mesma forma** | Só o nome |
| `bool emXeque(const Board *b)` | `is_square_attacked` (com o rei do lado a jogar) | Um `in_check` de uma linha |
| `int testeEmpateRegra(const Board *b)` | `halfmove_clock` existe; o resto não | 50 lances, material insuficiente, repetição |

O próprio `contrato.h` diz que é provisório e que deve ser substituído pelos headers das
regras quando elas existirem. Elas existem. **Recomendação: a IA chama os nomes do motor
(`generate_legal_moves`, `in_check`, ...) e o `contrato.h` é apagado** — duas funções com
nomes diferentes fazendo a mesma coisa é duplicação que diverge. É conversa com o responsável
pela IA, não decisão unilateral; registre o resultado no `project_context.md` §6.

### 9.2 `in_check` e o parâmetro de `is_square_attacked`

```c
bool in_check(const Board *b);   /* o rei do lado a jogar esta atacado? */
```

É uma linha sobre `is_square_attacked(b, b->king_square[side], side)` — e é aqui que a
armadilha do `project_context.md` §3 morde: `side` é o **dono** da casa, não o atacante.
Antes de escrever o `in_check`, renomeie o parâmetro de `is_square_attacked` para `defender`
(ou `us`) e ponha um comentário no header. O `in_check`, o `fen_parse` (9.4) e a IA vão
chamá-la; um deles vai passar o atacante por hábito.

### 9.3 Estado da partida

```c
typedef enum {
    GAME_ONGOING, GAME_CHECKMATE, GAME_STALEMATE,
    GAME_DRAW_50, GAME_DRAW_MATERIAL, GAME_DRAW_REPETITION
} GameStatus;
```

| Estado | Regra | Nota |
|---|---|---|
| Mate | Lista legal vazia **e** `in_check` | Tem precedência sobre o empate dos 50 lances, se os dois acontecem no mesmo lance |
| Afogamento | Lista legal vazia **sem** xeque | — |
| 50 lances | `halfmove_clock >= 100` | O campo já existe e já é mantido por make/unmake |
| Material insuficiente | K×K, K+B×K, K+N×K, K+B×K+B com bispos na mesma cor | O conjunto clássico; não tente cobrir toda "posição morta" da FIDE |
| Repetição tripla | A mesma posição (peças, lado, roque, en passant) três vezes | Precisa de histórico — 9.5 |

`generate_legal_moves` não é `const` (o filtro aplica e desfaz), então a função de estado
também não pode ser. Não é defeito da assinatura; é o mesmo motivo de 16/09.

### 9.4 `fen_parse` rejeita o lado que não joga em xeque (Bug #1)

É o último caminho de crash por entrada externa: a FEN ilegal entra, o filtro aceita a
captura do rei (ela não deixa o rei de **quem joga** em xeque), e a próxima geração aborta
em `movegen.c:309`. Com `is_square_attacked` existindo, a correção é uma checagem a mais no
fim do `fen_parse`.

**O cuidado é de dependência.** `is_square_attacked` mora em `movegen.c`; fazer `fen.c`
incluir `movegen.h` liga o parser ao gerador inteiro. A recomendação é um módulo
`attack.h/c`, que só depende de `board.h` e `square.h`, com `is_square_attacked` e `in_check`
— usado por `movegen.c`, `fen.c`, pela função de estado e pela IA. É a mesma regra do §9.2
abaixo: a função mora no módulo cujas dependências ela realmente tem.

### 9.5 Histórico para repetição

A decisão de 10/09 continua: `Undo` não é histórico, e a pilha de recursão não guarda a
partida. Quem guarda é a camada de protocolo — o `Game` que a Etapa 10 cria ao processar
`position ... moves`. O que ele precisa guardar são **chaves de posição**, não `Undo`s.

Dois detalhes que encurtam o trabalho:

- Só é preciso olhar para trás até o último lance irreversível (captura ou lance de peão) —
  e isso é exatamente o `halfmove_clock`. A janela é pequena.
- A chave pode ser, por enquanto, os quatro primeiros campos da FEN (peças, lado, roque, en
  passant). É lento e trivialmente correto, e uma partida tem no máximo `MAX_GAME_PLY`
  posições. O Zobrist (etapa O5) substitui isso quando a busca precisar de repetição dentro
  da árvore — e aí o `fen_write` vira o oráculo do hash, do mesmo jeito que foi oráculo do
  make/unmake.

### Critério de saída

Um caso de teste para cada estado, no `make test`: o mate do pastor ou o mate do louco
(mate), uma posição conhecida de afogamento, K×K, `halfmove_clock = 100`, e uma repetição
por sequência de lances (`g1f3 g8f6 f3g1 f6g8` duas vezes a partir da inicial). E
`R6k/8/8/8/8/8/8/K7 w - - 0 1` **rejeitada** pelo `fen_parse`.

---

## 5. Etapa 10 — UCI mínimo 🚧

**Esforço:** 1–2 sessões. É o que a equipe da interface espera: o `EngineBridge` de
`interface/` já faz o spawn e escreve no stdin do motor; falta o motor responder.

### 10.1 A linha mais importante do arquivo

```c
setvbuf(stdout, NULL, _IOLBF, 0);
```

Quando a stdout é um terminal, a libc usa buffer de linha. Quando é um **pipe** — exatamente
o caso da interface rodando o motor como subprocesso — ela troca para buffer de bloco de
4 KB. A resposta fica presa, a interface espera para sempre, e **não há sintoma para
depurar**: o motor está vivo, correto, e mudo.

### 10.2 Leitura de linha: `io.c`, depois de dois consertos

`read_line` já trata o que importa (linha longa, `\r` do Windows, última linha sem `\n`) e é
a base certa. Mas antes:

- **Bug #3 (EOF em `read_word`)**: quando a interface fecha o pipe, `read_line` devolve `0`.
  O laço UCI tem de **terminar** nesse caso. Com o bug, quem lê por `read_word` recebe a
  palavra anterior de novo, com sucesso — o motor repetiria o último comando para sempre. Use
  `read_line` direto no laço e trate o `0`.
- **Bug #5 (`join_args` sem espaço)**: se for montar a FEN de `position fen ...` juntando
  palavras, ela sai sem separadores. A alternativa mais simples é não juntar: a FEN é o
  trecho da linha entre `fen ` e ` moves` (ou o fim).
- Buffer: `LINE_CAP` é 4096. `position startpos moves ...` com 200 lances tem ~1 000 bytes;
  cabe, mas meça antes de assumir — e `read_line` devolve `-1` se não couber, que tem de virar
  erro visível, não comando truncado.

### 10.3 Conjunto mínimo de comandos

| Comando | Resposta | Fonte |
|---|---|---|
| `uci` | `id name …`, `id author …`, `uciok` | spec UCI |
| `isready` | `readyok` | spec UCI |
| `ucinewgame` | — (zera o histórico) | spec UCI |
| `position startpos [moves …]` | — | spec UCI |
| `position fen <FEN> [moves …]` | — | spec UCI |
| `go depth N` / `go movetime N` | `bestmove <lance>` | spec UCI; chama a IA |
| `legalmoves` | `moves e2e4 g1f3 …` | **não-UCI**, `arquitetura-xadrez.md` §3 — a interface destaca lances com ele |
| `d` | tabuleiro + FEN | não-UCI, depuração |
| `perft N` | contagem, formato do Stockfish | não-UCI, depuração (Etapa 8) |
| `quit` | encerra | spec UCI |
| desconhecido | **ignora em silêncio** | a spec manda |

`position … moves` é o consumidor natural de `movelist_find` sobre a lista **legal**: os
lances chegam em coordenada (`e2e4`, `e7e8q`) e é o gerador que sabe se `e5d6` é captura
comum ou en passant. Lance inválido no meio da lista → pare de aplicar e reporte (`info
string`), não aplique metade.

### 10.4 `go`: o ponto de contato com a IA

- `escolherJogada(board, variante, randomizador)` (`IA/src/ia.c`) **não recebe profundidade
  nem tempo** — o limite é a constante `PROF_MAX = 9000` em `ia.h`. `go depth N` precisa de
  um parâmetro; `go movetime N` precisa de aprofundamento iterativo com relógio (que é também
  a primeira etapa de O4). Acordar com o responsável pela IA antes de escrever o `go`.
- Até a IA estar ligada, o `go` pode sortear um elemento de `generate_legal_moves` — é o que
  a IA aleatória morta de `main.c:315-393` tentava ser, agora sem o contorno de captura de rei.
- **Antes de imprimir `bestmove`, conferir que o lance está na lista legal**, também no build
  de release. Custa uma varredura e garante que o motor **nunca** devolve lance ilegal —
  requisito funcional RF-M3.
- Sem lance legal: `bestmove (none)` (o que a spec e as GUIs aceitam). Como sinalizar **qual**
  fim de partida (mate, afogamento, empate por regra) é decisão com a equipe da interface — a
  `arquitetura-xadrez.md` §4 fala em "algum sinal de fim de partida no protocolo". Sugestão:
  um comando não-UCI `status` que devolve o `GameStatus` da Etapa 9.

### 10.5 `main.c` encolhe

```
./main.out              laço UCI  (o default, porque é o que a interface executa)
./main.out repl         o menu interativo de hoje
./main.out test         Etapa 8
./main.out perft N      Etapa 8
```

O menu é genuinamente útil para depurar à mão — mas não pode mais ser o default. Mova-o para
`repl.c`, junto com `read_coord`, `wait_enter` e `read_move` (8.2). E a FEN de partida deixa
de ser a posição 5 da CPW (`main.c:289`), sobra da bissecção.

### 10.6 Dívidas conscientes — anotar, não deixar implícitas

- A spec exige processar stdin **enquanto pensa**, para atender `stop`. Um laço bloqueante
  não faz isso. Para `go depth` raso é irrelevante; resolve-se depois com thread de leitura.
  Anote no header do módulo UCI.
- O lado da interface também precisa ler sem bloquear (`readCommand` ainda é `TODO` no
  `EngineBridge`, e o ramo Windows também) — `arquitetura-xadrez.md` §3 já registra.

### Critério de saída 🚧

1. Por pipe, sem terminal: `printf 'uci\nisready\nposition startpos moves e2e4 e7e5\ngo depth 2\nquit\n' | ./build/main.out`
   responde `uciok`, `readyok`, um `bestmove` legal, e **termina**.
2. Fechar o stdin no meio (EOF) encerra o motor, em vez de repetir comando.
3. Uma partida inteira contra a interface (quando o `readCommand` existir) ou contra o Cute
   Chess/Arena, até mate ou empate, sem travar e sem lance ilegal.

---

## 6. Etapa 11 — Fechar o build

**Esforço:** meia sessão. Pode entrar a qualquer momento — mas **antes da Parte II**: a
otimização vai mexer no código mais quente, e um aviso novo no meio de 18 velhos não é
notado.

### 11.1 Os 18 avisos de hoje

| Aviso | Onde | O que fazer |
|---|---|---|
| `-Wmissing-prototypes` × 8 | `genenare_moves_from_direction`, `generate_knight_moves`, `check_allowed_castles` (`movegen.c`); `print_move` (`move.c`); `string_to_cstr_static`, `string_split_next` (`io.c`); `perft`, `perft_divide` (`main.c`) | Cada uma é **ou** helper que devia ser `static`, **ou** API que devia estar no header. `generate_knight_moves` vai para `movegen.h`; `perft*` saem com a Etapa 8 |
| `-Wdiscarded-qualifiers` × 2 | `io.c:112`, `io.c:131` | `const char *p = line;` |
| variável não usada × 2 | `main.c:50` (`i`), `main.c:76` (`result_list`) | Apagar |
| `-Wpedantic` | `main.c:223` | Declaração logo depois de `case 10:` — C11 não permite (Bug #6). Bloco `{ }` no `case` |
| `-Wunused-function` | `main.c:368` (`ai_game`) | A IA aleatória: vira o `go` provisório (10.4) ou sai |
| `-Wenum-conversion` | `move.c:112` | `MV_QUIET` no lugar de `EMPTY` (Bug #8) |
| `-Wunused-but-set-variable` | `movegen.c:130` (`piece_to`) | Apagar (Bug #9) |
| "useless storage class" | `movegen.c:192` | `enum { LEFT, RIGHT };` |
| `-Wunused-const-variable` | `square.c:8` (`DIR_CHARMAP`) | Apagar |

Enquanto estiver em `movegen.c`, o typo `genenare_` vira `generate_`.

### 11.2 As flags que faltam, medidas contra esta árvore

| Flag | Custo hoje | Decisão |
|---|---|---|
| `-Wshadow` | +1 | Ligar. É o `i` morto de `perft` — some com 11.1 |
| `-Wcast-qual` | **0** | Ligar |
| `-Wwrite-strings` | +6 | Ligar. `fail_msg(const char *)` fecha os seis de uma vez |
| `-Wconversion` | +11 | Ligar **junto com a tabela `CASTLE_MASK[64]`** da etapa O2: ela resolve 6 dos 11 (os `&= ~(CASTLE_*)` em `u8`). Dois dos outros são o Bug #2 (`main.c:213/215`) — a flag apontando o bug antes de alguém achá-lo, como em setembro |

Mover também `-Wframe-larger-than=16384` de `DBGFLAGS` para `CFLAGS`, e pôr o `$(WARN)` no
alvo `release`, que hoje compila sem aviso nenhum.

### Critério de saída

`make`, `make debug` e `make release` sem avisos, com as quatro flags somadas.

---

## 7. Etapa 12 — Completar `board_check_invariants`

**Esforço:** uma hora. Cobre 4 das 6 checagens de `docs/guides.md`. Faltam:

- **`ep_square`, se definido, na fileira coerente com `side_to_move`** — fileira 6 (índice 5)
  com as brancas a jogar, fileira 3 (índice 2) com as pretas, e o peão adversário logo atrás.
- **Direitos de roque coerentes com rei e torres nas casas de origem.** Já está escrita:
  `castling_matches_board`, `static` em `fen.c:385`. Expor é a maior parte do trabalho — é a
  mesma verificação que `fen_parse` faz na entrada, e usá-la na saída de `make_move` fecha o
  ciclo que a decisão "FEN inconsistente é rejeitada, não corrigida" abriu de propósito.

E `fail_msg` ganha `const char *` e um `fail_log_reset()` (Bug #7).

---

# Parte II — Otimização, depois do UCI

## 8. Por que depois, e as regras desta fase

A ordem do `CLAUDE.md` §4 é: correção das regras, protocolo, uma IA qualquer, e **só então**
força e desempenho. A primeira está cumprida; as outras duas são a Parte I. Otimizar antes
delas é gastar tempo num motor que ainda não joga uma partida — e é mexer no código mais
delicado (make/unmake, filtro) sem ter um portão automático que avise quando quebrar.

A partir do UCI funcionando, a otimização deixa de ser prematura. As regras:

1. **Medir antes, medir depois, sempre no mesmo build e com o mesmo comando** (`bench`, etapa
   O1). Otimização sem número não entra.
2. **Uma otimização por commit**, com o número antes/depois na mensagem. Commit que otimiza e
   muda comportamento junto não é bissetável.
3. **O portão continua sendo o perft das seis posições.** Toda etapa abaixo termina com
   `make test` verde e com o perft profundo (d5) exato antes de PR. Otimização que muda uma
   contagem de perft não é otimização, é bug.
4. **Medir com `-O2 -DNDEBUG`**, não com o `make` padrão (`-Og`), que mede o depurador.
5. **Registrar** cada medição numa tabela (em `docs/perft_results.txt`, ou num
   `docs/benchmarks.md`). Vira material da Análise de Valor Agregado e da apresentação.

### 8.1 A linha de base (04/10)

Perft sem *bulk counting* (o filtro de legalidade roda até a última folha), nas seis posições
em profundidade 4–6 (inicial d5, Kiwipete d4, posição 3 d6, posições 4–6 d4) — 26,4 milhões de
folhas:

| Build | Mnós/s |
|---|---|
| `-Og -g` (`make`) | 10,5 |
| `-O2 -DNDEBUG` | 12,2 |
| `-O3` (`make release`) — com ou sem `NDEBUG`, igual | 14,4 |
| `-O2 -DNDEBUG -flto` | **19,0** |

Perfil (`gprof`, posição inicial prof. 6 + Kiwipete prof. 5, 312 milhões de folhas):

| Onde | Fatia | Chamadas por folha |
|---|---|---|
| Acessores de lance (`move_to`, `move_from`, `move_is_castle`, `move_is_promotion`, `move_type`, `move_is_ep_capture`) | ~28% | ~30 |
| `is_square_attacked` | ~26% | ~1,09 |
| `make_move` + `unmake_move` | ~23% | ~2,07 pares |
| Geração (peão, deslizantes, cavalo) | ~12% | — |
| `check_rook_squares`, `movelist_add`, `board_find_king` | ~6% | — |

Duas leituras. Primeira: quase um terço do tempo é chamada de função de uma linha que o
compilador não consegue expandir, porque mora em outro arquivo — o `-flto` prova o tamanho
disso (+57%). Segunda: cada folha custa **dois** pares make/unmake e um teste de ataque — um
par no filtro de legalidade e outro no laço de quem chamou. As etapas O2 e O3 atacam
exatamente essas duas coisas, nessa ordem, porque a primeira é mecânica e a segunda mexe em
regra.

Nota sobre o que este número mede: perft conta folhas de geração, não nós de busca. A busca
da IA tem o mesmo custo por nó (gera legais, aplica, desfaz), mais a avaliação — então O1–O3
aceleram a IA também, mas o ganho dela se mede com outro número (O4).

---

## 9. Etapa O1 — Medir direito

**Esforço:** meia sessão. Custo zero no motor; é a fundação das outras cinco.

- **Alvo `release` de verdade**: `$(STD) $(WARN) -O2 -DNDEBUG -flto`. O `-flto` sozinho já
  rendeu +57% medidos; os avisos voltam a valer também nele. `-march=native` **não** por
  padrão: o binário roda nas máquinas do grupo e na apresentação, e um binário compilado para
  o processador de um aluno pode não rodar no de outro.
- **Subcomando `./main.out bench`**: `perft_suite` em profundidade fixa (a da linha de base
  do §8.1: inicial d5, Kiwipete d4, posição 3 d6, posições 4–6 d4 — 26,4 milhões de nós, ~2 s) com
  nós, tempo e Mnós/s por posição e no total. Mais tarde, um segundo bloco para a busca
  (posições fixas, profundidade fixa → nós, tempo, lance escolhido — ver O4).
- **Perfil reproduzível**: `gprof` está instalado (`-pg`, rodar, `gprof -b -p`). O `perf` não
  está no WSL desta máquina. Registre o comando junto com a tabela, para o próximo perfil ser
  comparável com este.

### Critério de saída

`make release` sem avisos e com o perft das seis posições exato; `./main.out bench`
existindo; a linha de base registrada com data e commit.

---

## 10. Etapa O2 — Tirar o custo evitável do caminho quente

**Esforço:** 1 sessão. **Mesmo algoritmo**, menos trabalho por lance. Em ordem de peso
medido:

1. **Acessores de lance como `static inline` em `move.h`** — `encode_move`, `move_from`,
   `move_to`, `move_type` e os predicados. É o item de ~28%. Não quebra o encapsulamento que
   tornou a troca de `u32` para `u16` barata em setembro: o formato continua tocado num lugar
   só — só que esse lugar passa a ser o header. Por que explicitar em vez de depender do
   `-flto`: o `make` e o `make debug` não usam LTO, e a IA (outro diretório, outro build)
   também chama esses acessores.
2. **`CASTLE_MASK[64]`** no lugar das duas chamadas de `check_rook_squares` por lance:

   ```c
   b->castling_rights &= CASTLE_MASK[from] & CASTLE_MASK[to];
   ```

   Uma tabela com `0xF` em tudo, menos nas seis casas que importam. Faz as duas metades
   (origem e destino) caírem na mesma linha — é difícil lembrar de uma e esquecer da outra —,
   tira um `switch` do caminho quente e fecha 6 dos 11 avisos de `-Wconversion` (Etapa 11).
3. **Torre do roque por índice**, não por `fill_sq(b, "d1", ...)`: `CASTLE_POSITIONS` já tem
   as casas como números, e o `unmake_move` já a usa. As duas metades ficam simétricas.
4. **`check_allowed_castles` lê `b->king_square`** em vez de varrer o tabuleiro com
   `board_find_king` (~1,7%). O cache agora é confiável — os invariantes provam —, e isso
   elimina o último lugar com dois mecanismos para achar o rei.
5. **Uma passada de geração em vez de três.** Hoje peão, deslizantes e cavalo varrem as 64
   casas cada um (o rei usa o cache). Um laço só, `if (!is_own(b, sq)) continue;` uma vez, e
   `switch (PIECE_TYPE(p))` despachando — é o "P1" do plano de 10/09. Bônus: devolve o
   `const Board *` que a geração perdeu.
6. **Perft com *bulk counting***: na profundidade 1, devolver `list.count` em vez de aplicar e
   desfazer cada lance. **Só vale porque a lista é legal** (com lista pseudo-legal seria
   errado). Corta o segundo par make/unmake por folha — mas só no perft, não na busca. Vale
   pelo portão: a Etapa 8 e o perft d5 de antes de PR ficam ~2× mais rápidos.

### Critério de saída

Perft das seis posições exato depois de **cada** item; ganho de cada um medido no `bench` e
registrado. Meta para medir (não promessa): o build **sem** LTO alcançar ou passar os
19 Mnós/s que o `-flto` já mostra que existem.

---

## 11. Etapa O3 — Um filtro de legalidade mais barato

**Esforço:** 1–2 sessões. É a primeira otimização que **mexe em regra**, e por isso vem
depois das mecânicas.

### 11.1 O custo de hoje

Todo lance pseudo-legal paga `make_move` + `is_square_attacked` + `unmake_move` para
descobrir se é legal — perto de metade do tempo do perfil. Mas na maioria das posições, a maioria dos
lances **não tem como** deixar o próprio rei em xeque: se o lado a jogar não está em xeque e a
peça que se move não está cravada, o lance é legal por construção.

### 11.2 A ideia (CPW: *Pin*, *Legal Move Generation*)

Uma vez por nó, antes de testar qualquer lance:

1. `in_check` (já vai existir, Etapa 9).
2. **Peças cravadas**: de cada uma das 8 direções a partir do próprio rei, andar até a
   primeira peça; se for própria, continuar até a seguinte; se essa for um deslizante inimigo
   do tipo certo para a direção (torre/dama nas ortogonais, bispo/dama nas diagonais), a
   primeira está cravada. Os mesmos raios de `SQ_TO_EDGE` que `is_square_attacked` já percorre.

Depois, só passam pelo aplica-e-testa:

| Lance | Por quê |
|---|---|
| Lance do rei | A casa de destino pode estar atacada |
| Lance de peça cravada | Pode sair da linha da cravada (andar **ao longo** dela é legal) |
| En passant | Tira **dois** peões da mesma fileira de uma vez — a cravada horizontal que a posição 3 testa |
| Qualquer lance, se em xeque | Simplificação deliberada: gerar só evasões é mais rápido e muito mais código |

Todo o resto entra direto na lista legal.

### 11.3 Trade-off, honestamente

Mais código e mais casos de borda num lugar que hoje é simples e está provado. O que torna o
risco aceitável é que o perft das seis posições cobre exatamente esses casos: Kiwipete tem
cravadas, a posição 4 começa em xeque, a posição 3 tem o en passant horizontal. **Se o
cronograma apertar, esta é a etapa a pular** — O2 e O4 rendem mais por hora de trabalho.

### Critério de saída

Perft exato nas seis posições até d5; chamadas de `is_square_attacked` por folha caindo de
~1,09 (medir com `gprof`); ganho registrado no `bench`.

---

## 12. Etapa O4 — Busca: ordenação e poda (com o responsável pela IA)

**Esforço:** 2–4 sessões, incrementais. **O código é de `IA/`**; o motor fornece a
infraestrutura (lista legal, make/unmake, `in_check`, estado da partida). Decidam juntos a
divisão — este documento descreve o que entra e em que ordem, não quem escreve.

O ganho aqui é de outra natureza. O1–O3 aceleram cada nó por uma constante (2×, 3×). Ordenar
bem os lances muda **quantos nós** o alfa-beta visita: com ordenação perfeita ele visita da
ordem de 2·b^(d/2) nós em vez de b^d — numa posição de meio-jogo (b ≈ 35), na profundidade
6, é a diferença entre ~1,8 bilhão e ~85 mil folhas. Nenhum ganho de Mnós/s chega perto disso.

Em ordem de retorno:

1. **Aprofundamento iterativo com relógio.** Profundidade 1, 2, 3… até o tempo acabar; devolve
   o melhor lance da última iteração completa. É **obrigatório** para o `go movetime` (Etapa
   10.4) e é o que dá o melhor lance da iteração anterior para ordenar a próxima.
2. **Ordenação de lances**: o melhor da iteração anterior primeiro; depois capturas por
   **MVV-LVA** (vítima mais valiosa, atacante menos valioso); promoções; depois *killer moves*
   (dois lances quietos que causaram corte em cada profundidade) e *history heuristic*. O
   placar de ordenação mora **na busca**, num array paralelo `int score[MAX_MOVES]` — não
   dentro de `Move`, que continua um `u16` (decisão "Move sem score" do `project_context.md`
   §6).
3. **Busca de quiescência**: nas folhas, continuar só com capturas até a posição ficar quieta,
   com *stand-pat*. É o maior salto de **força** por linha escrita (roadmap Fase 7, v5) —
   sem ela o motor para no meio de uma troca e avalia uma posição que qualquer humano vê como
   perdida.
4. Só então, se sobrar: **PVS** (*Principal Variation Search*), janelas de aspiração,
   **null-move pruning** (desligar em finais com poucas peças — zugzwang) e **LMR** (*Late Move
   Reductions*). Cada um medido isoladamente.

**O teste de regressão que já existe e ninguém usa:** a IA tem `VARIANTE_MINIMAX` e
`VARIANTE_ALFA_BETA`. Com o sorteio desligado e a mesma profundidade, as duas **têm de devolver
o mesmo placar** — alfa-beta poda sem mudar o resultado do minimax. Rode isso a cada mudança
de ordenação; se divergir, a poda está errada. (Os itens do passo 4 mudam o resultado de
propósito — a partir deles, a métrica passa a ser só força e profundidade.)

### Critério de saída

No bloco de busca do `bench` (posições fixas): **nós visitados até a profundidade N**
(ordenação boa = menos nós) e **profundidade alcançada em tempo fixo**, antes e depois de
cada item; alfa-beta == minimax no placar até o passo 3.

---

## 13. Etapa O5 — Zobrist e tabela de transposição

**Esforço:** 2 sessões. Só depois de O4 — a tabela de transposição rende pouco sem
aprofundamento iterativo e ordenação.

### 13.1 Zobrist primeiro, e primeiro como depurador

Uma chave aleatória de 64 bits por (peça, casa), mais lado a jogar, direitos de roque (16) e
coluna do en passant (8). O hash da posição é o XOR das chaves presentes, atualizado
incrementalmente em `make_move`/`unmake_move`.

O uso que vale **antes** de qualquer tabela, já registrado no `roadmap-motor.md` §3.5:

```c
assert(b->hash == hash_from_scratch(b));   /* no fim de make e de unmake, em debug */
```

É um resumo de 64 bits de *tudo* que `make_move` deveria ter alterado. Rodado sob o perft
das seis posições, dispara no lance exato em que alguém esquecer de atualizar uma peça, um
direito de roque ou a casa de en passant.

Com o hash: a repetição da Etapa 9.5 troca as chaves de FEN pelo hash (e a busca ganha
detecção de repetição dentro da árvore, juntando a pilha da busca com o histórico do `Game`).

### 13.2 A tabela

Array de tamanho potência de 2, indexado pelos bits baixos do hash; entrada com chave,
profundidade, placar, tipo de limite (exato / inferior / superior) e melhor lance. Consulta
para corte e, principalmente, para **ordenação**: o lance da tabela vai primeiro.

As três armadilhas clássicas:

- **Placar de mate depende da distância à raiz**: ajuste pelo *ply* ao gravar e ao ler, senão
  o motor "acha" mates mais curtos ou mais longos do que são.
- **Colisão de chave**: o lance que vem da tabela pode não existir na posição atual. Confira
  que ele está na lista legal antes de usar. (A conferência do `bestmove` contra a lista legal
  da Etapa 10.4 continua sendo a última rede.)
- **Repetição e tabela não se entendem perfeitamente** (*graph history interaction*). Para um
  motor deste porte, aceite o erro e registre como dívida consciente.

### Critério de saída

O `assert` do hash verde no perft das seis posições até d5; nós até profundidade fixa caindo
no `bench` de busca; nenhum lance ilegal saindo da tabela (o `assert` do `bestmove` nunca
dispara).

---

## 14. Etapa O6 — Representação: provavelmente não

**Recomendação explícita: não migrar para bitboards dentro do prazo deste projeto.**

| Opção | O que custa | O que ganha |
|---|---|---|
| Piece lists | Manter lista + mailbox sincronizadas em make/unmake | Já medido em setembro: para varrer, **perdeu** (12,3 ns contra 9,5 ns). Para gerar por tipo de peça pode ser diferente — medir antes de discutir |
| 0x88 / 10×12 | Trocar o mecanismo de borda | Nada que importe sobre o `SQ_TO_EDGE`, que já não testa borda em runtime |
| Bitboards (+ *magic bitboards*) | Reescrever geração, ataque e make/unmake — **o núcleo inteiro que está provado** —, e revalidar o perft do zero | O caminho canônico dos motores fortes |

Só reabrir com O1–O5 feitos **e** com um perfil mostrando que a geração (não a busca, não a
avaliação) é o gargalo da IA. Se reabrir, o perft das seis posições é exatamente o que torna
a migração segura — a mesma bateria que provou o mailbox prova o substituto.

---

## 15. Boas práticas para este projeto

### 15.1 Onde vive uma variável global

| Categoria | Onde | Exemplo |
|---|---|---|
| Tabela read-only depois do init | `static` no `.c` dono, ou `extern` no header dele; escrita **só** pela função de init | `SQ_TO_EDGE`, `KNIGHT_ATTACKS`, futuramente `CASTLE_MASK` e as chaves Zobrist |
| Estado mutável | **Nunca global.** Mora numa struct passada por ponteiro | `Board`, `Game` |
| Constante | `static const` no `.c`, ou macro no header | `DIR_OFFSET`, `PAWN_PUSH` |

E se não é `static`, tem de estar num header: `PIECE_CHAR` e `KNIGHT_VECTORS` estão fora dos
dois.

### 15.2 Onde vive uma função

No módulo **cujas dependências ela realmente tem**. `is_square_attacked` depende só do
tabuleiro e da geometria, e por isso não devia morar no gerador (Etapa 9.4). `perft` é usada
por três camadas, e por isso não pode morar em `main.c` (Etapa 8.3).

### 15.3 Toda função não exportada é `static`

E a flag força a escolha, em vez de confiar na memória. Hoje são oito com linkage externo por
omissão (Etapa 11.1).

### 15.4 Assert como executável, não como comentário

Todo campo denormalizado merece um assert que o recalcula do zero e compara.
`board_check_invariants` é isso para `king_square`; o hash Zobrist (O5) vai ser isso para a
posição inteira.

### 15.5 A regra de ouro daqui

**Se o bug é detectável em compilação, ligue a flag em vez de lembrar dele.** Continua sendo
confirmada: dos 11 avisos que `-Wconversion` daria hoje, dois são o Bug #2.

Corolário: **nenhum sanitizer pega leitura de memória não inicializada** — o Bug #3 é
exatamente isso, e passou por ASan e UBSan. O aviso do compilador (e um teste que exercite
EOF) é a defesa.

### 15.6 `LOG_ERROR` não é tratamento de erro

Logar e seguir é pior do que não logar: dá a impressão de que o caso foi tratado. Logar **e**
devolver status. `movelist_add` é o caso atual (Bug #13).

### 15.7 Otimização é commit com número

Uma por commit, com antes/depois do `bench` na mensagem e o perft exato. Uma otimização que
não aparece no `bench` sai — complexidade sem ganho medido é custo puro num código que outras
pessoas vão ler.

### 15.8 Commits

Pequenos, um por etapa, em português (convenção do grupo), em PR. **Refatoração que move
código nunca muda comportamento no mesmo commit** — um commit que faz as duas coisas não é
revisável nem bissetável, e perft existe para bissetar.

---

## 16. Ordem, portões e progresso

```
Parte I                                              Parte II (depois do UCI)
8 🚧 ─── 9 ─── 10 🚧 UCI ─────────────────────────── O1 ─── O2 ─── O3 ─── O4 ─── O5 ─── (O6)
portão  fim de                                      medir  caminho filtro busca  Zobrist  repr.
        partida                                            quente  barato       + TT
  │
  11 (build) ─── antes da Parte II, em qualquer ponto
  12 (invariantes) ─── em qualquer ponto
```

| Etapa | Critério de saída | Feito |
|---|---|---|
| 1 — `unmake_move` (16/09) | Desfaz um lance simples | ☑ |
| 3 — Cavalo (16/09) | Soma 336; inicial 20; Kiwipete 48 | ☑ |
| 6 — Legalidade (16/09) 🚧 | perft(1)/perft(2) da inicial; Kiwipete perft(1) | ☑ (seis posições, d5–d7) |
| **8 — Portão no repositório** 🚧 | `make test` verde em < 5 s: round-trip + invariantes + `perft_suite(4)` | ☐ (o teste existe; o alvo não linka) |
| **9 — Fim de partida e contrato com a IA** | Um teste por estado; FEN com o lado que não joga em xeque rejeitada | ☐ |
| **10 — UCI** 🚧 | Pipe responde e termina; EOF encerra; partida inteira sem lance ilegal | ☐ |
| **11 — Build** | Zero avisos em `make`/`debug`/`release`, com as quatro flags | ☐ |
| **12 — Invariantes** | As 6 checagens de `guides.md` | ☐ |
| O1 — Medir | `release` com avisos e `-flto`; `bench`; linha de base registrada | ☐ |
| O2 — Caminho quente | Perft exato item a item; ganho medido de cada um | ☐ |
| O3 — Filtro barato | Perft exato; menos `is_square_attacked` por folha | ☐ |
| O4 — Busca | Menos nós até prof. N; mais profundidade em tempo fixo; alfa-beta == minimax | ☐ |
| O5 — Zobrist + TT | `assert` do hash verde no perft; nenhum lance ilegal da tabela | ☐ |
| O6 — Representação | Só com O1–O5 feitos e perfil apontando a geração | — |

**A Etapa 8 é portão permanente:** depois dela, toda etapa — da Parte I e da Parte II — termina
com `make test` verde. É o que vai dizer qual commit quebrou a geração quando mais gente
estiver mexendo, e é o que torna a Parte II segura.

---

## 17. Referências para estas etapas

- **Chess Programming Wiki** — Parte I: *Perft Results*, *Checkmate*, *Stalemate*,
  *Fifty-move Rule*, *Repetitions*, *Material* (insuficiente), *UCI*. Parte II: *Pin*, *Legal
  Move Generation*, *Move Ordering*, *MVV-LVA*, *Killer Heuristic*, *History Heuristic*,
  *Iterative Deepening*, *Quiescence Search*, *Principal Variation Search*, *Aspiration
  Windows*, *Null Move Pruning*, *Late Move Reductions*, *Zobrist Hashing*, *Transposition
  Table*, *Bitboards* (para a O6, se chegar lá)
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas, ler inteira antes da Etapa 10
- **Bruce Moreland's Programming Topics** — a melhor explicação curta de alfa-beta,
  ordenação, quiescência e tabela de transposição; boa leitura para quem for fazer a O4/O5
- **TSCP** — busca com quiescência e ordenação simples em C legível, mailbox como o nosso
- `man gcc` — *Options to Request or Suppress Warnings* (Etapa 11) e *Optimize Options*
  (`-flto`, O1); `gprof` (O1)
