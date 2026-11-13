# Contexto para continuidade — IA de xadrez (etapa 8: busca executada e medida; pendências com as regras)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa7.md`.

Como na etapa 7, **o repositório é a fonte da verdade** (`Xadrez-ES2`, branch `IA`). Este
arquivo guarda o que o repositório não guarda: decisões, motivos, medições, pendências e o
estado do trabalho. A §5 lista onde está cada coisa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Estado em 13/11/2026:** a busca **roda** sobre o motor real
(`generate_legal_moves`, `make_move`/`unmake_move`), com `PROF_MAX` 4. Minimax e alfa-beta
devolvem lances legais, deixam o tabuleiro intacto, acham mates com a distância certa e
concordam entre si (teste v1 = v2). A contagem de nós prova que a poda poda (~7% dos nós do
minimax). **Próximo passo:** redigir o pedido ao dev das regras (`in_check`, 50 lances,
repetição, material insuficiente) — §9.

---

## 0. Dependências da máquina de regras

### 0.1 Já disponível e usado pela IA

| Item | Onde | Observação |
|---|---|---|
| `Board`, `Color`, `Piece`, `Move`, `MoveList`, `MOVE_NONE` | `board.h`, `piece.h`, `move.h` | Como na etapa 7. `MOVE_NONE` agora é usado pela IA (§3.9) |
| `generate_legal_moves(Board*, MoveList*)` | `movegen.h` | **Legal.** Substituiu o `gerarLancesLegais` do contrato |
| `make_move` / `unmake_move`, `Undo` | `makemove.h` | Validados pelo perft |
| `is_square_attacked(b, sq, side)` | `movegen.h` | `side` = **dono/defensor** da casa, não o atacante |
| `perft(Board*, int)` | `perft.h` | Usado pela IA como oráculo da contagem de nós (§7.4) |
| `fen_parse` | `fen.h` | Ver Bug #1 abaixo |

**Perft passando:** `make test` das regras roda 6 posições da CPW + `make_unmake`, 7/7 até
profundidade 5 (conferido em 04/11). O bloqueante "perft sem data" da etapa 7 está encerrado.

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

**Mudança da etapa 8:** **o Claude escreve o código; eu reviso e decido.** Substitui o "não me
dê a solução pronta" das etapas anteriores.

- Respostas **sucintas e tecnicamente precisas**. Sem preâmbulo, sem repetir o que eu disse.
- Eu **decido o escopo e a ordem**. Sugira, mas não assuma.
- Ao entregar código: compile, rode a suíte e mostre o que mudou para eu revisar. Quando o
  código já estiver commitado, mostre o diff por bloco lógico.
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo. Se minha conclusão estiver certa e o motivo não, diga.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Avalie **só o escopo que eu pedir**.
- Mantenho o **pseudocódigo como especificação viva**: toda mudança no C que altera a
  especificação atualiza o `.txt` correspondente.
- Separe **erro** de **recomendação**.
- Ao conferir arquivos, leia **do disco** (ver padrões abaixo).
- **Eu faço os commits.** Não use linhas de coautoria. Sugira a mensagem; eu commito.
- Verifique o que entrou em cada commit meu (`git show --stat`) — já saiu commit com mais do
  que o combinado.

**Padrões de erro meus para vigiar:**
- Ramos espelhados corrigidos pela metade, ou errados do mesmo jeito nos dois lados.
- Consumidores não atualizados depois que mudo assinatura, nome ou decisão.
- Fundir casos distintos num mesmo retorno.
- Estado local declarado como global, e o inverso.
- Informação duplicada em dois lugares.
- Precedência e parênteses em macros e expressões fundidas.
- Erros de digitação em nomes e mensagens.
- Ponteiro sem objeto por trás; `extern` (declaração) confundido com definição; vírgulas entre
  literais em `printf` com `PRI...`; **condição de teste que nunca pode ser verdadeira**.
- **Novos na etapa 8:**
  - **Editor salvando buffer antigo por cima do disco:** a declaração `extern` do
    `NOS_VISITADOS` sumiu do `ia.h` assim, e o commit `447b019` subiu sem compilar (corrigido
    em `9e347ad`). Antes de commitar, compilar a partir do disco.
  - **Commit com mais do que o combinado** (`447b019` levou três blocos e a pasta `IA/docs/`)
    ou **antes do combinado**, e **mensagem de commit repetida** (`5770bd3`, ver §2).

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
"mesma FEN + profundidade → mesmo lance" — **ambos implementados** (§7.3).

**Roadmap (Fase 7):** v1 material + minimax → v2 alfa-beta → v3 aprofundamento iterativo +
movetime → v4 MVV-LVA → v5 quiescência → v6+ PST, tapered, TT. TT fora do MVP.

**Gantt (IA):** alfa-beta, ordenação e aprofundamento 03/11–20/11. Apresentação **23/11**.
**Situação em 13/11:** v1 e v2 executadas e testadas; aprofundamento e ordenação não iniciados.

**Commits:** `docs(IA): ...` para pseudocódigo/documentação, `feat(IA): ...` para código,
`fix(IA): ...` para correção; título sozinho na primeira linha sem ponto final.

| Commit | Conteúdo real | Estado |
|---|---|---|
| `447b019` | `PROF_MAX` 4 + pulo na conversão; **também** v1 = v2, contagem de nós, pseudocódigo e `IA/docs/` | em `origin/IA` e `origin/main`; **não compila sozinho** |
| `9e347ad` | `extern NOS_VISITADOS` no `ia.h` | idem |
| `e8063fa` | Guarda de `halfmove` + sentinela de `NOS_VISITADOS` no teste | idem |
| `5770bd3` | `LANCE_NULO` → `MOVE_NONE` | **local, não enviado**; mensagem repetida do `e8063fa` — corrigir com `git commit --amend -m "refactor(IA): substitui LANCE_NULO por MOVE_NONE do motor"` antes do push |

A `main` (local e remota) foi avançada até `e8063fa`.

`IA/docs/` entra no repositório **de propósito**: explicita o desenvolvimento orientado a
especificação em `.md`.

---

## 3. Decisões tomadas

### 3.1 Busca (inalteradas desde a etapa 1)

- Convenção absoluta: brancas positivas, pretas negativas; max/min literais.
- Vez lida de `side_to_move`; sem parâmetro `jogador`.
- Ordem do caso base: empate por regra → limite de profundidade → gerar lances → lista vazia
  (xeque = mate, senão afogamento) → laço.
- `MoveList` e `Undo` **locais ao nó**, como objetos.
- `MATE = 19999`, mate vale `±(MATE − profundidade)` com sinal pelo **lado derrotado**;
  `INFINITO = 20000`; empate = 0; `SCORE_INVALIDO = 30000`.
- Score em `int32_t`.
- Alfa-beta: na raiz alfa e beta **não são atualizados** (`profundidade > 0`).
- O mate mais profundo que a busca produz é `MATE − (PROF_MAX − 1)`: o nó em
  `profundidade == PROF_MAX` devolve `avaliar` antes de gerar lances.

### 3.2 Avaliação v1 (fechada e testada)

Só material, Michniewski (P 100, C 320, B 330, T 500, D 900, R 0). `AVALIACAO_MAX` = 10615
(margem para PST). Teste do rei adiado para a v6+.

### 3.3 Constantes públicas e `PROF_MAX`

`PROF_MAX`, `MATE`, `INFINITO`, `SCORE_INVALIDO`, `LIMIAR_MATE = (MATE − PROF_MAX)` no `ia.h`.

- **`PROF_MAX` = 4** (era 9000). Escolhido por medição: com 5, o minimax no kiwipete levaria
  30 s a 1+ min (perft 5 do kiwipete: 28 s). Com 4, o pior caso medido é 1,4 s (minimax) e
  157 ms (alfa-beta) — §7.5.
- `_Static_assert(LIMIAR_MATE > AVALIACAO_MAX)` continua no `ia.c`.

### 3.4 Chamadora e resultado da chamada

Inalterado da etapa 7: `Play.resultado` (0 sucesso, negativos bug, positivos fim de partida
na raiz); `verificarChamada` interna; valores padrão quando `resultado != SUCESSO`.

Ajuste da etapa 7 já no código (`0690d96`): enquanto os motivos de empate não existem, o stub
devolve `EMPATE_PROVISORIO (4)` e a verificação traduz para `RESULTADO_EMPATE_PROVISORIO (6)`;
motivo desconhecido → `RESULTADO_ERRO_INTERNO`.

### 3.5 Conversão de score

Inalterada. Pública, preenche só `score` e `tipo`.

### 3.6 Sorteio

Inalterado. `iniciarSorteio` uma vez; `rand() % quantidade`.

### 3.7 Organização do código

- `static`: `verificarChamada`, `emXeque`, `minimax`, `podaAlfaBeta`, `sorteioLance`,
  `listaCandidatos`. Públicas: `converterScore`, `escolherJogada`, `iniciarSorteio`, `JOGADA`,
  **`NOS_VISITADOS`**.
- Globais públicas: **declaradas** `extern` no `ia.h`, **definidas** uma vez no `ia.c`.
- `pendenciasRegras.h` (substituiu o `contrato.h`) é incluído pelo `ia.c` e pelos stubs, não
  pelo `ia.h`. Só declara `testeEmpateRegra` e os motivos de empate.

### 3.8 Descartado

Ponder e threads.

### 3.9 Novas na etapa 8

- **Contagem de nós:** `uint64_t NOS_VISITADOS`, público. Cada entrada em `minimax` ou
  `podaAlfaBeta` conta um nó, inclusive a raiz. Zerado no início de `escolherJogada`; fica em
  0 se a verificação falha. Fica ativo também fora dos testes (custo desprezível).
- **`MOVE_NONE` no lugar de `LANCE_NULO`:** mesmo sentinela, mesmo valor (`(Move)0`, a1→a1).
  Motivos: informação duplicada, e "lance nulo" (*null move*) é outra coisa em xadrez
  computacional (passar a vez; *null-move pruning*).
- **`emXeque` fica como está** até o motor publicar `in_check`; então é apagada e substituída
  (commit próprio, com pseudocódigo). **Não copiar** o `if (SQ_OFFBOARD(ksq)) return false`
  do `uci_in_check`: só esconderia o estado quebrado do Bug #1.

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo das funções~~ ✅
2. ~~Pseudocódigo: `verificarChamada` e `converterScore`~~ ✅
3. ~~C: `avaliar` + testes~~ ✅
4. ~~C: `converterScore` + testes~~ ✅
5. ~~C: verificação, busca, chamadora, sorteio~~ ✅
6. ~~Stubs + testes da `verificarChamada`~~ ✅ (`0690d96`)
7. Acertar com as regras as decisões da §8 — **em andamento** (perft resolvido; `MOVE_NONE`
   resolvido; resto na §9)
8. ~~Primeira execução da busca~~ ✅: perft conferido, `PROF_MAX` 4, execução em posições
   reais, teste v1 = v2, contagem de nós
9. Aprofundamento iterativo + `go movetime` / níveis de dificuldade (mora na chamadora;
   variável de profundidade com outro nome; atualizar caso base de `minimax` **e**
   `podaAlfaBeta`)
10. Ordenação MVV-LVA (estrutura própria lance + nota, local ao nó). **Quebra a premissa do
    teste v1 = v2** (§7.3)
11. Quiescência
12. TT (fora do MVP)

Ligar a IA ao `go` do `uci.c` e ao build do motor ainda não tem lugar na lista — decidir com
o grupo.

---

## 5. Onde está cada coisa (repositório, branch `IA`)

| Arquivo | Conteúdo | Estado |
|---|---|---|
| `IA/Pseudocode/minimax.txt` | minimax + definição de `nosVisitados` | fechado |
| `IA/Pseudocode/alfa-beta.txt` | poda alfa-beta | fechado |
| `IA/Pseudocode/avaliar.txt` | avaliação v1 | fechado |
| `IA/Pseudocode/escolherJogada.txt` | chamadora; zera `nosVisitados`; `MOVE_NONE` | fechado |
| `IA/Pseudocode/verificarChamada.txt` | verificação + `RESULTADO_*` | fechado |
| `IA/Pseudocode/converterScore.txt` | conversão para o UCI | fechado |
| `IA/header/ia.h` | constantes, `Play`, protótipos, `JOGADA`, `NOS_VISITADOS` | fechado |
| `IA/header/avaliar.h` | `avaliar`, `AVALIACAO_MAX` | fechado |
| `IA/header/pendenciasRegras.h` | `testeEmpateRegra` + motivos (provisório) | provisório |
| `IA/header/stubs.h`, `IA/src/stubs.c` | stub controlável de empate (`varTesteEmpate`) | para testes |
| `IA/src/avaliar.c` | avaliação v1 | testado |
| `IA/src/ia.c` | conversão, verificação, busca, chamadora, sorteio, contagem | **executado e testado** |
| `IA/src/teste.c` | 4 blocos de teste (§7) | passando |
| `IA/docs/` | contextos das etapas 2–8 | versionado |

`IA/Pseudocode/teste.exe` é um binário solto na pasta de pseudocódigo; não está no git.

---

## 6. Implementação em C — notas

### 6.1 Build e ambiente

- Compilador: **GCC TDM 4.9.2** (Dev-Cpp MinGW64). `make` não está no PATH: usar
  **`mingw32-make`** (ex.: `mingw32-make -C engine test`; `PERFT_MAX_DEPTH=3` reduz o perft).
- **Comando que compila o teste da IA** (da raiz, PowerShell):

  ```powershell
  $eng = Get-ChildItem engine/src/*.c | Where-Object Name -ne 'main.c' | ForEach-Object FullName
  gcc -std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Wmissing-prototypes -Og -g `
      -IIA/header -Iengine/include IA/src/teste.c IA/src/ia.c IA/src/avaliar.c IA/src/stubs.c `
      $eng -o IA\teste.exe
  .\IA\teste.exe; $LASTEXITCODE
  ```

  Todo o `engine/src` menos `main.c` (o teste usa `perft`, `generate_legal_moves` etc.). O
  `.vscode/tasks.json` (ignorado pelo git) **ainda não** está assim — acertar em cada máquina.
- **Avisos aceitos:** só os que vêm de `engine/` (`fen.c:153`, `io.c`, `move.c`, `movegen.c`,
  `uci.c`), cerca de 15, código das regras. **Qualquer aviso vindo de `IA/` é novo.**
- `teste.c` começa com `#define __USE_MINGW_ANSI_STDIO 1` (antes de qualquer include) para
  `PRIu64` não gerar aviso com `-Wpedantic` no GCC 4.9. Sem efeito fora do MinGW.
- O Windows (Controle de Aplicativo) às vezes bloqueia executáveis gerados no scratchpad do
  Claude; a execução fica comigo nesses casos.

### 6.2 Armadilhas de C que já apareceram

- Lista local: declarar o **objeto** (`MoveList lista;`) e passar `&lista`.
- `extern` no `.h` é declaração; definição uma vez num `.c`.
- Macros com expressão sempre parentizadas.
- `printf` com `<inttypes.h>`: literais colados, sem vírgulas.
- Parâmetros `int8_t` truncam valores de `int`: manter o tipo do chamador.
- **GCC 4.9 + msvcrt:** `%zu`, `%llu` e `PRIu64` avisam (ou falham) sem
  `__USE_MINGW_ANSI_STDIO`.

---

## 7. Testes (`IA/src/teste.c`, nesta ordem na `main`)

Formato: cada bloco roda até o fim, acumula uma flag, imprime um resumo; a `main` retorna 1 se
qualquer flag estiver ligada. Detecta **se** algo falhou, não **qual** caso.

### 7.1 `escolherJogada` / verificação — 7 casos (passando)

Variante inválida, randomizador inválido, os dois (→ `VARIANTE_INVALIDA`), empate provisório,
motivo desconhecido 99 (→ `ERRO_INTERNO`), mate na raiz, afogamento na raiz. Confere
`resultado` e os valores padrão (`MOVE_NONE`, `SCORE_INVALIDO`, `TIPO_INVALIDO`) e
**`NOS_VISITADOS == 0`**, com sentinela `NOS_VISITADOS = 1` antes de cada chamada (sem ela, o
teste passaria sem testar: a global já começa em 0).

### 7.2 Execução, v1 = v2 e contagem de nós — 9 FENs (passando)

FENs: inicial; após 1.e4 (pretas na vez); mate em 1 e em 2 dos dois lados; dama livre dos
dois lados; kiwipete. Para cada uma:
- **v1 = v2:** minimax e alfa-beta com `SORTEIO_DESLIGADO` → mesmo `lance`, `score` e `tipo`,
  e `resultado == SUCESSO` nas duas (sem isso, duas falhas iguais passariam).
- **Determinismo:** alfa-beta rodado duas vezes → mesma `JOGADA` e mesma contagem. O minimax
  fica coberto indiretamente: precisa igualar o alfa-beta e bater a contagem exata.
- **Contagem:** minimax visita **exatamente** `Σ perft(d)`, `d = 0..PROF_MAX`; alfa-beta ≤
  minimax em cada FEN; no total, alfa-beta **estritamente** menor (pega poda desligada).
- **Guarda:** `halfmove_clock + PROF_MAX >= 100` → falha com a FEN impressa. O perft ignora os
  50 lances e o minimax corta no empate; com a regra real, a soma deixaria de valer.

**Premissas registradas no código:** vale para o `PROF_MAX` compilado; depende de as duas
variantes percorrerem a raiz **na mesma ordem** (com MVV-LVA, o lance pode diferir entre
candidatos empatados — aí comparar o score ou o conjunto de candidatos); o esperado exato do
minimax supõe que nenhum empate por regra dispara dentro da árvore (hoje garantido pelo stub;
com regras reais, as FENs atuais continuam seguras: `halfmove` 0, repetição tripla exige ≥ 8
meios-lances, nenhum ramo de 4 meios-lances chega a material insuficiente).

### 7.3 `avaliar` — 14 casos (passando)

Inalterado da etapa 7.

### 7.4 `converterScore` — 20 casos, 1 pulado com `PROF_MAX` 4 (passando)

Casos de mate com `p = MATE − |scoreBruto| > PROF_MAX` são **pulados e contados**; o resumo
imprime "Conversao: N caso(s) de mate pulado(s)". Com `PROF_MAX` 4, só o mate em p = 5. Os
casos de fronteira (`±LIMIAR_MATE`, p = `PROF_MAX`) rodam com qualquer valor. Critério é
`p ≤ PROF_MAX` (classificação da conversão), **não** `p ≤ PROF_MAX − 1` (alcance da busca).

### 7.5 Medições (`PROF_MAX` 4, 04/11)

| Posição | Lance | Score | Minimax (nós / ms) | Alfa-beta (nós / ms) |
|---|---|---|---|---|
| Inicial | a2a3 | cp 0 | 206.604 / 58 | 18.373 / 7 |
| Após 1.e4 | — | cp −100 | 419.166 / — | 45.474 / — |
| Mate em 1 (B / P) | a1a8 / a8a1 | mate 1 | 19.885 / 5–7 | ~1.800 / 1–2 |
| Mate em 2 (B / P) | a1a7 / a8a2 | mate 2 | 17.305 / 5–7 | ~2.100 / 1 |
| Dama livre (B / P) | d1d5 / d8d4 | cp 900 | 182.513 / 55–61 | ~14.000 / 5–12 |
| Kiwipete | e2a6 | cp 10 | 4.185.553 / 1427 | 270.306 / 157 |
| **Total** | | | **5.250.729** | **369.742 (7,0%)** |

`a2a3` na inicial é o primeiro candidato empatado (só material, sorteio desligado).
**RNF01:** o alfa-beta no kiwipete (157 ms) já está perto dos 200 ms com `PROF_MAX` 4 — o
aprofundamento iterativo com `movetime` (item 9) é o que garante o limite.

---

## 8. Decisões a acertar com a máquina de regras (atualizado)

1. **Congelamento dos cabeçalhos** (`board.h`, `piece.h`, `move.h`, `makemove.h`, `movegen.h`).
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
8. **Ligar a IA ao `go`** do `uci.c` e ao build do motor (hoje `IA/` não tem Makefile).

O dev das regras é **@caioveigap** (CODEOWNER de `engine/`).

---

## 9. Próximo passo

1. Corrigir a mensagem do `5770bd3` (§2) e enviar.
2. **Redigir o pedido ao dev das regras** cobrindo §8 itens 2, 4 e 7 (e citar 8): nomes,
   assinaturas e onde cada coisa mora, alinhados ao que o próprio `next_steps.md` já planeja.
3. Depois: item 9 da §4 (aprofundamento iterativo + `movetime`), com a apresentação de 23/11
   em vista.

---

## 10. Mudanças de escopo da IA em relação à EAP (item 1.5)

**Trabalho realizado que não aparece na EAP** (acumulado):
- Especificação em pseudocódigo de todas as funções (6 arquivos, especificação viva).
- Verificação da chamada (`verificarChamada`) e campo `resultado`.
- Conversão de score para o protocolo (`converterScore`).
- Contrato com a máquina de regras (`pendenciasRegras.h`, ex-`contrato.h`) — ligado a 1.5.2.
- Testes automatizados da IA: verificação (7), v1 = v2 + determinismo + contagem de nós (9
  FENs), avaliação (14), conversão (20); stubs controláveis. Relaciona-se com 1.6.1.5.
- **Instrumentação por contagem de nós**, com o perft das regras como oráculo exato.
- Garantias em tempo de compilação (`_Static_assert`).
- Controle de semente do sorteio (`iniciarSorteio`).

**Itens da EAP e seu estado:**

| Item | Estado |
|---|---|
| 1.5.1 Pesquisa | Concluído |
| 1.5.2 Integração com a Máquina de Regras | Em andamento: gerador legal, make/unmake e perft integrados; falta `in_check`, empates e ligação ao `go` |
| 1.5.3 Simulação de Estados | Concluído: fazer/desfazer com `Undo` local, tabuleiro intacto conferido |
| 1.5.4.1 Avaliação Material | Concluído e testado |
| 1.5.4.2 Avaliação Posicional | Não iniciado (v6+) |
| 1.5.5.1 Minimax | **Concluído, executado e testado** |
| 1.5.5.2 Poda Alfa-Beta | **Concluído, executado e testado** (v1 = v2; ~7% dos nós) |
| 1.5.5.3 Aprofundamento Progressivo | Não iniciado (próximo) |
| 1.5.5.4 Ordenação de Movimentos | Não iniciado (MVV-LVA) |
| 1.5.6 Seleção de Lance Aleatório | Concluído (sorteio não exercitado nos testes: todos usam `SORTEIO_DESLIGADO`) |

**Tamanho (13/11, `main`, sem comentários nem linhas em branco):** `IA/` 501 linhas de C;
sistema inteiro 4.300 (motor 2.966, agente 479, interface 354).

Quiescência e TT continuam fora da EAP — levar ao grupo.
