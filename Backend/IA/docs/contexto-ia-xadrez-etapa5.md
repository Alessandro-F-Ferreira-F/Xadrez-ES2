# Contexto para continuidade — IA de xadrez (etapa 5: implementação em C)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa4.md`.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. **Todo o pseudocódigo do MVP está fechado e validado**: minimax,
avaliação v1, poda alfa-beta e função chamadora `escolherJogada` (§5–§8). **Próximo passo:
acertar seis decisões com os responsáveis pela máquina de regras (§9) e começar a primeira
implementação da IA em C (§10).** A §0 lista o que precisa estar pronto do lado das regras
para eu escrever e testar a IA.

## 0. Dependências da máquina de regras — o que precisa estar pronto para eu começar

Esta seção existe para responder, de relance: **o que falta do lado das regras para eu escrever
a IA em C e poder testá-la?**

### 0.1 Já disponível (posso escrever e compilar contra isso hoje)

| Item | Onde | Observação |
|---|---|---|
| `Board` (mailbox `Piece array[64]`, a1 = 0, `side_to_move`, `king_square[2]`, `castling_rights`, `ep_square`, `halfmove_clock`, `fullmove_number`) | `board.h` | Suficiente para `avaliar` |
| `Color` (`WHITE = 1`, `BLACK = 0`) | `piece.h` | Comparar sempre pelos nomes |
| `Piece` = `u8`, `PIECE_TYPE` / `PIECE_COLOR` / `PIECE_MAKE`, `PAWN=1 … KING=6`, `EMPTY` | `piece.h` | `PIECE_COLOR(EMPTY) == BLACK` — testar `== EMPTY` antes de usar a cor |
| `Move` = `u16` (6 origem, 6 destino, 4 tipo) | — | Sem campo de score; **conferir nomes das macros de leitura** |
| `MoveList` (`moves[256]`, `count`) | — | **Conferir quem aloca** |
| `make_move(Board*, Move, Undo*)` / `unmake_move(Board*, Move, const Undo*)` | — | Assinaturas existem, mas ver 0.2 |

### 0.2 Bloqueantes para **rodar** qualquer busca

Sem estes, `minimax` e `podaAlfaBeta` podem ser escritos e compilados, mas a primeira descida já
corrompe o tabuleiro:

1. **`make_move` preenchendo o `Undo`.** Hoje não preenche.
2. **`unmake_move` implementado.** Hoje está vazio. Precisa restaurar peças, casos especiais,
   roque, en passant, relógio de 50 lances e vez.
3. **`gerarLancesLegais(estado)`** — gerador completo **e filtrado por legalidade**. Hoje faltam
   cavalo, rei, roque, en passant e o filtro de legalidade.
4. **`emXeque(estado)`** — necessária para distinguir mate de afogamento no caso base.
5. **`testeEmpateRegra(estado)`** — 50 lances, tripla repetição e material insuficiente.
   **Tripla repetição exige histórico de posições, que não existe no `Board`** — ver decisão 4 da §9.
6. **Perft passando** nas posições padrão. É a regra do grupo e é o que prova que 1 a 5 estão
   corretos. **Sem data definida no cronograma.**

Itens 3, 4 e 5 ainda **não têm nome nem assinatura acordados** — são os nomes que uso no
pseudocódigo. Fechar isso é a decisão 2 da §9.

### 0.3 O que **não** depende de nada disso (posso implementar e testar já)

- `avaliar(estado)` — só varre `Board.array[64]`. Testável com FENs montadas à mão.
- Conversão de score e preenchimento de `JOGADA` (perspectiva, classificação por faixa,
  `mate N`) — lógica pura sobre inteiros. Testável com valores fabricados, incluindo os casos
  p = 1..5 da tabela da §8 e o caso do motor levando mate.
- Constantes (`MATE`, `INFINITO`, `LIMIAR_MATE`, tipos de score, `LANCE_NULO`), tabela de
  valores das peças, estrutura `JOGADA`.

### 0.4 Ordem prática

Começar por 0.3, que não espera ninguém. Escrever `minimax`, `podaAlfaBeta` e `escolherJogada`
em seguida, compilando contra os cabeçalhos — compilar já pega tipos, nomes de macro,
assinaturas e largura de inteiros, que o pseudocódigo não valida. Rodar só quando 0.2 estiver
completo e o perft bater.

---

## Como quero que você conduza

- Respostas **sucintas e tecnicamente precisas**. Sem preâmbulo, sem repetir o que eu disse.
- **Não me dê a solução pronta**: aponte onde está o erro e por quê, e me deixe corrigir. Só
  escreva código ou pseudocódigo se eu pedir explicitamente. Exceção para o que não é
  algorítmico (textos, comentários, mensagens de commit, configuração).
- Eu **decido o escopo e a ordem**. Sugira, mas não assuma.
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Avalie **só o escopo que eu pedir**; o resto fica para quando eu chegar lá.
- Mantenho o **pseudocódigo como especificação viva**; valide-o a cada versão, incluindo os
  comentários.
- Separe **erro** de **recomendação**. Não insista em recomendação como se fosse erro.

**Padrões de erro meus para vigiar:**
- Ramos espelhados corrigidos pela metade — e, ao uniformizar, copiar o ramo errado para o
  certo em vez do contrário.
- Consumidores não atualizados depois que mudo uma assinatura, um nome ou uma decisão (ex.:
  renomear uma constante e a comparação continuar com o nome antigo; comentário citando
  assinatura antiga).
- Fundir casos distintos num mesmo retorno.
- Estado que deveria ser local ao nó declarado como global.
- Informação duplicada em dois lugares (comentário repetindo valor de constante, constante
  definida em dois arquivos).
- Precedência em expressões fundidas (multiplicar antes de dividir por engano).

---

## 1. O que já domino (do jogo da velha)

- Minimax: valor recursivo do nó, alternância max/min, valor que sobe vs. jogada da raiz.
- Estado mutável com fazer/desfazer em vez de cópia.
- Alfa-beta: alfa piso do max, beta teto do min, corte quando a janela fecha; mesmo valor do
  minimax; ganho depende da ordenação.
- Desconto por profundidade em escala inteira.
- Desempate aleatório entre jogadas de valor máximo, com lista de candidatos na raiz.

**Armadilhas que já me morderam:**
- Sob alfa-beta, o valor de um filho pode ser **limite**, não exato. A lista de candidatos só
  funciona mantendo **janela completa nos filhos da raiz**.
- `melhor` deve ser inicializado **fora da faixa** de valores possíveis.
- I/O contamina benchmark; `-O2` rende mais que micro-otimização; **contagem de nós** é métrica
  melhor que segundos.

---

## 2. O projeto (o que afeta a IA)

**Arquitetura:** motor C (regras + IA) é um binário que fala protocolo estilo UCI por
stdin/stdout; cliente desktop C++/SFML o executa como subprocesso. Posição trafega como FEN,
com sufixo opcional `moves ...`.

**Estado da máquina de regras (setembro/2026):** compila, mas `make_move` não preenche o
`Undo`, `unmake_move` está vazio, faltam cavalo, rei, roque, en passant, filtro de legalidade,
xeque, perft e protocolo. **Regra do grupo: nenhuma IA em C antes do perft bater.** O
pseudocódigo pode avançar; a implementação espera ou usa stub.

**Tipos já existentes no repositório:**
- `Board` (`board.h`): mailbox `Piece array[64]` (índice = casa, **a1 = 0**; conteúdo = peça
  ou `EMPTY`); `side_to_move` (tipo `Color`); `king_square[2]`; `castling_rights`;
  `ep_square`; `halfmove_clock`; `fullmove_number`.
- `Color` (`piece.h`): **`WHITE = 1`, `BLACK = 0`**. Comparar sempre com os nomes.
- `Piece` = `u8`, `PIECE_TYPE`/`PIECE_COLOR`/`PIECE_MAKE`; tipos `PAWN=1 … KING=6`.
  **`PIECE_COLOR(EMPTY) == BLACK`** — testar `== EMPTY` antes de usar a cor.
- `Move` = `u16` (6 bits origem, 6 destino, 4 tipo). Valor, não estrutura. **Sem campo de
  score** — daí a estrutura `JOGADA` na chamadora e a estrutura própria da ordenação.
- `MoveList` com `moves[256]` e `count`.
- `void make_move(Board *b, Move m, Undo *u)` / `void unmake_move(Board *b, Move m, const Undo *u)`.
  `Undo` é alocado por quem chama; `make_move` o preenche; `unmake_move` restaura peças,
  casos especiais, roque, en passant, relógio de 50 lances e vez.

**Contrato que a IA precisa da máquina de regras:**
`gerarLancesLegais(estado)`, `emXeque(estado)`, `testeEmpateRegra(estado)`, `make_move`,
`unmake_move`, leitura das peças. Hash Zobrist, se entrar, incremental dentro de make/unmake.

**Requisitos e docs do grupo que afetam a IA:**
- RF03: níveis de dificuldade. RNF01: resposta < 200 ms nos níveis básicos. Protocolo usa
  `go movetime` / `go depth` (e `score cp` na resposta).
- Roadmap (Fase 7): v1 material + minimax prof. 3 → v2 alfa-beta → v3 aprofundamento
  iterativo + movetime → v4 ordenação MVV-LVA → v5 quiescência → v6+ PST, avaliação tapered, TT.
- **Tabela de transposição fora do MVP.**
- **Quiescência não está na EAP (1.5) nem no Gantt**, mas o roadmap diz que não se corta —
  levar ao grupo.
- Testes previstos: "v2 devolve o mesmo lance que v1" e "mesma FEN + profundidade → mesmo
  lance". **Ambos quebram com desempate aleatório** → resolvido pelo parâmetro `randomizador`
  da chamadora (`SORTEIO_DESLIGADO` usa `listaCandidatos[0]`).
- Invariante: `bestmove` sempre pertence à lista legal.
- Gantt (IA): integração com regras até 02/10; avaliação 05/10–12/10; minimax 13/10–02/11;
  alfa-beta, ordenação e aprofundamento em paralelo 03/11–20/11. Apresentações: 05/10 e 23/11.
- **Situação de prazo (levantada em 17/09/2026; hoje é 01/10/2026, véspera da data de integração):** adiantado no escopo próprio — avaliação, minimax,
  alfa-beta e chamadora já fechados em pseudocódigo, semanas antes das datas do Gantt. O risco
  está fora do meu escopo: a máquina de regras está incompleta e o perft **não tem data**.

**Commits:** padrão `docs(IA): ...`, título sozinho na primeira linha sem ponto final, corpo
opcional separado por linha em branco.

---

## 3. Decisões tomadas

**Busca (etapa 1):**
- Convenção de valores: brancas positivas, pretas negativas; max/min literais.
- Sem parâmetro `jogador`: vez lida de `estado.side_to_move`.
- Empates por regra automáticos (convenção Chess.com): a partida termina antes de a IA ser
  chamada; **a raiz nunca é terminal** (chamador verifica).
- `profMax` global, `> 0`. Justificativa: tempo (limitante prático) e pilha (limitante teórico).
  **Não muda durante a execução** — valor definido após testes. Por isso `LIMIAR_MATE` pode ser
  constante.
- Lista de lances gerada e `undo` locais ao nó.
- Ordem do caso base: empate por regra → limite de profundidade → gerar lances → lista vazia
  (xeque = mate, senão afogamento) → laço.

**Avaliação v1:**
- Só material; PST e tapered ficam para v6+.
- Valores de Michniewski em centipeões: P 100, C 320, B 330, T 500, D 900, R 0.
- Tabela de 7 posições, índice = `PIECE_TYPE` (posição 0 = 0).
- Varredura das 64 casas; `EMPTY` pulada; sinal por desvio condicional comparando a cor pelo nome.
- Dois acumuladores **locais** (brancas, pretas); retorno = brancas − pretas (inteiro).
  Estrutura global com os totais foi **abandonada** (seria sobrescrita a cada folha).
- Faixa: ±4000 (material inicial); ±10400 (com promoções, v1); ±10615 (com PSTs de
  Michniewski, pior caso, v6+ — margem adotada).

**Valores terminais:**
- `MATE = 49999`; mate vale `±(MATE − profundidade)`, sinal pelo **lado derrotado**
  (negativo se as brancas levam mate, positivo se as pretas). Conferir os dois ramos.
- `INFINITO = 50000`; `melhor` inicializa em `−INFINITO` (brancas) / `+INFINITO` (pretas).
- Empate (regra ou afogamento) = 0.
- Ordem em módulo: avaliação < mate com desconto < INFINITO.
- Refinamento futuro possível (não v1): contempt.

**Alfa-beta:**
- Alfa e beta são parâmetros; valores iniciais (`ALFA = −INFINITO`, `BETA = +INFINITO`)
  definidos na chamadora.
- Na raiz, alfa e beta **não são atualizados** (janela completa → valores exatos para
  `listaCandidatos`). Com isso o corte nunca dispara na raiz.
- O bloco de comentário que justifica isso **está no `alfabeta.txt` do repositório** (conferido).
  Decisão: ele fica só lá, junto da condição `profundidade > 0` que explica; **não duplicar na
  chamadora**. Se quiser vínculo, uma linha de referência na chamadora, não cópia.

**Chamadora `escolherJogada` (etapa 3 — fechada):**
- Não retorna nada: grava tudo na estrutura global `JOGADA` (lance, score, tipo).
  Global escolhida conscientemente — o score da raiz é escrito **uma vez por busca**, não é
  estado de nó, e já há precedente na `listaCandidatos`. Não é o meu padrão de erro.
- Validade de `variante` e de `randomizador`, e a garantia de raiz não terminal, são
  **pré-condições** verificadas por uma função externa (ainda não nomeada, não existe).
- Score convertido para o formato do UCI dentro da chamadora: perspectiva de quem joga e
  `mate N` em lances completos.

**Ponder e threads: descartados.** Avaliados e rejeitados — ponder exige suporte no protocolo e
depende de acertar o lance do humano; threads paralelizam mal a alfa-beta, dependem de TT
compartilhada (fora do MVP) e conflitam com os testes determinísticos do grupo. Se um dia
entrarem, a refatoração correta é agrupar tudo que é "de uma busca" (listaCandidatos, profMax,
score, contadores) num **contexto passado por parâmetro**, com um tabuleiro por thread — e aí a
global do score é o menor dos problemas.

**Ordenação (item 7):**
- Nota do lance em **estrutura própria da IA (lance + nota)**; a lista desse tipo é **local a
  cada nó**. Critério previsto: MVV-LVA (não usar `avaliar`: caro e cego a recaptura).

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo do minimax~~ ✅
2. ~~Função de avaliação v1~~ ✅
3. ~~Valores de empate, mate com desconto, inicialização de `melhor`~~ ✅
4. ~~Pseudocódigo da alfa-beta~~ ✅
5. ~~Função chamadora `escolherJogada`~~ ✅
6. **Acertar as seis decisões com a máquina de regras (§9)** ← próximo
7. **Primeira implementação da IA em C (§10)** ← próximo
8. Aprofundamento iterativo + controle de tempo (`go movetime`, níveis de dificuldade) — mora
   na chamadora (laço com profundidade crescente)
9. Ordenação de lances (MVV-LVA)
10. Quiescência
11. Tabela de transposição (opcional, fora do MVP)

**Decisões já antecipadas para o aprofundamento iterativo:**
- A profundidade da iteração será uma **variável com outro nome**, não `profMax` (que fica como
  teto fixo). Consequência: o caso base de `minimax` e de `podaAlfaBeta` passa a comparar com
  essa nova variável — **atualizar os dois** (ramo espelhado).
- Em aberto: **ativação condicional** dentro da busca **ou variante separada** do algoritmo.
  Variante separada implica constante e ramo a mais no desvio da chamadora, além de listá-la no
  comentário da pré-condição; ativação condicional mantém a assinatura, ao custo de um teste no
  caminho quente.

---

## 5. minimax.txt (fechado)

```
-- testeEmpateRegra(estado) é a função que testa se o estado atual no tabuleiro resulta em um empate pelas regras, de 50 lances, tripla repetição e material insuficiente
-- gerarLancesLegais(estado) é a função que gera todos os lances válidos, dado um estado
-- emXeque(estado) é a função que testa se o rei do lado da vez está em xeque, dado um estado
-- profMax é a variável global que estipula a profundidade máxima do minimax, dado que a pilha não é infinita e o tempo é limitado (profMax deve ser maior que 0)
-- listaCandidatos variável global que guarda a lista de lances possíveis para serem escolhidos, todos empatados no melhor valor, com base nos critérios do minimax. Preenchida apenas na raiz da árvore de busca
-- Valores utilizados como limite para os estados intermediários (ou seja, que não são fim de partida) no retorno de avaliar explicitados em avaliar.txt
-- O valor retornado em um xeque-mate passa a ser (MATE - profundidade), com sinal, sendo negativo se as brancas perdem, positivo se as pretas perdem e a inicialização será feita em + ou - INFINITO. Empate foi convencionado como 0

MATE = 49999
INFINITO = 50000

função minimax(estado, profundidade):
    se testeEmpateRegra(estado) então
        retorna 0
    senão se profMax <= profundidade então
        retorna avaliar(estado)
    senão
        listaGerada = gerarLancesLegais(estado)
        se listaGerada == vazio então
            se emXeque(estado) então
                se estado.side_to_move == WHITE então
                    retorna -(MATE - profundidade)
                senão retorna MATE - profundidade
            senão retorna 0
        senão:
            se estado.side_to_move == WHITE faça
                melhor = -INFINITO
                para todo lance em listaGerada faça
                    undo = novo registro
                    make_move(estado, lance, undo)
                    atual = minimax(estado, profundidade + 1)
                    unmake_move(estado, lance, undo)

                    se atual > melhor então
                        melhor = atual
                        se profundidade == 0 então
                            listaCandidatos.limpar()
                            listaCandidatos.adiciona(lance)
                    senão se atual == melhor e profundidade == 0 então:
                        listaCandidatos.adiciona(lance)
                retornar melhor
            senão:
                melhor = INFINITO
                para todo lance em listaGerada faça
                    undo = novo registro
                    make_move(estado, lance, undo)
                    atual = minimax(estado, profundidade + 1)
                    unmake_move(estado, lance, undo)

                    se atual < melhor então
                        melhor = atual
                        se profundidade == 0 então
                            listaCandidatos.limpar()
                            listaCandidatos.adiciona(lance)
                    senão se atual == melhor e profundidade == 0 então:
                        listaCandidatos.adiciona(lance)
                retornar melhor
```

**Pendências conhecidas (não são erros agora):**
- Mate exatamente no último nível é avaliado como posição normal (quiescência atenua).
- Raiz misturada aos nós internos (`listaCandidatos` global).
- Modo determinístico — **resolvido na chamadora** (`randomizador`).

---

## 6. avaliar.txt (fechado)

```
-- Valores atribuídos às peças, em módulo (Valores de centipeão de Michniewski adotados. Brancas > 0 e pretas < 0): Peão = 100, Cavalo = 320, Bispo = 330, Torre = 500, Dama = 900 e Rei = 0 (Rei zero para não afetar os valores de estado dos tabuleiros, visto que o jogo acaba com o xeque-mate, então ele sempre está no tabuleiro)

-- Com as ponderações da PSTs de Michniewski para peso por posição, temos o maior valor da diferença, em módulo, retornado pela função avaliar(estado) sendo 10615 (cenário em que um lado tem apenas um rei e o outro tem nove rainhas, com oito delas sendo peões promovidos, duas torres, dois cavalos e dois bispos, todos posicionados nas melhores posições possíveis). Não será implementado ainda, sendo usado agora o limiar de 10400 (com sinal, dependendo do lado), sem as ponderações por peso

-- tabelaValores = [0, 100, 320, 330, 500, 900, 0] tal que o índice da tabela é PIECE_TYPE(p)

função avaliar(estado):
    totBrancas = 0
    totPretas = 0
    para toda casa = 0...63:
        se estado.array[casa] != EMPTY então:
            se PIECE_COLOR(estado.array[casa]) == WHITE então
                totBrancas += tabelaValores[PIECE_TYPE(estado.array[casa])]
            senão totPretas += tabelaValores[PIECE_TYPE(estado.array[casa])]

    retorna totBrancas - totPretas
```

---

## 7. alfabeta.txt (fechado)

```
-- alfa e beta são, respectivamente, piso do maximizador e teto do minimizador.

-- Todo o bloco inicial de comentários de minimax.txt é válido aqui, além dos valores MATE e INFINITO

-- Na raiz (profundidade == 0), alfa e beta não são atualizados: os filhos da raiz
-- recebem a janela completa e devolvem valores exatos, não limites. Isso é necessário
-- porque listaCandidatos compara valores por igualdade; com a janela apertada, um
-- filho pode devolver um limite igual a melhor e entrar como empate sem ser.
-- Pressupõe chamada inicial com alfa = -INFINITO e beta = +INFINITO. Se a raiz
-- passar a receber janela estreita (ex.: aspiration windows no aprofundamento
-- iterativo), o corte pode ocorrer na raiz e esta garantia deixa de valer.

função podaAlfaBeta(estado, profundidade, alfa, beta):
    se testeEmpateRegra(estado) então
        retorna 0
    senão se profMax <= profundidade então
        retorna avaliar(estado)
    senão
        listaGerada = gerarLancesLegais(estado)
        se listaGerada == vazio então
            se emXeque(estado) então
                se estado.side_to_move == WHITE então
                    retorna -(MATE - profundidade)
                senão retorna MATE - profundidade
            senão retorna 0
        senão:
            se estado.side_to_move == WHITE faça
                melhor = -INFINITO
                para todo lance em listaGerada faça
                    undo = novo registro
                    make_move(estado, lance, undo)
                    atual = podaAlfaBeta(estado, profundidade + 1, alfa, beta)
                    unmake_move(estado, lance, undo)

                    se atual > melhor então
                        melhor = atual
                        se melhor > alfa e profundidade > 0 então
                            alfa = melhor
                        se profundidade == 0 então
                            listaCandidatos.limpar()
                            listaCandidatos.adiciona(lance)
                    senão se atual == melhor e profundidade == 0 então:
                        listaCandidatos.adiciona(lance)

                    se alfa >= beta então:
                        interromper
                retornar melhor
            senão:
                melhor = INFINITO
                para todo lance em listaGerada faça
                    undo = novo registro
                    make_move(estado, lance, undo)
                    atual = podaAlfaBeta(estado, profundidade + 1, alfa, beta)
                    unmake_move(estado, lance, undo)

                    se atual < melhor então
                        melhor = atual
                        se melhor < beta e profundidade > 0 então
                            beta = melhor
                        se profundidade == 0 então
                            listaCandidatos.limpar()
                            listaCandidatos.adiciona(lance)
                    senão se atual == melhor e profundidade == 0 então:
                        listaCandidatos.adiciona(lance)

                    se alfa >= beta então:
                        interromper
                retornar melhor
```

---

## 8. escolherJogada.txt (fechado)

```
-- Convenção adotada: VARIANTE_MINIMAX representa minimax puro, VARIANTE_ALFA_BETA representa poda alfa-beta. Esta convenção será usada dentro da função escolherJogada(estado, variante, randomizador)

-- A chamada de escolherJogada dentro da implementação tem a seguinte pré-condição: validade do parâmetro variante que foi passado para a função. Esta validação irá ocorrer em uma função externa, que chamará escolherJogada(estado, variante, randomizador). Os valores válidos são VARIANTE_ALFA_BETA e VARIANTE_MINIMAX. A raiz deve ser nó não terminal, e esta garantia deve ser dada pela função que chamar escolherJogada(estado, variante, randomizador)

-- alfa e beta convencionados como -INFINITO e +INFINITO, respectivamente, para ficarem inicialmente fora do intervalo de valores possíveis do tabuleiro

-- Na função escolherJogada, o parâmetro randomizador deve receber SORTEIO_DESLIGADO ou SORTEIO_LIGADO, para desativar/ativar o sorteio da lista de listaCandidatos (a validade do valor passado será verificada na função externa que chama escolherJogada(estado, variante, randomizador) e testa pré-condição da raiz não terminal. Esta função externa ainda não foi nomeada, e não existe ainda).

-- JOGADA é a estrutura que contém uma jogada com lance (tipo Move), score (alterado pela variante do minimax que foi executada) e tipo de score (se é mate, centipeão ou movimento inválido)
-- JOGADA.tipo deve ser TIPO_MATE (representando xeque-mate), TIPO_CENTIPEAO (representando centipeão) ou TIPO_INVALIDO (representando movimento inválido)
-- Caso LANCE_NULO seja gravado em JOGADA pela função escolherJogada, não houve jogada válida. A função que chamará escolherJogada(estado, variante, randomizador) deve verificar se LANCE_NULO foi gravado em JOGADA antes de usar o movimento, já que LANCE_NULO representa a1->a1, que é um movimento inválido. LANCE_NULO é gravado apenas se a raiz for terminal, violando a pré-condição
-- Os valores de JOGADA são válidos após o término de uma chamada de escolherJogada(estado, variante, randomizador)

VARIANTE_MINIMAX = 0
VARIANTE_ALFA_BETA = 1

ALFA = -INFINITO
BETA = INFINITO

LANCE_NULO = 0 -- gravado em JOGADA.lance quando a lista de candidatos é vazia, mesmo após a execução de minimax(estado, profundidade) ou de podaAlfaBeta(estado, profundidade, alfa, beta)
SCORE_INVALIDO = 60000 -- valor default ilustrativo de score, para quando o lance for inválido

TIPO_INVALIDO = 0 -- Tipo que representa uma jogada inválida
TIPO_CENTIPEAO = 1
TIPO_MATE = -1


LIMIAR_MATE = MATE - profMax

SORTEIO_LIGADO = 1
SORTEIO_DESLIGADO = 0

função escolherJogada(estado, variante, randomizador):
    listaCandidatos.limpar()
    JOGADA.lance = LANCE_NULO
    JOGADA.score = SCORE_INVALIDO
    JOGADA.tipo = TIPO_INVALIDO

    se variante == VARIANTE_MINIMAX então
        JOGADA.score = minimax(estado, 0)
    senão se variante == VARIANTE_ALFA_BETA então
        JOGADA.score = podaAlfaBeta(estado, 0, ALFA, BETA)

    se estado.side_to_move == BLACK então JOGADA.score *= -1

    se listaCandidatos != lista vazia então:
        se LIMIAR_MATE <= módulo(JOGADA.score) então
            JOGADA.tipo = TIPO_MATE

            se JOGADA.score > 0 então
                sinalMate = 1
            senão sinalMate = -1

            JOGADA.score = sinalMate * ((MATE - módulo(JOGADA.score) + 1)/2) -- esta linha considera a divisão em C, que é sempre inteira
            -- Aqui conta lances completos ao invés de meio-lance (este é contado por profundidade)

        senão JOGADA.tipo = TIPO_CENTIPEAO

        se randomizador == SORTEIO_LIGADO então
            JOGADA.lance = listaCandidatos[random(0, tamanho(listaCandidatos) - 1)] -- Supõe intervalo fechado nos dois extremos
        senão:
            JOGADA.lance = listaCandidatos[0]

    se JOGADA.lance == LANCE_NULO então
        JOGADA.tipo = TIPO_INVALIDO
        JOGADA.score = SCORE_INVALIDO
```

**Por que a conversão de score mora aqui (e o que ela faz):**
- **Perspectiva.** Minha convenção interna é absoluta (brancas positivas). O UCI reporta do
  ponto de vista de **quem está na vez** — por isso a inversão quando `side_to_move == BLACK`.
  Com as brancas na vez as duas coincidem, então o bug só apareceria em metade dos casos
  (ramo espelhado).
- **Ordem obrigatória:** busca → inversão de perspectiva → classificação pelo limiar →
  conversão de N **só** no ramo de mate. Inverter essa ordem quebra tudo: converter antes de
  classificar torna `TIPO_MATE` código morto, e classificar antes de inverter dá o sinal errado.
- **Reconhecer mate é por faixa**, não por igualdade com `MATE` (o valor é `MATE − profundidade`,
  e pode ser negativo).
- **`mate N` conta lances completos; profundidade conta meios-lances.** Com
  `p = MATE − módulo(score)`: p=1→N=1, p=2→N=1, p=3→N=2, p=4→N=2, p=5→N=3. Daí `(p+1)/2`.
  p ímpar = mate a favor, p par = mate contra.
- **Sinal de N** é capturado antes do módulo (`sinalMate`) e reaplicado depois. Sem isso, a
  interface nunca mostra que o motor **leva** mate.
- **Parentização:** `sinalMate * ((...)/2)`, não `sinalMate * (...)/2` — em C, `*` e `/` têm a
  mesma precedência e associam à esquerda.

**Recomendações não adotadas (decisão consciente, não esquecimento):**
- Notação `alfa = -INFINITO` no `alfabeta.txt` em vez de citar as constantes `ALFA`/`BETA` da
  chamadora — mantido como está.

---

## 9. Seis decisões a acertar com a máquina de regras (próximo passo)

**Maiores:**

1. **Congelamento dos cabeçalhos.** Preciso de `board.h`, `piece.h` e das assinaturas de
   `make_move`/`unmake_move` estáveis, ou de um commit fixo contra o qual escrever, com aviso
   quando mudarem. É o que protege o código que vou escrever sem poder rodar.
2. **As funções do meu contrato que ainda não existem:** `gerarLancesLegais`, `emXeque`,
   `testeEmpateRegra`. Acordar nomes, assinaturas e **onde a `MoveList` é alocada** — pressuponho
   parâmetro de saída, para manter a lista local ao nó.
3. **Data prevista do perft.** Hoje não há data no cronograma, apesar de a integração estar
   marcada para 02/10, e o perft é o que libera a IA em C.
4. **Semântica de `testeEmpateRegra`.** Tripla repetição exige **histórico de posições**, que não
   está entre os campos do `Board`. Confirmar se as regras vão mantê-lo e como; sem isso, o
   primeiro teste do meu caso base fica sem base.

**Menores:**

5. **Nomes das macros de leitura do `Move`** e se já existe um **lance nulo** definido no
   repositório (se existir, usar o dele em vez de declarar `LANCE_NULO`, para não duplicar).
6. **Zobrist**, se um dia entrar: precisa ser incremental dentro de `make_move`/`unmake_move` —
   decisão do lado das regras.

---

## 10. Primeira implementação em C — plano

**Regra do grupo respeitada:** escrevo e **compilo**, mas não rodo a busca até o perft bater.
Compilar já pega uma classe de erro que o pseudocódigo esconde: tipos, nomes de macro,
assinaturas, sinais, largura de inteiros.

**O que dá para implementar e testar já, sem o gerador:**
- `avaliar` — testável com posições em FEN montadas à mão.
- A conversão de score/`JOGADA` — lógica pura sobre inteiros, testável com valores fabricados
  (incluindo os casos p=1..5 da tabela acima e o caso do motor levando mate).
- Constantes, tabela de valores, estrutura `JOGADA`.

**O que fica escrito mas sem validação até o perft:** `minimax`, `podaAlfaBeta` e a chamadora
como um todo — todas dependem de `gerarLancesLegais`/`make_move`/`unmake_move`. Com
`unmake_move` vazio, a primeira descida já corrompe o tabuleiro.

**O que é o perft, para referência:** conta as posições folha a partir de uma posição, até uma
profundidade dada, e compara com valores publicados (posição inicial: 20, 400, 8.902, 197.281,
4.865.609). É teste **do gerador de lances**, não da IA — não depende de nada meu. Cobre roque,
en passant, promoção, cravada e xeque de uma vez, e testa o `unmake_move` junto. Rodando por
divisão (perft de cada lance da raiz separadamente) localiza o lance onde diverge.

**Riscos a vigiar durante a implementação:**
- Código escrito e nunca executado acumula bugs que aparecem todos juntos no dia em que roda —
  daí testar `avaliar` e a conversão desde já.
- Mudança de assinatura ou semântica do gerador enquanto escrevo (ver decisão 1).
