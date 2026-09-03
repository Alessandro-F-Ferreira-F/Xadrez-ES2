# Biblioteca de Referências — Programação de um Chess Engine

> Complementa `arquitetura-xadrez.md` e `descricao-projeto-xadrez.md`. Reúne artigos, páginas de referência, engines open source e tutoriais organizados pelos temas que vocês vão efetivamente implementar: representação do tabuleiro, geração/validação de lances, busca e avaliação — mais uma seção específica sobre o protocolo UCI, já que é a decisão de integração C ↔ Node do projeto.

## Como usar esta biblioteca

A maior parte do conteúdo vem da **Chess Programming Wiki (CPW)** — é, disparado, a referência mais citada por quem escreve engines, e o ponto de partida de quase todo projeto sério na área. Cada seção abaixo tem: (1) a página da CPW sobre o tema, (2) leituras complementares que explicam a mesma coisa de um jeito mais narrativo/aplicado, e (3) onde relevante, um exemplo de código real pra ver o conceito implementado de ponta a ponta.

No fim do documento tem uma **tabela de sequência sugerida**, mapeando cada recurso à etapa do roadmap de `arquitetura-xadrez.md` (§9) em que ele fica útil — assim vocês não precisam ler tudo de uma vez antes de começar a codar.

---

## 0. Ponto de partida

- **[Chess Programming Wiki](https://www.chessprogramming.org/)** — a wiki-mãe. Mais de 4 mil artigos cobrindo desde conceitos básicos até tópicos de pesquisa (redes neurais, busca paralela, etc.). Não precisa ler linearmente: usem como uma enciclopédia, voltando a ela cada vez que baterem numa dúvida específica de implementação.
- **[Getting Started (CPW)](https://www.chessprogramming.org/Getting_Started)** — a página "por onde começar" da própria wiki. Reforça exatamente a ordem que vocês já adotaram no roadmap: primeiro uma representação de tabuleiro **sem bugs**, só depois busca e interface — e recomenda o Perft (seção 2 abaixo) como a ferramenta de verificação central.

---

## 1. Representação do tabuleiro e das peças

| Recurso | O que traz |
|---|---|
| **[Board Representation (CPW)](https://www.chessprogramming.org/Board_Representation)** | Visão geral das famílias de representação — *piece-centric* (listas de peças) vs. *square-centric* (arrays indexados por casa, como o mailbox que vocês escolheram) vs. bitboards. Bom para entender o mapa geral antes de entrar no detalhe de cada uma. |
| **[Mailbox (CPW)](https://www.chessprogramming.org/Mailbox)** | A técnica específica que está no `arquitetura-xadrez.md`. A própria wiki recomenda mailbox como ponto de partida para quem está aprendendo, antes de partir pra bitboards — exatamente a lógica de "profundidade de busca boa o suficiente sem a complexidade extra" que está no doc de vocês. |
| **[8x8 Board (CPW)](https://www.chessprogramming.org/8x8_Board)** | O array linear de 64 posições puro — a versão mais simples do mailbox, sem casas-sentinela. |
| **[10x12 Board (CPW)](https://www.chessprogramming.org/10x12_Board)** | A variante com bordas-sentinela (casas fora do tabuleiro marcadas com um valor especial) que resolve o problema de "wrap-around" na geração de lances de peças que saltam ou deslizam. Vale ler mesmo decidindo pelo 8x8 puro — é a otimização natural caso a checagem de limites vire gargalo. |
| **[Forsyth-Edwards Notation — FEN (CPW)](https://www.chessprogramming.org/Forsyth-Edwards_Notation)** | Especificação formal do formato FEN, campo a campo (posição, lado a mover, direitos de roque, alvo de *en passant*, contador de 50 lances, número da jogada). É o formato que atravessa motor, backend e potencialmente o cliente no design de vocês — vale ler a especificação completa, não só exemplos. |
| **[FEN — explicação prática (Chess.com)](https://www.chess.com/terms/fen-chess)** | Mesma notação, explicada de forma mais didática/visual, boa como complemento antes de ler a especificação formal acima. |

---

## 2. Geração e validação das jogadas

| Recurso | O que traz |
|---|---|
| **[Move Generation (CPW)](https://www.chessprogramming.org/Move_Generation)** | A distinção central que o `arquitetura-xadrez.md` já adota: gerar lances **pseudo-legais** primeiro (obedecem ao movimento da peça, mas podem deixar o próprio rei em xeque) e filtrar a legalidade depois, separadamente — é mais simples de implementar corretamente do que gerar só lances 100% legais de uma vez. |
| **[Check (CPW)](https://chessprogramming.org/Check)** | Como detectar se um lance deixa (ou tira) o rei de xeque — a peça central do filtro de legalidade. Cobre xeque direto e xeque descoberto. |
| **[Square Attacked By (CPW)](https://chessprogramming.org/Square_Attacked_By)** | A técnica de "existe alguma peça adversária que ataca esta casa?" — usada tanto pra detectar xeque quanto pra validar roque (as casas que o rei atravessa não podem estar sob ataque). É o bloco de código que sustenta boa parte da legalidade. |
| **[Perft (CPW)](https://chessprogramming.org/Perft)** | Já está no `arquitetura-xadrez.md` como teste recomendado — aqui está a página de referência completa: o que é, como funciona, e por que é o padrão-ouro para achar bugs sutis (en passant errado, roque através de casa atacada, promoção mal tratada) que teste manual dificilmente pega. |
| **[Debugging a Chess Move Generator (chessprogramming.net)](https://www.chessprogramming.net/debugging-a-chess-move-generator/)** | Artigo prático de Steve Maughan com técnicas específicas de depuração via Perft: usar o comando `divide` pra isolar qual sublance está errado, testar a posição "espelhada" (brancas ↔ pretas) pra pegar bugs assimétricos, e usar asserts liberalmente. |
| **[perftree (GitHub)](https://github.com/agausmani/perftree)** | Ferramenta que compara o Perft do motor de vocês contra o do Stockfish automaticamente, destacando exatamente onde a árvore diverge — evita ter que comparar números manualmente contra tabelas de referência. |

---

## 3. Algoritmos de busca e avaliação

### 3.1 Busca

| Recurso | O que traz |
|---|---|
| **[Search (CPW)](https://www.chessprogramming.org/Search)** | Visão geral: por que quase todo engine usa alfa-beta em vez de minimax puro (ordem de magnitude mais rápido), e por que a ordenação de lances passa a importar muito quando se usa poda — no minimax puro ela não faz diferença, porque tudo é visitado de qualquer jeito. |
| **[Alpha-Beta (CPW)](https://chessprogramming.org/Alpha-Beta)** | A página de referência do algoritmo em si — poda por *branch-and-bound* que elimina ramos da árvore sem alterar o resultado do minimax. |
| **[Quiescence Search (CPW)](https://www.chessprogramming.org/Quiescence_Search)** | Não está explicitamente no roadmap de vocês, mas é quase indispensável na prática: busca alfa-beta parando numa profundidade fixa sofre do "efeito horizonte" (para de buscar bem no meio de uma sequência de capturas, achando que perdeu uma peça de graça). A busca de quiescência estende a busca só nos lances de captura até a posição "estabilizar". Vale considerar incluir na Etapa 3. |
| **[Move Ordering (CPW)](https://www.chessprogramming.org/Move_Ordering)** e **[MVV-LVA (CPW)](https://chessprogramming.org/MVV-LVA)** | A heurística que já está no `arquitetura-xadrez.md` (testar capturas mais valiosas primeiro) explicada em detalhe — por que ordenar bem os lances multiplica a eficácia da poda alfa-beta. |
| **[Zobrist Hashing (CPW)](https://chessprogramming.org/Zobrist_Hashing)** | Pré-requisito técnico pra transposition table (Etapa 7, opcional, conforme o roadmap): como gerar um "hash" quase-único pra cada posição usando XOR, de forma que fazer/desfazer um lance só exige atualizar o hash incrementalmente, sem recalculá-lo do zero. |

### 3.2 Avaliação

| Recurso | O que traz |
|---|---|
| **[Evaluation (CPW)](https://chessprogramming.org/Evaluation)** | Visão geral de como uma função de avaliação combina features (material, posição, estrutura de peões etc.) numa única pontuação. |
| **[Piece-Square Tables (CPW)](https://www.chessprogramming.org/Piece-Square_Tables)** | A técnica citada no `arquitetura-xadrez.md` §3: uma tabela de bônus/penalidade por casa, pra cada tipo de peça — captura conhecimento posicional (ex.: cavalo no centro vale mais que na borda) sem precisar codificar regras explícitas. |
| **[Simplified Evaluation Function (CPW)](https://www.chessprogramming.org/Simplified_Evaluation_Function)** | **Provavelmente o recurso mais direto pra Etapa 3.** É um conjunto completo e pronto de valores de peça + piece-square tables, publicado por Tomasz Michniewski especificamente como ponto de partida "material + PST" — o mesmo escopo que o `arquitetura-xadrez.md` define pra primeira versão da avaliação em C. Dá pra copiar os valores e já ter uma IA jogável, refinando depois. |

---

## 4. Protocolo UCI (motor ↔ backend)

Como o `arquitetura-xadrez.md` já define o protocolo texto entre C e Node como "inspirado no UCI", vale ler a especificação de verdade — o motor de vocês não precisa implementar o UCI inteiro (é um protocolo grande, pensado pra interoperar com qualquer GUI), mas entender o subconjunto relevante (`position`, `go`, `bestmove`, `isready`, `stop`) evita reinventar decisões que o protocolo já resolveu (como lidar com `\r\n` no Windows, ou como sinalizar que o motor está pronto pra receber comandos).

| Recurso | O que traz |
|---|---|
| **[Descrição oficial do protocolo UCI](http://page.mi.fu-berlin.de/block/uci.htm)** | O texto original de especificação, por Rudolf Huber e Stefan Meyer-Kahlen (autor do Shredder) — todos os comandos, formato de entrada/saída, e detalhes de portabilidade (como o próprio aviso de que `\n` pode vir como `0x0d` ou `0x0a0d` dependendo do SO, que é exatamente o cuidado que o `arquitetura-xadrez.md` já menciona). |
| **[Cópia da especificação no GitHub](https://github.com/fsmosca/UCIChessEngineProtocol)** | Mesmo texto, em formato mais fácil de navegar/pesquisar. |
| **[UCI (CPW)](https://www.chessprogramming.org/UCI)** | Contexto histórico e prático: por que o UCI virou o protocolo dominante (substituindo o antigo XBoard/WinBoard), e como a GUI (no caso de vocês, o backend Node) é responsável por manter o estado do jogo e arbitrar o resultado — o motor em si é *stateless* entre chamadas, reforçando a decisão de vocês de sempre mandar o FEN completo. |

---

## 5. Engines de referência — código real para estudar

Ler um engine pequeno e didático de ponta a ponta ajuda muito a enxergar como as peças (representação, geração, busca, avaliação, protocolo) se encaixam num programa de verdade. Os dois primeiros são em **C**, o que os torna a leitura mais diretamente aplicável ao motor final de vocês.

| Engine | Linguagem | Por que vale a pena |
|---|---|---|
| **[TSCP — Tom Kerrigan's Simple Chess Program](https://www.tckerrigan.com/Chess/TSCP/)** | C | Escrito em 1997 explicitamente como engine didático — talvez o exemplo mais citado da comunidade pra quem está começando. Usa representação 10x12 com sentinelas, muito próxima do que está descrito no `arquitetura-xadrez.md`. **Atenção de licença**: o código é aberto pra leitura e estudo, mas não é GPL — reaproveitamento/derivação exige permissão do autor, então tratem como material de leitura/inspiração, não como base de código a copiar diretamente. |
| **[VICE — Video Instructional Chess Engine (GitHub)](https://github.com/bluefeversoft/vice)** | C | Construído ao vivo numa série de **87 vídeos no YouTube** ("Programming a Chess Engine in C", por Bluefever Software/Richard Allbert), cobrindo literalmente todos os temas desta biblioteca em ordem: representação (array de 120 casas), geração de lances, busca alfa-beta com poda de quiescência, MVV-LVA, killer moves, transposition table, e interface UCI/WinBoard completa. É provavelmente o recurso único mais alinhado ao que vocês vão construir. Licença permissiva (WTFPL) — pode ser usado livremente como base ou referência. |
| **[Sunfish](https://github.com/thomasahle/sunfish)** | Python | Só ~111 linhas de código, mas com busca (MTD-bi, uma variante de busca com janelas, embutida em *iterative deepening*), quiescência e avaliação por PST — tudo legível numa sentada só. Não é C, mas por ser tão compacto é uma ótima forma de ver o **algoritmo de busca inteiro** sem se perder em detalhes de linguagem. Bom complemento de leitura pra Etapa 3. |
| **[Guide to Programming a Chess Engine (PDF)](https://www.adamberent.com/wp-content/uploads/2019/02/GuideToProgrammingChessEngine.pdf)** | C# (mas os conceitos são independentes de linguagem) | Um relato em formato de livro/diário de bordo de alguém construindo um engine do zero até vencer torneios — bom pra contexto de "como as etapas se encadeiam na prática", incluindo os mesmos temas (FEN, Zobrist, transposition table) já linkados individualmente acima. |

---

## 6. Tutoriais passo a passo (bons para o protótipo em TS)

Como o `descricao-projeto-xadrez.md` define que a primeira entrega é um protótipo em TypeScript com minimax raso e avaliação só por material, estes tutoriais — todos em JavaScript/conceitualmente simples — mapeiam bem direto para essa fase, mais do que os engines em C acima (que já assumem mais performance/complexidade).

| Recurso | O que traz |
|---|---|
| **[A step-by-step guide to building a simple chess AI (freeCodeCamp)](https://www.freecodecamp.org/news/simple-chess-ai-step-by-step-1d55a9266977/)** | Tutorial em JavaScript cobrindo exatamente a progressão do protótipo de vocês: geração de lances → avaliação só por material → minimax → alfa-beta → adicionar piece-square tables. Cada etapa tem um link jogável (jsFiddle) pra testar o resultado imediatamente. Provavelmente o ponto de partida mais direto pra quem vai escrever o spike em TS. |
| **["Coding Adventure: Chess" — Sebastian Lague (vídeo)](https://www.youtube.com/watch?v=U4ogK0MIzqk)** + **[código-fonte (GitHub)](https://github.com/SebLague/Chess)** | Em C#, não JS/TS — mas é citado com frequência como uma das explicações mais claras e visuais que existem de minimax + alfa-beta (as animações de poda de árvore ajudam bastante a criar intuição antes de implementar). Vale como reforço conceitual mesmo fora da linguagem do protótipo. |

---

## 7. Comunidade (para quando travar num bug específico)

- **[TalkChess.com](https://talkchess.com/)** — o fórum mais ativo da comunidade de programação de xadrez; tanto a CPW quanto praticamente todo autor de engine independente linkado acima participam ou são citados lá. Bom lugar pra buscar (antes de perguntar) se um bug específico de geração de lances ou de protocolo já foi discutido.
- Praticamente toda página da CPW linkada acima termina com uma seção **"Forum Posts"** — discussões antigas do CCC (Computer Chess Club) e do próprio TalkChess sobre casos-limite daquele tema específico. Vale checar quando um conceito da página principal não cobrir o caso exato que vocês encontrarem.

---

## Sequência sugerida (mapeada ao roadmap de `arquitetura-xadrez.md` §9)

| Etapa do roadmap | Ler nesta ordem |
|---|---|
| **Protótipo TS (Iteração 1, produto do escopo do projeto)** | §6 (freeCodeCamp) → §3.2 Simplified Evaluation Function, versão simplificada → §3.1 Alpha-Beta (conceito, não precisa da versão em C ainda) |
| **Etapa 1 — Setup e contratos** | §0 (Getting Started) → §4 (UCI, pra rascunhar o protocolo texto com o grupo) |
| **Etapa 2 — Motor de regras (C), isolado** | §1 completo (representação + FEN) → §2 completo (geração, check, square attacked by, Perft, perftree) → ler `board.c`/`data.c` do TSCP em paralelo |
| **Etapa 3 — IA (C), isolada** | §3.1 completo (search, alpha-beta, quiescence, move ordering/MVV-LVA) → §3.2 completo (evaluation, PST, Simplified Evaluation Function) → acompanhar os episódios correspondentes da série VICE (§5) |
| **Etapa 4 — Backend: integração + API** | §4 completo (spec UCI oficial), revisitando o protocolo rascunhado na Etapa 1 |
| **Etapa 7 — Polimento (opcional)** | §3.1 Zobrist Hashing, transposition table — só se o grupo decidir investir tempo aqui, conforme já sinalizado como opcional no `descricao-projeto-xadrez.md` |

