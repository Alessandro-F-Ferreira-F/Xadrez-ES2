# Contexto para continuidade — IA de xadrez (etapa 9: aprofundamento iterativo e limite de tempo)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa8.md`.

Como nas etapas anteriores, **o repositório é a fonte da verdade** (`Xadrez-ES2`, branch `IA`).
Este arquivo guarda o que o repositório não guarda: decisões, motivos, medições, pendências e
o estado do trabalho. A §5 lista onde está cada coisa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Estado em 14/11/2026:** a busca roda sobre o motor real com
**aprofundamento iterativo** e **limite de tempo** (`c95a223`). `escolherJogada` recebe
`profundidade` (`go depth`, até `PROF_MAX` = 64) e `tempoMs` (`go movetime`); a iteração
interrompida pelo tempo é descartada e a iteração 1 sempre termina. Minimax e alfa-beta
continuam concordando (v1 = v2), a contagem de nós continua exata, e o novo teste de tempo
passa. **Próximo passo:** decidir entre o pedido ao dev das regras (pendente desde a etapa 8)
e a ordenação MVV-LVA (item 10) — §9.

---

## 0. Dependências da máquina de regras

### 0.1 Já disponível e usado pela IA

| Item | Onde | Observação |
|---|---|---|
| `Board`, `Color`, `Piece`, `Move`, `MoveList`, `MOVE_NONE` | `board.h`, `piece.h`, `move.h` | `MoveList` tem `MAX_MOVES` = 256 lances |
| `generate_legal_moves(Board*, MoveList*)` | `movegen.h` | **Legal** |
| `make_move` / `unmake_move`, `Undo` | `makemove.h` | Validados pelo perft |
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
  (foi assim no item 9, §3.10).
- Ao entregar código: compile **a partir do disco**, rode a suíte e mostre o que mudou para eu
  revisar. Quando o código já estiver commitado, mostre o diff por bloco lógico.
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo. Se minha conclusão estiver certa e o motivo não, diga.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Avalie **só o escopo que eu pedir**.
- Mantenho o **pseudocódigo como especificação viva**: toda mudança no C que altera a
  especificação atualiza o `.txt` correspondente, com os **mesmos nomes** do C.
- Separe **erro** de **recomendação**.
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
- **Editor salvando buffer antigo por cima do disco** (etapa 8: a declaração `extern` do
  `NOS_VISITADOS` sumiu do `ia.h` assim). Antes de commitar, compilar a partir do disco.
- **Commit com mais do que o combinado**, **antes do combinado**, ou **mensagem repetida**.

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
"mesma FEN + profundidade → mesmo lance" — **ambos implementados** (§7.2).

**Roadmap (Fase 7):** v1 material + minimax → v2 alfa-beta → v3 aprofundamento iterativo +
movetime → v4 MVV-LVA → v5 quiescência → v6+ PST, tapered, TT. TT fora do MVP.

**Gantt (IA):** alfa-beta, ordenação e aprofundamento 03/11–20/11. Apresentação **23/11**.
**Situação em 14/11:** v1, v2 e v3 executadas e testadas; ordenação não iniciada.

**Commits:** `docs(IA): ...` para pseudocódigo/documentação, `feat(IA): ...` para código,
`fix(IA): ...` para correção, `refactor(IA): ...` para mudança sem efeito no comportamento;
título sozinho na primeira linha sem ponto final.

| Commit | Conteúdo | Estado |
|---|---|---|
| `447b019` | `PROF_MAX` 4 + pulo na conversão; **também** v1 = v2, contagem de nós, pseudocódigo e `IA/docs/` | em `origin/IA` e `origin/main`; **não compila sozinho** |
| `9e347ad` | `extern NOS_VISITADOS` no `ia.h` | idem |
| `e8063fa` | Guarda de `halfmove` + sentinela de `NOS_VISITADOS` no teste | idem; é o topo de `origin/main` |
| `179c7f3` | `LANCE_NULO` → `MOVE_NONE` (ex-`5770bd3`, mensagem corrigida) + `IA/docs/contexto-ia-xadrez-etapa8.md` | em `origin/IA` |
| `c95a223` | Aprofundamento iterativo + limite de tempo: 7 arquivos (`ia.h`, `ia.c`, `teste.c`, 4 `.txt`) | em `origin/IA`; conferido com `git show --stat` |

`origin/main` está em `e8063fa`: `179c7f3` e `c95a223` ainda não foram levados para a `main`.

`IA/docs/` entra no repositório **de propósito**: explicita o desenvolvimento orientado a
especificação em `.md`.

---

## 3. Decisões tomadas

### 3.1 Busca (inalteradas desde a etapa 1, salvo o limite)

- Convenção absoluta: brancas positivas, pretas negativas; max/min literais.
- Vez lida de `side_to_move`; sem parâmetro `jogador`.
- Ordem do caso base: **tempo esgotado** → empate por regra → limite da iteração → gerar
  lances → lista vazia (xeque = mate, senão afogamento) → laço.
- `MoveList` e `Undo` **locais ao nó**, como objetos.
- `MATE = 19999`, mate vale `±(MATE − profundidade)` com sinal pelo **lado derrotado**;
  `INFINITO = 20000`; empate = 0; `SCORE_INVALIDO = 30000`.
- Score em `int32_t`.
- Alfa-beta: na raiz alfa e beta **não são atualizados** (`profundidade > 0`). Cada iteração
  chama a raiz com a janela completa (`-INFINITO`, `+INFINITO`); sem *aspiration windows*.
- O mate mais profundo que uma iteração produz é `MATE − (limiteIteracao − 1)`: o nó em
  `profundidade == limiteIteracao` devolve `avaliar` antes de gerar lances.

### 3.2 Avaliação v1 (fechada e testada)

Só material, Michniewski (P 100, C 320, B 330, T 500, D 900, R 0). `AVALIACAO_MAX` = 10615
(margem para PST). Teste do rei adiado para a v6+.

### 3.3 Constantes públicas e `PROF_MAX`

`PROF_MAX`, `TEMPO_SEM_LIMITE`, `MATE`, `INFINITO`, `SCORE_INVALIDO`,
`LIMIAR_MATE = (MATE − PROF_MAX)` no `ia.h`.

- **`PROF_MAX` = 64** (era 4). Mudou de papel: é o **teto** do parâmetro `profundidade`
  (`go depth`) e define `LIMIAR_MATE` (= 19935); **não** é a profundidade da busca. Quem limita
  a busca é `limiteIteracao` (§3.10). O 4 da etapa 8 virou `PROF_TESTE` no `teste.c`.
- Pilha com 64 níveis: ~64 × (`MoveList` ≈ 516 B + `Undo`) ≈ 35 KB. Sem problema.
- `_Static_assert(LIMIAR_MATE > AVALIACAO_MAX)` continua no `ia.c` (19935 > 10615).

### 3.4 Chamadora e resultado da chamada

`Play.resultado`: 0 sucesso, negativos bug, positivos fim de partida na raiz;
`verificarChamada` interna; valores padrão (`MOVE_NONE`, `SCORE_INVALIDO`, `TIPO_INVALIDO`)
quando `resultado != SUCESSO`.

| Valor | Nome |
|---|---|
| −5 | `RESULTADO_TEMPO_INVALIDO` (`tempoMs < 0`) — **novo** |
| −4 | `RESULTADO_PROFUNDIDADE_INVALIDA` (fora de `[1, PROF_MAX]`) — **novo** |
| −3 | `RESULTADO_RANDOMIZADOR_INVALIDO` |
| −2 | `RESULTADO_VARIANTE_INVALIDA` |
| −1 | `RESULTADO_ERRO_INTERNO` |
| 0 | `RESULTADO_SUCESSO` |
| 1, 2 | `RESULTADO_RAIZ_MATE`, `RESULTADO_RAIZ_AFOGAMENTO` |
| 3, 4, 5 | `RESULTADO_EMPATE_50_LANCES`, `_REPETICAO`, `_MATERIAL` (provisórios) |
| 6 | `RESULTADO_EMPATE_PROVISORIO` (stub devolve `EMPATE_PROVISORIO` = 4) |

**Ordem da verificação:** variante → randomizador → profundidade → tempo → raiz (empate por
regra, depois lista vazia). Só o primeiro problema é reportado.

### 3.5 Conversão de score

Inalterada. Pública, preenche só `score` e `tipo`. Com `PROF_MAX` 64, classifica como mate
todo `|score| ≥ 19935`.

### 3.6 Sorteio

Inalterado. `iniciarSorteio` uma vez; `rand() % quantidade`. Agora sorteia sobre
`candidatosCompletos` (§3.10).

### 3.7 Organização do código

- `static`: `verificarChamada`, `emXeque`, `estourouTempo`, `minimax`, `podaAlfaBeta`,
  `sorteioLance`, `listaCandidatos`, `limiteIteracao`, `relogioAtivo`, `prazo`,
  `tempoEsgotado`. Públicas: `converterScore`, `escolherJogada`, `iniciarSorteio`, `JOGADA`,
  `NOS_VISITADOS`.
- Globais públicas: **declaradas** `extern` no `ia.h`, **definidas** uma vez no `ia.c`.
- `pendenciasRegras.h` é incluído pelo `ia.c` e pelos stubs, não pelo `ia.h`. Só declara
  `testeEmpateRegra` e os motivos de empate.

### 3.8 Descartado

Ponder e threads.

### 3.9 Da etapa 8

- **Contagem de nós:** `uint64_t NOS_VISITADOS`, público. Cada entrada em `minimax` ou
  `podaAlfaBeta` conta um nó, inclusive a raiz. Zerado no início de `escolherJogada`; fica em
  0 se a verificação falha. **Desde a etapa 9, soma todas as iterações, também a
  interrompida.**
- **`MOVE_NONE` no lugar de `LANCE_NULO`:** mesmo sentinela (`(Move)0`, a1→a1). "Lance nulo"
  (*null move*) é outra coisa em xadrez computacional.
- **`emXeque` fica como está** até o motor publicar `in_check`; então é apagada e substituída
  (commit próprio, com pseudocódigo). **Não copiar** o `if (SQ_OFFBOARD(ksq)) return false`
  do `uci_in_check`: só esconderia o estado quebrado do Bug #1.

### 3.10 Novas na etapa 9 (aprofundamento iterativo, `c95a223`)

Escolhidas por mim entre opções apresentadas, todas na recomendação:

- **Interface:** dois parâmetros novos,
  `escolherJogada(board, variante, randomizador, profundidade, tempoMs)`.
  `tempoMs = TEMPO_SEM_LIMITE (0)` desliga o relógio. Struct de limites descartada (mais um
  tipo público e um ponteiro que pode vir `NULL`).
- **Iteração interrompida pelo tempo é descartada:** lance e score vêm da última iteração
  **completa** (`candidatosCompletos`, cópia local de `listaCandidatos`, e `scoreBruto`). Sem
  ordenação, uma iteração parcial só viu parte da raiz. Aproveitar a parcial só faz sentido
  depois do item 10.
- **Iteração 1 sempre termina:** roda com `relogioAtivo` falso. Garante `bestmove` legal para
  qualquer `movetime`; custa ~20–50 nós.
- **`PROF_MAX` vira teto, 64** (§3.3).

Decisões de implementação (minhas por padrão, sem objeção):

- **Relógio:** `clock()` do C11, porque o GCC 4.9/msvcrt não deve ter `timespec_get`. No
  Windows `clock()` mede tempo de parede; no POSIX, tempo de CPU — numa busca single-thread
  dá quase o mesmo. `prazo = inicio + tempoMs × CLOCKS_PER_SEC / 1000` (conta em `int64_t`).
- **O prazo conta do início de `escolherJogada`**, incluindo a verificação.
- **Consulta ao relógio:** `estourouTempo()` na entrada do nó, só com `relogioAtivo` e a cada
  `INTERVALO_RELOGIO` = 1024 nós (`NOS_VISITADOS % 1024 == 0`). Também antes de começar cada
  iteração ≥ 2 (aí sem o intervalo).
- **Saída por tempo:** o nó devolve 0 na entrada; no laço, `if(tempoEsgotado) return 0;` logo
  **depois** do `unmake_move` e **antes** de comparar com `melhor`. Tabuleiro volta intacto e
  nenhum valor descartado entra nos candidatos. Está nos **quatro** laços (dois ramos × duas
  variantes).
- **Estado da iteração como `static` do arquivo** (`limiteIteracao`, `relogioAtivo`, `prazo`,
  `tempoEsgotado`): quem define é a chamadora e quem lê é a busca, como `listaCandidatos`.
  Alternativa (passar o limite por parâmetro na recursão) não adotada.
- O laço `for` usa a própria `limiteIteracao` como variável.
- **Sem ordenação, o aprofundamento iterativo não economiza nós** — gasta um pouco mais
  (§7.6). O ganho agora é só respeitar o tempo; a economia vem com o item 10 (melhor lance da
  iteração anterior primeiro).

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo das funções~~ ✅
2. ~~Pseudocódigo: `verificarChamada` e `converterScore`~~ ✅
3. ~~C: `avaliar` + testes~~ ✅
4. ~~C: `converterScore` + testes~~ ✅
5. ~~C: verificação, busca, chamadora, sorteio~~ ✅
6. ~~Stubs + testes da `verificarChamada`~~ ✅ (`0690d96`)
7. Acertar com as regras as decisões da §8 — **em andamento** (perft e `MOVE_NONE`
   resolvidos; pedido ao dev ainda não redigido)
8. ~~Primeira execução da busca~~ ✅
9. ~~Aprofundamento iterativo + `go movetime`~~ ✅ (`c95a223`). **Níveis de dificuldade (RF03)
   não mapeados:** a IA aceita `profundidade` e `tempoMs`; a tabela nível → (profundidade,
   tempo) fica para quando a IA for ligada ao `go`
10. Ordenação MVV-LVA (estrutura própria lance + nota, local ao nó). **Quebra a premissa do
    teste v1 = v2** (§7.2). Com o aprofundamento, incluir o melhor lance da iteração anterior
    primeiro na raiz
11. Quiescência
12. TT (fora do MVP)

Ligar a IA ao `go` do `uci.c` e ao build do motor ainda não tem lugar na lista — decidir com
o grupo.

---

## 5. Onde está cada coisa (repositório, branch `IA`)

| Arquivo | Conteúdo | Estado |
|---|---|---|
| `IA/Pseudocode/minimax.txt` | minimax; `profMax` (teto), `limiteIteracao`, `tempoEsgotado`, `estourouTempo`, `nosVisitados` | fechado |
| `IA/Pseudocode/alfa-beta.txt` | poda alfa-beta, com as mesmas saídas por tempo | fechado |
| `IA/Pseudocode/avaliar.txt` | avaliação v1 | fechado |
| `IA/Pseudocode/escolherJogada.txt` | chamadora com aprofundamento iterativo e relógio | fechado |
| `IA/Pseudocode/verificarChamada.txt` | verificação + `RESULTADO_*` (−4, −5 novos) | fechado |
| `IA/Pseudocode/converterScore.txt` | conversão para o UCI | fechado |
| `IA/header/ia.h` | constantes, `Play`, protótipos, `JOGADA`, `NOS_VISITADOS` | fechado |
| `IA/header/avaliar.h` | `avaliar`, `AVALIACAO_MAX` | fechado |
| `IA/header/pendenciasRegras.h` | `testeEmpateRegra` + motivos (provisório) | provisório |
| `IA/header/stubs.h`, `IA/src/stubs.c` | stub controlável de empate (`varTesteEmpate`) | para testes |
| `IA/src/avaliar.c` | avaliação v1 | testado |
| `IA/src/ia.c` | conversão, verificação, busca, relógio, chamadora, sorteio, contagem | **executado e testado** |
| `IA/src/teste.c` | 5 blocos de teste (§7) | passando |
| `IA/docs/` | contextos das etapas 2–9 | versionado |

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
      -IIA/header -Iengine/include IA/src/teste.c IA/src/ia.c IA/src/avaliar.c IA/src/stubs.c `
      $eng -o IA\teste.exe
  .\IA\teste.exe; $LASTEXITCODE
  ```

  Todo o `engine/src` menos `main.c`. A suíte leva ~3 s. O `.vscode/tasks.json` (ignorado pelo
  git) **ainda não** está assim — acertar em cada máquina.
- **Avisos aceitos:** só os que vêm de `engine/` (`fen.c:153`, `io.c`, `move.c`, `movegen.c`,
  `uci.c`), cerca de 15, código das regras. **Qualquer aviso vindo de `IA/` é novo.** Em
  `c95a223`: nenhum.
- `teste.c` começa com `#define __USE_MINGW_ANSI_STDIO 1` (antes de qualquer include) para
  `PRIu64` não gerar aviso com `-Wpedantic` no GCC 4.9. Sem efeito fora do MinGW.
- **Fim de linha:** `core.autocrlf = true`. No repositório os arquivos estão em LF; na cópia
  de trabalho, em **CRLF**. Ferramenta que reescreve arquivo inteiro (script Python, `sed`
  sem cuidado) pode trocar para LF — o git normaliza no commit, mas a cópia de trabalho fica
  misturada. Conferir com `file` depois de editar.
- O Windows (Controle de Aplicativo) às vezes bloqueia executáveis gerados no scratchpad do
  Claude; compilar em `IA\teste.exe` funciona. Se bloquear, a execução fica comigo.

### 6.2 Armadilhas de C que já apareceram

- Lista local: declarar o **objeto** (`MoveList lista;`) e passar `&lista`.
- `extern` no `.h` é declaração; definição uma vez num `.c`.
- Macros com expressão sempre parentizadas.
- `printf` com `<inttypes.h>`: literais colados, sem vírgulas.
- Parâmetros `int8_t` truncam valores de `int`: manter o tipo do chamador.
- **GCC 4.9 + msvcrt:** `%zu`, `%llu` e `PRIu64` avisam (ou falham) sem
  `__USE_MINGW_ANSI_STDIO`.
- **`clock_t` no Windows é `long` de 32 bits** e `CLOCKS_PER_SEC` = 1000; no Linux,
  `CLOCKS_PER_SEC` = 1.000.000. Conversões de ms em `int64_t`.

---

## 7. Testes (`IA/src/teste.c`, nesta ordem na `main`)

Formato: cada bloco roda até o fim, acumula uma flag, imprime um resumo; a `main` retorna 1 se
qualquer flag estiver ligada. Detecta **se** algo falhou, não **qual** caso.

`PROF_TESTE` = 4 é a profundidade dos testes de busca (sem limite de tempo). O `PROF_MAX` do
`ia.h` só aparece como teto (casos inválidos, teste de tempo, conversão).

### 7.1 `escolherJogada` / verificação — 12 casos (passando)

Variante inválida, randomizador inválido, os dois (→ `VARIANTE_INVALIDA`), **profundidade 0,
profundidade `PROF_MAX + 1`, tempo −1, randomizador + profundidade + tempo inválidos (→
`RANDOMIZADOR_INVALIDO`), profundidade + tempo inválidos (→ `PROFUNDIDADE_INVALIDA`)**,
empate provisório, motivo desconhecido 99 (→ `ERRO_INTERNO`), mate na raiz, afogamento na
raiz. Confere `resultado`, os valores padrão e **`NOS_VISITADOS == 0`**, com sentinela
`NOS_VISITADOS = 1` antes de cada chamada.

### 7.2 Execução, v1 = v2 e contagem de nós — 9 FENs (passando)

FENs: inicial; após 1.e4 (pretas na vez); mate em 1 e em 2 dos dois lados; dama livre dos
dois lados; kiwipete. Todas com `PROF_TESTE` e `TEMPO_SEM_LIMITE`. Para cada uma:
- **v1 = v2:** minimax e alfa-beta com `SORTEIO_DESLIGADO` → mesmo `lance`, `score` e `tipo`,
  e `resultado == SUCESSO` nas duas.
- **Determinismo:** alfa-beta rodado duas vezes → mesma `JOGADA` e mesma contagem.
- **Contagem:** minimax visita **exatamente** `Σ_{D=1..PROF_TESTE} Σ_{d=0..D} perft(d)` (soma
  das iterações); alfa-beta ≤ minimax em cada FEN; no total, alfa-beta **estritamente** menor.
- **Guarda:** `halfmove_clock + PROF_TESTE >= 100` → falha com a FEN impressa.

**Premissas registradas no código:** vale para `PROF_TESTE` **sem limite de tempo** (com
tempo, o resultado depende da máquina); depende de as duas variantes percorrerem a raiz **na
mesma ordem** (com MVV-LVA, o lance pode diferir entre candidatos empatados — aí comparar o
score ou o conjunto de candidatos); o esperado exato do minimax supõe que nenhum empate por
regra dispara dentro da árvore (hoje garantido pelo stub).

### 7.3 Limite de tempo — 3 casos (passando) — **novo**

Kiwipete, `profundidade = PROF_MAX` (não termina no prazo), `SORTEIO_DESLIGADO`:
alfa-beta 1 ms, alfa-beta 100 ms, minimax 100 ms. Para cada um:
- `resultado == SUCESSO` e lance **entre os legais da raiz** (`generate_legal_moves`);
- **tabuleiro intacto:** `fen_write` antes e depois, comparadas com `strcmp`;
- **iteração 1 completa:** `NOS_VISITADOS ≥ 1 + lances da raiz`;
- **tempo gasto ≤ `tempoMs` + `FOLGA_TEMPO_MS` (50 ms)**, medido com `clock()`.

A folga é o único ponto dependente da máquina: numa máquina carregada, pode falhar sem bug.

### 7.4 `avaliar` — 14 casos (passando)

Inalterado.

### 7.5 `converterScore` — 20 casos (passando)

O código que pula casos de mate com `p = MATE − |scoreBruto| > PROF_MAX` continua lá, mas com
`PROF_MAX` 64 **nenhum caso é pulado** (o mate em p = 5 voltou a rodar). Critério é
`p ≤ PROF_MAX` (classificação da conversão).

### 7.6 Medições (14/11)

**Contagem com aprofundamento iterativo até 4, sem tempo:**

| Posição | Lance | Score | Minimax (nós) | Alfa-beta (nós) |
|---|---|---|---|---|
| Inicial | a2a3 | cp 0 | 216.369 | 20.119 |
| Após 1.e4 | a7a6 | cp −100 | 433.589 | 48.225 |
| Mate em 1 (B / P) | a1a8 / a8a1 | mate 1 | 22.508 | 2.623 / 2.683 |
| Mate em 2 (B / P) | a1a7 / a8a2 | mate 2 | 20.521 | 3.088 / 3.176 |
| Dama livre (B / P) | d1d5 / d8d4 | cp 900 | 191.632 | 16.183 / 15.150 |
| Kiwipete | e2a6 | cp 10 | 4.287.640 | 290.549 |
| **Total** | | | **5.406.920** | **401.796 (7,4%)** |

Comparado à etapa 8 (busca direta a 4): minimax +3,0% (5.250.729), alfa-beta +8,7%
(369.742). Mesmos lances e scores. O custo extra é o das iterações 1–3, que sem ordenação
nada devolvem à iteração 4.

**Limite de tempo (kiwipete, profundidade `PROF_MAX`):**

| Variante | `tempoMs` | Gasto | Nós | Lance |
|---|---|---|---|---|
| Alfa-beta | 1 | 5 ms | 3.072 | e2a6 (só a iteração 1 valeu) |
| Alfa-beta | 100 | 104 ms | 289.792 | e2a6 (completou a 3, cortou na 4) |
| Minimax | 100 | 100 ms | 534.528 | e2a6 |

Estouro do prazo medido: até ~4 ms com `tempoMs` 100. No 1 ms, o gasto (5 ms) é a iteração 1
mais três intervalos de 1024 nós da iteração 2 (o corte só veio na consulta do nó 3072) —
provavelmente efeito da resolução do `clock()` no Windows; não investigado.

**RNF01:** com `movetime` o limite de 200 ms passa a ser garantido pela chamadora (menos o
estouro de alguns ms), em vez de depender de `PROF_MAX`.

---

## 8. Decisões a acertar com a máquina de regras

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
8. **Ligar a IA ao `go`** do `uci.c` e ao build do motor (hoje `IA/` não tem Makefile). A
   interface já está pronta: `go depth N` → `profundidade`, `go movetime T` → `tempoMs`;
   `score cp` / `score mate` via `converterScore`. Falta acordar o padrão quando o `go` vem
   sem parâmetros, e o mapa de níveis de dificuldade (RF03).

O dev das regras é **@caioveigap** (CODEOWNER de `engine/`).

---

## 9. Próximo passo

Opções, na ordem que eu recomendaria (a escolha é minha):

1. **Redigir o pedido ao dev das regras** (§8 itens 2, 4 e 7, citando 8): nomes, assinaturas
   e onde cada coisa mora, alinhados ao `next_steps.md`. É curto, e a demora fica do lado dele
   — quanto antes sai, mais folga até 23/11.
2. **Item 10: ordenação MVV-LVA**, mais o melhor lance da iteração anterior primeiro na raiz
   (é o que faz o aprofundamento iterativo pagar). Exige revisar o teste v1 = v2 (§7.2).
3. **Recomendações pendentes do item 9** (não são erros):
   - **Parar ao achar mate:** hoje `go depth 64` sem tempo acha o mate na iteração 2 e segue
     buscando quase indefinidamente. Uma iteração mais funda não acha mate mais curto; dá para
     parar quando `|scoreBruto| ≥ LIMIAR_MATE`.
   - **`clock()` falhando (`(clock_t)-1`)** faz o relógio nunca disparar (equivale a
     `TEMPO_SEM_LIMITE`); `tempoMs` perto de 2³¹ estoura o `clock_t` de 32 bits no Windows.
     Improváveis; registrar ou validar.
   - **Teste do sorteio:** o 1.5.6 está concluído, mas nenhum teste exercita
     `SORTEIO_LIGADO` (lance sorteado ∈ candidatos; mesma semente → mesmo lance).
4. Levar `179c7f3` e `c95a223` para a `main` quando o grupo combinar.

---

## 10. Mudanças de escopo da IA em relação à EAP (item 1.5)

**Trabalho realizado que não aparece na EAP** (acumulado):
- Especificação em pseudocódigo de todas as funções (6 arquivos, especificação viva).
- Verificação da chamada (`verificarChamada`) e campo `resultado`.
- Conversão de score para o protocolo (`converterScore`).
- Contrato com a máquina de regras (`pendenciasRegras.h`, ex-`contrato.h`) — ligado a 1.5.2.
- Testes automatizados da IA: verificação (12), v1 = v2 + determinismo + contagem de nós (9
  FENs), **limite de tempo (3)**, avaliação (14), conversão (20); stubs controláveis.
  Relaciona-se com 1.6.1.5.
- **Instrumentação por contagem de nós**, com o perft das regras como oráculo exato.
- Garantias em tempo de compilação (`_Static_assert`).
- Controle de semente do sorteio (`iniciarSorteio`).

**Itens da EAP e seu estado:**

| Item | Estado |
|---|---|
| 1.5.1 Pesquisa | Concluído |
| 1.5.2 Integração com a Máquina de Regras | Em andamento: gerador legal, make/unmake e perft integrados; falta `in_check`, empates e ligação ao `go` |
| 1.5.3 Simulação de Estados | Concluído: fazer/desfazer com `Undo` local, tabuleiro intacto conferido (também com corte por tempo) |
| 1.5.4.1 Avaliação Material | Concluído e testado |
| 1.5.4.2 Avaliação Posicional | Não iniciado (v6+) |
| 1.5.5.1 Minimax | Concluído, executado e testado |
| 1.5.5.2 Poda Alfa-Beta | Concluído, executado e testado (v1 = v2; ~7% dos nós) |
| 1.5.5.3 Aprofundamento Progressivo | **Concluído e testado** (`c95a223`): `go depth` + `go movetime`; níveis de dificuldade não mapeados |
| 1.5.5.4 Ordenação de Movimentos | Não iniciado (MVV-LVA, próximo na busca) |
| 1.5.6 Seleção de Lance Aleatório | Concluído (sorteio não exercitado nos testes: todos usam `SORTEIO_DESLIGADO`) |

**Tamanho (14/11, `c95a223`, sem comentários nem linhas em branco):** `IA/` **590** linhas de
C (eram 501 em `e8063fa`; mesmo critério). O resto do sistema não foi recontado (em 13/11:
motor 2.966, agente 479, interface 354).

Quiescência e TT continuam fora da EAP — levar ao grupo.
