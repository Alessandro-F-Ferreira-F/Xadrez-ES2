# Contexto para continuidade — IA de xadrez (etapa 14: níveis de dificuldade e RNF01)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa13.md`.

Como nas etapas anteriores, **o repositório é a fonte da verdade** (`Xadrez-ES2`, branch `IA`).
Este arquivo guarda o que o repositório não guarda: decisões, motivos, medições, pendências e
o estado do trabalho. A §5 lista onde está cada coisa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Estado em 16/11/2026 (fim da etapa 14):** pedido ao dev das
regras **enviado** (§8), sem resposta. **RNF01 alterado** (`468ca0f`): IA < 200 ms no fácil e
no médio, < 1 s no difícil. **Níveis de dificuldade (RF03) feitos** (`501fdbf`): módulo
`niveis`, mapa nível → parâmetros de `escolherJogada`, sem a ligação ao `go` (§3.20). **PSTs
conferidas** contra o CPW: idênticas (§3.19). Recomendações pequenas aplicadas (`6ddca64`).
**Merge feito:** `main`, `IA` e os remotos iguais (até `6ddca64`, mais o commit desta
atualização). **Complemento ao dev enviado** (§8). Suíte passando (~6 s), nenhum aviso vindo
de `IA/`. **9 de 10 itens da EAP concluídos**; falta só o 1.5.2 (integração), que depende do
dev das regras.
**Próximo passo:** aguardar a resposta do dev e fazer a integração (§9).

---

## 0. Dependências da máquina de regras

### 0.1 Já disponível e usado pela IA

| Item | Onde | Observação |
|---|---|---|
| `Board`, `Color`, `Piece`, `Move`, `MoveList`, `MOVE_NONE` | `board.h`, `piece.h`, `move.h` | `MoveList` tem `MAX_MOVES` = 256 lances |
| `generate_legal_moves(Board*, MoveList*)` | `movegen.h` | **Legal**. Não há gerador só de capturas: a quiescência gera tudo e filtra |
| `make_move` / `unmake_move`, `Undo` | `makemove.h` | Validados pelo perft |
| `move_is_capture`, `move_is_ep_capture`, `move_is_promotion`, `move_from`, `move_to` | `move.h` | Ordenação (§3.12), filtro da quiescência (§3.15), delta (§3.18) |
| `movelist_clear`, `movelist_add` | `move.h` | Lista de capturas da quiescência (§3.17) |
| `SQ_TO_EDGE`, `DIR_OFFSET`, `KNIGHT_ATTACKS`, `PAWN_ATTACKS`, `PAWN_PUSH`, `RANK_OF`, `FILE_OF` | `square.h` | `lanceGarantido` (§3.17). **Nova dependência**: incluir no congelamento (§8) |
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

5. **`KING_TARGETS` nunca é inicializada** (achado na etapa 13): declarada em `square.h` e
   definida em `square.c`, mas `init_square_tables()` não a preenche — fica toda em 0 (casa a1).
   O motor não a usa, por isso ninguém viu. A IA usa `SQ_TO_EDGE` + `DIR_OFFSET` no lugar.
   **Avisar o dev.**

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
  (etapa 12: comentário do caso de 1 ms desatualizado pelo 10b, corrigido em `d874b94`;
  etapa 13: "os 10 itens da EAP ficam concluídos" — eram 9, o 1.5.2 segue em andamento).
- **Escopo de escrita do Claude:** a pasta do projeto (com minha aprovação prévia), a pasta de
  memória dele e o scratchpad da sessão. **Qualquer outra pasta é proibida** sem pedido ou
  autorização minha (etapa 13: uma tentativa de gravar em `/tmp` foi barrada). Saídas e
  programas de medição vão para o scratchpad, não para `IA/`.
- **Fontes externas:** quando o Claude escrever dados de memória (ex.: as PSTs), deve dizer e
  me pedir para conferir contra a fonte.
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
- **Mensagem de commit fora da convenção** (etapa 10: `0615267`; etapa 14: `6ddca64`, dois
  commits sugeridos fundidos num só, com as duas mensagens unidas por `;` — já no remoto,
  deixado assim).

**Mudanças sem efeito cruzado podem ir em commits separados a partir de uma cópia de trabalho
só** (etapa 14): `git add` só dos arquivos de cada commit, um por vez.

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

**Requisitos que afetam a IA:** RF03 (níveis de dificuldade, feito sem a ligação ao `go`,
§3.20); **RNF01, alterado na etapa 14** (`468ca0f`, `docs/requisitos.md`): "O tempo de
resposta do backend para validações deve ser inferior a 200 milissegundos, e o das jogadas da
IA deve ser inferior a 200 milissegundos nos níveis Fácil e Médio e a 1 segundo no nível
Difícil" (antes: < 200 ms "nos níveis básicos", sem dizer quais). **A equipe de IA sou só eu:
os requisitos da IA posso alterar** (o limite das validações é das regras e ficou como estava).
Protocolo com `go movetime` / `go depth` e `score cp` / `score mate`;
invariante `bestmove` ∈ lista legal (o `uci.c` já confere, RF-M3); testes do grupo "v2 devolve
o mesmo lance que v1" e "mesma FEN + profundidade → mesmo lance" — implementados (§7.2). **Com
a ordenação, "v2 = v1" vale para score e conjunto de candidatos, não para o lance** (§3.12).

**Roadmap (Fase 7):** v1 material + minimax → v2 alfa-beta → v3 aprofundamento iterativo +
movetime → v4 MVV-LVA → v5 quiescência → v6+ PST, tapered, TT. TT fora do MVP.

**Gantt (IA):** alfa-beta, ordenação e aprofundamento 03/11–20/11. Apresentação **23/11**.
**Situação em 16/11:** v1 a **v6** executadas e testadas (v6 = PST + tapered só no rei), e os
três níveis de dificuldade mapeados. TT continua fora (depende do Zobrist do motor, §8).

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
| `d874b94` | Quiescência nas duas variantes, passada de segurança, parada por mate nova, `NOS_QUIESCENCIA`; `quiescencia.txt` (novo) + 7 arquivos | |
| `c57cdd0` | Contexto da etapa 12 | |
| `f57f549` | Stand-pat antes de gerar (`lanceGarantido`) + só capturas na ordenação; `iaTeste.h` (novo) + 3 arquivos | |
| `4121a26` | *Delta pruning* (margem 200), `ligarDeltaPruning`, `VALOR_PECA` público (7 arquivos) | topo de `origin/main` antes do merge da etapa 14 |
| `2a38ca7` | PSTs de Michniewski + rei interpolado por fase; sorteio com FEN nova (4 arquivos) | |
| `e0bedbc` | Contexto da etapa 13 | |
| `468ca0f` | RNF01: 200 ms no fácil e no médio, 1 s no difícil (`docs/requisitos.md`) | |
| `501fdbf` | Níveis de dificuldade: `niveis.h/.c`, `niveis.txt` (novos) + `teste.c` | |
| `7745f35` | Contexto da etapa 14 | |
| `6ddca64` | Pseudocódigo com os nomes do C (`PROF_MAX`, `clock()`, ms → tiques) + `FEN_KIWIPETE` e `fen_parse` do sorteio no teste (4 arquivos) | mensagem dupla, fora da convenção, deixado assim |
| (este) | Contexto da etapa 14 atualizado após o merge | |

Todos em `origin/main` (merge por *fast-forward* na etapa 14). `IA/docs/` entra no repositório **de propósito**: explicita o
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

### 3.2 Avaliação (v6 desde `2a38ca7`; detalhes na §3.19)

Material de Michniewski (P 100, C 320, B 330, T 500, D 900, R 0), agora na tabela pública
`VALOR_PECA` (`avaliar.h`), + PSTs de Michniewski + rei interpolado por fase. `AVALIACAO_MAX`
= 10615 (pior caso com PST, conferido na etapa 13).

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
  `prazo`, `tempoEsgotado`, **`deltaAtivo`, `podaDelta`, `valorVitima`** (`ia.c`);
  `notaMVVLVA` (`ordenar.c`); **`pesoFase`, `pst`, `pstReiMeio`, `pstReiFinal`** (`avaliar.c`);
  **`tabelaNiveis`** (`niveis.c`, etapa 14).
  Públicas: `converterScore`, `escolherJogada`, `iniciarSorteio`, `JOGADA`, `CANDIDATOS`,
  `NOS_VISITADOS`, `NOS_QUIESCENCIA`, `PROF_ALCANCADA`, `PARCIAL_USADA` (`ia.h`);
  `LanceOrdenado`, `NOTA_PRIMEIRO`, `ordenarLances` (`ordenar.h`); **`avaliar`, `VALOR_PECA`,
  `AVALIACAO_MAX`** (`avaliar.h`); **`NIVEL_FACIL`, `NIVEL_MEDIO`, `NIVEL_DIFICIL`,
  `RESPOSTA_MAX_BASICO_MS`, `RESPOSTA_MAX_DIFICIL_MS`, `ParametrosNivel`, `parametrosNivel`**
  (`niveis.h`, etapa 14). Internas do `niveis.c` (macros): `TEMPO_BASICO_MS`,
  `TEMPO_DIFICIL_MS`, `PROF_FACIL`, `PROF_MEDIO`.
- **Módulo `niveis` separado** (etapa 14), no molde do `ordenar`; só usa constantes do `ia.h`.
- **`iaTeste.h` (etapa 13):** cabeçalho **só de teste**, incluído pelo `ia.c` e pelo `teste.c`,
  nunca pelo motor. Expõe `lanceGarantido` e `ligarDeltaPruning`, definidas (não `static`) no
  `ia.c`. A interface pública (`ia.h`) não mudou.
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
Etapa 13: stand-pat antes de gerar **sem** guarda (A1: a quiescência deixaria de ver
afogamento fora de xeque); guarda só com os lances do rei (falha em ~55% dos cortes); TT agora
(fora da EAP e depende do Zobrist); rei de Michniewski com troca brusca de tabela (salto de até
60 cp na troca da última dama).
Etapa 14 (§3.20): 4 ou 5 níveis; minimax no fácil (mesmo score, só mais lento); erros de
propósito — sorteio entre os N melhores (exigiria o valor de cada filho da raiz) e ruído na
avaliação (quebraria o determinismo); fácil com profundidade 1 (entrega peça); médio com
`PROF_MAX` (seria o difícil com menos tempo); `escolherJogadaNivel(board, nivel)` (fundiria os
eixos; `escolherJogada` não muda); prazos nominais (200 e 1000) com tolerância no requisito.

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
- **Ordem do caso base da `quiescenciaMinimax`:** tempo → empate por regra → `PROF_LIMITE` →
  gerar → lista vazia → *stand-pat* (fora de xeque) → laço. **Na `quiescenciaAlfaBeta`, desde
  a etapa 13:** tempo → empate → `PROF_LIMITE` → xeque → *stand-pat* com corte antecipado
  (guardado por `lanceGarantido`) → gerar → lista vazia → *stand-pat* como piso/teto → laço
  (§3.17).
- **Chamada:** o nó do horizonte chama a quiescência **na mesma profundidade** (é a raiz
  dela). Nunca é a raiz da busca, então alfa e beta sempre podem ser atualizados nela.
- **5A — `uint64_t NOS_QUIESCENCIA`:** entradas nas funções de quiescência. O nó do horizonte
  conta duas vezes em `NOS_VISITADOS`, então `NOS_VISITADOS − NOS_QUIESCENCIA` é a busca
  principal — no minimax, continua exata pelo perft.
- *Delta pruning* entrou na etapa 13 (§3.18). SEE continua fora (opcional).

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

### 3.17 Da etapa 13: stand-pat antes de gerar (`f57f549`), escolha **A2 amplo**

Medição: com quiescência, metade dos nós da quiescência corta no stand-pat, mas só depois de
gerar todos os lances legais (o passo mais caro do nó).

- **Na `quiescenciaAlfaBeta`:** xeque → (fora de xeque) `standPat = avaliar` → se cortaria
  (`standPat >= beta` nas brancas, `<= alfa` nas pretas) **e** `lanceGarantido(board)`, devolve
  `standPat` sem gerar. Senão gera e segue como antes (lista vazia → mate/afogamento; o
  stand-pat ainda pode cortar depois).
- **Por que exato:** com a prova, a lista não seria vazia e o corte seria o mesmo. A árvore não
  muda (mesmos nós), a `quiescenciaMinimax` **não mudou**, e o v1 = v2 segue exato.
- **`lanceGarantido` (pré-condição: fora de xeque)** — condição **suficiente**, não necessária
  (falso ≠ afogamento). Uma peça fora da fileira, coluna e diagonais do próprio rei não pode
  estar cravada; fora de xeque, qualquer lance pseudolegal dela é legal: peão (avanço simples ou
  captura normal; **en passant fica de fora**), cavalo (destino vazio ou inimigo), bispo/torre/
  dama (um passo numa direção). Se nenhuma servir, o rei: fora de xeque ele não bloqueia raio
  nenhum, então `is_square_attacked` com o rei no lugar é exato. Na medição, a prova falhou em
  465 de 349 mil cortes (kiwipete).
- **Só capturas na ordenação:** fora de xeque, as capturas vão para `listaCapturas` (na ordem do
  gerador) antes de `ordenarLances`; o `continue` do filtro saiu do laço do alfa-beta. Mesma
  ordem (inserção estável), ~7% de tempo a menos.
- **Ganho:** kiwipete prof. 3 de 2,04 s para ~0,99 s (`-O2`), mesmos 697.974 nós.

### 3.18 Da etapa 13: *delta pruning* (`4121a26`)

- **Onde:** só na `quiescenciaAlfaBeta`, no laço, fora de xeque, **sem promoções**. Pula a
  captura se `standPat + valorVitima + MARGEM_DELTA <= alfa` (brancas) ou `standPat −
  valorVitima − MARGEM_DELTA >= beta` (pretas), com o alfa/beta correntes. `valorVitima` =
  `VALOR_PECA` da peça no destino; en passant = peão.
- **`MARGEM_DELTA` = 200.** Medido (kiwipete prof. 3, nós): sem 697.974; 300 → 443.826;
  200 → 412.655; 100 → 247.960. Mesmos lances e scores nas 8 posições em todas. 100 recusada:
  com a PST, a mudança posicional de uma captura passa a contar (escolha feita antes da PST).
- **Quebra a exatidão do v1 = v2** (o corte depende de alfa e beta; o minimax não tem como
  reproduzir). **Chave interna** `static bool deltaAtivo = true` + `ligarDeltaPruning(bool)`
  em `iaTeste.h`: o v1 = v2 com quiescência desliga antes do laço e religa logo depois (o
  usuário sugeriu religar no fim do teste; ficou logo após o uso, mais seguro).
- **`VALOR_PECA`:** a `tabelaValores` (`static` no `avaliar.c`) virou pública e renomeada, para
  não duplicar os valores.

### 3.19 Da etapa 13: PST (`2a38ca7`), escolha **(c) interpolação por fase**

- **Tabelas:** Michniewski (*Simplified Evaluation Function*, CPW), em `avaliar.c`, escritas
  **como na fonte** (primeira linha = fileira 8). Peça branca usa `pst[tipo][casa ^ 56]`;
  preta (espelhada), `pst[tipo][casa]`. Escritas de memória pelo Claude na etapa 13;
  **conferidas na etapa 14**: a página do CPW foi baixada (HTML; o `action=raw` dá 404) e os
  448 números das 7 tabelas, comparados por script, são **idênticos**. A orientação (primeira
  linha = fileira 8) foi confirmada pelo texto da página (f3/c3 −10, g3 −5, d2/e2 −20). O
  material também confere (P 100, C 320, B 330, T 500, D 900); o rei vale 20000 na fonte e 0
  no `VALOR_PECA`, de propósito (nenhum lance legal o captura).
- **Rei:** duas tabelas (`pstReiMeio`, `pstReiFinal`) misturadas pela fase, em vez do critério
  de final de Michniewski (troca brusca, descartada). `fase` = soma dos pesos do **PeSTO** (não
  de Michniewski): cavalo e bispo 1, torre 2, dama 4; `FASE_MAX` = 24 (posição inicial), com
  limite em 24. `rei = (reiMeio × fase + reiFinal × (24 − fase)) / 24`, sobre os termos
  **líquidos** (brancas − pretas): a divisão trunca em direção a zero, simétrica entre as cores.
- **Efeito (200 ms):** inicial `b1c3` (antes `a2a3`), após 1.e4 `g8f6`, italiana `b1c3`. Custo:
  o alfa-beta poda menos (menos empates de score) — total da §7.2 sem quiescência subiu de
  174.492 para 297.182 nós (3,3% → 5,5% do minimax); em 200 ms, várias posições perderam 1 nível.

### 3.20 Da etapa 14: níveis de dificuldade (`501fdbf`) e RNF01 (`468ca0f`)

Todas as recomendações aceitas:

| Nível | Variante | Sorteio | Quiescência | Profundidade | `tempoMs` | Teto RNF01 |
|---|---|---|---|---|---|---|
| `NIVEL_FACIL` (1) | alfa-beta | ligado | não | `PROF_FACIL` = 2 | `TEMPO_BASICO_MS` = 180 | 200 ms |
| `NIVEL_MEDIO` (2) | alfa-beta | ligado | sim | `PROF_MEDIO` = 3 | 180 | 200 ms |
| `NIVEL_DIFICIL` (3) | alfa-beta | ligado | sim | `PROF_MAX` | `TEMPO_DIFICIL_MS` = 950 | 1000 ms |

- **3 níveis**, só **pensando menos** (profundidade, quiescência, prazo); não erram de propósito.
- **Alfa-beta em todos:** o minimax dá o mesmo score (v1 = v2), só mais devagar.
- **Sorteio em todos:** sem custo; com a PST os empates são raros, então muda pouco o jogo.
- **Fácil com profundidade 2:** com 1, captura peça defendida (o `d1d5` da §7.4) e parece
  quebrado. **Médio com 3:** fica sempre abaixo do difícil; no kiwipete nem chega a 3 em 180
  ms (2, parcial), então em posições táticas também é limitado pelo tempo.
- **Prazos abaixo dos tetos** (180 e 950): o requisito vale sem tolerância. O relógio passou do
  prazo em até 6 ms nas medições (§7.10).
- **Forma:** `bool parametrosNivel(int nivel, ParametrosNivel *p)` preenche a struct com os
  cinco parâmetros; quem chama repassa a `escolherJogada`, que **não mudou**. Nível fora de
  [`NIVEL_FACIL`, `NIVEL_DIFICIL`] devolve `false` sem alterar `*p`. `tabelaNiveis` `static
  const`, indexada por `nivel − NIVEL_FACIL`.
- **`_Static_assert`** no `niveis.c`: prazos > `TEMPO_SEM_LIMITE` (0 desligaria o relógio) e
  abaixo dos tetos; profundidades em [1, `PROF_MAX`]; uma linha da tabela por nível.
- **RNF01:** a equipe de IA sou só eu, e eu autorizei a alteração. Medido antes de fixar
  (§7.10): com 1 s, +1 meio-lance em 4 de 7 posições, 0 nas táticas (kiwipete, meio-jogo).
- **Fora desta etapa (integração, §8):** como o nível chega pelo protocolo, e quem prevalece
  quando o `go` traz `depth`/`movetime` junto com o nível.

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo das funções~~ ✅
2. ~~Pseudocódigo: `verificarChamada` e `converterScore`~~ ✅
3. ~~C: `avaliar` + testes~~ ✅
4. ~~C: `converterScore` + testes~~ ✅
5. ~~C: verificação, busca, chamadora, sorteio~~ ✅
6. ~~Stubs + testes da `verificarChamada`~~ ✅
7. Acertar com as regras as decisões da §8 — **pedido enviado em 16/11, aguardando resposta**
8. ~~Primeira execução da busca~~ ✅
9. ~~Aprofundamento iterativo + `go movetime`~~ ✅. ~~Níveis de dificuldade (RF03)~~ ✅
   (`501fdbf`); só a ligação ao `go` depende das regras (§8)
10. ~~Ordenação MVV-LVA~~ ✅, ~~melhor anterior primeiro~~ ✅, ~~10b: iteração parcial~~ ✅
11. ~~Quiescência~~ ✅ (`37f6831`, `d874b94`); ~~custo (stand-pat antes de gerar, delta)~~ ✅
    (`f57f549`, `4121a26`)
12. ~~PST (v6) + rei interpolado~~ ✅ (`2a38ca7`); ~~PSTs conferidas contra o CPW~~ ✅ (etapa 14)
13. TT (fora do MVP; depende do Zobrist do motor)

Ligar a IA ao `go` depende do dev das regras (§8).

---

## 5. Onde está cada coisa (repositório, branch `IA`)

| Arquivo | Conteúdo | Estado |
|---|---|---|
| `IA/Pseudocode/minimax.txt` | minimax; `PROF_MAX`, `PROF_LIMITE`, `limiteIteracao`, `tempoEsgotado`, `estourouTempo`, `nosVisitados`, `quiescenciaAtiva` | fechado |
| `IA/Pseudocode/alfa-beta.txt` | poda alfa-beta; ordenação; `primeiro`; `scoreParcial`; chamada da quiescência | fechado |
| `IA/Pseudocode/quiescencia.txt` | `quiescenciaMinimax`, `lanceGarantido`, `podaDelta`, `valorVitima`, `quiescenciaAlfaBeta` | fechado (etapa 13) |
| `IA/Pseudocode/ordenarLances.txt` | `notaMVVLVA`, `ordenarLances` | fechado |
| `IA/Pseudocode/avaliar.txt` | avaliação v6: material + PST + rei por fase (tabelas no `avaliar.c`) | fechado (etapa 13) |
| `IA/Pseudocode/escolherJogada.txt` | chamadora: aprofundamento, relógio, parcial, passada de segurança, parada por mate | fechado |
| `IA/Pseudocode/verificarChamada.txt` | verificação + `RESULTADO_*` | fechado |
| `IA/Pseudocode/converterScore.txt` | conversão para o UCI | fechado |
| `IA/Pseudocode/niveis.txt` | motivos dos níveis, `tabelaNiveis`, `parametrosNivel` | novo (etapa 14) |
| `IA/header/ia.h` | constantes, `Play`, protótipos, globais públicas | fechado |
| `IA/header/avaliar.h` | `avaliar`, `VALOR_PECA`, `AVALIACAO_MAX` | fechado |
| `IA/header/iaTeste.h` | **só para testes**: `lanceGarantido`, `ligarDeltaPruning` | novo (etapa 13) |
| `IA/header/ordenar.h` | `LanceOrdenado`, `NOTA_PRIMEIRO`, `ordenarLances` | fechado |
| `IA/header/niveis.h` | `NIVEL_*`, `RESPOSTA_MAX_*_MS`, `ParametrosNivel`, `parametrosNivel` | novo (etapa 14) |
| `IA/header/pendenciasRegras.h` | `testeEmpateRegra` + motivos (provisório) | provisório |
| `IA/header/stubs.h`, `IA/src/stubs.c` | stub controlável de empate (`varTesteEmpate`) | para testes |
| `IA/src/avaliar.c` | avaliação v6: `VALOR_PECA`, PSTs, `pesoFase` | testado |
| `IA/src/ordenar.c` | MVV-LVA + inserção estável | testado |
| `IA/src/niveis.c` | `tabelaNiveis`, prazos e profundidades dos níveis | testado (etapa 14) |
| `IA/src/ia.c` | conversão, verificação, `lanceGarantido`, busca, quiescência com delta, relógio, chamadora, sorteio, contagem | **executado e testado** |
| `IA/src/teste.c` | 9 blocos de teste (§7) | passando |
| `IA/docs/` | contextos das etapas 2–14 | versionado |
| `docs/requisitos.md` | requisitos do grupo; RNF01 alterado pela IA (etapa 14) | do grupo |

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
      IA/src/niveis.c IA/src/stubs.c $eng -o IA\teste.exe
  .\IA\teste.exe; $LASTEXITCODE
  ```

  Todo o `engine/src` menos `main.c`; **`niveis.c` novo na etapa 14**. A suíte leva **~6 s**
  (etapa 14; ~1,2 s a mais pelo teste dos níveis). O `.vscode/tasks.json` (ignorado pelo git)
  **foi acertado nesta máquina** na etapa 14 (mesmos arquivos e flags; testado); em outra
  máquina, acertar à mão.
- **Medições** (etapas 13 e 14): programas à parte no **scratchpad** do Claude, compilados com `-O2
  -DNDEBUG` contra o `ia.c` do disco, chamando `init_square_tables()` antes de tudo (sem ela, os
  resultados saem errados sem aviso: aconteceu na primeira medição da etapa 13).
- **Avisos aceitos:** só os que vêm de `engine/` (`fen.c:153`, `io.c`, `move.c`, `movegen.c`,
  `uci.c`), cerca de 17. **Qualquer aviso vindo de `IA/` é novo.** Em `501fdbf`: nenhum.
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
  inclusive `IA\teste.exe`. Se bloquear, a execução fica comigo. Para programas de medição,
  recompilar com um `-D` qualquer a mais (muda o binário) costuma resolver.

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
chamado pelas duas tabelas); etapa 13: **`percorrerLanceGarantido`**, **`espelharCores`**,
**`percorrerSimetria`** (percorrem a árvore como o perft). `fensLanceGarantido` (inicial,
kiwipete, perft 3, perft 4) serve aos dois percursos; etapa 14: **`mesmosParametros`**. O teste
inclui `iaTeste.h`, `makemove.h` e (etapa 14) `niveis.h`.

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
lados (3); torre solta (3, rica em capturas; era a FEN do sorteio). **Roda com o delta
desligado** (`ligarDeltaPruning(false)` antes do laço, `true` logo depois). **Fora por custo** (medido > 60 s): após 1.e4
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

### 7.4 Quiescência em posições escolhidas (passando; ampliado na etapa 13)

Alfa-beta, profundidade 1:
- **Horizonte** (`casosHorizonte`, dos dois lados): dama captura peão defendido por peão
  (`4k3/8/4p3/3p4/8/8/8/3QK3 w`, `d1d5`; espelho preto, `d8d4`). Sem quiescência, a captura é o
  lance; com ela, nem é candidata.
- **Stand-pat:** posição inicial — `NOS_QUIESCENCIA == 20` (um por horizonte) e mesmo score,
  tipo e `CANDIDATOS` (mesma ordem) do desligado.
- **Mate na quiescência** (`casosMateQuiescencia`, dos dois lados): com quiescência, mate em 1
  com o lance certo; sem ela, centipeão.
- **`lanceGarantido` em afogamentos** (`fensAfogamento`, 4 FENs: dama, peão bloqueado fora das
  linhas do rei, peão e rei brancos): lista vazia, fora de xeque, `false`.
- **`lanceGarantido` nas árvores** (prof. `PROF_LANCE_GARANTIDO` = 3, ~120 mil nós fora de
  xeque): `true` ⇒ lista não vazia. Foi este teste que achou o `KING_TARGETS` não inicializado.
- **Delta** (posições do v1 = v2 com quiescência + kiwipete prof. `PROF_DELTA_KIWIPETE` = 2):
  nós com delta ≤ sem delta por posição; no total, estritamente menos. Sem igualdade de lance ou
  score (o delta pode mudá-los por construção).
- **Lacuna conhecida:** nenhum caso em que o caminho "prova falhou → gera → afogamento" mude o
  resultado da busca (é o caminho de antes; montar a posição é trabalhoso).

### 7.5 Sorteio (passando; FEN nova na etapa 13)

FEN `6k1/5ppp/8/8/8/8/8/RRR3K1 w - - 0 1`: três mates em 1 (Ta8, Tb8, Tc8), empatados em
`MATE − 1` **qualquer que seja a avaliação** (com a PST, as quatro capturas da FEN antiga
deixaram de empatar). Para na iteração 1 (mate achado). Alfa-beta, `PROF_TESTE`, sem
quiescência, sementes 1..8: os três lances aparecem.

### 7.6 Ordenação (passando)

Inalterado (FEN `7k/8/8/q2pP1r1/1P6/1n3N2/2B5/3QK3 w - d6 0 1`, três casos de `primeiro`).

### 7.7 Níveis de dificuldade (passando; novo na etapa 14)

- **Inválidos** (`niveisInvalidos`: `NIVEL_FACIL − 1`, `NIVEL_DIFICIL + 1`, −1): `false` e a
  sentinela (`{-7, -7, -7, -7, -7}`) intacta.
- **Válidos** (`casosNiveis {nivel, esperado, respostaMaxMs}`): parâmetros **exatos**, escritos
  como literais (2, 3, 180, 950) — duplicação proposital, o teste é a especificação, como os
  esperados do `avaliar`. No kiwipete (`FEN_KIWIPETE`): `SUCESSO`, lance ∈ `CANDIDATOS`,
  `PROF_ALCANCADA ≤ profundidade` e **gasto < teto do RNF01, sem folga** (`>=` falha; a margem
  vem dos prazos). Medido: fácil 0 ms (2), médio 181 ms (2, parcial), difícil 954 ms (3,
  parcial).

### 7.8 `avaliar` — 20 casos + simetria (passando; refeito na etapa 13)

Esperados **calculados à mão**, com a conta no comentário de cada caso: inicial (0); extremos de
material (±10340: 10400 − 60 de PST, fase limitada a 24); a1/h8 com fase 4 e truncamento
(±949); só os reis, fase 0 (∓70); fase 24 com o branco rocado (±100); uma peça por vez nos dois
ramos de cor. **Simetria:** espelhar fileiras e trocar cores inverte o sinal, em todos os nós
das árvores de `fensLanceGarantido` (prof. `PROF_SIMETRIA` = 3, ~122 mil nós).

### 7.9 `converterScore` — 20 casos (passando)

Os casos da fronteira do limiar são relativos a `LIMIAR_MATE`: testam 19871 (mate, 64 lances)
e 19870 (centipeão). A regra que pula casos usa `PROF_LIMITE`.

### 7.10 Medições (16/11, etapas 13 e 14; as de 14/11 abaixo, marcadas)

**Totais da suíte com a PST:** sem quiescência (`PROF_TESTE` = 4), minimax 5.362.232 (exato
pelo perft, não muda), alfa-beta **297.182** (5,5%; antes da PST, 174.492 = 3,3%). Com
quiescência (delta desligado), minimax 211.985, alfa-beta **35.122** (antes da PST, 17.944).
Delta: total 189.459 → 126.525 nós.

**Kiwipete, alfa-beta com quiescência, prof. 3, `-O2`, antes da PST:** original 697.974 nós,
2,04 s → A2 (mesmos nós) 0,99 s → + delta 200: 412.655 nós, 0,73 s.

**Profundidade alcançada** (alfa-beta com quiescência, `PROF_MAX`, `-O2`; "p" = iteração
parcial aproveitada). Colunas antes da PST, depois as da etapa 14 (PST, 3 execuções cada,
resultados iguais nas 3; todas parciais):

| Posição | Sem quiescência | Quiescência original | + A2 + só capturas | + delta | PST, 200 ms | PST, 1 s |
|---|---|---|---|---|---|---|
| Inicial | 6 | 6 | 6 | 6 | 5p (`b1c3`, +40) | 6p (`b1c3`, 0) |
| Após 1.e4 | 6 | 5 | 5 | 5 | 5p (`g8f6`, −10) | 5p (`e7e5`, 0) |
| Italiana | 5 | 4 | 5 | 5 | 4p (`b1c3`, −12) | 5p (`b1c3`, +5) |
| Kiwipete | 5 | 2 | 3 | 3 | 3p (`e2a6`, +45) | 3p (`e2a6`, +45) |
| Ordenação | 6 | 5 | 5 | 6 | 6p (`b4a5`, +1033) | 7p (`b4a5`, +1190) |
| perft3 | 7 | 6 | 7 | 7 | 6p (`b4f4`, +47) | 7p (`b4f4`, +30) |
| Meio-jogo¹ | 5 | 4 | 5 | 5 | 4p (`f1d3`, −243) | 4p (`f1d3`, −243) |

- **1 s contra 200 ms:** +1 meio-lance em 4 de 7; 0 nas táticas (kiwipete, meio-jogo), onde o
  gargalo é o custo da quiescência e não o prazo (a SEE atacaria isso). Após 1.e4 a
  profundidade não muda, mas a parcial avança e o lance muda.
- **Gasto:** até 1006 ms com prazo 1000 e até 203 ms com 200.
- **Divergência com a etapa 13:** lá a coluna "+ PST" registrou kiwipete 2 e ordenação 5; na
  etapa 14, com o mesmo código, 3 e 6. Provável carga da máquina na medição anterior; a tabela
  acima substitui aquela coluna.
- Programa: `medir1s.c` no scratchpad da sessão da etapa 14 (não versionado).

¹ `r1bq1rk1/pp2bppp/2n1pn2/3p4/2PP4/2N1PN2/PP3PPP/R2QKB1R w KQ - 0 8`. Italiana:
`r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4`. perft3:
`8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1`.

**Observado, não investigado:** em algumas execuções de 200 ms o gasto foi 204–215 ms (uma em
duas, posições variadas, também antes das mudanças da etapa 13). Provável granularidade do
relógio do Windows; a folga do teste é 50 ms.

**(14/11)** Com quiescência (tabela da §7.2), antes da etapa 13: total minimax 211.985,
alfa-beta 17.944 (8,5%). Ex.: torre solta, prof. 3: minimax 108.334 nós (90.361 de
quiescência), alfa-beta 5.558 (3.021).

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

**(14/11) Alfa-beta com quiescência, kiwipete** (`-O2`): prof. 1 = 62.486 nós, 187 ms; prof. 2 =
128.700, 381 ms; prof. 3 = 697.974, 2,0 s; prof. 4 = 1.472.308 (1.369.475 de quiescência),
4,1 s. Sem quiescência, prof. 4 = 102.261 nós. Tratado na etapa 13 (acima). O RNF01 em si é
cumprido pelo relógio; o custo se mede em profundidade alcançada.

**(14/11) Limite de tempo (kiwipete, `PROF_MAX`, `-Og`, 3 execuções):**

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

Pedido redigido no chat em 14/11 (itens 2, 4, 7 e 8, citando 1). **Enviado em 16/11**,
aguardando resposta. **Complemento também enviado em 16/11** (etapa 14): `square.h` no
congelamento, o `KING_TARGETS` não inicializado (§0.2, item 5), o **mapa de níveis pronto**
(`niveis.h`, §3.20) com as três perguntas do item 8, os arquivos do build e o RNF01 novo.

1. **Congelamento dos cabeçalhos** (`board.h`, `piece.h`, `move.h`, `makemove.h`, `movegen.h`,
   **e agora `square.h`**). A IA depende também de `move_is_capture`, `move_is_ep_capture`,
   `move_is_promotion`, `move_from`, `move_to`, `movelist_clear`, `movelist_add` e das tabelas
   de `square.h` (§0.1) (e o teste, de `move_to_str` e `movelist_find`).
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
   `avaliar.c`, `ordenar.c`, **`niveis.c`**; `stubs.c` não). `go depth N` → `profundidade`,
   `go movetime T` → `tempoMs`; `score cp` / `score mate` via `converterScore`. O mapa de
   níveis (RF03) está pronto (§3.20). Falta acordar: **como o nível chega pelo protocolo** (ex.:
   `setoption name Nivel value N`, ou parâmetro no `go`); **quem prevalece** quando o `go` traz
   `depth`/`movetime` junto com o nível; o padrão do `go` sem parâmetros e sem nível (sugestão:
   `NIVEL_MEDIO`).

O dev das regras é **@caioveigap** (CODEOWNER de `engine/`).

---

## 9. Próximo passo

1. **Integração** (1.5.2), quando o dev responder (§8, itens 2, 4 e 8).
2. **Apresentação (23/11).**
3. **Avisar o grupo da mudança no RNF01**, se ainda não foi feito (`docs/requisitos.md` está
   fora de `/IA/`; o complemento ao dev já cita o RNF01 novo).

Feitos na etapa 14: ~~níveis (RF03)~~ (§3.20), ~~conferir as PSTs~~ (§3.19), ~~merge `IA` →
`main`~~ (*fast-forward*), ~~complemento ao dev~~ (§8).

**Recomendações resolvidas na etapa 14** (`6ddca64`, mais o `tasks.json` local):
- ~~`fen_parse` sem conferir o retorno no sorteio~~: agora liga a flag, no padrão do bloco da
  quiescência (o bloco segue rodando; o padrão `else { ... }` da ordenação reindentaria tudo).
- ~~FEN do kiwipete repetida~~: `#define FEN_KIWIPETE` no topo do `teste.c`; `fenTempo` e
  `fenKiwipete` removidos.
- ~~Pseudocódigo~~: `PROF_MAX` (era `profMax`), `clock()` (era `relógio()`), `prazo = inicio +
  tempoMs * CLOCKS_PER_SEC / 1000`.
- ~~`.vscode/tasks.json`~~ igual ao comando da §6.1 (também corrigido `Ia\src\stubs.c`).

**Recomendações pendentes** (não são erros; mudam comportamento ou pedem medição):
- Opcionais fora da EAP: SEE na quiescência (ajudaria nas posições táticas, §7.10); teste de
  afogamento que passe pelo caminho "prova falhou" da `quiescenciaAlfaBeta` (§7.4).
- Seleção preguiçosa na ordenação.
- Medir a profundidade alcançada antes da ordenação (§7.10, herdado).
- Pequena, no teste: a FEN inicial aparece 18 vezes no `teste.c` (`fenInicial`,
  `fensLanceGarantido`, `casosV1V2`, casos da verificação...), como a do kiwipete aparecia.

---

## 10. Mudanças de escopo da IA em relação à EAP (item 1.5)

**Trabalho realizado que não aparece na EAP** (acumulado):
- Especificação em pseudocódigo de todas as funções (**9 arquivos**, especificação viva).
- Verificação da chamada (`verificarChamada`) e campo `resultado`.
- Conversão de score para o protocolo (`converterScore`).
- Contrato com a máquina de regras (`pendenciasRegras.h`) — ligado a 1.5.2.
- Testes automatizados da IA: verificação (15), v1 = v2 + determinismo + contagem de nós (9 +
  8 FENs), limite de tempo (5), quiescência em posições escolhidas (5 + afogamentos (4) +
  percurso do `lanceGarantido` + delta (9 posições)), sorteio (8 sementes), ordenação (3
  casos), níveis (3 inválidos + 3 válidos com o prazo do RNF01), avaliação (20 + simetria em
  ~122 mil nós), conversão (20); stubs controláveis.
  Relaciona-se com 1.6.1.5.
- **Instrumentação por contagem de nós**, com o perft das regras como oráculo exato, e
  `NOS_QUIESCENCIA`, `PROF_ALCANCADA`, `PARCIAL_USADA`.
- Garantias em tempo de compilação (`_Static_assert`).
- Controle de semente do sorteio (`iniciarSorteio`).
- Robustez do relógio (falha do `clock()`, estouro de 32 bits), parada por mate, iteração
  parcial e passada de segurança.
- **Quiescência** (fora da EAP; levar ao grupo), com stand-pat antes de gerar (prova barata de
  lance legal) e *delta pruning*.
- **Rei interpolado por fase** (*tapered eval*) na avaliação posicional.
- Bug achado no motor pelos testes da IA: `KING_TARGETS` não inicializada.
- **RNF01 revisto com medição** (etapa 14): tetos por nível em vez de "níveis básicos".
- PSTs conferidas por script contra a fonte (448 valores).

**Itens da EAP e seu estado:**

| Item | Estado |
|---|---|
| 1.5.1 Pesquisa | Concluído |
| 1.5.2 Integração com a Máquina de Regras | Em andamento: gerador legal, make/unmake e perft integrados; mapa de níveis pronto do lado da IA; falta `in_check`, empates e ligação ao `go` (pedido enviado em 16/11) |
| 1.5.3 Simulação de Estados | Concluído |
| 1.5.4.1 Avaliação Material | Concluído e testado |
| 1.5.4.2 Avaliação Posicional | **Concluído e testado** (etapa 13): PSTs de Michniewski (conferidas na etapa 14) + rei interpolado por fase |
| 1.5.5.1 Minimax | Concluído, executado e testado |
| 1.5.5.2 Poda Alfa-Beta | Concluído, executado e testado (~5,5% dos nós do minimax com ordenação e PST; 3,3% só com material) |
| 1.5.5.3 Aprofundamento Progressivo | Concluído e testado, com iteração parcial; **níveis de dificuldade mapeados e testados** (etapa 14) |
| 1.5.5.4 Ordenação de Movimentos | Concluído e testado: MVV-LVA −56% de nós no alfa-beta |
| 1.5.6 Seleção de Lance Aleatório | Concluído e testado |

**Quanto está pronto (16/11):** **9 de 10 itens da EAP** concluídos e testados (90% por
contagem); falta o 1.5.2, que depende do dev das regras. **Por esforço, ~85%** (contando a
quiescência no escopo; antes dos níveis, ~83%):

| Parte | Peso aproximado | Estado |
|---|---|---|
| Núcleo da busca (minimax, alfa-beta, aprofundamento, tempo, sorteio, parcial) + testes + especificação | ~40% | pronto |
| Avaliação material, conversão de score, verificação da chamada | ~10% | pronto |
| Integração (`go`, build, níveis, `in_check` e empates do motor) | ~20% | níveis prontos do lado da IA; o resto depende do dev (pedido enviado) |
| Ordenação MVV-LVA + revisão do v1 = v2 | ~10% | pronto |
| Quiescência | ~10% | **pronto**, com o custo tratado (stand-pat antes de gerar, delta) |
| Avaliação posicional (PST) | ~10% | **pronto** |

Para apresentação: "9 de 10 itens da EAP concluídos e testados, mais a quiescência; o
alfa-beta visita ~5% dos nós do minimax; a quiescência elimina o efeito horizonte e teve o
custo reduzido em ~65% (kiwipete, prof. 3: 2,04 s → 0,73 s); com a PST a IA desenvolve as
peças em vez de jogar `a2a3`; três níveis de dificuldade, com resposta abaixo de 200 ms no
fácil e no médio e de 1 s no difícil (RNF01); falta a integração com o motor".

Quiescência e TT continuam fora da EAP — levar ao grupo.
