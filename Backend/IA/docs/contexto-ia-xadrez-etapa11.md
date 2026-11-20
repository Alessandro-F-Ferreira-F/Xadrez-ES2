# Contexto para continuidade — IA de xadrez (etapa 11: ordenação de lances)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa10.md`.

Como nas etapas anteriores, **o repositório é a fonte da verdade** (`Xadrez-ES2`, branch `IA`).
Este arquivo guarda o que o repositório não guarda: decisões, motivos, medições, pendências e
o estado do trabalho. A §5 lista onde está cada coisa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Estado em 14/11/2026 (fim da etapa 11):** recomendações pendentes da
etapa 10 resolvidas (`d79d752`, `dd8dc2d`, `b54beb8`); **item 10 feito**: MVV-LVA no alfa-beta
(`86985a8`) e melhor lance da iteração anterior primeiro na raiz (`97c18dc`). `origin/IA` e
`origin/main` no mesmo commit (`97c18dc`). Suíte passando, nenhum aviso vindo de `IA/`.
**Próximo passo, já decidido:** aproveitar a iteração parcial (§3.13, decisões já tomadas) e, em
seguida, **no mesmo dia**, redigir o pedido ao dev das regras (§8) — §9.

---

## 0. Dependências da máquina de regras

### 0.1 Já disponível e usado pela IA

| Item | Onde | Observação |
|---|---|---|
| `Board`, `Color`, `Piece`, `Move`, `MoveList`, `MOVE_NONE` | `board.h`, `piece.h`, `move.h` | `MoveList` tem `MAX_MOVES` = 256 lances |
| `generate_legal_moves(Board*, MoveList*)` | `movegen.h` | **Legal** |
| `make_move` / `unmake_move`, `Undo` | `makemove.h` | Validados pelo perft |
| `move_is_capture`, `move_is_ep_capture`, `move_from`, `move_to` | `move.h` | Usados pela ordenação (§3.12) |
| `move_to_str`, `movelist_find` | `move.h` | Usados no teste da ordenação |
| `is_square_attacked(b, sq, side)` | `movegen.h` | `side` = **dono/defensor** da casa, não o atacante |
| `perft(Board*, int)` | `perft.h` | Oráculo da contagem de nós (§7.2) |
| `fen_parse`, `fen_write` | `fen.h` | `fen_write` usado no teste de tempo (tabuleiro intacto). Ver Bug #1 |

**Perft passando:** `make test` das regras, 6 posições da CPW + `make_unmake`, 7/7 até
profundidade 5 (conferido em 04/11).

### 0.2 Ainda faltando (do lado das regras)

1. **`in_check` público.** Planejado em `engine/docs/next_steps.md` §9.2, num módulo
   `attack.h/c`. Hoje há três cópias da lógica: `emXeque` (IA), `uci_in_check` (`uci.c`,
   `static`) e inline na geração de roque (`movegen.c`).
2. **Empate por 50 lances como função pública.** O motor já detecta (`halfmove_clock >= 100`),
   mas dentro de função `static` do `uci.c`.
3. **Repetição e material insuficiente:** não implementados. O plano das regras
   (`project_context.md`, `next_steps.md` §9.5) é o histórico ficar na **camada de protocolo**
   (chaves de posição, depois Zobrist), não no `Board`.
4. **Bug #1 do motor:** `fen_parse` aceita FEN em que o lado que **não** joga está em xeque;
   daí o gerador deixa capturar o rei, `king_square` vira `SQ_NONE` e a próxima chamada aborta
   num `assert`. O UCI recusa essas FENs na fronteira; **a IA, nos testes, chama `fen_parse`
   direto e não tem essa defesa** — não montar FEN assim. Correção prevista: `next_steps.md` §9.4.

Até 2 e 3 existirem, `testeEmpateRegra` continua vindo do stub (`stubs.c`), declarado em
`IA/header/pendenciasRegras.h`.

---

## Como quero que você conduza

**Desde a etapa 8:** **o Claude escreve o código; eu reviso e decido.**

- Respostas **sucintas e tecnicamente precisas**. Sem preâmbulo, sem repetir o que eu disse.
- Eu **decido o escopo e a ordem**. Sugira, mas não assuma. Decisões de projeto com mais de
  uma saída razoável: apresente as opções com uma recomendação e espere a minha escolha
  (foi assim nos itens 9 e 10, §3.10–§3.13).
- **Antes de recomendar, cruze a recomendação com as decisões já tomadas.** Etapa 11: o Claude
  recomendou "melhor lance anterior primeiro na raiz" esperando economia de nós; a janela
  completa na raiz (§3.1) anula esse ganho, e só a medição mostrou (§3.12).
- Ao entregar código: compile **a partir do disco**, rode a suíte e mostre o que mudou para eu
  revisar. Quando o código já estiver commitado, mostre o diff por bloco lógico.
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo. Se minha conclusão estiver certa e o motivo não, diga.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Avalie **só o escopo que eu pedir**. Decisão de estrutura tomada sem me consultar (etapa 11:
  módulo `ordenar` separado) tem de ser **sinalizada** na entrega.
- Mantenho o **pseudocódigo como especificação viva**: toda mudança no C que altera a
  especificação atualiza o `.txt` correspondente, com os **mesmos nomes** do C.
- Separe **erro** de **recomendação**.
- **Eu faço os commits.** Não use linhas de coautoria. Sugira a mensagem; eu commito.
- Verifique o que entrou em cada commit meu (`git show --stat`) — já saiu commit com mais do
  que o combinado. Ao anunciar quantos arquivos um commit terá, **contar com `git status`**
  (etapa 11: anunciados 6, eram 8).

**Padrões de erro meus para vigiar:**
- Ramos espelhados corrigidos pela metade, ou errados do mesmo jeito nos dois lados.
- Consumidores não atualizados depois que mudo assinatura, nome ou decisão.
- Fundir casos distintos num mesmo retorno.
- Estado local declarado como global, e o inverso.
- Informação duplicada em dois lugares.
- Precedência e parênteses em macros e expressões fundidas.
- Erros de digitação em nomes e mensagens.
- Ponteiro sem objeto por trás; **índice que pode ser −1** (etapa 11: `moves[movelist_find(...)]`,
  pego antes de compilar); `extern` (declaração) confundido com definição; vírgulas entre
  literais em `printf` com `PRI...`; **condição de teste que nunca pode ser verdadeira**.
- **Editor salvando buffer antigo por cima do disco** (etapa 8). Antes de commitar, compilar a
  partir do disco.
- **Commit com mais do que o combinado**, **antes do combinado**, ou **mensagem repetida**.
- **Mensagem de commit fora da convenção** (etapa 10: `0615267` saiu com ponto final e juntou
  um `fix` e um `feat`). Na etapa 11, os cinco commits saíram na convenção.

---

## 1. O que já domino (do jogo da velha)

- Minimax, alfa-beta, fazer/desfazer em vez de cópia, desconto por profundidade, desempate
  aleatório com lista de candidatos na raiz.
- Sob alfa-beta, valores de filhos podem ser limites; a lista de candidatos exige **janela
  completa nos filhos da raiz**.
- `melhor` inicializado fora da faixa; **contagem de nós** é métrica melhor que segundos.

---

## 2. O projeto (o que afeta a IA)

**Arquitetura:** motor C (regras + IA) fala protocolo estilo UCI por stdin/stdout; cliente
desktop C++/SFML o executa como subprocesso. Posição trafega como FEN. O `go` do motor
**ainda sorteia** um lance legal: a IA não está ligada ao `uci.c` nem ao build do motor.

**Requisitos que afetam a IA:** RF03 (níveis de dificuldade); RNF01 (resposta < 200 ms nos
níveis básicos); protocolo com `go movetime` / `go depth` e `score cp` / `score mate`;
invariante `bestmove` ∈ lista legal; testes do grupo "v2 devolve o mesmo lance que v1" e
"mesma FEN + profundidade → mesmo lance" — implementados (§7.2). **Com a ordenação, "v2 = v1"
passou a valer para score e conjunto de candidatos, não para o lance** (§3.12); a versão do
grupo precisa ser lida assim.

**Roadmap (Fase 7):** v1 material + minimax → v2 alfa-beta → v3 aprofundamento iterativo +
movetime → v4 MVV-LVA → v5 quiescência → v6+ PST, tapered, TT. TT fora do MVP.

**Gantt (IA):** alfa-beta, ordenação e aprofundamento 03/11–20/11. Apresentação **23/11**.
**Situação em 14/11:** v1, v2, v3 e **v4** executadas e testadas.

**Commits:** `docs(IA): ...` para pseudocódigo/documentação, `feat(IA): ...` para código
(inclusive mudanças só de teste), `fix(IA): ...` para correção, `refactor(IA): ...` para mudança
sem efeito no comportamento; título sozinho na primeira linha **sem ponto final**; uma mudança
de comportamento por commit.

| Commit | Conteúdo | Estado |
|---|---|---|
| `447b019` | `PROF_MAX` 4 + pulo na conversão; **também** v1 = v2, contagem de nós, pseudocódigo e `IA/docs/` | `origin/main`; **não compila sozinho** |
| `9e347ad` | `extern NOS_VISITADOS` no `ia.h` | `origin/main` |
| `e8063fa` | Guarda de `halfmove` + sentinela de `NOS_VISITADOS` no teste | `origin/main` |
| `179c7f3` | `LANCE_NULO` → `MOVE_NONE` + contexto da etapa 8 | `origin/main` |
| `c95a223` | Aprofundamento iterativo + limite de tempo | `origin/main` |
| `3f2885b` | Contexto da etapa 9 | `origin/main` |
| `30d8530` | Parada ao achar mate | `origin/main` |
| `0615267` | Relógio em `int64_t` + `prazoPassou` + `CANDIDATOS` + teste do sorteio | `origin/main`; mensagem com ponto final, deixado assim |
| `279c468` | Contexto da etapa 10 | `origin/main` |
| `d79d752` | v1 = v2 compara `CANDIDATOS` inteira (`teste.c`) | `origin/main` |
| `dd8dc2d` | Sorteio com poucos candidatos (`teste.c`) | `origin/main` |
| `b54beb8` | Nomes do relógio no pseudocódigo (`escolherJogada.txt`) | `origin/main` |
| `86985a8` | MVV-LVA no alfa-beta: `ordenar.h/.c`, `ordenarLances.txt` (novos), `ia.c`, `ia.h`, `alfa-beta.txt`, `teste.c` (7 arquivos) | `origin/main` |
| `97c18dc` | Melhor lance anterior primeiro na raiz: `ordenar.h/.c`, `ia.c`, `ia.h`, `teste.c`, `ordenarLances.txt`, `alfa-beta.txt`, `escolherJogada.txt` (8 arquivos) | **topo de `origin/IA` e `origin/main`** |

`IA/docs/` entra no repositório **de propósito**: explicita o desenvolvimento orientado a
especificação em `.md`.

---

## 3. Decisões tomadas

### 3.1 Busca (inalteradas desde a etapa 1, salvo o limite e a ordenação)

- Convenção absoluta: brancas positivas, pretas negativas; max/min literais.
- Vez lida de `side_to_move`; sem parâmetro `jogador`.
- Ordem do caso base: **tempo esgotado** → empate por regra → limite da iteração → gerar
  lances → lista vazia (xeque = mate, senão afogamento) → **ordenar (só alfa-beta)** → laço.
- `MoveList`, `Undo` e (no alfa-beta) `LanceOrdenado listaOrdenada[MAX_MOVES]` **locais ao nó**.
- `MATE = 19999`, mate vale `±(MATE − profundidade)` com sinal pelo **lado derrotado**;
  `INFINITO = 20000`; empate = 0; `SCORE_INVALIDO = 30000`.
- Score em `int32_t`.
- Alfa-beta: na raiz alfa e beta **não são atualizados** (`profundidade > 0`). Cada iteração
  chama a raiz com a janela completa (`-INFINITO`, `+INFINITO`); sem *aspiration windows*.
  **Consequência (etapa 11):** todo filho da raiz é buscado com a janela completa, então a
  **ordem dos lances na raiz não muda a contagem de nós** — só a ordem de `CANDIDATOS`.
- O mate mais profundo que uma iteração produz é `MATE − (limiteIteracao − 1)`: mate em p
  meios-lances só aparece a partir da iteração p + 1.

### 3.2 Avaliação v1 (fechada e testada)

Só material, Michniewski (P 100, C 320, B 330, T 500, D 900, R 0). `AVALIACAO_MAX` = 10615
(margem para PST). Teste do rei adiado para a v6+.

### 3.3 Constantes públicas e `PROF_MAX`

`PROF_MAX`, `TEMPO_SEM_LIMITE`, `MATE`, `INFINITO`, `SCORE_INVALIDO`,
`LIMIAR_MATE = (MATE − PROF_MAX)` no `ia.h`; `NOTA_PRIMEIRO` no `ordenar.h`.

- **`PROF_MAX` = 64**: **teto** do parâmetro `profundidade` (`go depth`) e define `LIMIAR_MATE`
  (= 19935); **não** é a profundidade da busca. Quem limita a busca é `limiteIteracao`.
- Pilha com 64 níveis: ~64 × (`MoveList` ≈ 516 B + `Undo` + `listaOrdenada` 1 KB) ≈ 100 KB.
  Sem problema (pilha padrão do Windows: 1 MB).
- `_Static_assert(LIMIAR_MATE > AVALIACAO_MAX)` no `ia.c`;
  `_Static_assert(NOTA_PRIMEIRO > 8 * QUEEN - PAWN)` no `ordenar.c`.

### 3.4 Chamadora e resultado da chamada

`Play.resultado`: 0 sucesso, negativos bug, positivos fim de partida na raiz;
`verificarChamada` interna; valores padrão (`MOVE_NONE`, `SCORE_INVALIDO`, `TIPO_INVALIDO`, e
`CANDIDATOS` vazia) quando `resultado != SUCESSO`.

| Valor | Nome |
|---|---|
| −5 | `RESULTADO_TEMPO_INVALIDO` (`tempoMs < 0`) |
| −4 | `RESULTADO_PROFUNDIDADE_INVALIDA` (fora de `[1, PROF_MAX]`) |
| −3 | `RESULTADO_RANDOMIZADOR_INVALIDO` |
| −2 | `RESULTADO_VARIANTE_INVALIDA` |
| −1 | `RESULTADO_ERRO_INTERNO` |
| 0 | `RESULTADO_SUCESSO` |
| 1, 2 | `RESULTADO_RAIZ_MATE`, `RESULTADO_RAIZ_AFOGAMENTO` |
| 3, 4, 5 | `RESULTADO_EMPATE_50_LANCES`, `_REPETICAO`, `_MATERIAL` (provisórios) |
| 6 | `RESULTADO_EMPATE_PROVISORIO` (stub devolve `EMPATE_PROVISORIO` = 4) |

**Ordem da verificação:** variante → randomizador → profundidade → tempo → raiz (empate por
regra, depois lista vazia). Só o primeiro problema é reportado. Falha do `clock()` **não** é
resultado de erro (§3.11).

### 3.5 Conversão de score

Inalterada. Pública, preenche só `score` e `tipo`. Classifica como mate todo `|score| ≥ 19935`.

### 3.6 Sorteio

`iniciarSorteio` uma vez; `rand() % quantidade`. Sorteia sobre `CANDIDATOS`.

### 3.7 Organização do código

- `static`: `verificarChamada`, `emXeque`, `prazoPassou`, `estourouTempo`, `minimax`,
  `podaAlfaBeta`, `sorteioLance`, `listaCandidatos`, `limiteIteracao`, `relogioAtivo`, `prazo`,
  `tempoEsgotado` (`ia.c`); `notaMVVLVA` (`ordenar.c`). Públicas: `converterScore`,
  `escolherJogada`, `iniciarSorteio`, `JOGADA`, `CANDIDATOS`, `NOS_VISITADOS` (`ia.h`);
  `LanceOrdenado`, `NOTA_PRIMEIRO`, `ordenarLances` (`ordenar.h`).
- Globais públicas: **declaradas** `extern` no `ia.h`, **definidas** uma vez no `ia.c`.
- `pendenciasRegras.h` é incluído pelo `ia.c` e pelos stubs, não pelo `ia.h`.
- **Módulo `ordenar` separado** (etapa 11), no molde do `avaliar`: o teste chama
  `ordenarLances` direto, sem o `ia.c` expor nada. Decisão do Claude, sinalizada e aceita.

### 3.8 Descartado

Ponder e threads. Função de leitura dos candidatos (API desnecessária). Ordenação no minimax
(§3.12). Variável `lanceAnterior` (duplicaria `CANDIDATOS.moves[0]`, §3.12).

### 3.9 Da etapa 8

- **Contagem de nós:** `uint64_t NOS_VISITADOS`, público. Cada entrada em `minimax` ou
  `podaAlfaBeta` conta um nó, inclusive a raiz. Zerado no início de `escolherJogada`; fica em
  0 se a verificação falha. Soma todas as iterações, também a interrompida.
- **`MOVE_NONE` no lugar de `LANCE_NULO`:** mesmo sentinela (`(Move)0`, a1→a1).
- **`emXeque` fica como está** até o motor publicar `in_check`; então é apagada e substituída
  (commit próprio, com pseudocódigo). **Não copiar** o `if (SQ_OFFBOARD(ksq)) return false`
  do `uci_in_check`.

### 3.10 Da etapa 9 (aprofundamento iterativo, `c95a223`)

- **Interface:** `escolherJogada(board, variante, randomizador, profundidade, tempoMs)`;
  `tempoMs = TEMPO_SEM_LIMITE (0)` desliga o relógio.
- **Iteração interrompida pelo tempo é descartada:** lance e score vêm da última iteração
  **completa**. **A ser revisto no próximo passo (§3.13).**
- **Iteração 1 sempre termina:** roda com `relogioAtivo` falso.
- **Relógio:** `clock()` do C11. No Windows mede tempo de parede; no POSIX, tempo de CPU.
  O prazo conta do início de `escolherJogada`.
- **Consulta ao relógio:** `estourouTempo()` na entrada do nó, só com `relogioAtivo` e a cada
  `INTERVALO_RELOGIO` = 1024 nós; também antes de cada iteração ≥ 2.
- **Saída por tempo:** o nó devolve 0 na entrada; no laço, `if(tempoEsgotado) return 0;` logo
  **depois** do `unmake_move` e **antes** de comparar com `melhor`, nos **quatro** laços.
- **Estado da iteração como `static` do arquivo** (`limiteIteracao`, `relogioAtivo`, `prazo`,
  `tempoEsgotado`).

### 3.11 Da etapa 10 (`30d8530`, `0615267`)

- **Parada ao achar mate:** depois de gravar uma iteração completa,
  `if(abs(scoreBruto) >= LIMIAR_MATE) break;` — para qualquer lado. Teste: `CasoV1V2 {fen,
  iteracoes}`; só o mate em 1 exercita a parada.
- **`clock()` falhando (`(clock_t)-1`) = prazo passado** (relógio indisponível; postura segura
  é parar). `static bool prazoPassou(void)`; falha no início → `prazo = INT64_MIN`; `prazo` em
  `int64_t`. Sem teste. O pseudocódigo usa agora os nomes do C (`b54beb8`).
- **`extern MoveList CANDIDATOS`** pública: candidatos da última iteração completa. Esvaziada
  no início de `escolherJogada` e no bloco defensivo. `listaCandidatos` (`static`) é a lista de
  trabalho da iteração corrente.

### 3.12 Novas na etapa 11 (`86985a8`, `97c18dc`)

Escolhidas por mim entre opções apresentadas:

- **Ordenação só no `podaAlfaBeta`.** O minimax segue na ordem do gerador como referência (v1):
  ordenar não economiza nada nele, e a contagem exata pelo perft continua valendo.
- **Nota MVV-LVA:** `8 × tipo(vítima) − tipo(atacante)` com `PieceType` (P=1 … R=6), de 2 a 39.
  En passant: vítima = peão. Promoção com captura: atacante = peão. Sem captura (inclusive
  promoção simples): 0. Bônus de promoção **não** entra agora.
- **Inserção estável** antes do laço: capturas por nota, empates e lances de nota 0 na ordem do
  gerador. **Seleção preguiçosa (pick-next) fica como otimização futura.**
- **`LanceOrdenado {Move lance; int16_t nota}`**: `int16_t` para 4 bytes por elemento (1 KB por
  nível).
- **Melhor lance da iteração anterior primeiro na raiz:** `ordenarLances(board, lista, primeiro,
  ordenados)`; o `primeiro` recebe `NOTA_PRIMEIRO` = 64. No `podaAlfaBeta`, `primeiro =
  CANDIDATOS.moves[0]` se `profundidade == 0` e `CANDIDATOS.count > 0`; senão `MOVE_NONE` (a1a1,
  nunca legal). Sem variável nova: `CANDIDATOS` já guarda a última iteração completa e está
  vazia na iteração 1.
  - **Não economiza nenhum nó** (medido: total idêntico) por causa da janela completa na raiz
    (§3.1). A recomendação original (do Claude) supunha economia — erro de não cruzar com §3.1.
  - **Mantido por decisão minha** porque torna seguro aproveitar a iteração parcial (§3.13).
- **`CANDIDATOS` na ordem da busca:** minimax, a do gerador; alfa-beta, o melhor anterior e
  depois MVV-LVA. Com `SORTEIO_DESLIGADO`, **minimax e alfa-beta podem devolver lances
  diferentes** entre candidatos empatados (ex.: após 1.e4, minimax a7a6, alfa-beta a7a5, ambos
  cp −100).

### 3.13 Decididas na etapa 11, **ainda não implementadas** (próximo passo)

**Aproveitar a iteração parcial** (opção B; aceitas as quatro recomendações):

1. **Quando:** se **ao menos o primeiro filho da raiz** (o melhor anterior) terminou na
   iteração D, a parcial substitui a D − 1: candidatos e score vêm dos filhos que completaram,
   todos com valor exato na profundidade D. **Ressalva aceita:** empates entre os filhos ainda
   não buscados ficam de fora — a `CANDIDATOS` da parcial pode ter menos candidatos que a
   completa teria; o sorteio fica restrito, mas correto.
   Descartada: aproveitar só quando um lance novo superou o melhor anterior.
2. **Variantes:** só o alfa-beta (o único com o melhor anterior primeiro). O minimax segue
   descartando a parcial.
3. **Como a raiz entrega o parcial:** hoje a raiz devolve 0 com o tempo esgotado e o `melhor`
   se perde. Uma `static int32_t scoreParcial` gravada só em `profundidade == 0`, junto com a
   `listaCandidatos` (mesmo estilo do estado da iteração).
4. **Teste:** sem teste exato (depende do relógio). No caso de tempo do alfa-beta 100 ms:
   candidatos legais, lance ∈ candidatos, tabuleiro intacto, mais uma medição (não asserção)
   indicando se a parcial foi usada. O caso de raiz completa já está coberto pelo v1 = v2.

**Pontos a resolver na implementação** (apresentar antes de codificar, se tiverem mais de uma
saída): como saber que o primeiro filho terminou (ex.: `listaCandidatos.count > 0` ao sair por
tempo, já que a raiz só adiciona candidato depois de um filho completo); interação com a parada
por mate (score parcial ≥ `LIMIAR_MATE`); o que muda no pseudocódigo (`escolherJogada.txt`,
`alfa-beta.txt`) e na doc de `escolherJogada`/`CANDIDATOS` no `ia.h`.

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo das funções~~ ✅
2. ~~Pseudocódigo: `verificarChamada` e `converterScore`~~ ✅
3. ~~C: `avaliar` + testes~~ ✅
4. ~~C: `converterScore` + testes~~ ✅
5. ~~C: verificação, busca, chamadora, sorteio~~ ✅
6. ~~Stubs + testes da `verificarChamada`~~ ✅ (`0690d96`)
7. Acertar com as regras as decisões da §8 — **em andamento** (pedido ao dev ainda não
   redigido; **próximo depois do 10b**)
8. ~~Primeira execução da busca~~ ✅
9. ~~Aprofundamento iterativo + `go movetime`~~ ✅, ~~pendências~~ ✅. **Níveis de dificuldade
   (RF03) não mapeados:** fica para quando a IA for ligada ao `go`
10. ~~Ordenação MVV-LVA~~ ✅ (`86985a8`), ~~melhor lance anterior primeiro na raiz~~ ✅ (`97c18dc`)
    - **10b. Aproveitar a iteração parcial** (§3.13) — **próximo**
11. Quiescência
12. TT (fora do MVP)

Ligar a IA ao `go` do `uci.c` e ao build do motor ainda não tem lugar na lista — decidir com
o grupo.

---

## 5. Onde está cada coisa (repositório, branch `IA`)

| Arquivo | Conteúdo | Estado |
|---|---|---|
| `IA/Pseudocode/minimax.txt` | minimax; `profMax`, `limiteIteracao`, `tempoEsgotado`, `estourouTempo`, `nosVisitados` | fechado |
| `IA/Pseudocode/alfa-beta.txt` | poda alfa-beta; ordenação; `primeiro` na raiz | fechado (muda no 10b) |
| `IA/Pseudocode/ordenarLances.txt` | `notaMVVLVA`, `ordenarLances` com `primeiro` | fechado |
| `IA/Pseudocode/avaliar.txt` | avaliação v1 | fechado |
| `IA/Pseudocode/escolherJogada.txt` | chamadora: aprofundamento, relógio, parada por mate, `CANDIDATOS` | fechado (muda no 10b) |
| `IA/Pseudocode/verificarChamada.txt` | verificação + `RESULTADO_*` | fechado |
| `IA/Pseudocode/converterScore.txt` | conversão para o UCI | fechado |
| `IA/header/ia.h` | constantes, `Play`, protótipos, `JOGADA`, `CANDIDATOS`, `NOS_VISITADOS` | fechado |
| `IA/header/avaliar.h` | `avaliar`, `AVALIACAO_MAX` | fechado |
| `IA/header/ordenar.h` | `LanceOrdenado`, `NOTA_PRIMEIRO`, `ordenarLances` | fechado |
| `IA/header/pendenciasRegras.h` | `testeEmpateRegra` + motivos (provisório) | provisório |
| `IA/header/stubs.h`, `IA/src/stubs.c` | stub controlável de empate (`varTesteEmpate`) | para testes |
| `IA/src/avaliar.c` | avaliação v1 | testado |
| `IA/src/ordenar.c` | MVV-LVA + inserção estável | testado |
| `IA/src/ia.c` | conversão, verificação, busca, relógio, chamadora, sorteio, contagem | **executado e testado** |
| `IA/src/teste.c` | 7 blocos de teste (§7) | passando |
| `IA/docs/` | contextos das etapas 2–11 | versionado |

`IA/Pseudocode/teste.exe` é um binário solto na pasta de pseudocódigo; não está no git.
`IA/teste.exe` (saída do comando da §6.1) é ignorado pelo `*.exe` do `.gitignore`.

---

## 6. Implementação em C — notas

### 6.1 Build e ambiente

- Compilador: **GCC TDM 4.9.2** (Dev-Cpp MinGW64). `make` não está no PATH: usar
  **`mingw32-make`** (ex.: `mingw32-make -C engine test`; `PERFT_MAX_DEPTH=3` reduz o perft).
- **Comando que compila o teste da IA** (da raiz, PowerShell) — **ganhou `IA/src/ordenar.c`
  na etapa 11**:

  ```powershell
  $eng = Get-ChildItem engine/src/*.c | Where-Object Name -ne 'main.c' | ForEach-Object FullName
  gcc -std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Og -g `
      -IIA/header -Iengine/include IA/src/teste.c IA/src/ia.c IA/src/avaliar.c IA/src/ordenar.c `
      IA/src/stubs.c $eng -o IA\teste.exe
  .\IA\teste.exe; $LASTEXITCODE
  ```

  Todo o `engine/src` menos `main.c`. A suíte leva ~3 s. O `.vscode/tasks.json` (ignorado pelo
  git) **ainda não** está assim — acertar em cada máquina, **incluindo o `ordenar.c`**.
- **Avisos aceitos:** só os que vêm de `engine/` (`fen.c:153`, `io.c`, `move.c`, `movegen.c`,
  `uci.c`), cerca de 15. **Qualquer aviso vindo de `IA/` é novo.** Em `97c18dc`: nenhum.
- `teste.c` começa com `#define __USE_MINGW_ANSI_STDIO 1` (antes de qualquer include) para
  `PRIu64` não gerar aviso com `-Wpedantic` no GCC 4.9.
- **Fim de linha:** `core.autocrlf = true`. No repositório os arquivos estão em LF; na cópia
  de trabalho, em **CRLF**. Depois de editar com ferramenta de linha de comando, conferir com
  `file` (deve dizer "with CRLF line terminators"). Arquivos **novos** criados pelo Claude nascem
  em LF: converter (`sed -i 's/$/\r/'` só em arquivo sem `\r`) e conferir.
- **Ferramenta de edição do Claude** pode apagar espaços no fim de linhas em branco (etapa 11,
  no `ia.c`; restaurado via PowerShell antes do commit). Conferir o `git diff` por mudanças só
  de espaço.
- O Windows (Controle de Aplicativo) bloqueia executáveis gerados. Na etapa 11 bloqueou também
  `IA\teste.exe` de forma intermitente (um executável com outro nome foi bloqueado sempre). Se
  bloquear, a execução fica comigo.

### 6.2 Armadilhas de C que já apareceram

- Lista local: declarar o **objeto** (`MoveList lista;`) e passar `&lista`.
- `extern` no `.h` é declaração; definição uma vez num `.c`.
- Macros com expressão sempre parentizadas.
- `printf` com `<inttypes.h>`: literais colados, sem vírgulas.
- Parâmetros `int8_t` truncam valores de `int`: manter o tipo do chamador.
- **GCC 4.9 + msvcrt:** `%zu`, `%llu` e `PRIu64` avisam (ou falham) sem
  `__USE_MINGW_ANSI_STDIO`.
- **`clock_t` no Windows é `long` de 32 bits** e `CLOCKS_PER_SEC` = 1000; no Linux,
  `CLOCKS_PER_SEC` = 1.000.000. Conversões de ms e o `prazo` em `int64_t`.
- **`movelist_find` devolve −1** quando não acha: nunca indexar `moves[]` com o retorno sem
  conferir.

---

## 7. Testes (`IA/src/teste.c`, nesta ordem na `main`)

Formato: cada bloco roda até o fim, acumula uma flag, imprime um resumo; a `main` retorna 1 se
qualquer flag estiver ligada. Detecta **se** algo falhou, não **qual** caso.

`PROF_TESTE` = 4 é a profundidade dos testes de busca (sem limite de tempo).

Funções auxiliares (`static`): `mesmosCandidatos` (mesma ordem), `contemLance`,
`mesmoConjunto` (qualquer ordem, listas sem repetição).

### 7.1 `escolherJogada` / verificação — 12 casos (passando)

Inalterado. Confere `resultado`, os valores padrão, `NOS_VISITADOS == 0` e
`CANDIDATOS.count == 0`, com sentinelas antes de cada chamada.

### 7.2 Execução, v1 = v2 e contagem de nós — 9 FENs (passando)

FENs (`CasoV1V2 {fen, iteracoes}`): inicial; após 1.e4; mate em 1 e em 2 dos dois lados; dama
livre dos dois lados; kiwipete. Para cada uma:
- **v1 = v2 (revisto na etapa 11):** minimax e alfa-beta com `SORTEIO_DESLIGADO` → mesmo
  `score` e `tipo`, `resultado == SUCESSO` nas duas, **mesmo conjunto** de `CANDIDATOS`
  (`mesmoConjunto`) e lance do alfa-beta ∈ `CANDIDATOS` do minimax. **Não** exige o mesmo lance.
- **Determinismo:** alfa-beta duas vezes → mesma `JOGADA`, mesma contagem e mesma `CANDIDATOS`
  **na mesma ordem** (`mesmosCandidatos`).
- **Contagem:** minimax visita **exatamente** `Σ_{D=1..iteracoes} Σ_{d=0..D} perft(d)`;
  alfa-beta ≤ minimax em cada FEN (vale em qualquer ordem: a poda visita um subconjunto da
  árvore); no total, alfa-beta **estritamente** menor.
- **Guarda:** `halfmove_clock + PROF_TESTE >= 100` → falha com a FEN impressa.

`CANDIDATOS` coincide entre as variantes porque os filhos da raiz são buscados com janela
completa (valores exatos). O esperado exato do minimax supõe que nenhum empate por regra
dispara dentro da árvore (hoje garantido pelo stub).

### 7.3 Limite de tempo — 3 casos (passando)

Inalterado: kiwipete, `profundidade = PROF_MAX`, alfa-beta 1 ms e 100 ms, minimax 100 ms.
`SUCESSO`, lance legal, tabuleiro intacto, iteração 1 completa, gasto ≤ `tempoMs` + 50 ms.
**Recebe as checagens do 10b** (§3.13 item 4).

### 7.4 Sorteio (passando; FEN trocada em `dd8dc2d`)

FEN `7k/8/8/3r4/5N2/1BN5/8/3QK3 w - - 0 1` (torre preta solta, capturável por D, B, C, C):
**4 candidatos entre 35 lances legais**. Alfa-beta, `PROF_TESTE`, sementes 1..8.
- Desligado: `SUCESSO`, **`2 ≤ CANDIDATOS.count < lances legais`** e lance ==
  `CANDIDATOS.moves[0]`. O `< legais` garante que "lance ∈ `CANDIDATOS`" distingue candidato
  de lance legal qualquer (a posição inicial antiga tinha 20 de 20).
- Ligado, para cada semente: `SUCESSO`; lance ∈ `CANDIDATOS`; mesmo número de candidatos,
  score e tipo do desligado; mesma semente → mesmo lance.
- Ao menos uma semente dá lance diferente do desligado. Medido: os 4 lances aparecem nas 8
  sementes.

Não fixa o conjunto esperado (depende da avaliação v1; quebraria com PST).

### 7.5 Ordenação — novo (passando)

FEN `7k/8/8/q2pP1r1/1P6/1n3N2/2B5/3QK3 w - d6 0 1`: uma captura de cada nota, todas distintas,
incluindo en passant — `b4a5` (39), `f3g5` (30), `c2b3` (13), `e5d6` e.p. (7), `d1d5` (3).
Três casos de `primeiro`: `MOVE_NONE`; o último lance sem captura do gerador (o que mais anda
até a frente; hoje `f3d2`); a captura de menor nota (`d1d5`, sai do meio das capturas). Para
cada um:
- `primeiro` (se houver) em `ordenados[0]` com `NOTA_PRIMEIRO`;
- depois as capturas na ordem esperada, sem o `primeiro`;
- depois os lances sem captura na ordem do gerador, com nota 0, sem o `primeiro`;
- total de lances conferido (nenhum some ou repete; as capturas esperadas são todas as capturas).

### 7.6 `avaliar` — 14 casos (passando)

Inalterado.

### 7.7 `converterScore` — 20 casos (passando)

Inalterado.

### 7.8 Medições (14/11, `97c18dc`)

**Contagem com aprofundamento iterativo até 4, sem tempo:**

| Posição | Lance minimax / alfa-beta | Score | Minimax (nós) | Alfa-beta etapa 10 | Alfa-beta etapa 11 |
|---|---|---|---|---|---|
| Inicial | a2a3 / a2a3 | cp 0 | 216.369 | 20.119 | 14.537 |
| Após 1.e4 | a7a6 / **a7a5** | cp −100 | 433.589 | 48.225 | 29.757 |
| Mate em 1 (B / P) | a1a8 / a8a1 | mate 1 | 164 | 164 / 164 | 164 / 164 |
| Mate em 2 (B / P) | a1a7 / a8a2 | mate 2 | 20.521 | 3.088 / 3.176 | 3.088 / 3.176 |
| Dama livre (B / P) | d1d5 / d8d4 | cp 900 | 191.632 | 16.183 / 15.150 | 10.818 / 10.527 |
| Kiwipete | e2a6 / e2a6 | cp 10 | 4.287.640 | 290.549 | 102.261 |
| **Total** | | | **5.362.232** | **396.818 (7,4%)** | **174.492 (3,3%)** |

Toda a economia da etapa 11 vem do MVV-LVA (`86985a8`); o melhor anterior primeiro na raiz
(`97c18dc`) deixou **todas** as contagens idênticas (§3.1). Mates sem capturas: sem mudança.

**Limite de tempo (kiwipete, profundidade `PROF_MAX`):**

| Variante | `tempoMs` | Gasto | Nós etapa 10 | Nós etapa 11 | Lance |
|---|---|---|---|---|---|
| Alfa-beta | 1 | 1–3 ms | 8.192 | 3.072 | e2a6 |
| Alfa-beta | 100 | 101–102 ms | 279.552 | 129.024–130.048 | e2a6 |
| Minimax | 100 | 100 ms | 513.024 | 502.784–520.192 | e2a6 |

Faixas: duas execuções na etapa 11 (depois de `86985a8` e de `97c18dc`); a coluna da etapa 10
é uma execução.

**Não investigado:** no alfa-beta com 100 ms, os nós no prazo caíram à metade. Pode ser o custo
da ordenação por nó ou outra proporção entre nós internos e folhas; a contagem não separa os
dois. Medir **a profundidade alcançada** no mesmo prazo antes de concluir (§9).

---

## 8. Decisões a acertar com a máquina de regras

1. **Congelamento dos cabeçalhos** (`board.h`, `piece.h`, `move.h`, `makemove.h`, `movegen.h`).
   A ordenação passou a depender também de `move_is_capture`, `move_is_ep_capture`, `move_from`,
   `move_to` (e o teste, de `move_to_str` e `movelist_find`).
2. **`in_check`:** adotar o do `next_steps.md` §9.2 (`bool in_check(const Board *b)`, em
   `attack.h/c`). A IA apaga `emXeque` quando existir.
3. ~~Data do perft~~ — resolvido.
4. **Empates:** função pública com motivo (`SEM_EMPATE` = 0 é requisito da IA); 50 lances já
   detectável; repetição precisa do histórico (planejado na camada de protocolo) — falta
   acordar **como a IA recebe esse histórico** e se a repetição dentro da árvore fica para o
   Zobrist.
5. ~~Lance nulo~~ — resolvido: `MOVE_NONE`.
6. **Zobrist**, se entrar: incremental em `make_move`/`unmake_move`.
7. **Bug #1 (`fen_parse`)**: a correção no parser protege também os testes da IA.
8. **Ligar a IA ao `go`** do `uci.c` e ao build do motor (hoje `IA/` não tem Makefile; o
   build precisa de `ia.c`, `avaliar.c`, `ordenar.c`). `go depth N` → `profundidade`,
   `go movetime T` → `tempoMs`; `score cp` / `score mate` via `converterScore`. Falta acordar o
   padrão quando o `go` vem sem parâmetros, e o mapa de níveis de dificuldade (RF03).

O dev das regras é **@caioveigap** (CODEOWNER de `engine/`).

---

## 9. Próximo passo (decidido)

1. **10b: aproveitar a iteração parcial** — §3.13, com as quatro decisões já tomadas. Começar
   apresentando os "pontos a resolver na implementação" que tiverem mais de uma saída.
2. **Redigir o pedido ao dev das regras** (§8 itens 2, 4 e 7, citando 1 e 8): nomes,
   assinaturas e onde cada coisa mora, alinhados ao `next_steps.md`. **Combinado fazer no mesmo
   dia do 10b.**

**Recomendações pendentes** (não são erros):
- **Medir a profundidade alcançada** no teste de tempo antes e depois da ordenação (§7.8).
- **`fen_parse` sem conferir o retorno** no bloco do sorteio (`teste.c`), herdado.
- **Pseudocódigo:** `relógio()` em vez de `clock()`; `prazo = inicio + tempoMs` omite a
  conversão de ms para tiques (`tempoMs * CLOCKS_PER_SEC / 1000`), que no Linux importa.
- **`.vscode/tasks.json`** com o comando da §6.1 (inclui `ordenar.c`).
- Seleção preguiçosa na ordenação (otimização futura, §3.12).

---

## 10. Mudanças de escopo da IA em relação à EAP (item 1.5)

**Trabalho realizado que não aparece na EAP** (acumulado):
- Especificação em pseudocódigo de todas as funções (7 arquivos, especificação viva).
- Verificação da chamada (`verificarChamada`) e campo `resultado`.
- Conversão de score para o protocolo (`converterScore`).
- Contrato com a máquina de regras (`pendenciasRegras.h`) — ligado a 1.5.2.
- Testes automatizados da IA: verificação (12), v1 = v2 + determinismo + contagem de nós (9
  FENs), limite de tempo (3), sorteio (8 sementes), **ordenação (3 casos)**, avaliação (14),
  conversão (20); stubs controláveis. Relaciona-se com 1.6.1.5.
- **Instrumentação por contagem de nós**, com o perft das regras como oráculo exato.
- Garantias em tempo de compilação (`_Static_assert`).
- Controle de semente do sorteio (`iniciarSorteio`).
- Robustez do relógio (falha do `clock()`, estouro de 32 bits) e parada por mate.

**Itens da EAP e seu estado:**

| Item | Estado |
|---|---|
| 1.5.1 Pesquisa | Concluído |
| 1.5.2 Integração com a Máquina de Regras | Em andamento: gerador legal, make/unmake e perft integrados; falta `in_check`, empates e ligação ao `go` |
| 1.5.3 Simulação de Estados | Concluído |
| 1.5.4.1 Avaliação Material | Concluído e testado |
| 1.5.4.2 Avaliação Posicional | Não iniciado (v6+) |
| 1.5.5.1 Minimax | Concluído, executado e testado |
| 1.5.5.2 Poda Alfa-Beta | Concluído, executado e testado (~3,3% dos nós do minimax com ordenação) |
| 1.5.5.3 Aprofundamento Progressivo | Concluído e testado; aproveitamento da iteração parcial decidido (§3.13); níveis de dificuldade não mapeados |
| 1.5.5.4 Ordenação de Movimentos | **Concluído e testado** (`86985a8`, `97c18dc`): MVV-LVA −56% de nós no alfa-beta |
| 1.5.6 Seleção de Lance Aleatório | Concluído e testado |

**Quanto está pronto (14/11):** **8 de 10 itens da EAP** concluídos e testados (80% por
contagem). **Por esforço, ~65–70%** (~60% se a quiescência entrar no escopo):

| Parte | Peso aproximado | Estado |
|---|---|---|
| Núcleo da busca (minimax, alfa-beta, aprofundamento, tempo, sorteio) + testes + especificação | ~45% | pronto (10b pendente) |
| Avaliação material, conversão de score, verificação da chamada | ~10% | pronto |
| Integração (`go`, build, níveis, `in_check` e empates do motor) | ~20% | pouco feito |
| Ordenação MVV-LVA + revisão do v1 = v2 | ~10% | **pronto** |
| Avaliação posicional (PST) | ~10% | não iniciado |
| (Quiescência, se contar) | +10% | não iniciado |

Para apresentação: "8 de 10 itens da EAP concluídos e testados; com a ordenação, o alfa-beta
visita 3,3% dos nós do minimax; falta a integração com o motor e a avaliação posicional".

**Tamanho:** não recontado nesta etapa.

Quiescência e TT continuam fora da EAP — levar ao grupo.
