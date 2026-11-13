# Contexto para continuidade — IA de xadrez (etapa 2: avaliação em diante)

Cole isto no início de uma nova conversa.

---

## Quem sou e o que estou fazendo

Estudante de computação. Já implementei minimax e alfa-beta do zero em C para jogo da velha,
com instrumentação e análise empírica. Agora faço parte de um **projeto em grupo** (disciplina
Engenharia de Software 2, repositório `Xadrez-ES2`, branch `IA`) e **minha responsabilidade é
só a IA**. Máquina de regras e interface são de outros membros.

A implementação será em **C11**. O pseudocódigo do minimax (etapa 1) está fechado — ver §5.
**Próximo passo: função de avaliação**, depois valores de terminais, alfa-beta e o resto (§6).

## Como quero que você conduza

- Respostas **sucintas e tecnicamente precisas**. Sem preâmbulo, sem repetir o que eu disse.
- **Não me dê a solução pronta**: aponte onde está o erro e por quê, e me deixe corrigir. Só
  escreva código ou pseudocódigo se eu pedir explicitamente. Exceção para o que não é
  algorítmico (textos, configuração).
- Eu **decido o escopo e a ordem**. Sugira, mas não assuma.
- Vou **discordar** quando achar que você errou; reconsidere de verdade, e mantenha a posição
  se estiver certo.
- Quando eu pedir para aguardar, aguarde. Quando eu pedir para ignorar algo, ignore.
- Mantenho o **pseudocódigo como especificação viva**; valide-o a cada versão.
- Separe **erro** de **recomendação**. Não insista em recomendação como se fosse erro.

**Padrões de erro meus para vigiar:**
- Ramos espelhados corrigidos pela metade — e, ao uniformizar, copiar o ramo errado para o
  certo em vez do contrário.
- Consumidores não atualizados depois que mudo uma assinatura ou uma decisão (ex.: tirar um
  parâmetro e ele continuar sendo usado).
- Fundir casos distintos num mesmo retorno (ex.: limite de profundidade e empate).
- Estado que deveria ser local ao nó declarado como global.

---

## 1. O que já domino (do jogo da velha)

- Minimax: valor recursivo do nó, alternância max/min, valor que sobe vs. jogada da raiz.
- Estado mutável com fazer/desfazer em vez de cópia.
- Alfa-beta: alfa piso do max, beta teto do min, corte quando a janela fecha; mesmo valor do
  minimax; ganho depende da ordenação.
- Desconto por profundidade em escala inteira (`±(N - profundidade)`).
- Desempate aleatório entre jogadas de valor máximo, com lista de candidatos na raiz.

**Armadilhas que já me morderam:**
- Sob alfa-beta, o valor de um filho pode ser **limite**, não exato. A lista de candidatos só
  funciona mantendo **janela completa nos filhos da raiz** (poda ativa abaixo). Podar na raiz
  não trouxe ganho e custou a variedade.
- `melhor` deve ser inicializado **fora da faixa** de valores possíveis (com desconto/mate, a
  faixa muda).
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
- `Board` (`board.h`): mailbox `Piece array[64]`, **a1 = 0**; `side_to_move` (tipo `Color`);
  `king_square[2]`; `castling_rights`; `ep_square`; `halfmove_clock`; `fullmove_number`.
- `Color` (`piece.h`): **`WHITE = 1`, `BLACK = 0`**. Comparar sempre com os nomes.
- `Piece` = `u8`, `PIECE_TYPE`/`PIECE_COLOR`/`PIECE_MAKE`; tipos `PAWN=1 … KING=6`.
  **`PIECE_COLOR(EMPTY) == BLACK`** — na avaliação, usar `is_own`/`is_enemy`, nunca a cor crua.
- `Move` = `u16` (6 bits origem, 6 destino, 4 tipo); `MoveList` com `moves[256]` e `count`.
  **`Move` não tem campo de score** — decisão em aberto, deixada para a IA.
- `void make_move(Board *b, Move m, Undo *u)` / `void unmake_move(Board *b, Move m, const Undo *u)`.
  `Undo` é alocado por quem chama; `make_move` o preenche.

**Contrato que a IA precisa da máquina de regras (nomes do pseudocódigo):**
`gerarLancesLegais(estado)`, `emXeque(estado)`, `testeEmpateRegra(estado)`, `make_move`,
`unmake_move`, leitura das peças para a avaliação. Hash Zobrist, se entrar, deveria ser
incremental dentro de make/unmake (código deles).

**Requisitos e docs do grupo que afetam a IA:**
- RF03: níveis de dificuldade. RNF01: resposta < 200 ms nos níveis básicos. Protocolo usa
  `go movetime` / `go depth`.
- Roadmap do grupo (Fase 7): v1 material + minimax prof. 3 → v2 alfa-beta → v3 aprofundamento
  iterativo + movetime → v4 ordenação MVV-LVA → v5 quiescência → v6+ PST, avaliação tapered, TT.
- **Tabela de transposição está fora do MVP** (primeira da ordem de sacrifício).
- **Quiescência não está na EAP (1.5) nem no Gantt**, mas o roadmap diz que não se corta —
  levar ao grupo.
- Testes previstos pelo grupo: "v2 devolve o mesmo lance que v1" e "mesma FEN + profundidade →
  mesmo lance". **Ambos quebram com desempate aleatório** → preciso de modo determinístico.
- Invariante previsto: `bestmove` sempre pertence à lista legal.
- Gantt (IA): integração com regras até 02/10; avaliação 05/10–12/10; minimax 13/10–02/11;
  alfa-beta, ordenação e aprofundamento em paralelo 03/11–20/11. Apresentações: 05/10 e 23/11.

---

## 3. Decisões tomadas na etapa 1

- **Convenção de valores:** brancas positivas, pretas negativas; max/min literais.
- **Sem parâmetro `jogador`:** a vez é lida de `estado.side_to_move` (o `make_move` já a
  inverte). Duas fontes da mesma informação podem divergir.
- **Empates por regra seguem a convenção do Chess.com:** 50 lances e repetição tripla são
  **automáticos** (material insuficiente e afogamento também). Consequência: a partida termina
  antes de a IA ser chamada; **a raiz nunca é posição terminal** (responsabilidade de quem
  chama; verificar lista de candidatos vazia no chamador).
- **`profMax > 0`** é pré-condição.
- **Lista de lances gerada e `undo` são locais ao nó** (um `undo` por iteração do laço).
- **`avaliar(estado)` não recebe profundidade.** Desconto por profundidade só no valor de mate.
- **Ordem do caso base:** empate por regra → limite de profundidade → gerar lances → lista
  vazia? (em xeque = mate; senão = afogamento) → laço.

---

## 4. Ordem de trabalho combinada

1. ~~Pseudocódigo do minimax~~ ✅
2. **Função de avaliação** ← próximo
3. Valores concretos: empate, mate com desconto, inicialização de `melhor`
4. Alfa-beta (com janela completa na raiz, por causa dos candidatos)
5. Aprofundamento iterativo + controle de tempo (`go movetime`, níveis de dificuldade)
6. Ordenação de lances (MVV-LVA)
7. Quiescência
8. Tabela de transposição (opcional, fora do MVP)

Itens 3 e 2 podem ser feitos juntos: a escala da avaliação (centipeões) define onde fica o
valor de mate e a inicialização de `melhor`.

---

## 5. Pseudocódigo do minimax (etapa 1, fechado)

Valores (`empate`, `derrota do jogador da vez`, ±3) ainda são ilustrativos; só o sinal está
convencionado.

```
-- testeEmpateRegra(estado): testa se o estado atual resulta em empate por regra
-- gerarLancesLegais(estado): gera todos os lances legais do estado
-- emXeque(estado): testa se o rei do lado da vez está em xeque
-- profMax: global; profundidade máxima da busca (deve ser maior que 0)
-- listaCandidatos: global; lances da raiz que empatam no melhor valor
--                  (preenchida só quando profundidade == 0)

função minimax(estado, profundidade):
    se testeEmpateRegra(estado) então
        retorna empate
    senão se profMax <= profundidade então
        retorna avaliar(estado)
    senão
        listaGerada = gerarLancesLegais(estado)
        se listaGerada == vazio então
            se emXeque(estado) então retorna derrota do jogador da vez
            senão retorna empate
        senão:
            se estado.side_to_move == WHITE faça
                melhor = -3
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
                melhor = 3
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

**Pendências conhecidas neste pseudocódigo (não são erros agora):**
- Limite de profundidade é testado antes da geração: um mate exatamente no último nível é
  avaliado como posição normal. Aceitável; quiescência atenua depois.
- Valores de mate/empate e inicialização de `melhor` (item 3 da ordem).
- Tratamento da raiz ainda misturado ao dos nós internos (`listaCandidatos` global);
  reavaliar ao introduzir alfa-beta e aprofundamento iterativo.
- Modo determinístico (desligar o sorteio) para os testes do grupo.

---

## 6. Por onde começar a próxima conversa

Discutir a **função de avaliação** (`avaliar(estado)`): o que ela mede (material; depois
posicional/PST), em que escala (centipeões), como o sinal segue a convenção brancas-positivas,
como ler as peças do `Board` sem cair na armadilha `PIECE_COLOR(EMPTY) == BLACK`, e como essa
escala fixa o valor de mate e a inicialização de `melhor`.
