# Contexto para continuidade — IA de xadrez (etapa 7: IA escrita em C, aguardando stubs e perft)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa6.md`.

**Mudança de formato em relação à etapa 6:** o pseudocódigo e o código C deixaram de ser
copiados aqui. Eles estão no repositório (`Xadrez-ES2`, branch `IA`) e **o repositório é a
fonte da verdade**. Este arquivo guarda o que o repositório não guarda: decisões, motivos,
pendências e o estado do trabalho. A §5 lista onde está cada coisa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Estado em 03/10/2026:** toda a IA do MVP está escrita em C e
compila sem avisos (`avaliar`, `converterScore`, `verificarChamada`, `minimax`, `podaAlfaBeta`,
`escolherJogada`, sorteio). `avaliar` e `converterScore` estão **testadas e aprovadas** sem
depender das regras. A busca **não pode rodar** até o perft bater. **Próximo passo:** stubs
controláveis do contrato com as regras, para testar a `verificarChamada` (§9).

O escopo da IA cresceu em relação à EAP original; a §10 resume a diferença, para atualizar o
item 1.5 da EAP.

---

## 0. Dependências da máquina de regras

### 0.1 Já disponível no repositório

| Item | Onde | Observação |
|---|---|---|
| `Board` (mailbox `Piece array[64]`, a1 = 0, `side_to_move`, `king_square[2]`, `castling_rights`, `ep_square`, `halfmove_clock`, `fullmove_number`) | `board.h` | |
| `Color` (`WHITE = 1`, `BLACK = 0`) | `piece.h` | Comparar sempre pelos nomes |
| `Piece`, `PIECE_TYPE` / `PIECE_COLOR` / `PIECE_MAKE`, `PAWN=1 … KING=6`, `EMPTY` | `piece.h` | `PIECE_COLOR(EMPTY) == BLACK` — testar `== EMPTY` antes |
| `Move` = `u16`, `MOVE_NONE ((Move)0)`, `MoveList` (`moves[256]`, `int count`), `movelist_clear` / `movelist_add` | `move.h` / `move.c` | `MOVE_NONE` já existe — ver §8, decisão 5 |
| `make_move` / `unmake_move`, `Undo` | `makemove.h` / `makemove.c` | `unmake_move` implementado segundo commit das regras; validação só pelo perft |
| `generate_pseudo_legal_moves(Board*, MoveList*)` | `movegen.h` | **Pseudo-legal**: não filtra lances que deixam o próprio rei em xeque |
| `is_square_attacked(const Board*, int sq, Color side)` | `movegen.h` | Base possível para `emXeque` |
| `bool fen_parse(const char*, Board*)` | `fen.h` / `fen.c` | Ver armadilhas na §7.3 |

### 0.2 Bloqueantes para **rodar** a busca

1. **Gerador legal** (`gerarLancesLegais`), filtrado por legalidade. Hoje só há o pseudo-legal.
2. **`emXeque`**.
3. **`testeEmpateRegra`** com motivo (50 lances, tripla repetição, material insuficiente).
   Tripla repetição exige **histórico de posições**, que não está no `Board`.
4. **Perft passando.** Regra do grupo: nenhuma busca roda antes. **Ainda sem data.**

Os nomes e assinaturas que a IA espera estão propostos em `IA/header/contrato.h` (§6.3).

### 0.3 O que já foi testado sem depender das regras

- `avaliar` — 14 casos via `fen_parse` (§7.1). ✅
- `converterScore` — 20 casos com valores fabricados (§7.2). ✅
- `verificarChamada` — **pendente**, via stubs (§9).

---

## Como quero que você conduza

- Respostas **sucintas e tecnicamente precisas**. Sem preâmbulo, sem repetir o que eu disse.
- **Não me dê a solução pronta**: aponte onde está o erro e por quê, e me deixe corrigir. Só
  escreva código ou pseudocódigo se eu pedir explicitamente. Exceção para o que não é
  algorítmico (textos, comentários, mensagens de commit, configuração).
- Quando eu delegar uma escrita, a autorização vale **só para aquele trecho**; depois volte ao
  modo padrão.
- Eu **decido o escopo e a ordem**. Sugira, mas não assuma.
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Avalie **só o escopo que eu pedir**; o resto fica para quando eu chegar lá.
- Mantenho o **pseudocódigo como especificação viva**; valide-o a cada versão, incluindo os
  comentários.
- Separe **erro** de **recomendação**. Não insista em recomendação como se fosse erro.
- Ao conferir arquivos, leia **do disco**: já aconteceu de eu esquecer de salvar.
- Não quero o Claude como contribuidor no repositório remoto: eu faço os commits; não use
  linhas de coautoria.

**Padrões de erro meus para vigiar** (todos ocorreram de novo na etapa 7):
- Ramos espelhados corrigidos pela metade — ou **errados do mesmo jeito nos dois lados**
  (ex.: `alfa > melhor` e `beta < melhor` invertidos juntos, o que desligava a poda sem mudar o
  resultado).
- Consumidores não atualizados depois que mudo assinatura, nome ou decisão (ex.: protótipo
  antigo esquecido no `.h`; comentário citando parâmetro com nome antigo).
- Fundir casos distintos num mesmo retorno (ex.: motivo de empate desconhecido tratado como
  repetição).
- Estado que deveria ser local ao nó declarado como global — e o inverso: candidatos gravados
  na lista local que estava sendo percorrida.
- Informação duplicada em dois lugares.
- Precedência e parênteses em macros e expressões fundidas.
- Erros de digitação em nomes e mensagens.
- **Novos na etapa 7:** ponteiro sem objeto por trás (`MoveList *lista;` ou `= NULL` passado
  para quem escreve nele) quando o certo era o objeto local; confundir declaração (`extern`)
  com definição; vírgulas entre literais em `printf` com macros `PRI...`; condição de teste que
  nunca pode ser verdadeira (o teste passa sem testar nada).

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
desktop C++/SFML o executa como subprocesso. Posição trafega como FEN.

**Requisitos que afetam a IA:** RF03 (níveis de dificuldade); RNF01 (resposta < 200 ms nos
níveis básicos); protocolo com `go movetime` / `go depth` e `score cp` / `score mate`;
invariante `bestmove` ∈ lista legal; testes do grupo "v2 devolve o mesmo lance que v1" e
"mesma FEN + profundidade → mesmo lance" (atendidos com `SORTEIO_DESLIGADO` ou semente fixa).

**Roadmap (Fase 7):** v1 material + minimax → v2 alfa-beta → v3 aprofundamento iterativo +
movetime → v4 MVV-LVA → v5 quiescência → v6+ PST, tapered, TT. TT fora do MVP. Quiescência não
está na EAP nem no Gantt.

**Gantt (IA):** integração com regras até 02/10; avaliação 05/10–12/10; minimax 13/10–02/11;
alfa-beta, ordenação e aprofundamento 03/11–20/11. Apresentações 05/10 e 23/11.
**Situação em 03/10:** avaliação, minimax, alfa-beta e chamadora já estão **escritos em C**,
semanas antes das datas do Gantt; avaliação e conversão de score já testadas. O risco
continua fora do meu escopo: gerador legal, xeque, empates e perft sem data.

**Commits:** `docs(IA): ...` para pseudocódigo/documentação, `feat(IA): ...` para código;
título sozinho na primeira linha sem ponto final, corpo opcional após linha em branco.
Últimos: `db57413` (pseudocódigo da verificação e conversão), `04be8d3` (converterScore +
testes), `5c82e88` (busca e chamadora em C).

---

## 3. Decisões tomadas

### 3.1 Busca (inalteradas desde a etapa 1)

- Convenção absoluta: brancas positivas, pretas negativas; max/min literais.
- Vez lida de `side_to_move`; sem parâmetro `jogador`.
- Ordem do caso base: empate por regra → limite de profundidade → gerar lances → lista vazia
  (xeque = mate, senão afogamento) → laço.
- `MoveList` e `Undo` **locais ao nó**, como objetos (não ponteiros).
- `MATE = 19999`, mate vale `±(MATE − profundidade)` com sinal pelo **lado derrotado**;
  `INFINITO = 20000`; empate = 0; `SCORE_INVALIDO = 30000`.
- Score em `int32_t` em tudo que o carrega.
- Alfa-beta: na raiz alfa e beta **não são atualizados** (`profundidade > 0` na atualização).
- Consequência conhecida: a busca nunca produz exatamente `LIMIAR_MATE` (o mate mais profundo
  possível vale `MATE − (PROF_MAX − 1)`); uma unidade de folga, sem efeito.

### 3.2 Avaliação v1 (fechada e testada)

- Só material, Michniewski: P 100, C 320, B 330, T 500, D 900, R 0; tabela indexada por
  `PIECE_TYPE`.
- Faixa: ±4000 inicial; **±10400** na v1 (lado com K, 9D, 2T, 2C, 2B contra rei só);
  **10615** com PSTs (v6+) — margem adotada, agora como `AVALIACAO_MAX` em `avaliar.h`.
- **Teste do rei adiado para a v6+**: com um rei por lado (exigido por `fen_parse`), qualquer
  valor de rei se cancela em `brancas − pretas`; com PST de rei deixa de cancelar.

### 3.3 Constantes públicas (mudança da etapa 7)

`PROF_MAX`, `MATE`, `INFINITO`, `SCORE_INVALIDO` e `LIMIAR_MATE = (MATE − PROF_MAX)` passaram
para o **`ia.h`**, porque os testes calculam os esperados a partir delas. **Antes:** "`PROF_MAX`
definido só no `ia.c`" e "`SCORE_INVALIDO`/`INFINITO` internos". Não redefinir no `ia.c`.

- `PROF_MAX` está em **9000 (chute provisório)**. Profundidade 9000 não termina: **baixar para
  3–4 antes da primeira execução** e ajustar por medição (RNF01).
- `_Static_assert(LIMIAR_MATE > AVALIACAO_MAX)` no `ia.c`: o compilador recusa `PROF_MAX ≥ 9384`.
  Testado: com 9384 a compilação falha.

### 3.4 Chamadora e resultado da chamada (novo na etapa 7)

- **Pré-condições deixaram de ser do chamador.** `escolherJogada` chama `verificarChamada`
  (interna) e só busca se ela devolver sucesso.
- **Novo campo `Play.resultado` (`int8_t`)**, separado de `tipo` (que continua só tipo de
  score). Struct segue com 8 bytes.
- Convenção: **0 = sucesso, negativos = bug, positivos = partida terminada na raiz**.
  - `RESULTADO_ERRO_INTERNO (-1)`, `RESULTADO_VARIANTE_INVALIDA (-2)`,
    `RESULTADO_RANDOMIZADOR_INVALIDO (-3)`
  - `RESULTADO_SUCESSO 0`
  - `RESULTADO_RAIZ_MATE 1`, `RESULTADO_RAIZ_AFOGAMENTO 2`, `RESULTADO_EMPATE_50_LANCES 3`,
    `RESULTADO_EMPATE_REPETICAO 4`, `RESULTADO_EMPATE_MATERIAL 5` (empates provisórios)
- Ordem das verificações: variante → randomizador → empate por regra → lista vazia (mesma
  ordem do caso base). Só o primeiro problema é reportado.
- Empates **distinguidos** por motivo (combinado com o dev das regras para a versão final).
  `verificarChamada` traduz caso a caso o motivo das regras para `RESULTADO_EMPATE_*`; motivo
  desconhecido vira `RESULTADO_ERRO_INTERNO`.
- `LANCE_NULO`, `SCORE_INVALIDO` e `TIPO_INVALIDO` mantidos **como valores padrão**: sempre que
  `resultado != SUCESSO`, toda a struct acompanha. O chamador confere **`resultado`**.
- Bloco defensivo no fim da `escolherJogada`: candidatos vazios com raiz válida é impossível;
  se ocorrer, grava `RESULTADO_ERRO_INTERNO`.

### 3.5 Conversão de score (nova função, etapa 7)

- `converterScore(int32_t scoreBruto, Color lado, Play *p)` — **pública** para ser testada
  sem a busca. Preenche só `score` e `tipo`; não toca `lance` nem `resultado`.
- Ordem obrigatória: inversão de perspectiva → classificação por faixa (`LIMIAR_MATE`) →
  `mate N = sinal × ((MATE − |score| + 1) / 2)` só no ramo de mate.
- Em `escolherJogada`, o resultado da busca vai para `scoreBruto` local (inicializado com
  `SCORE_INVALIDO`) e `converterScore` só é chamada com candidatos não vazios.

### 3.6 Sorteio (novo na etapa 7)

- `iniciarSorteio(unsigned int semente)` — **pública**, chama `srand` uma única vez. Motor:
  `time(NULL)`; testes: semente fixa (reproduz até o `SORTEIO_LIGADO`).
- Motivo de não ressemear a cada jogada: o `rand()` do msvcrt (MinGW) é linear; sementes de
  segundos consecutivos dão primeiros valores quase lineares no tempo.
- `sorteioLance(int quantidade)` interna: `rand() % quantidade`.

### 3.7 Organização do código

- `static`: `verificarChamada`, `minimax`, `podaAlfaBeta`, `sorteioLance`, `listaCandidatos`
  (`MoveList` reaproveitada). Públicas: `converterScore`, `escolherJogada`, `iniciarSorteio`,
  `JOGADA`.
- `JOGADA`: **declarada** `extern` no `ia.h`, **definida** uma vez no `ia.c`.
- `contrato.h` é incluído pelo `ia.c` (e pelos stubs), **não** pelo `ia.h`.

### 3.8 Descartado

Ponder e threads (motivos na etapa 6: protocolo, TT compartilhada, testes determinísticos).

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo: minimax, avaliar, valores terminais, alfa-beta, chamadora~~ ✅
2. ~~Pseudocódigo: `verificarChamada` e `converterScore`~~ ✅
3. ~~C: `avaliar` + testes~~ ✅
4. ~~C: `converterScore` + testes~~ ✅
5. ~~C: `verificarChamada`, `minimax`, `podaAlfaBeta`, `escolherJogada`, sorteio~~ ✅ (só compila)
6. **Stubs controláveis + testes da `verificarChamada`** ← próximo (§9)
7. Acertar com as regras as decisões da §8; aguardar perft
8. Primeira execução da busca: `PROF_MAX` pequeno, teste v1 = v2, **contagem de nós**
   (única forma de ver se a poda poda)
9. Aprofundamento iterativo + `go movetime` / níveis de dificuldade (mora na chamadora;
   variável de profundidade com outro nome, atualizar caso base de `minimax` **e**
   `podaAlfaBeta`)
10. Ordenação MVV-LVA (estrutura própria lance + nota, local ao nó)
11. Quiescência
12. TT (fora do MVP)

---

## 5. Onde está cada coisa (repositório, branch `IA`)

| Arquivo | Conteúdo | Estado |
|---|---|---|
| `IA/Pseudocode/minimax.txt` | minimax (renomeado de `minmax.txt`) | fechado |
| `IA/Pseudocode/avaliar.txt` | avaliação v1 | fechado |
| `IA/Pseudocode/alfa-beta.txt` | poda alfa-beta | fechado |
| `IA/Pseudocode/escolherJogada.txt` | chamadora; contém `SCORE_INVALIDO` | fechado |
| `IA/Pseudocode/verificarChamada.txt` | verificação + `RESULTADO_*` | fechado |
| `IA/Pseudocode/converterScore.txt` | conversão para o UCI | fechado |
| `IA/header/ia.h` | constantes públicas, `Play`, protótipos e contratos em comentário | fechado |
| `IA/header/avaliar.h` | `avaliar`, `AVALIACAO_MAX` | fechado |
| `IA/header/contrato.h` | contrato provisório com as regras | provisório |
| `IA/src/avaliar.c` | avaliação v1 | testado |
| `IA/src/ia.c` | conversão, verificação, busca, chamadora, sorteio | compila; busca não executada |
| `IA/src/teste.c` | testes de `avaliar` e `converterScore` | **não linka** até existirem stubs |

**Divergências da etapa 6 já resolvidas (o repositório prevalece):**
- `SCORE_INVALIDO` continua no `escolherJogada.txt` (não foi para o minimax).
- `minmax.txt` renomeado para `minimax.txt`.
- A conversão de score **foi isolada** (`converterScore`); a pendência "isolar a conversão"
  da etapa 6 está encerrada.
- A "função externa de pré-condições" da etapa 6 virou a `verificarChamada`, interna.

---

## 6. Implementação em C — notas

### 6.1 Build e ambiente

- Compilador: **GCC TDM 4.9.2** (Dev-Cpp MinGW64), o mesmo do PATH. `-std=c11` funciona.
  Não aceita `%zu` (só avisa); para `size_t`, cast ou índice `int`.
- `.vscode/tasks.json` (ignorado pelo git): tarefa "IA: compilar teste.c", `cwd` na raiz,
  saída `IA\teste.exe`. Rodar com `.\IA\teste.exe; $LASTEXITCODE`.
- **Arquivos para linkar o teste da IA hoje:** `IA/src/teste.c`, `ia.c`, `avaliar.c`,
  `stubs.c` (a escrever), e do motor `fen.c`, `piece.c`, `square.c`, `log.c`, `move.c`,
  `makemove.c`, `board.c`, `utils.c`. Conferido: com eles, só faltam os três símbolos do
  contrato. **A tarefa do `tasks.json` ainda precisa ganhar `ia.c` (se ainda não tiver),
  `stubs.c`, `move.c`, `makemove.c`, `board.c` e `utils.c`.**
- Avisos aceitos: só os três de `fen.c:153` (`Board b = {0};`), falso positivo do GCC 4.9.
  Qualquer outro aviso é novo.
- Para o Claude: o Windows (Controle de Aplicativo) às vezes bloqueia executáveis gerados no
  scratchpad; a execução fica comigo nesses casos.
- `engine/include/fen.h.gch` (cabeçalho pré-compilado solto) foi removido. Não passar `.h` na
  linha de comando do `gcc`.

### 6.2 Armadilhas de C que já apareceram

- Lista local: declarar o **objeto** (`MoveList lista;`) e passar `&lista`; acessar com `.`.
- `extern` no `.h` é declaração; a definição vai uma vez num `.c`; nunca definir no `.h`
  (com GCC ≥ 10, `-fno-common` vira erro de definição múltipla).
- Macros com expressão sempre parentizadas: `(MATE - PROF_MAX)`, `(-1)`.
- `printf` com `<inttypes.h>`: o formato é um bloco de literais colados (`"%" PRId32 "\n"`),
  sem vírgulas; os valores vêm depois.
- Parâmetros `int8_t` truncam valores de `int` e podem mascarar entradas inválidas: manter o
  tipo do chamador.

### 6.3 contrato.h (provisório)

- `void gerarLancesLegais(Board *b, MoveList *lista)` — lista alocada por quem chama; `b` não
  é `const` (o filtro de legalidade pode fazer/desfazer lances).
- `bool emXeque(const Board *b)`.
- `int testeEmpateRegra(const Board *b)` — devolve `SEM_EMPATE (0)` ou `EMPATE_50_LANCES (1)`,
  `EMPATE_REPETICAO (2)`, `EMPATE_MATERIAL (3)`. `SEM_EMPATE` precisa ser 0: `minimax` e
  `podaAlfaBeta` usam o retorno como condição.
- Vocabulário **separado** do `ia.h` (o arquivo vai para o dev das regras e não deve depender
  da IA); a tradução fica na `verificarChamada`.

---

## 7. Testes

### 7.1 avaliar — 14 casos (passando)

Vetor `casosAvaliacao` (FEN + esperado) no `teste.c`: posição inicial (0); extremos
`k7/8/8/8/8/1NBB4/1QQQRR1N/1QQQQQQK w - - 0 1` (+10400) e
`1qqqqqqk/1qqqrr1n/1nbb4/8/8/8/8/K7 b - - 0 1` (−10400); `7R/8/8/3k4/8/8/8/R3K3 w - - 0 1`
(+1000, casas a1/h8); cada peça (P, C, B, T, D) nos dois ramos de cor, reis em e1/e8.

### 7.2 converterScore — 20 casos (passando)

Vetor `casos` (scoreBruto, lado, tipo e score esperados): centipeão e perspectiva (5); mate
com brancas na vez p = 1..5; mate com pretas na vez p = 1..4; fronteira do limiar (6, com
`LIMIAR_MATE` e `LIMIAR_MATE − 1`, nos dois sinais e com inversão). Sentinelas não nulos em
`lance` e `resultado` verificam que não são alterados. Esperados calculados a partir de `MATE`
e `LIMIAR_MATE`.

**Formato do `teste.c`:** os dois laços rodam até o fim, acumulam flags (parser, avaliação,
conversão), imprimem um resumo e a `main` retorna 1 se qualquer flag estiver ligada.
Decisão consciente: o teste só detecta **se** algo falhou, não **qual** caso; o índice será
acrescentado quando um teste falhar.

### 7.3 fen_parse — o que ele valida

Exatamente um rei de cada cor; no máximo 8 peões e 16 peças por lado; nenhum peão na 1ª/8ª
fileira; direitos de roque coerentes com as peças (usar `-` em posições montadas à mão);
mínimo de 4 campos. `out` só é escrito se tudo validar — **sempre conferir o retorno**.

---

## 8. Decisões a acertar com a máquina de regras (atualizado)

1. **Congelamento dos cabeçalhos** (`board.h`, `piece.h`, `move.h`, `makemove.h`).
2. **Funções do contrato:** levar o `contrato.h` como proposta de nomes, assinaturas e de
   quem aloca a `MoveList` (quem chama). Decidir se `emXeque` será construída sobre
   `is_square_attacked` e se o gerador legal filtra o `generate_pseudo_legal_moves`.
3. **Data do perft.**
4. **Empates:** já combinado que haverá distinção de motivo na versão final. Falta acordar os
   **valores** dos motivos (`SEM_EMPATE` = 0 é requisito da IA) e **como a tripla repetição
   acessa o histórico** de posições.
5. **Lance nulo:** `move.h` já define `MOVE_NONE ((Move)0)`. O `LANCE_NULO 0` do `ia.h`
   duplica essa informação. Recomendação pendente: usar `MOVE_NONE` (ou definir
   `LANCE_NULO` a partir dele) — decidir.
6. **Zobrist**, se entrar: incremental dentro de `make_move`/`unmake_move`.

---

## 9. Próximo passo — stubs e testes da verificarChamada

Decidido: stubs **controláveis**, em **arquivo separado**.

- `IA/header/stubs.h`: `extern` de duas variáveis de controle (motivo de empate a devolver:
  `int`; resposta de xeque: `bool`).
- `IA/src/stubs.c`: inclui `contrato.h` e `stubs.h`, define as variáveis e implementa as três
  funções. `gerarLancesLegais` devolve **sempre lista vazia** (`count = 0`): todos os casos
  terminam antes da busca. Silenciar parâmetros não usados com `(void)nome;`.
- O teste chama `escolherJogada` (a `verificarChamada` é `static`) com um `Board` válido
  (ex.: posição inicial via `fen_parse`) e confere `JOGADA.resultado` **e** os valores padrão
  dos outros campos.
- Casos: variante inválida; randomizador inválido; os dois inválidos (espera
  `VARIANTE_INVALIDA`); 50 lances; repetição; material; motivo desconhecido (99 →
  `ERRO_INTERNO`); `SEM_EMPATE` + xeque → `RAIZ_MATE`; `SEM_EMPATE` sem xeque →
  `RAIZ_AFOGAMENTO`. **9 casos.**
- Caminho de sucesso **fora** deste teste: exigiria busca real com lances inventados e
  `PROF_MAX` 9000.
- Estimativa: ~1h a 1h30 se eu escrever tudo; ~45 min a 1h se o Claude escrever os stubs.

---

## 10. Mudanças de escopo da IA em relação à EAP (item 1.5)

EAP atual:

```
1.5 IA
    1.5.1 Pesquisa
    1.5.2 Integração com a Máquina de Regras
    1.5.3 Simulação de Estados
    1.5.4 Avaliação de Posição
        1.5.4.1 Avaliação Material
        1.5.4.2 Avaliação Posicional
    1.5.5 Busca de Melhor Lance
        1.5.5.1 Algoritmo Minimax
        1.5.5.2 Poda Alfa-Beta
        1.5.5.3 Aprofundamento Progressivo
        1.5.5.4 Ordenação de Movimentos
    1.5.6 Seleção de Lance Aleatório
```

**Trabalho realizado que não aparece na EAP:**
- **Especificação em pseudocódigo** de todas as funções (6 arquivos, mantidos como
  especificação viva).
- **Verificação da chamada** (`verificarChamada`): validação de parâmetros e classificação de
  raiz terminal (mate, afogamento, três tipos de empate), com o campo `resultado` e a
  convenção sucesso / bug / fim de partida.
- **Conversão de score para o protocolo** (`converterScore`): perspectiva do lado da vez,
  classificação centipeão/mate, `mate N` em lances completos.
- **Contrato com a máquina de regras** (`contrato.h`): proposta de interface, ligada a 1.5.2.
- **Testes automatizados da IA**: avaliação (14 casos), conversão (20), verificação (9,
  pendente), stubs controláveis. Relaciona-se com 1.6.1.5 (Framework de Testes).
- **Garantias em tempo de compilação** (`_Static_assert` separando avaliação e mate).
- **Controle de semente** do sorteio (`iniciarSorteio`), parte de 1.5.6, com requisito de
  reprodutibilidade para testes.

**Itens da EAP e seu estado:**

| Item | Estado |
|---|---|
| 1.5.1 Pesquisa | Concluído |
| 1.5.2 Integração com a Máquina de Regras | Em andamento: contrato proposto; bloqueado por gerador legal, xeque, empates e perft |
| 1.5.3 Simulação de Estados | Depende de `make_move`/`unmake_move` das regras; a IA usa fazer/desfazer com `Undo` local |
| 1.5.4.1 Avaliação Material | Concluído e testado |
| 1.5.4.2 Avaliação Posicional | Não iniciado (v6+); margem já reservada (`AVALIACAO_MAX` = 10615) |
| 1.5.5.1 Minimax | Escrito em C; aguarda perft para executar |
| 1.5.5.2 Poda Alfa-Beta | Escrito em C; aguarda perft; validar com contagem de nós |
| 1.5.5.3 Aprofundamento Progressivo | Não iniciado |
| 1.5.5.4 Ordenação de Movimentos | Não iniciado (MVV-LVA) |
| 1.5.6 Seleção de Lance Aleatório | Concluído (lista de candidatos + `iniciarSorteio`); não executado |

**Itens previstos no roadmap e ausentes da EAP:** quiescência (v5) e tabela de transposição
(fora do MVP). Levar ao grupo se entram na EAP.

**Sugestão de reorganização para discutir** (a decidir por mim e pelo grupo): um subitem para a
**interface da IA com o motor** (verificação da chamada, conversão de score, `JOGADA`) e um
para **testes da IA**, ou a inclusão destes em 1.6.1.5.