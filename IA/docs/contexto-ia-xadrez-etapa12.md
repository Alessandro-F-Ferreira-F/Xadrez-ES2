# Contexto para continuidade — IA de xadrez (etapa 12: iteração parcial e quiescência)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa11.md`.

Como nas etapas anteriores, **o repositório é a fonte da verdade** (`Xadrez-ES2`, branch `IA`).
Este arquivo guarda o que o repositório não guarda: decisões, motivos, medições, pendências e
o estado do trabalho. A §5 lista onde está cada coisa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Estado em 14/11/2026 (fim da etapa 12):** item 10b feito
(iteração parcial, `3c952b0`); teto absoluto `PROF_LIMITE` (`37f6831`); **item 11 feito**:
quiescência como opção nas duas variantes (`d874b94`). `IA`, `main`, `origin/IA` e
`origin/main` no mesmo commit (`d874b94`). Suíte passando (3–4 s), nenhum aviso vindo de `IA/`.
Pedido ao dev das regras **redigido no chat em 14/11** (§8), ainda não enviado.
**Próximo passo:** enviar o pedido; depois, custo da quiescência contra o RNF01 ou PST — §9.

---

## 0. Dependências da máquina de regras

### 0.1 Já disponível e usado pela IA

| Item | Onde | Observação |
|---|---|---|
| `Board`, `Color`, `Piece`, `Move`, `MoveList`, `MOVE_NONE` | `board.h`, `piece.h`, `move.h` | `MoveList` tem `MAX_MOVES` = 256 lances |
| `generate_legal_moves(Board*, MoveList*)` | `movegen.h` | **Legal**. Não há gerador só de capturas: a quiescência gera tudo e filtra |
| `make_move` / `unmake_move`, `Undo` | `makemove.h` | Validados pelo perft |
| `move_is_capture`, `move_is_ep_capture`, `move_from`, `move_to` | `move.h` | Ordenação (§3.12) e filtro da quiescência (§3.15) |
| `move_to_str`, `movelist_find` | `move.h` | Usados nos testes |
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
   mas dentro de `uci_status`, `static` no `uci.c`.
3. **Repetição e material insuficiente:** não implementados. O plano das regras
   (`project_context.md`, `next_steps.md` §9.5) é o histórico ficar na **camada de protocolo**
   (chaves de posição, depois Zobrist), não no `Board`.
4. **Bug #1 do motor:** `fen_parse` aceita FEN em que o lado que **não** joga está em xeque;
   daí o gerador deixa capturar o rei, `king_square` vira `SQ_NONE` e a próxima chamada aborta
   num `assert`. O UCI recusa essas FENs na fronteira (`uci_position_is_legal`); **a IA, nos
   testes, chama `fen_parse` direto e não tem essa defesa** — não montar FEN assim. Correção
   prevista: `next_steps.md` §9.4.

Até 2 e 3 existirem, `testeEmpateRegra` continua vindo do stub (`stubs.c`), declarado em
`IA/header/pendenciasRegras.h`.

---

## Como quero que você conduza

**Desde a etapa 8:** **o Claude escreve o código; eu reviso e decido.**

- Respostas **sucintas e tecnicamente precisas**. Sem preâmbulo, sem repetir o que eu disse.
- Eu **decido o escopo e a ordem**. Sugira, mas não assuma. Decisões de projeto com mais de
  uma saída razoável: apresente as opções com uma recomendação e espere a minha escolha
  (foi assim em todo o 10b e na quiescência, §3.13–§3.16).
- **Antes de recomendar, cruze a recomendação com as decisões já tomadas.** Etapa 11: "melhor
  lance anterior primeiro na raiz" não economiza nós por causa da janela completa (§3.1).
  Etapa 12: o teto da quiescência em `PROF_MAX` desligaria a quiescência com `go depth 64` —
  eu recusei e a opção certa foi um teto separado (§3.14).
- **Meça antes de fixar parâmetros de teste com custo desconhecido** (etapa 12: o minimax com
  quiescência explodiu; a profundidade por posição saiu da medição, §7.2).
- Ao entregar código: compile **a partir do disco**, rode a suíte e mostre o que mudou para eu
  revisar. Quando o código já estiver commitado, mostre o diff por bloco lógico.
- Mudança grande com duas mudanças de comportamento: **entregar em passos, um commit por
  passo**, esperando meu commit antes do seguinte (etapa 12: `PROF_LIMITE` antes da quiescência).
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo. Se minha conclusão estiver certa e o motivo não, diga.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Avalie **só o escopo que eu pedir**. Decisão de estrutura tomada sem me consultar (etapa 12:
  `executarV1V2` no teste, testes exatos das globais de instrumentação) tem de ser
  **sinalizada** na entrega.
- Mantenho o **pseudocódigo como especificação viva**: toda mudança no C que altera a
  especificação atualiza o `.txt` correspondente, com os **mesmos nomes** do C.
- Separe **erro** de **recomendação**. Erro do próprio Claude em entrega anterior também
  (etapa 12: comentário do caso de 1 ms desatualizado pelo 10b, corrigido em `d874b94`).
- **Eu faço os commits.** Não use linhas de coautoria. Sugira a mensagem; eu commito.
- Verifique o que entrou em cada commit meu (`git show --stat`). Ao anunciar quantos arquivos
  um commit terá, **contar com `git status`**, incluindo os não rastreados.

**Padrões de erro meus para vigiar:**
- Ramos espelhados corrigidos pela metade, ou errados do mesmo jeito nos dois lados.
- Consumidores não atualizados depois que mudo assinatura, nome ou decisão.
- Fundir casos distintos num mesmo retorno.
- Estado local declarado como global, e o inverso.
- Informação duplicada em dois lugares.
- Precedência e parênteses em macros e expressões fundidas.
- Erros de digitação em nomes e mensagens.
- Ponteiro sem objeto por trás; **índice que pode ser −1** (`movelist_find`); `extern`
  (declaração) confundido com definição; vírgulas entre literais em `printf` com `PRI...`;
  **condição de teste que nunca pode ser verdadeira**.
- **Editor salvando buffer antigo por cima do disco** (etapa 8). Antes de commitar, compilar a
  partir do disco.
- **Commit com mais do que o combinado**, **antes do combinado**, ou **mensagem repetida**.
- **Mensagem de commit fora da convenção** (etapa 10: `0615267`). Nas etapas 11 e 12, todos
  os commits saíram na convenção.

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
**ainda sorteia** um lance legal (`uci_pick_move`, `uci.c`): a IA não está ligada ao `uci.c`
nem ao build do motor.

**Requisitos que afetam a IA:** RF03 (níveis de dificuldade); RNF01 (resposta < 200 ms nos
níveis básicos); protocolo com `go movetime` / `go depth` e `score cp` / `score mate`;
invariante `bestmove` ∈ lista legal (o `uci.c` já confere, RF-M3); testes do grupo "v2 devolve
o mesmo lance que v1" e "mesma FEN + profundidade → mesmo lance" — implementados (§7.2). **Com
a ordenação, "v2 = v1" vale para score e conjunto de candidatos, não para o lance** (§3.12).

**Roadmap (Fase 7):** v1 material + minimax → v2 alfa-beta → v3 aprofundamento iterativo +
movetime → v4 MVV-LVA → v5 quiescência → v6+ PST, tapered, TT. TT fora do MVP.

**Gantt (IA):** alfa-beta, ordenação e aprofundamento 03/11–20/11. Apresentação **23/11**.
**Situação em 14/11:** v1 a **v5** executadas e testadas.

**Commits:** `docs(IA): ...` para pseudocódigo/documentação, `feat(IA): ...` para código
(inclusive mudanças só de teste), `fix(IA): ...` para correção, `refactor(IA): ...` para mudança
sem efeito no comportamento; título sozinho na primeira linha **sem ponto final**; uma mudança
de comportamento por commit.

| Commit | Conteúdo | Estado |
|---|---|---|
| `447b019` | `PROF_MAX` 4 + pulo na conversão; **também** v1 = v2, contagem de nós, pseudocódigo e `IA/docs/` | **não compila sozinho** |
| `9e347ad` | `extern NOS_VISITADOS` no `ia.h` | |
| `e8063fa` | Guarda de `halfmove` + sentinela de `NOS_VISITADOS` no teste | |
| `179c7f3` | `LANCE_NULO` → `MOVE_NONE` + contexto da etapa 8 | |
| `c95a223` | Aprofundamento iterativo + limite de tempo | |
| `3f2885b` | Contexto da etapa 9 | |
| `30d8530` | Parada ao achar mate | |
| `0615267` | Relógio em `int64_t` + `prazoPassou` + `CANDIDATOS` + teste do sorteio | mensagem com ponto final, deixado assim |
| `279c468` | Contexto da etapa 10 | |
| `d79d752` | v1 = v2 compara `CANDIDATOS` inteira (`teste.c`) | |
| `dd8dc2d` | Sorteio com poucos candidatos (`teste.c`) | |
| `b54beb8` | Nomes do relógio no pseudocódigo (`escolherJogada.txt`) | |
| `86985a8` | MVV-LVA no alfa-beta: `ordenar.h/.c`, `ordenarLances.txt` (novos) + 4 arquivos | |
| `97c18dc` | Melhor lance anterior primeiro na raiz (8 arquivos) | |
| `fc46605` | Contexto da etapa 11 | |
| `3c952b0` | Iteração parcial do alfa-beta; `PROF_ALCANCADA`, `PARCIAL_USADA` (5 arquivos) | |
| `37f6831` | `PROF_LIMITE` = 2 × `PROF_MAX`; `LIMIAR_MATE` = `MATE − PROF_LIMITE` (5 arquivos) | |
| `d874b94` | Quiescência nas duas variantes, passada de segurança, parada por mate nova, `NOS_QUIESCENCIA`; `quiescencia.txt` (novo) + 7 arquivos | **topo de `IA`, `main` e remotos** |

Todos em `origin/main`. `IA/docs/` entra no repositório **de propósito**: explicita o
desenvolvimento orientado a especificação em `.md`.

---

## 3. Decisões tomadas

### 3.1 Busca (inalteradas desde a etapa 1, salvo limite, ordenação e quiescência)

- Convenção absoluta: brancas positivas, pretas negativas; max/min literais.
- Vez lida de `side_to_move`; sem parâmetro `jogador`.
- Ordem do caso base da busca principal: **tempo esgotado** → empate por regra → limite da
  iteração (**com `quiescenciaAtiva`, chama a quiescência; senão `avaliar`**) → gerar lances →
  lista vazia (xeque = mate, senão afogamento) → **ordenar (só alfa-beta)** → laço.
- `MoveList`, `Undo` e (no alfa-beta) `LanceOrdenado listaOrdenada[MAX_MOVES]` **locais ao nó**.
- `MATE = 19999`, mate vale `±(MATE − profundidade)` com sinal pelo **lado derrotado**;
  `INFINITO = 20000`; empate = 0; `SCORE_INVALIDO = 30000`.
- Score em `int32_t`.
- Alfa-beta: na raiz alfa e beta **não são atualizados** (`profundidade > 0`). Cada iteração
  chama a raiz com a janela completa; sem *aspiration windows*. **Consequência:** todo filho
  da raiz é buscado com a janela completa e tem valor exato, então a **ordem dos lances na
  raiz não muda a contagem de nós** — só a ordem de `CANDIDATOS`. É também o que torna exata
  a iteração parcial (§3.13) e o v1 = v2 com quiescência.
- **Sem quiescência**, o mate mais profundo que uma iteração produz é `MATE − (limiteIteracao −
  1)`: mate em p meios-lances só aparece a partir da iteração p + 1. **Com quiescência**, o nó
  do horizonte gera os lances e reconhece o mate: aparece já na iteração p.

### 3.2 Avaliação v1 (fechada e testada)

Só material, Michniewski (P 100, C 320, B 330, T 500, D 900, R 0). `AVALIACAO_MAX` = 10615
(margem para PST). Teste do rei adiado para a v6+.

### 3.3 Constantes públicas, `PROF_MAX` e `PROF_LIMITE`

`PROF_MAX`, `PROF_LIMITE`, `TEMPO_SEM_LIMITE`, `MATE`, `INFINITO`, `SCORE_INVALIDO`,
`LIMIAR_MATE`, `QUIESCENCIA_LIGADA` (1), `QUIESCENCIA_DESLIGADA` (0) no `ia.h`; `NOTA_PRIMEIRO`
no `ordenar.h`.

- **`PROF_MAX` = 64**: **só** o teto do parâmetro `profundidade` (`go depth`). Não é a
  profundidade da busca (quem limita é `limiteIteracao`) e não define mais o `LIMIAR_MATE`.
- **`PROF_LIMITE` = 2 × `PROF_MAX` = 128** (etapa 12, `37f6831`): teto absoluto de
  profundidade de **qualquer** nó, inclusive na quiescência. Em `PROF_LIMITE` a quiescência
  devolve `avaliar`, mesmo em xeque.
- **`LIMIAR_MATE` = `MATE − PROF_LIMITE` = 19871**: todo mate a até 128 meios-lances é
  classificado como mate (antes, 19935).
- Pilha: ~128 níveis × (`MoveList` ≈ 516 B + `Undo` + `listaOrdenada` 1 KB) ≈ 200 KB. Sem
  problema (pilha padrão do Windows: 1 MB).
- `_Static_assert(LIMIAR_MATE > AVALIACAO_MAX)` no `ia.c` (falha se `PROF_LIMITE` ≥ 9384);
  `_Static_assert(NOTA_PRIMEIRO > 8 * QUEEN - PAWN)` no `ordenar.c`.

### 3.4 Chamadora e resultado da chamada

`Play.resultado`: 0 sucesso, negativos bug, positivos fim de partida na raiz;
`verificarChamada` interna; valores padrão (`MOVE_NONE`, `SCORE_INVALIDO`, `TIPO_INVALIDO`,
`CANDIDATOS` vazia, `PROF_ALCANCADA` 0, `PARCIAL_USADA` falso) quando `resultado != SUCESSO`.

| Valor | Nome |
|---|---|
| −6 | `RESULTADO_QUIESCENCIA_INVALIDA` (etapa 12) |
| −5 | `RESULTADO_TEMPO_INVALIDO` (`tempoMs < 0`) |
| −4 | `RESULTADO_PROFUNDIDADE_INVALIDA` (fora de `[1, PROF_MAX]`) |
| −3 | `RESULTADO_RANDOMIZADOR_INVALIDO` |
| −2 | `RESULTADO_VARIANTE_INVALIDA` |
| −1 | `RESULTADO_ERRO_INTERNO` |
| 0 | `RESULTADO_SUCESSO` |
| 1, 2 | `RESULTADO_RAIZ_MATE`, `RESULTADO_RAIZ_AFOGAMENTO` |
| 3, 4, 5 | `RESULTADO_EMPATE_50_LANCES`, `_REPETICAO`, `_MATERIAL` (provisórios) |
| 6 | `RESULTADO_EMPATE_PROVISORIO` (stub devolve `EMPATE_PROVISORIO` = 4) |

**Ordem da verificação:** variante → randomizador → **quiescência** → profundidade → tempo →
raiz (empate por regra, depois lista vazia). Só o primeiro problema é reportado. Falha do
`clock()` **não** é resultado de erro (§3.11).

### 3.5 Conversão de score

Inalterada no código. Classifica como mate todo `|score| ≥ LIMIAR_MATE`, agora 19871 (§3.3).

### 3.6 Sorteio

`iniciarSorteio` uma vez; `rand() % quantidade`. Sorteia sobre `CANDIDATOS`.

### 3.7 Organização do código

- `static`: `verificarChamada`, `emXeque`, `prazoPassou`, `estourouTempo`,
  `quiescenciaMinimax`, `quiescenciaAlfaBeta`, `minimax`, `podaAlfaBeta`, `sorteioLance`,
  `listaCandidatos`, `scoreParcial`, `limiteIteracao`, `quiescenciaAtiva`, `relogioAtivo`,
  `prazo`, `tempoEsgotado` (`ia.c`); `notaMVVLVA` (`ordenar.c`). Públicas: `converterScore`,
  `escolherJogada`, `iniciarSorteio`, `JOGADA`, `CANDIDATOS`, `NOS_VISITADOS`,
  `NOS_QUIESCENCIA`, `PROF_ALCANCADA`, `PARCIAL_USADA` (`ia.h`); `LanceOrdenado`,
  `NOTA_PRIMEIRO`, `ordenarLances` (`ordenar.h`).
- Globais públicas: **declaradas** `extern` no `ia.h`, **definidas** uma vez no `ia.c`.
- `pendenciasRegras.h` é incluído pelo `ia.c` e pelos stubs, não pelo `ia.h`.
- **Módulo `ordenar` separado** (etapa 11), no molde do `avaliar`.
- **Quiescência no `ia.c`** (etapa 12): precisa do estado da iteração (`static`), de `emXeque`
  e de `estourouTempo`.

### 3.8 Descartado

Ponder e threads. Função de leitura dos candidatos (API desnecessária). Ordenação no minimax
(§3.12). Variável `lanceAnterior` (§3.12). Aproveitar a parcial só quando um lance novo supera
o melhor anterior (§3.13). Quiescência só no alfa-beta, substituindo (quebraria o v1 = v2) e
como variantes 2 e 3 (fundiria dois eixos) (§3.15). Teto da quiescência em `PROF_MAX`
(§3.14). Iteração 1 sempre sem quiescência e prazo não garantido com quiescência (§3.16).

### 3.9 Da etapa 8

- **Contagem de nós:** `uint64_t NOS_VISITADOS`, público. Cada entrada em `minimax`,
  `podaAlfaBeta` **ou numa função de quiescência** conta um nó, inclusive a raiz. Zerado no
  início de `escolherJogada`; fica em 0 se a verificação falha. Soma todas as iterações, também
  a interrompida e a passada de segurança.
- **`MOVE_NONE` no lugar de `LANCE_NULO`:** mesmo sentinela (`(Move)0`, a1→a1).
- **`emXeque` fica como está** até o motor publicar `in_check`; então é apagada e substituída
  (commit próprio, com pseudocódigo). **Não copiar** o `if (SQ_OFFBOARD(ksq)) return false`
  do `uci_in_check`.

### 3.10 Da etapa 9 (aprofundamento iterativo, `c95a223`)

- **Interface (etapa 12):** `escolherJogada(board, variante, randomizador, quiescencia,
  profundidade, tempoMs)`; `tempoMs = TEMPO_SEM_LIMITE (0)` desliga o relógio.
- **Iteração interrompida pelo tempo:** no alfa-beta pode ser aproveitada (§3.13); no minimax
  é descartada.
- **Iteração 1 sempre termina** quando não há passada de segurança: roda com `relogioAtivo`
  falso. Com quiescência e relógio, ver §3.16.
- **Relógio:** `clock()` do C11. No Windows mede tempo de parede; no POSIX, tempo de CPU.
  O prazo conta do início de `escolherJogada`.
- **Consulta ao relógio:** `estourouTempo()` na entrada do nó (também na quiescência), só com
  `relogioAtivo` e a cada `INTERVALO_RELOGIO` = 1024 nós; também antes de cada iteração com
  relógio.
- **Saída por tempo:** o nó devolve 0 na entrada; no laço, `if(tempoEsgotado) return 0;` logo
  **depois** do `unmake_move` e **antes** de comparar com `melhor`, nos **oito** laços (quatro
  da busca principal, quatro da quiescência).
- **Estado da iteração como `static` do arquivo.**

### 3.11 Da etapa 10 (`30d8530`, `0615267`), revista na etapa 12

- **Parada ao achar mate (revista em `d874b94`):** depois de gravar uma iteração completa,
  `if(abs(scoreBruto) >= MATE - limiteIteracao) break;` — mate a até `limiteIteracao`
  meios-lances, ou seja, dentro da parte de largura completa. Mate achado só na quiescência
  (mais fundo) **não para**: lá a defesa não é completa e o mate pode não ser o mais curto.
  Sem quiescência, equivale à condição antiga `>= LIMIAR_MATE`.
- **`clock()` falhando (`(clock_t)-1`) = prazo passado.** `prazo = INT64_MIN` se falha no
  início: só a iteração 1 (ou a passada de segurança) roda. Sem teste.
- **`extern MoveList CANDIDATOS`** pública: candidatos da **última iteração aproveitada**
  (completa, parcial no alfa-beta, ou a passada de segurança). Esvaziada no início de
  `escolherJogada` e no bloco defensivo.

### 3.12 Da etapa 11 (`86985a8`, `97c18dc`)

- **Ordenação só no `podaAlfaBeta`** (e, na etapa 12, na `quiescenciaAlfaBeta`). O minimax e a
  `quiescenciaMinimax` seguem na ordem do gerador como referência.
- **Nota MVV-LVA:** `8 × tipo(vítima) − tipo(atacante)`, de 2 a 39. En passant: vítima = peão.
  Promoção com captura: atacante = peão. Sem captura: 0. Sem bônus de promoção.
- **Inserção estável** antes do laço. **Seleção preguiçosa fica como otimização futura.**
- **`LanceOrdenado {Move lance; int16_t nota}`**.
- **Melhor lance da iteração anterior primeiro na raiz** (`NOTA_PRIMEIRO` = 64, `primeiro =
  CANDIDATOS.moves[0]` na raiz). **Não economiza nós** (§3.1); mantido porque torna segura a
  iteração parcial (§3.13).
- **`CANDIDATOS` na ordem da busca:** minimax e alfa-beta podem devolver lances diferentes
  entre candidatos empatados com `SORTEIO_DESLIGADO`.

### 3.13 Da etapa 12: iteração parcial (`3c952b0`)

Opção B da etapa 11, com as quatro recomendações; na implementação, **A1** e **E1**:

- **Quando:** só alfa-beta. Se o tempo acabou e `listaCandidatos.count > 0` (A1: só filho
  completo entra na lista, e o primeiro filho completo sempre entra), a parcial substitui a
  iteração anterior: o primeiro filho é o melhor anterior, e os filhos completos têm valor
  exato (janela completa). Empates entre filhos não buscados ficam de fora (aceito).
- **Score parcial:** `static int32_t scoreParcial`, gravado junto com `listaCandidatos` na raiz
  do `podaAlfaBeta` (ramo `atual > melhor`), zerado para `SCORE_INVALIDO` a cada iteração.
- **Instrumentação (E1):** `int PROF_ALCANCADA` (limite da iteração aproveitada) e `bool
  PARCIAL_USADA`, públicas, separadas. Zeradas no início e no bloco defensivo.

### 3.14 Da etapa 12: teto absoluto (`37f6831`)

A quiescência pode passar de `limiteIteracao` em ~60 meios-lances (≤ 30 capturas, mais
evasões). Sem teto, um mate no ply > 64 valeria menos que o `LIMIAR_MATE` antigo e sairia como
centipeão, e a pilha perderia o limite. **Decisão:** `PROF_LIMITE` separado (§3.3). O teto em
`PROF_MAX` foi recusado por mim: desligaria a quiescência com `go depth` perto de 64.

### 3.15 Da etapa 12: quiescência (`d874b94`)

Escolhas: **1C, 2A, 3B, 4 (teto §3.14 + parada §3.11), 5A**.

- **1C — chave nas duas variantes:** parâmetro `quiescencia` em `escolherJogada`. Desligada,
  tudo como antes (perft exato, v1 = v2). Ligada, o v1 = v2 é o oráculo da quiescência com
  poda (`quiescenciaMinimax` sem poda como referência).
- **2A — só capturas:** `move_is_capture` (inclui en passant e promoção com captura, todas as
  peças). Promoção simples não entra. Filtro por `continue`, sem depender de as capturas
  virem primeiro na ordenação.
- **3B — xeque à parte:** gera os lances legais; lista vazia → mate/afogamento como na busca
  principal; em xeque, todas as evasões e sem *stand-pat*; fora de xeque, *stand-pat*
  (`avaliar`) + capturas.
- **Ordem do caso base da quiescência:** tempo → empate por regra → `PROF_LIMITE` → gerar →
  lista vazia → *stand-pat* (fora de xeque; no alfa-beta, pode cortar) → laço.
- **Chamada:** o nó do horizonte chama a quiescência **na mesma profundidade** (é a raiz
  dela). Nunca é a raiz da busca, então alfa e beta sempre podem ser atualizados nela.
- **5A — `uint64_t NOS_QUIESCENCIA`:** entradas nas funções de quiescência. O nó do horizonte
  conta duas vezes em `NOS_VISITADOS`, então `NOS_VISITADOS − NOS_QUIESCENCIA` é a busca
  principal — no minimax, continua exata pelo perft.
- Fora do escopo: *delta pruning*, SEE.

### 3.16 Da etapa 12: passada de segurança (`d874b94`), escolha **A1**

Com quiescência, nem a iteração 1 tem custo limitado (medido: minimax no kiwipete > 60 s com
limite 1; alfa-beta ~250 ms com `-Og`). **Com quiescência e relógio:** antes do laço, uma
passada de limite 1 **sem** quiescência e com o relógio desligado (sempre termina: raiz + um nó
por lance) preenche `CANDIDATOS` e `scoreBruto`; depois **todas** as iterações, inclusive a 1,
rodam com quiescência e sob o relógio. A iteração 1 cortada segue a regra da parcial (o
primeiro filho é o melhor da passada de segurança). Se só a passada valer, **`PROF_ALCANCADA`
= 0 com `RESULTADO_SUCESSO`** (A1; distinguível da falha pela `resultado`). Sem limite de
tempo, não há passada.

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo das funções~~ ✅
2. ~~Pseudocódigo: `verificarChamada` e `converterScore`~~ ✅
3. ~~C: `avaliar` + testes~~ ✅
4. ~~C: `converterScore` + testes~~ ✅
5. ~~C: verificação, busca, chamadora, sorteio~~ ✅
6. ~~Stubs + testes da `verificarChamada`~~ ✅
7. Acertar com as regras as decisões da §8 — **pedido redigido em 14/11, falta enviar**
8. ~~Primeira execução da busca~~ ✅
9. ~~Aprofundamento iterativo + `go movetime`~~ ✅. **Níveis de dificuldade (RF03) não
   mapeados:** fica para quando a IA for ligada ao `go`
10. ~~Ordenação MVV-LVA~~ ✅, ~~melhor anterior primeiro~~ ✅, ~~10b: iteração parcial~~ ✅
11. ~~Quiescência~~ ✅ (`37f6831`, `d874b94`)
12. TT (fora do MVP)

Candidatos para o próximo item (decidir): custo da quiescência contra o RNF01 (*delta
pruning*), PST (v6). Ligar a IA ao `go` depende do dev das regras (§8).

---

## 5. Onde está cada coisa (repositório, branch `IA`)

| Arquivo | Conteúdo | Estado |
|---|---|---|
| `IA/Pseudocode/minimax.txt` | minimax; `profMax`, `PROF_LIMITE`, `limiteIteracao`, `tempoEsgotado`, `estourouTempo`, `nosVisitados`, `quiescenciaAtiva` | fechado |
| `IA/Pseudocode/alfa-beta.txt` | poda alfa-beta; ordenação; `primeiro`; `scoreParcial`; chamada da quiescência | fechado |
| `IA/Pseudocode/quiescencia.txt` | `quiescenciaMinimax`, `quiescenciaAlfaBeta` | **novo**, fechado |
| `IA/Pseudocode/ordenarLances.txt` | `notaMVVLVA`, `ordenarLances` | fechado |
| `IA/Pseudocode/avaliar.txt` | avaliação v1 | fechado |
| `IA/Pseudocode/escolherJogada.txt` | chamadora: aprofundamento, relógio, parcial, passada de segurança, parada por mate | fechado |
| `IA/Pseudocode/verificarChamada.txt` | verificação + `RESULTADO_*` | fechado |
| `IA/Pseudocode/converterScore.txt` | conversão para o UCI | fechado |
| `IA/header/ia.h` | constantes, `Play`, protótipos, globais públicas | fechado |
| `IA/header/avaliar.h` | `avaliar`, `AVALIACAO_MAX` | fechado |
| `IA/header/ordenar.h` | `LanceOrdenado`, `NOTA_PRIMEIRO`, `ordenarLances` | fechado |
| `IA/header/pendenciasRegras.h` | `testeEmpateRegra` + motivos (provisório) | provisório |
| `IA/header/stubs.h`, `IA/src/stubs.c` | stub controlável de empate (`varTesteEmpate`) | para testes |
| `IA/src/avaliar.c` | avaliação v1 | testado |
| `IA/src/ordenar.c` | MVV-LVA + inserção estável | testado |
| `IA/src/ia.c` | conversão, verificação, busca, quiescência, relógio, chamadora, sorteio, contagem | **executado e testado** |
| `IA/src/teste.c` | 8 blocos de teste (§7) | passando |
| `IA/docs/` | contextos das etapas 2–12 | versionado |

`IA/Pseudocode/teste.exe` é um binário solto na pasta de pseudocódigo; não está no git.
`IA/teste.exe` (saída do comando da §6.1) é ignorado pelo `*.exe` do `.gitignore`.

---

## 6. Implementação em C — notas

### 6.1 Build e ambiente

- Compilador: **GCC TDM 4.9.2** (Dev-Cpp MinGW64). `make` não está no PATH: usar
  **`mingw32-make`** (ex.: `mingw32-make -C engine test`; `PERFT_MAX_DEPTH=3` reduz o perft).
- **Comando que compila o teste da IA** (da raiz, PowerShell):

  ```powershell
  $eng = Get-ChildItem engine/src/*.c | Where-Object Name -ne 'main.c' | ForEach-Object FullName
  gcc -std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Og -g `
      -IIA/header -Iengine/include IA/src/teste.c IA/src/ia.c IA/src/avaliar.c IA/src/ordenar.c `
      IA/src/stubs.c $eng -o IA\teste.exe
  .\IA\teste.exe; $LASTEXITCODE
  ```

  Todo o `engine/src` menos `main.c`. A suíte leva **3–4 s**. O `.vscode/tasks.json` (ignorado
  pelo git) **ainda não** está assim — acertar em cada máquina, incluindo o `ordenar.c`.
- **Avisos aceitos:** só os que vêm de `engine/` (`fen.c:153`, `io.c`, `move.c`, `movegen.c`,
  `uci.c`), cerca de 17. **Qualquer aviso vindo de `IA/` é novo.** Em `d874b94`: nenhum.
- `teste.c` começa com `#define __USE_MINGW_ANSI_STDIO 1` (antes de qualquer include).
- **Fim de linha:** `core.autocrlf = true`. No repositório os arquivos estão em LF; na cópia
  de trabalho, em **CRLF**. Arquivos **novos** criados pelo Claude nascem em LF: converter e
  conferir com `file`. **Não usar `sed -i` do Git Bash** em arquivo CRLF: na etapa 12 ele
  converteu o `teste.c` inteiro para LF. Usar PowerShell (`ReadAllText`/`WriteAllText`, UTF-8
  sem BOM).
- **Ferramenta de edição do Claude** apaga espaços no fim de linhas (etapa 11 no `ia.c`; etapa
  12 numa linha de `printf` do `teste.c`, restaurada). Comparar `git diff --numstat` com
  `git diff -w --numstat`; diferença por mudança de indentação (código movido) é inofensiva.
- O Windows (Controle de Aplicativo) bloqueia executáveis gerados de forma intermitente,
  inclusive `IA\teste.exe`. Se bloquear, a execução fica comigo.

### 6.2 Armadilhas de C que já apareceram

- Lista local: declarar o **objeto** (`MoveList lista;`) e passar `&lista`.
- `extern` no `.h` é declaração; definição uma vez num `.c`.
- Macros com expressão sempre parentizadas.
- `printf` com `<inttypes.h>`: literais colados, sem vírgulas.
- Parâmetros `int8_t` truncam valores de `int`: manter o tipo do chamador.
- **GCC 4.9 + msvcrt:** `%zu`, `%llu` e `PRIu64` avisam sem `__USE_MINGW_ANSI_STDIO`.
- **`clock_t` no Windows é `long` de 32 bits** e `CLOCKS_PER_SEC` = 1000; no Linux,
  1.000.000. Conversões de ms e o `prazo` em `int64_t`.
- **`movelist_find` devolve −1** quando não acha: nunca indexar `moves[]` sem conferir.
- Saída do teste redirecionada para arquivo fica em buffer de bloco: se o processo for morto
  (timeout), as últimas linhas não aparecem. Medir custo com programa à parte e `fflush`.

---

## 7. Testes (`IA/src/teste.c`, nesta ordem na `main`)

Formato: cada bloco roda até o fim, acumula uma flag, imprime um resumo; a `main` retorna 1 se
qualquer flag estiver ligada. Detecta **se** algo falhou, não **qual** caso.

`PROF_TESTE` = 4 é a profundidade dos testes de busca sem quiescência (sem limite de tempo).

Funções auxiliares (`static`): `mesmosCandidatos` (mesma ordem), `contemLance`,
`mesmoConjunto` (qualquer ordem), **`executarV1V2`** (etapa 12: corpo do v1 = v2 para uma FEN,
chamado pelas duas tabelas).

### 7.1 `escolherJogada` / verificação — 15 casos (passando)

Confere `resultado`, os valores padrão, `NOS_VISITADOS == 0`, `CANDIDATOS.count == 0`,
`PROF_ALCANCADA == 0` e `!PARCIAL_USADA`, com sentinelas antes de cada chamada. Etapa 12:
coluna `quiescencia`; casos novos: quiescência inválida (−6), randomizador e quiescência
inválidos (−3), quiescência e profundidade inválidas (−6).

### 7.2 v1 = v2, determinismo e contagem de nós (passando)

**Sem quiescência — `casosV1V2 {fen, iteracoes}`, 9 FENs, `PROF_TESTE`:** inicial; após 1.e4;
mate em 1 e em 2 dos dois lados; dama livre dos dois lados; kiwipete.

**Com quiescência — `casosV1V2Quiescencia {fen, profundidade, iteracoes}`, 8 FENs:** inicial
(2); mates em 1 (prof. 2, para na 1); mates em 2 (prof. 4, para na 3); dama livre dos dois
lados (3); FEN do sorteio (3, rica em capturas). **Fora por custo** (medido > 60 s): após 1.e4
já na profundidade 2, kiwipete já na 1.

Para cada FEN (`executarV1V2`):
- **v1 = v2:** mesmo `score` e `tipo`, `SUCESSO` nas duas, **mesmo conjunto** de `CANDIDATOS`,
  lance do alfa-beta ∈ `CANDIDATOS` do minimax. **Não** exige o mesmo lance.
- **Determinismo:** alfa-beta duas vezes → mesma `JOGADA`, contagem e `CANDIDATOS` na mesma
  ordem.
- **`PROF_ALCANCADA == iteracoes`** e `!PARCIAL_USADA`.
- **Contagem:** minimax visita exatamente `Σ_{D=1..iteracoes} Σ_{d=0..D} perft(d)` na busca
  principal (`NOS_VISITADOS − NOS_QUIESCENCIA`); alfa-beta ≤ minimax por FEN; no total de cada
  tabela, alfa-beta **estritamente** menor. Desligada: `NOS_QUIESCENCIA == 0`; ligada: `> 0`.
- **Guarda:** `halfmove_clock + profundidade >= 100` → falha com a FEN impressa.

### 7.3 Limite de tempo — 5 casos (passando)

Kiwipete, `profundidade = PROF_MAX`: alfa-beta 1 ms e 100 ms, minimax 100 ms (sem
quiescência); alfa-beta e minimax 100 ms **com quiescência**. Confere: `SUCESSO`, lance legal,
tabuleiro intacto, **candidatos legais, lance ∈ candidatos**, `PROF_ALCANCADA ≥ 1` sem
quiescência (com ela, 0 é válido), `PROF_ALCANCADA == 0` ⇒ `!PARCIAL_USADA`, minimax nunca com
`PARCIAL_USADA`, gasto ≤ `tempoMs` + 50 ms. Imprime `PROF_ALCANCADA`, `PARCIAL_USADA` e
`NOS_QUIESCENCIA` como medição.

### 7.4 Quiescência em posições escolhidas — novo (passando)

Alfa-beta, profundidade 1:
- **Horizonte** (`casosHorizonte`, dos dois lados): dama captura peão defendido por peão
  (`4k3/8/4p3/3p4/8/8/8/3QK3 w`, `d1d5`; espelho preto, `d8d4`). Sem quiescência, a captura é o
  lance; com ela, nem é candidata.
- **Stand-pat:** posição inicial — `NOS_QUIESCENCIA == 20` (um por horizonte) e mesmo score,
  tipo e `CANDIDATOS` (mesma ordem) do desligado.
- **Mate na quiescência** (`casosMateQuiescencia`, dos dois lados): com quiescência, mate em 1
  com o lance certo; sem ela, centipeão.

### 7.5 Sorteio (passando)

FEN `7k/8/8/3r4/5N2/1BN5/8/3QK3 w - - 0 1`: 4 candidatos entre 35 lances legais. Alfa-beta,
`PROF_TESTE`, sem quiescência, sementes 1..8. Inalterado.

### 7.6 Ordenação (passando)

Inalterado (FEN `7k/8/8/q2pP1r1/1P6/1n3N2/2B5/3QK3 w - d6 0 1`, três casos de `primeiro`).

### 7.7 `avaliar` — 14 casos; 7.8 `converterScore` — 20 casos (passando)

Os casos da fronteira do limiar são relativos a `LIMIAR_MATE`: testam 19871 (mate, 64 lances)
e 19870 (centipeão). A regra que pula casos usa `PROF_LIMITE`.

### 7.9 Medições (14/11)

**Sem quiescência, `PROF_TESTE` = 4:** inalteradas desde a etapa 11 — total minimax 5.362.232,
alfa-beta 174.492 (3,3%).

**Com quiescência (tabela da §7.2):** total minimax 211.985, alfa-beta 17.944 (8,5%). Ex.: FEN
do sorteio, prof. 3: minimax 108.334 nós (90.361 de quiescência), alfa-beta 5.558 (3.021).

**Custo do minimax com quiescência** (`-O2`, programa à parte):

| Posição | Prof. 2 | Prof. 3 |
|---|---|---|
| Inicial | 1 ms | > 60 s |
| Após 1.e4 | > 60 s | — |
| Mate em 2 | 0 ms | 13 ms |
| Dama livre | 4 ms | 61 ms (prof. 4: 1,4 s) |
| Kiwipete | > 60 s já na prof. 1 | — |
| FEN do sorteio | 6 ms | 141 ms |
| FEN da ordenação | 4,2 s | > 60 s |

**Alfa-beta com quiescência, kiwipete** (`-O2`): prof. 1 = 62.486 nós, 187 ms; prof. 2 = 128.700,
381 ms; prof. 3 = 697.974, 2,0 s; prof. 4 = 1.472.308 (1.369.475 de quiescência), 4,1 s. Sem
quiescência, prof. 4 = 102.261 nós. **Contra o RNF01 (< 200 ms), isso é a principal pendência
técnica.**

**Limite de tempo (kiwipete, `PROF_MAX`, `-Og`, 3 execuções):**

| Variante | Quiescência | `tempoMs` | Gasto | Nós | Prof. alcançada | Parcial |
|---|---|---|---|---|---|---|
| Alfa-beta | não | 1 | 1–11 ms | 2.048–8.192 | 2–3 | sim |
| Alfa-beta | não | 100 | 100–105 ms | 129.024–132.096 | 5 | sim |
| Minimax | não | 100 | 100–104 ms | ~500.000 | 3 | não |
| Alfa-beta | sim | 100 | 100–101 ms | 24.576 | 1 | sim |
| Minimax | sim | 100 | 100–101 ms | ~37.000 | 0 (só a passada de segurança) | não |

**Não investigado (herdado):** no alfa-beta sem quiescência com 100 ms, os nós no prazo caíram
à metade com a ordenação. Agora dá para medir com `PROF_ALCANCADA`, voltando ao `0615267`.

---

## 8. Decisões a acertar com a máquina de regras

Pedido redigido no chat em 14/11 (itens 2, 4, 7 e 8, citando 1). **Ainda não enviado.**

1. **Congelamento dos cabeçalhos** (`board.h`, `piece.h`, `move.h`, `makemove.h`, `movegen.h`).
   A IA depende também de `move_is_capture`, `move_is_ep_capture`, `move_from`, `move_to` (e o
   teste, de `move_to_str` e `movelist_find`).
2. **`in_check`:** adotar o do `next_steps.md` §9.2 (`bool in_check(const Board *b)`, em
   `attack.h/c`). A IA apaga `emXeque` quando existir.
3. ~~Data do perft~~ — resolvido.
4. **Empates:** função pública com motivo (`SEM_EMPATE` = 0 é requisito da IA); 50 lances já
   detectável; material insuficiente; repetição precisa do histórico (planejado na camada de
   protocolo) — falta acordar **como a IA recebe esse histórico** e se a repetição dentro da
   árvore fica para o Zobrist.
5. ~~Lance nulo~~ — resolvido: `MOVE_NONE`.
6. **Zobrist**, se entrar: incremental em `make_move`/`unmake_move`.
7. **Bug #1 (`fen_parse`)**: a correção no parser protege também os testes da IA.
8. **Ligar a IA ao `go`** do `uci.c` (troca do `uci_pick_move`) e ao build do motor (`ia.c`,
   `avaliar.c`, `ordenar.c`; `stubs.c` não). `go depth N` → `profundidade`, `go movetime T` →
   `tempoMs`; `score cp` / `score mate` via `converterScore`. Falta acordar: padrão do `go` sem
   parâmetros, se a quiescência é ligada por padrão, e o mapa de níveis (RF03).

O dev das regras é **@caioveigap** (CODEOWNER de `engine/`).

---

## 9. Próximo passo

1. **Enviar o pedido ao dev das regras** (texto no chat de 14/11).
2. Escolher entre (recomendação: medir primeiro):
   - **Custo da quiescência contra o RNF01:** medir a profundidade alcançada em 200 ms em
     algumas posições; depois *delta pruning* (mais barato) ou SEE.
   - **PST (v6)**, último item da EAP não iniciado.

**Recomendações pendentes** (não são erros):
- **`fen_parse` sem conferir o retorno** no bloco do sorteio (`teste.c`), herdado.
- **FEN do sorteio duplicada** na `casosV1V2Quiescencia` (o `static const char*` não serve de
  inicializador; alternativa: `#define`).
- **Pseudocódigo:** `relógio()` em vez de `clock()`; `prazo = inicio + tempoMs` omite a
  conversão de ms para tiques (`tempoMs * CLOCKS_PER_SEC / 1000`); `profMax` em vez de
  `PROF_MAX` (nome diferente do C).
- **`.vscode/tasks.json`** com o comando da §6.1.
- Seleção preguiçosa na ordenação.
- Medir a profundidade alcançada antes da ordenação (§7.9, herdado).

---

## 10. Mudanças de escopo da IA em relação à EAP (item 1.5)

**Trabalho realizado que não aparece na EAP** (acumulado):
- Especificação em pseudocódigo de todas as funções (**8 arquivos**, especificação viva).
- Verificação da chamada (`verificarChamada`) e campo `resultado`.
- Conversão de score para o protocolo (`converterScore`).
- Contrato com a máquina de regras (`pendenciasRegras.h`) — ligado a 1.5.2.
- Testes automatizados da IA: verificação (15), v1 = v2 + determinismo + contagem de nós (9 +
  8 FENs), limite de tempo (5), quiescência em posições escolhidas (5), sorteio (8 sementes),
  ordenação (3 casos), avaliação (14), conversão (20); stubs controláveis. Relaciona-se com
  1.6.1.5.
- **Instrumentação por contagem de nós**, com o perft das regras como oráculo exato, e
  `NOS_QUIESCENCIA`, `PROF_ALCANCADA`, `PARCIAL_USADA`.
- Garantias em tempo de compilação (`_Static_assert`).
- Controle de semente do sorteio (`iniciarSorteio`).
- Robustez do relógio (falha do `clock()`, estouro de 32 bits), parada por mate, iteração
  parcial e passada de segurança.
- **Quiescência** (fora da EAP; levar ao grupo).

**Itens da EAP e seu estado:**

| Item | Estado |
|---|---|
| 1.5.1 Pesquisa | Concluído |
| 1.5.2 Integração com a Máquina de Regras | Em andamento: gerador legal, make/unmake e perft integrados; falta `in_check`, empates e ligação ao `go` (pedido redigido) |
| 1.5.3 Simulação de Estados | Concluído |
| 1.5.4.1 Avaliação Material | Concluído e testado |
| 1.5.4.2 Avaliação Posicional | Não iniciado (v6+) |
| 1.5.5.1 Minimax | Concluído, executado e testado |
| 1.5.5.2 Poda Alfa-Beta | Concluído, executado e testado (~3,3% dos nós do minimax com ordenação) |
| 1.5.5.3 Aprofundamento Progressivo | Concluído e testado, com iteração parcial; níveis de dificuldade não mapeados |
| 1.5.5.4 Ordenação de Movimentos | Concluído e testado: MVV-LVA −56% de nós no alfa-beta |
| 1.5.6 Seleção de Lance Aleatório | Concluído e testado |

**Quanto está pronto (14/11):** **8 de 10 itens da EAP** concluídos e testados (80% por
contagem). **Por esforço, ~75%** (contando a quiescência no escopo):

| Parte | Peso aproximado | Estado |
|---|---|---|
| Núcleo da busca (minimax, alfa-beta, aprofundamento, tempo, sorteio, parcial) + testes + especificação | ~40% | pronto |
| Avaliação material, conversão de score, verificação da chamada | ~10% | pronto |
| Integração (`go`, build, níveis, `in_check` e empates do motor) | ~20% | pouco feito (pedido redigido) |
| Ordenação MVV-LVA + revisão do v1 = v2 | ~10% | pronto |
| Quiescência | ~10% | **pronto**; custo contra o RNF01 em aberto |
| Avaliação posicional (PST) | ~10% | não iniciado |

Para apresentação: "8 de 10 itens da EAP concluídos e testados, mais a quiescência; com a
ordenação, o alfa-beta visita 3,3% dos nós do minimax; a quiescência elimina o efeito
horizonte, mas custa caro em posições táticas; falta a integração com o motor e a avaliação
posicional".

Quiescência e TT continuam fora da EAP — levar ao grupo.
