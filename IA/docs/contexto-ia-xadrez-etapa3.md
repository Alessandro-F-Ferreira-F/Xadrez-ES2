# Contexto para continuidade — IA de xadrez (etapa 3: função chamadora em diante)

Cole isto no início de uma nova conversa. Substitui o `contexto-ia-xadrez-etapa2.md`.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

Implementação em **C11**. Já estão fechados (e commitados) os pseudocódigos de: minimax,
avaliação v1 e poda alfa-beta (§5–§7). **Em andamento: função chamadora `escolherJogada`** (§8).

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
  renomear a função e a chamada recursiva continuar com o nome antigo; comentário citando
  assinatura antiga).
- Fundir casos distintos num mesmo retorno.
- Estado que deveria ser local ao nó declarado como global.
- Informação duplicada em dois lugares (comentário repetindo valor de constante, constante
  definida em dois arquivos).

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
- `Move` = `u16` (6 bits origem, 6 destino, 4 tipo). Valor, não estrutura. Sem campo de score.
  Conferir no repositório os nomes das macros de leitura e se existe um "lance nulo"
  (0 = a1→a1 nunca é legal).
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
  lance". **Ambos quebram com desempate aleatório** → preciso de modo determinístico.
- Invariante: `bestmove` sempre pertence à lista legal.
- Gantt (IA): integração com regras até 02/10; avaliação 05/10–12/10; minimax 13/10–02/11;
  alfa-beta, ordenação e aprofundamento em paralelo 03/11–20/11. Apresentações: 05/10 e 23/11.

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
- Lista de lances gerada e `undo` locais ao nó.
- Ordem do caso base: empate por regra → limite de profundidade → gerar lances → lista vazia
  (xeque = mate, senão afogamento) → laço.

**Avaliação v1:**
- Só material; PST e tapered ficam para v6+.
- Valores de Michniewski em centipeões: P 100, C 320, B 330, T 500, D 900, R 0.
- Tabela de 7 posições, índice = `PIECE_TYPE` (posição 0 = 0).
- Varredura das 64 casas; `EMPTY` pulada; sinal por desvio condicional comparando a cor pelo nome.
- Dois acumuladores **locais** (brancas, pretas); retorno = brancas − pretas (inteiro).
  Estrutura global com os totais foi **abandonada** (seria sobrescrita a cada folha). Se o
  chamador precisar dos totais, usar função auxiliar ou variável local.
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

**Ordenação (formalizar no item 6):**
- Nota do lance em **estrutura própria da IA (lance + nota)**; a lista desse tipo é **local a
  cada nó**. Critério previsto: MVV-LVA (não usar `avaliar`: caro e cego a recaptura).

---

## 4. Ordem de trabalho

1. ~~Pseudocódigo do minimax~~ ✅
2. ~~Função de avaliação v1~~ ✅
3. ~~Valores de empate, mate com desconto, inicialização de `melhor`~~ ✅
4. ~~Pseudocódigo da alfa-beta~~ ✅
5. **Função chamadora `escolherJogada`** ← em andamento (§8)
6. Aprofundamento iterativo + controle de tempo (`go movetime`, níveis de dificuldade) — mora
   na chamadora (laço com `profMax` crescente)
7. Ordenação de lances (MVV-LVA)
8. Quiescência
9. Tabela de transposição (opcional, fora do MVP)

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
- Modo determinístico (resolver na chamadora).

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

(Confirmar se o bloco de comentário sobre a raiz foi incluído no arquivo real.)

---

## 8. Função chamadora (em andamento)

**Estado atual do pseudocódigo:**

```
-- Convenção adotada: 0 representa minimax puro, 1 representa poda alfa-beta. Esta convenção será usada dentro da função escolherJogada(estado, variante)
-- alfa e beta convencionados como -INFINITO e +INFINITO, respectivamente, para ficarem inicialmente fora do intervalo de valores possíveis do tabuleiro

VARIANTE_MINIMAX = 0
VARIANTE_ALFA-BETA = 1

ALFA = -INFINITO
BETA = INFINITO

função escolherJogada(estado, variante):
    listaCandidatos.limpar()
```

**Já resolvido:**
- Entrada: `estado` e `variante` (`profMax` é global, não é parâmetro).
- Escolha da busca: parâmetro `variante` com constantes nomeadas.
- Limpeza da `listaCandidatos` antes da busca.
- Constantes `ALFA`/`BETA`.
- `minimax` e `podaAlfaBeta` **não mudam**: continuam devolvendo inteiro; só a chamadora
  devolve `Move`, tirado da `listaCandidatos`.

**Recomendações pendentes (não são erros):**
- `VARIANTE_ALFA-BETA` não é identificador válido em C (hífen) → `VARIANTE_ALFABETA` ou
  `VARIANTE_ALFA_BETA`.
- O comentário cita 0/1; citar os nomes das constantes evita duas fontes.

**Pontos ainda a resolver:**
1. **Saída:** devolve um `Move`.
2. **Chamada da busca:** desvio por `variante` chamando `minimax(estado, 0)` ou
   `podaAlfaBeta(estado, 0, ALFA, BETA)`; decidir o que fazer com `variante` inválida.
3. **Retorno da busca:** descartar ou guardar o valor da raiz (útil para `score cp`).
4. **Lista vazia:** tratamento de violação de pré-condição (possível "lance nulo").
5. **Escolha do lance:** sorteio uniforme entre candidatos ou, no modo determinístico, regra
   fixa (ex.: o primeiro); decidir como o modo é ligado.
