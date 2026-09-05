# O Motor de Xadrez — Guia de Contexto para a Equipe

> **Para quem está entrando agora no desenvolvimento do motor.**
> Não é preciso saber C para ler este documento — nenhuma linha de código é exigida.
> Tempo estimado de leitura: 20–25 minutos.
>
> Estado descrito: **5 de setembro de 2026**.
> Documentos relacionados: `roadmap-motor.md` (o que vem a seguir),
> `project_context.md` (estado técnico detalhado, esse sim exige C),
> `arquitetura-xadrez.md` e `descricao-projeto-xadrez.md` (o sistema completo).

---

## Como usar este documento

| Se você quer... | Leia |
|---|---|
| Entender do que as pessoas estão falando nas reuniões | §1 a §3 |
| Entender por que o motor foi construído deste jeito | §4 e §5 |
| Saber o que está pronto e o que falta | §6 |
| Rodar e testar o motor na sua máquina | §7 e §8 |
| Contribuir a partir de hoje | §9 |
| Entender o encaixe com a disciplina | §10 |
| Consultar um termo solto | Apêndice A |

Não é preciso ler linearmente. As seções 1 a 3 são o mínimo para participar de
qualquer conversa técnica sobre o motor.

---

## 1. O que é o motor, em uma frase

> **O motor é um programa que lê linhas de texto, entende de regras de xadrez, e escreve linhas de texto de volta.**

É só isso. Ele não tem interface gráfica, não sabe o que é uma janela, não sabe o
que é HTTP, não sabe que existem outras partidas acontecendo, não guarda nada em
disco. Você abre um terminal, digita comandos, e ele responde.

Duas capacidades justificam sua existência:

1. **Ele sabe as regras.** Dada uma posição, ele consegue listar exatamente quais
   lances são legais — incluindo roque, en passant, promoção, e a proibição de
   deixar o próprio rei em xeque.
2. **Ele sabe escolher.** Dada uma posição, ele consegue calcular qual lance
   jogar. Isso é "a IA".

Tudo o mais no sistema — a interface, a comunicação em rede, o gerenciamento de
partidas — existe **fora** do motor, e conversa com ele por texto.

### Por que ele existe separado do resto

Três motivos, em ordem de importância:

**Testabilidade.** Um programa que só lê e escreve texto pode ser testado sozinho,
no terminal, sem depender de nenhuma outra parte do sistema estar pronta. Isso
significa que o desenvolvimento do motor não bloqueia o da interface, e vice-versa.

**Isolamento de falha.** O motor é escrito em C, uma linguagem sem rede de
proteção contra erros de memória. Se ele travar, queremos que trave sozinho, sem
derrubar a aplicação inteira junto. Como ele roda como processo separado, um crash
dele é recuperável.

**Desempenho.** A IA precisa avaliar centenas de milhares de posições por segundo
para jogar razoavelmente. Essa parte precisa ser rápida de verdade, e C é a
linguagem certa para esse tipo de laço.

---

## 2. Onde o motor fica no sistema

```
                    ┌─────────────────────────┐
                    │   Cliente / Interface   │   ← outra equipe, outro repositório
                    │  (tabuleiro, cliques)   │
                    └───────────┬─────────────┘
                                │
                                │  (ver §11 — camada em definição)
                                │
                    ┌───────────▼─────────────┐
                    │      MOTOR (C)          │   ← ESTE REPOSITÓRIO
                    │                         │
                    │  • regras do xadrez     │
                    │  • geração de lances    │
                    │  • busca / IA           │
                    └─────────────────────────┘
                       ↑ texto      ↓ texto
                     (stdin)      (stdout)
```

**A fronteira deste repositório é estreita e deliberada:** um executável que recebe
comandos por linha e responde por linha. Nada além disso vive aqui. O cliente
desktop e o backend são de outras frentes.

Essa estreiteza é uma decisão, não uma limitação. Ela é o que permite que o motor
seja desenvolvido, testado e depurado de forma completamente independente do resto.

---

## 3. Como se conversa com o motor

O protocolo é inspirado no **UCI** (*Universal Chess Interface*), o padrão que
motores reais como o Stockfish usam para conversar com interfaces gráficas. Usar
um padrão existente traz duas vantagens concretas:

- Podemos plugar o nosso motor em interfaces prontas (Arena, Cute Chess) e ver ele
  jogando, ou jogar contra ele, antes de a nossa interface existir.
- Podemos rodar o nosso motor **contra o Stockfish** para comparar resultados —
  o que é a base do nosso principal método de teste (§4.5).

### Um diálogo real

Linhas com `>` são o que mandamos; linhas com `<` são o que o motor responde.

```
> uci
< id name ChessEngine 0.1
< id author Equipe
< uciok

> isready
< readyok

> position startpos
> go movetime 2000
< bestmove e2e4

> position startpos moves e2e4 e7e5
> go movetime 2000
< bestmove g1f3

> quit
```

Três coisas a notar:

**Os lances são escritos como "casa de origem + casa de destino".** `e2e4` significa
"da casa e2 para a casa e4". Promoção ganha uma letra no fim: `e7e8q` = "promova a
dama (queen)".

**O comando `position` carrega a partida inteira, sempre.** Não existe "faça o
lance X" seguido de "agora faça o lance Y". A cada vez, mandamos a posição inicial
mais toda a lista de lances. Isso se chama **motor stateless** e é explicado em §5.6.

**O motor responde `bestmove` e volta a esperar.** Ele nunca toma iniciativa.

---

## 4. Os cinco problemas que um motor de xadrez precisa resolver

Esta seção é o núcleo conceitual. Se você entender estes cinco problemas, você
entende o que está sendo construído — e por que leva o tempo que leva.

### 4.1 Representar uma posição

Antes de qualquer coisa, o programa precisa de uma forma de guardar "onde está cada
peça". A escolha parece trivial e não é: ela determina o quanto todo o resto do
código vai ser rápido e o quanto vai ser fácil de errar.

Nossa escolha: **um vetor de 64 posições**, uma por casa do tabuleiro, onde cada
posição guarda qual peça está ali (ou "vazia"). Em jargão, isso se chama
**mailbox** ("caixa de correio" — cada casa é uma caixa que está vazia ou contém
uma peça).

```
   índice:  0    1    2    3    4    5    6    7
   casa:    a1   b1   c1   d1   e1   f1   g1   h1

   índice:  56   57   58   59   60   61   62   63
   casa:    a8   b8   c8   d8   e8   f8   g8   h8
```

Repare que **a1 é o índice 0**. Isso é uma convenção — poderia ser a8 = 0 — e a
escolha foi deliberada. O porquê está em §5.2.

Além do tabuleiro em si, uma posição de xadrez precisa carregar mais coisas:

| Informação | Por que não dá para deduzir do tabuleiro |
|---|---|
| De quem é a vez | Um tabuleiro parado não diz quem joga |
| Direitos de roque | Se o rei está em e1 e a torre em h1, eles podem ter se movido e voltado — não há como saber olhando |
| Casa de *en passant* | O fato de um peão ter acabado de avançar duas casas não fica visível |
| Contador dos 50 lances | Histórico puro |
| Número da jogada | Histórico puro |

Essa é uma sutileza que costuma surpreender: **a posição não é só o tabuleiro.**
É o tabuleiro **mais** um resumo mínimo da história da partida.

### 4.2 Escrever uma posição como texto: a notação FEN

**FEN** (*Forsyth–Edwards Notation*) é o formato padrão para escrever uma posição
de xadrez como uma única linha de texto. É o formato que circula por todo o
sistema — motor, backend, interface, arquivos de teste.

A posição inicial:

```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
```

São seis campos separados por espaço:

| # | Valor no exemplo | Significado |
|---|---|---|
| 1 | `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR` | As peças, fileira por fileira, da 8ª para a 1ª |
| 2 | `w` | Vez das brancas (`b` = pretas) |
| 3 | `KQkq` | Roques ainda possíveis (maiúsculas = brancas) |
| 4 | `-` | Casa de en passant (`-` = nenhuma) |
| 5 | `0` | Meios-lances desde a última captura ou lance de peão |
| 6 | `1` | Número da jogada |

No campo 1: **letras maiúsculas são peças brancas, minúsculas são pretas.**
`r` = torre (*rook*), `n` = cavalo (*knight*), `b` = bispo, `q` = dama (*queen*),
`k` = rei (*king*), `p` = peão (*pawn*). Um **número** significa "N casas vazias
seguidas" — então `8` é uma fileira inteira vazia, e `4P3` é "4 vazias, peão branco,
3 vazias".

Você vai ver FENs em toda parte no projeto: nos arquivos de teste, nos comandos do
protocolo, nos relatórios de bug. Vale saber ler.

### 4.3 Descobrir quais lances são possíveis

Este é o problema central, e ele é resolvido em **duas etapas**, o que costuma
confundir quem chega agora.

**Etapa 1 — lances pseudo-legais.** Gerar todos os lances que respeitam *como a peça
anda*, ignorando se o lance deixa o próprio rei em xeque. Um bispo anda na
diagonal até bater em alguma peça; um cavalo tem 8 destinos possíveis; um peão
anda para frente e captura na diagonal.

**Etapa 2 — filtro de legalidade.** Para cada lance pseudo-legal, aplicar o lance,
perguntar "o meu rei está sendo atacado agora?", e desfazer. Se estiver, o lance é
ilegal e sai da lista.

> **Por que duas etapas em vez de uma?** Porque gerar direto só os lances 100%
> legais exige raciocinar sobre peças cravadas, xeques descobertos e casos
> exóticos — é significativamente mais difícil de acertar. O filtro em duas etapas
> é mais lento, mas **acerta de graça** situações que a abordagem esperta erra:
> peça cravada, xeque descoberto ao capturar en passant, e o rei tentando fugir
> na mesma linha do ataque.

Um efeito colateral valioso: com a lista de lances legais em mãos, três regras
importantes saem sem código adicional nenhum.

| Situação | Definição operacional |
|---|---|
| **Em xeque** | A casa do meu rei está atacada |
| **Xeque-mate** | Estou em xeque **e** minha lista de lances legais está vazia |
| **Afogamento** | **Não** estou em xeque **e** minha lista está vazia |

### 4.4 Aplicar e desfazer lances

A IA precisa "experimentar" lances: aplica um, vê o que acontece, desfaz, tenta
outro. Isso acontece centenas de milhares de vezes por segundo.

A forma ingênua seria copiar o tabuleiro inteiro antes de cada tentativa. Nós
usamos a alternativa: **aplicar o lance sobre o tabuleiro existente e desfazê-lo
depois**, guardando apenas o que se perdeu.

E o que se perde? Nem tudo é reversível:

| Reversível (basta inverter o lance) | Irreversível (precisa ser salvo antes) |
|---|---|
| A peça volta para a origem | **Qual peça foi capturada** — o tabuleiro depois não sabe |
| A vez volta a alternar | **Os direitos de roque** — mover a torre destrói o direito sem deixar rastro |
| | **A casa de en passant** |
| | **O contador dos 50 lances** |

Essa distinção é uma das ideias mais importantes do motor, e é a razão de existir
uma estrutura chamada `Undo` no código.

### 4.5 Provar que está correto: o teste **perft**

Esta seção é a mais relevante para a disciplina, porque é uma técnica de teste
com nome, propósito e resultado mensurável — exatamente o tipo de coisa que a
apresentação precisa mostrar.

**O problema:** como você prova que um gerador de lances de xadrez está correto?
Testar manualmente é inviável — as regras têm dezenas de casos-limite (roque
através de casa atacada, promoção com captura, en passant que expõe o rei), e um
bug em qualquer um deles só aparece em partidas específicas.

**A solução: perft** (*performance test*). Você conta quantas posições distintas
são alcançáveis a partir de uma posição inicial, em N lances. Esse número é
determinístico e foi calculado e publicado pela comunidade de programação de
xadrez. Se o seu número bate, sua geração de lances está correta até aquela
profundidade. Se não bate, você tem um bug.

Da posição inicial:

| Profundidade | Número de posições |
|---|---|
| 1 | 20 |
| 2 | 400 |
| 3 | 8.902 |
| 4 | 197.281 |
| 5 | 4.865.609 |
| 6 | 119.060.324 |

Ou seja: nas primeiras 3 jogadas de uma partida de xadrez existem 8.902 posições
possíveis. Nosso motor precisa contar exatamente 8.902 — nem 8.901, nem 8.903.

**A posição inicial não basta**, porque nela não existe roque disponível, nem en
passant, nem promoção. Por isso a comunidade padronizou outras posições de
referência. A mais famosa se chama **Kiwipete**:

```
r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1
```

| Profundidade | Posições | Capturas | En passant | Roques | Promoções |
|---|---|---|---|---|---|
| 1 | 48 | 8 | 0 | 2 | 0 |
| 2 | 2.039 | 351 | 1 | 91 | 0 |
| 3 | 97.862 | 17.102 | 45 | 3.162 | 0 |
| 4 | 4.085.603 | 757.163 | 1.929 | 128.013 | 15.172 |

As colunas de detalhe são a parte esperta: se o total está errado mas as capturas
e os roques estão certos, o bug está em promoção ou en passant. O teste não só
detecta o erro, ele **localiza a categoria** do erro.

**E existe uma técnica de depuração em cima disso, chamada `divide`:** em vez de
contar tudo junto, o motor imprime a contagem separada por cada lance inicial.
Você compara com a saída do Stockfish, acha o primeiro lance cuja contagem
divergiu, entra nele, e repete. É uma busca binária dentro da árvore de
possibilidades — transforma "meu número está errado por 2" em "nesta posição
específica, meu motor gera dois lances a mais".

> **Regra do projeto:** nenhuma linha de IA é escrita antes do perft bater.
> Um bug de regra se manifesta na IA como comportamento inexplicável seis níveis
> de profundidade abaixo, e você perde dias procurando no lugar errado.

### 4.6 Escolher um lance: a IA

A IA é conceitualmente mais simples do que a geração de lances, e será construída
por etapas, cada uma jogável:

| Versão | O que faz |
|---|---|
| v0 | Escolhe um lance legal ao acaso |
| v1 | **Minimax**: simula alguns lances à frente, assume que o adversário joga o melhor para ele, escolhe o melhor para si. Avalia posições só por material (dama vale 9, torre 5, etc.) |
| v2 | **Poda alfa-beta**: mesma resposta do minimax, mas descartando ramos que já se provaram piores. Cerca de 10× menos posições avaliadas — o que na prática vira 2 níveis a mais de profundidade |
| v3 | **Aprofundamento iterativo**: busca profundidade 1, depois 2, depois 3... dentro de um orçamento de tempo. Sempre existe um "melhor lance até agora" pronto |
| v4 | **Ordenação de lances**: testar capturas boas primeiro faz a poda alfa-beta funcionar muito melhor |
| v5 | **Busca de quiescência**: continuar buscando enquanto houver capturas pendentes. É o maior salto de força de jogo por linha de código escrita |

Um detalhe importante para os testes: **v2 deve devolver exatamente o mesmo lance
que v1** na mesma profundidade. A poda alfa-beta é uma otimização que não altera o
resultado. Se divergir, tem bug. Esse é um teste de regressão barato e valioso.

---

## 5. As decisões arquiteturais já tomadas (e o porquê de cada uma)

Estas são escolhas conscientes, com alternativa avaliada. Mudá-las agora custa
caro — por isso estão documentadas.

### Tabela resumo

| # | Decisão | Escolha | Alternativa descartada | Motivo em uma linha |
|---|---|---|---|---|
| 5.1 | Representação do tabuleiro | Vetor de 64 (mailbox) | Bitboards | Correção antes de velocidade |
| 5.2 | Indexação das casas | a1 = 0 | a8 = 0 | Convenção da literatura; aritmética natural |
| 5.3 | Codificação da peça | 1 byte (cor + tipo) | Enum plano, structs | Extração por bit + tabelas indexáveis |
| 5.4 | Geração de lances | Pseudo-legal + filtro | Só legais direto | Mais fácil de acertar; pega casos raros de graça |
| 5.5 | Aplicar lances | Aplica/desfaz | Copiar o tabuleiro | Desempenho na busca |
| 5.6 | Estado entre comandos | Stateless | Estado incremental | Reprodutibilidade em teste |
| 5.7 | Validação de entrada | Fronteira valida, núcleo confia | Validar em toda camada | Evita ciclo de dependência; custo O(n²) |

### 5.1 Vetor de 64, não bitboards

**Bitboards** são a técnica usada por motores de ponta: em vez de um vetor de casas,
você usa números de 64 bits onde cada bit representa uma casa. Isso permite
processar o tabuleiro inteiro com uma única instrução da CPU, e é muito mais
rápido.

**Não vamos usar**, e a justificativa é de gestão de projeto, não de gosto: bitboards
são consideravelmente mais difíceis de implementar e de depurar corretamente,
especialmente para peças deslizantes (torre, bispo, dama), onde exigem técnicas
avançadas com nomes como *magic bitboards*. O gargalo deste projeto não é
velocidade — é acertar as regras dentro do prazo.

Um efeito colateral medido durante o desenvolvimento: um vetor de 64 bytes ocupa
**exatamente uma linha de cache** do processador. Varrer o tabuleiro inteiro custa
cerca de 9,5 nanossegundos. Uma alternativa testada (manter uma lista das ~30 casas
ocupadas para não varrer as vazias) foi medida em ~12,3 ns — **mais lenta**, porque
a indireção custa mais do que a varredura que economiza. Decisão registrada com
medição, não com achismo.

### 5.2 a1 = 0, não a8 = 0

Poderíamos numerar as casas começando do canto inferior esquerdo (a1) ou do
superior esquerdo (a8). Escolhemos a1 = 0. Motivos:

- **A aritmética fica natural.** Andar uma casa para o norte é somar 8. O peão
  branco avança `+8`, o preto `-8`. Com a outra convenção os sinais se invertem e
  contradizem a intuição.
- **É a convenção da literatura.** As páginas de referência, os valores de perft
  publicados e as tabelas de avaliação prontas assumem essa numeração. Traduzir
  mentalmente cada exemplo lido é uma fonte constante de erro.
- **O número da fileira bate com o índice.** A 2ª fileira é o índice 1, que é onde
  os peões brancos começam. Isso importa em várias regras.

O único custo: a FEN é escrita da 8ª fileira para a 1ª, então o leitor de FEN
precisa começar pelo fim. São três linhas, em um lugar só.

### 5.3 A peça cabe em um byte

Cada casa guarda um único byte que codifica cor **e** tipo simultaneamente, usando
bits diferentes do mesmo número. Peão branco = 9, peão preto = 1, casa vazia = 0.

Isso permite tabelas indexadas diretamente pelo valor da peça — por exemplo, o
caractere da FEN sai de uma string de 16 posições sem nenhum `if`.

**Armadilha conhecida e documentada:** por causa de como os bits foram
distribuídos, uma casa vazia "parece" uma peça preta se você perguntar a cor dela
diretamente. A regra do projeto é sempre testar "está vazia?" antes de testar a
cor — e a correção prevista no roadmap é encapsular isso em funções que tornam o
erro impossível, em vez de apenas documentado.

### 5.6 Motor stateless entre comandos

A posição chega sempre completa, nunca como "aplique este lance sobre o que você
tinha antes". Duas consequências valiosas:

- **Qualquer posição é reproduzível isoladamente em teste.** Você cola a FEN, roda
  o comando, vê o resultado. Não precisa reconstruir uma partida inteira para
  investigar um bug.
- **Cliente e motor não podem dessincronizar.** Não existe estado compartilhado
  para divergir.

**Limitação conhecida:** empate por repetição tripla **não é detectável a partir de
uma FEN** — ela é uma fotografia sem histórico. A regra dos 50 lances funciona
(vem do contador que a FEN carrega), mas repetição não. Por isso o protocolo aceita
o formato `position startpos moves e2e4 e7e5 ...` — o motor reconstrói o histórico
reaplicando os lances. Continua stateless (tudo veio no comando), mas agora dá para
detectar repetição.

O invariante a defender: **protocolo stateless, processo stateful.** Nenhum comando
*precisa* do anterior; mas o processo pode guardar o que quiser em cache.

### 5.7 Fronteira valida, núcleo confia

Esta é sutil e vale entender, porque parece uma falha de segurança e não é.

```
Camada de protocolo   →  recebe texto do usuário. VALIDA aqui.
        ↓
Núcleo do motor       →  aplica o lance. CONFIA na entrada. Não verifica nada.
```

A função que aplica um lance no tabuleiro **não verifica se o lance é legal**. Ela
é uma primitiva, como a função `free` do C, que não confere se o ponteiro que
recebeu veio de uma alocação válida.

O motivo decisivo: **o filtro de legalidade precisa aplicar lances que talvez sejam
ilegais** — é aplicando que se descobre. Se a função de aplicar recusasse lances
ilegais, o filtro seria impossível de escrever, e teríamos um ciclo de dependência
(geração → validação → aplicação → geração).

O motivo secundário é custo: durante a busca da IA, regerar todos os lances dentro
de cada aplicação transformaria cada nó em uma operação quadrática.

A pré-condição — "este lance saiu do gerador para esta posição" — é satisfeita por
construção nos dois únicos lugares que chamam a função: a busca (que itera a lista
que acabou de gerar) e a camada de protocolo (que valida antes).

---

## 6. Estado atual, honestamente

**O projeto tem ~750 linhas de C em 5 módulos.** O leitor de FEN é a parte madura;
a geração de lances mal começou.

### Funciona e está verificado

| Área | Estado |
|---|---|
| Leitura de FEN | Valida e constrói o tabuleiro numa passada só. Rejeita posições ilegais (peão na última fileira, rei duplicado, fileira incompleta, contagem de peças) |
| Indexação a1 = 0 | Convertida e verificada casa a casa |
| Escrita de FEN | Emite o campo das peças corretamente |
| Tabela de distâncias até a borda | Verificada casa a casa contra valores calculados à mão |
| Conversão de coordenadas | `e2` ↔ índice 12, nos dois sentidos |

### Incompleto ou ausente

| Área | Estado |
|---|---|
| Estrutura da posição | Faltam os campos de roque, en passant e contadores |
| Leitura de FEN | Lê os campos 3 a 6 e os **descarta** |
| Geração de lances | Só peão, e só o avanço simples. Sem captura, avanço duplo, en passant ou promoção |
| Torre, bispo, dama | Função existe vazia |
| Cavalo e rei | Não existem |
| Vez de jogar | A geração **ignora de quem é a vez** e produz lances das duas cores |
| Aplicar lance | Move a peça e nada mais |
| Desfazer lance | Não existe. **Sem ele não há busca, logo não há IA** |
| Legalidade | Não existe. Todo lance gerado é pseudo-legal |
| Perft | Não existe. **Nada da geração está validado ainda** |
| Protocolo | Não existe. A interação é um menu de teste provisório |
| Testes automatizados | Nenhum |

### A leitura honesta disso

**As decisões estão maduras; a execução não começou.** Isso não é um problema de
planejamento — é onde o projeto está no calendário. Mas tem uma consequência
prática: **o gargalo não é decidir mais coisas, é fechar o ciclo.**

Há um segundo diagnóstico, registrado no `project_context.md` e que orienta a
primeira fase do roadmap:

> Das últimas seis sessões de desenvolvimento, **todo bug encontrado era detectável
> em tempo de compilação.**

Seis de seis. A causa é única: o projeto compila sem otimização ligada, e o
compilador GCC só faz a análise de fluxo de dados que alimenta seus avisos quando
há otimização. Ou seja, os avisos que teriam apontado os bugs simplesmente não
disparavam. A correção é uma linha no arquivo de build, e é o item de maior
retorno de todo o roadmap.

---

## 7. Mapa do repositório

Se você for abrir o código, esta é a ordem que faz sentido.

| Arquivo | O que faz | Ler se... |
|---|---|---|
| `src/types.h` | O vocabulário do projeto: as estruturas e constantes que todos os outros usam | **Comece aqui.** ~100 linhas, é o dicionário |
| `src/board.c` | Leitura e escrita de FEN, conversão de coordenadas | Quer entender como a posição é montada |
| `src/movegen.c` | Direções, codificação de lances, geração, aplicação | É onde o trabalho dos próximos meses acontece |
| `src/utils.c` | Impressão do tabuleiro, leitura de entrada | Utilitários de depuração |
| `src/log.c` | Registro de erros | Trivial |
| `src/main.c` | Menu provisório de teste | Será substituído pela camada de protocolo |
| `docs/` | Referências e anotações | Links para a Chess Programming Wiki |
| `tscp183b/` | Um motor de xadrez de referência, completo, em ~2000 linhas de C legível | Quer ver como fica um motor pronto |

**Se você quer aprender C junto:** `log.c` (5 linhas) → `utils.c` → `types.h` →
`board.c`. O `movegen.c` é o mais difícil e deve ser o último.

---

## 8. Como rodar

```bash
# compilar
make

# rodar
make run
```

Hoje isso abre um menu provisório de teste que carrega uma posição fixa, imprime
o tabuleiro e aceita um lance digitado. A partir da Fase 6a do roadmap, isso será
substituído pelo protocolo de verdade, e você poderá fazer:

```bash
./build/main.out
uci
position startpos
go movetime 1000
```

**Comando útil:** `make context` gera um arquivo único com todas as decisões e
interfaces do projeto, pronto para colar numa conversa ou consultar offline.

---

## 9. Como contribuir a partir de hoje

Há trabalho real e valioso que **não exige escrever C**. Estes itens não são
tarefas inventadas para ocupar gente — cada um destrava ou acelera algo concreto.

### 9.1 Construir o corpus de posições de teste ⭐

**O que:** um arquivo de texto com ~30 FENs, uma por linha, com um comentário
explicando o que cada uma exercita.

**Por que importa:** é o insumo do teste mais valioso do projeto — o *round-trip*
(ler a FEN, escrever de volta, conferir se saiu idêntica) e o teste de aplicar e
desfazer lances. Sem um corpus variado, esses testes não provam nada.

**O que precisa cobrir:** posição inicial; Kiwipete; posições com en passant ativo;
com roque parcial (só `Kq`, ou `-`); com peão prestes a promover; com peão prestes
a promover **capturando**; finais com poucas peças; posições de mate e de afogamento.

**Habilidade necessária:** saber xadrez e saber ler FEN (§4.2). Zero C.

### 9.2 Verificação cruzada de perft com o Stockfish ⭐

**O que:** instalar o Stockfish, rodar `go perft N` nas posições do corpus, e
registrar os números de referência num arquivo.

**Por que importa:** quando o perft do nosso motor divergir, precisamos de um
oráculo confiável para comparar. Ter esses números já tabelados economiza horas na
fase mais difícil do projeto (Fase 6b).

**Habilidade necessária:** linha de comando. Zero C.

### 9.3 Teste exploratório do protocolo

**O que:** assim que a Fase 6a estiver pronta, jogar partidas inteiras contra o
motor pelo terminal e relatar qualquer comportamento estranho — travamento,
lance ilegal aceito, resposta fora do formato.

**Por que importa:** é QA de verdade. Bugs de protocolo (travamentos por buffer,
quebras de linha diferentes entre sistemas operacionais) só aparecem em uso real.

**Habilidade necessária:** paciência e saber xadrez.

### 9.4 Fuzzing do leitor de FEN

**O que:** um script (Python, shell, o que preferir) que gera strings malformadas
— FENs cortadas no meio, com caracteres inválidos, com barras a mais — e alimenta
o motor com elas, verificando que ele rejeita em vez de quebrar.

**Por que importa:** é uma técnica de teste com nome, rende bem na apresentação, e
encontra estouros de buffer que teste manual não encontra.

**Habilidade necessária:** qualquer linguagem de script. Zero C.

### 9.5 Revisão do protocolo contra a especificação

**O que:** ler a especificação do UCI (são cerca de 6 páginas de texto puro) e
conferir se nossa implementação está de acordo — formato dos comandos, o que fazer
com comando desconhecido, formato dos lances.

**Por que importa:** aderir ao padrão é o que permite plugar o motor em interfaces
prontas e testá-lo contra outros motores.

**Habilidade necessária:** leitura atenta. Zero C.

### 9.6 Métricas para o acompanhamento do projeto

**O que:** medir e registrar, a cada iteração, quantos nós por segundo o motor
processa e até que profundidade ele chega em tempo fixo.

**Por que importa:** vira dado concreto para o burndown e para a Análise de Valor
Agregado da disciplina, e é evidência objetiva de progresso na apresentação.

**Habilidade necessária:** planilha.

---

## 10. Como isso se conecta com a disciplina

O motor corresponde aos itens **3 (Motor de Regras)** e **4 (IA)** da EAP, e é a
frente técnica mais arriscada do projeto — a que tem mais casos-limite e menos
tolerância a erro parcial.

Três pontos que vale ter claros para a apresentação:

**A limitação da APF é conhecida e deve ser dita.** Análise de Pontos de Função
mede complexidade transacional (entradas, saídas, arquivos lógicos). O motor tem
complexidade **algorítmica**, não transacional — ele tem essencialmente duas
transações (recebe posição, devolve lance) e milhares de linhas de lógica atrás
disso. Forçar uma contagem produziria um número sem significado. **Reconhecer essa
limitação explicitamente é mais defensável do que forjar um número**, e demonstra
compreensão da técnica em vez de aplicação mecânica.

**O perft é o melhor material de teste que o projeto tem.** É um teste
determinístico, com oráculo externo, cobertura comprovada de casos-limite e uma
técnica de depuração associada (`divide`, que é literalmente uma busca binária
dentro do espaço de estados). Vale um slide próprio.

**Os itens de IA da EAP devem ser quebrados.** O item "4.2 Busca minimax +
alfa-beta + aprofundamento iterativo" é grande demais para o burndown — ele fica
0% por três semanas e depois pula para 100%. Quebrado nas versões v1 a v5 da §4.6,
cada uma é entregável, demonstrável e verificável isoladamente. Isso melhora a
visibilidade do progresso e é uma decisão de gestão defensável na apresentação.

---

## 11. Uma decisão em aberto que afeta a equipe inteira

Os documentos do projeto divergem sobre um ponto, e vale resolver em grupo:

- O `arquitetura-xadrez.md` estabelece como princípio central que **o cliente nunca
  fala diretamente com o motor** — sempre através de um backend em Node/TypeScript.
- O `project_context.md` descreve o **cliente C++ rodando o motor como subprocesso
  direto**, sem mencionar o backend no caminho.

Não é claro se a decisão mudou ou se os documentos descrevem coisas diferentes. Mas
a diferença tem consequência concreta:

| Se... | Então o motor precisa... |
|---|---|
| O cliente fala direto com o motor | Expor mais comandos (ex.: listar lances legais para a interface destacar), e o requisito de partidas simultâneas some |
| Há um backend no meio | Ficar mais fino; o backend é quem monta as respostas para a interface |

**Isso precisa ser resolvido antes da Fase 6a do roadmap**, porque é ela que define
a superfície do protocolo. É uma conversa de 15 minutos que evita implementar
comandos que ninguém vai chamar — ou descobrir na integração que falta um.

---

## Apêndice A — Glossário

| Termo | Significado |
|---|---|
| **Afogamento** (*stalemate*) | Empate: o jogador não está em xeque mas não tem nenhum lance legal |
| **Alfa-beta** | Otimização do minimax que descarta ramos comprovadamente piores. Não muda o resultado, só a velocidade |
| **Aprofundamento iterativo** | Buscar profundidade 1, depois 2, depois 3... dentro de um orçamento de tempo |
| **Bitboard** | Representar o tabuleiro como números de 64 bits, um bit por casa. Rápido e complexo. **Fora do escopo deste projeto** |
| **Cravada** (*pin*) | Peça que não pode se mover porque expõe o próprio rei |
| **En passant** | Captura especial de peão, na casa por onde o peão adversário passou ao avançar duas casas |
| **FEN** | *Forsyth–Edwards Notation.* Uma posição de xadrez escrita como uma linha de texto (§4.2) |
| **Kiwipete** | Posição de referência padrão para perft, escolhida por exercitar roque, en passant e promoção simultaneamente |
| **Mailbox** | Representar o tabuleiro como um vetor de casas, cada uma "vazia ou com uma peça". O que usamos |
| **Minimax** | Algoritmo de busca: simula lances assumindo que cada lado joga o melhor para si |
| **Nó** | Uma posição visitada durante a busca da IA. "Nós por segundo" é a métrica de velocidade |
| **Perft** | Teste que conta posições alcançáveis em N lances e compara com valores publicados (§4.5) |
| **Poda** | Descartar um ramo da busca sem explorá-lo |
| **Promoção** | Peão que chega à última fileira e vira outra peça. Gera **quatro** lances distintos (dama, torre, bispo, cavalo) |
| **Pseudo-legal** | Lance que respeita como a peça anda, mas que pode deixar o próprio rei em xeque |
| **Quiescência** | Continuar buscando enquanto houver capturas pendentes, para não avaliar posições no meio de uma troca |
| **Roque** | Lance duplo de rei e torre. Único lance cuja legalidade depende de casas *intermediárias* estarem atacadas |
| **Stateless** | Sem memória entre comandos: cada comando carrega tudo que precisa |
| **Stockfish** | O motor de xadrez de código aberto mais forte do mundo. Usamos como oráculo de referência para o perft |
| **UCI** | *Universal Chess Interface.* O protocolo de texto padrão entre motores e interfaces (§3) |
| **Xeque descoberto** | Xeque causado por mover uma peça que estava bloqueando o ataque de outra |

---

## Apêndice B — Leituras, por ordem de utilidade

**Se você vai ler uma coisa só:**
[Getting Started](https://www.chessprogramming.org/Getting_Started) da Chess
Programming Wiki — a página "por onde começar", que confirma exatamente a ordem que
adotamos: primeiro uma representação de tabuleiro sem bugs, depois busca, depois
interface.

**Para entender os conceitos deste documento com mais profundidade:**

- [Board Representation](https://www.chessprogramming.org/Board_Representation) — §4.1 e §5.1
- [Forsyth-Edwards Notation](https://www.chessprogramming.org/Forsyth-Edwards_Notation) — §4.2
- [Move Generation](https://www.chessprogramming.org/Move_Generation) — §4.3
- [Perft](https://www.chessprogramming.org/Perft) e [Perft Results](https://www.chessprogramming.org/Perft_Results) — §4.5
- [Search](https://www.chessprogramming.org/Search) e [Alpha-Beta](https://www.chessprogramming.org/Alpha-Beta) — §4.6
- [UCI](https://www.chessprogramming.org/UCI) — §3

**Especificação do protocolo UCI:** o texto original tem ~6 páginas e vale ler
inteiro se você for mexer na camada de protocolo. Há uma
[cópia navegável no GitHub](https://github.com/fsmosca/UCIChessEngineProtocol).

**Se quiser ver um motor completo:** o `tscp183b/` neste repositório é o TSCP
(*Tom Kerrigan's Simple Chess Program*), escrito em 1997 explicitamente como motor
didático. São ~2000 linhas de C legível cobrindo tudo o que este documento
descreve. *Atenção de licença:* é aberto para leitura e estudo, mas não é
software livre — trate como material de leitura, não como base de código a copiar.

**Se preferir vídeo:** a série
[Programming a Chess Engine in C](https://github.com/bluefeversoft/vice)
(*VICE*, 87 vídeos) constrói um motor completo do zero, na mesma ordem deste
roadmap. É a referência mais citada da comunidade para quem está começando.

---

*Documento mantido à mão. Ao alterar uma decisão arquitetural, atualize a §5 aqui
e o `project_context.md` juntos — eles descrevem a mesma coisa em níveis de
detalhe diferentes.*
