# Contexto do Projeto — Chess Engine em C

> Documento de contexto para retomar o projeto ou apresentá-lo a quem for trabalhar nele,
> sem precisar ler a codebase inteira.
> Estado em **10 de setembro de 2026**.
>
> **Guia de implementação das próximas etapas: `docs/next_steps.md`.** Ele detalha
> `make_move`/`unmake_move`, cavalo e rei, polimento do gerador e o UCI minimo, com
> os critérios de saída de cada etapa. Este documento continua sendo o contexto; o
> next_steps é o plano de execução.

---

## 1. O que é este repositório

Um **motor de xadrez em C**, escrito do zero como projeto de aprendizado. O objetivo
declarado não é força de jogo — é entender como uma engine funciona por dentro.

A fronteira do repositório é estreita e deliberada: **um binário que fala um protocolo
texto por stdin/stdout.** Nada além disso vive aqui.

- O **cliente desktop (C++)** é de outra equipe, em outro repositório.
- O **backend (Node/TS)** também não vive aqui.
- Este repo entrega um executável que recebe comandos por linha e responde por linha.

A fase atual é **protótipo/demo**: um binário que joga xadrez legalmente do início ao fim,
com uma IA qualquer. Correção das regras vem primeiro; força de jogo vem por último, e só
se sobrar tempo. Bitboards, transposition table e magic bitboards estão explicitamente
fora de escopo.

---

## 2. Estado atual, honestamente

O projeto tem **~750 linhas** em 5 módulos e 6 commits. O parser de FEN é a parte madura;
a geração de lances mal começou.

### Funciona e está verificado

| Área | Estado |
|---|---|
| `parse_fen` | **Os seis campos.** Uma função por campo, todas `static`. Valida peças, lado, roque (sintaxe + coerência com o tabuleiro), en passant (fileira + as três casas envolvidas) e os relógios. Aceita de 4 a 6 campos. Invariante: `*out` só é escrito se a FEN inteira validar. Ver `docs/fen.md` |
| Indexação `a1 = 0` | Convertida e verificada: round-trip FEN idêntico, `king_square` correto, tabuleiro na orientação certa |
| `board_to_fen` | **Emite os seis campos.** Round-trip `FEN -> Board -> FEN` devolve string idêntica, verificado em 5 posições |
| `Board` | Completa: `castling_rights`, `ep_square`, `halfmove_clock`, `fullmove_number` |
| Build limpo | Zero warnings com `-std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Og`. O Makefile passou a usar esse conjunto |
| `SQ_TO_EDGE` | Tabela de distância até a borda, verificada casa a casa contra valores calculados à mão |
| Conversão de coordenadas | `sq_from_coord` / `coord_from_sq` |
| Camadas de validação | `find_move` valida na fronteira; `make_move` confia na entrada |
| Headers | Os cinco são auto-contidos (cada um compila sozinho) |

### Incompleto ou ausente

| Área | Estado |
|---|---|
| `parse_fen` | Falta a última validação: rei do lado que **não** joga em xeque é posição ilegal. Depende de `is_square_attacked` |
| Geração de lances | Peão: empurrada simples, dupla e captura. A dupla está **em paralelo** ao simples e sem checar casas — peão salta sobre peças. Promoção emite só a dama. Sem en passant nem roque |
| Peças deslizantes | Torre, bispo e dama geram; falta o filtro de `side_to_move` |
| Cavalo e rei | Não existem |
| `side_to_move` | A geração **ignora de quem é a vez** e produz lances das duas cores. Reproduzido: com as pretas na vez, `e3e4` (branco) é aceito e jogado |
| `make_move` | Move a peça e inverte `side_to_move`. Não atualiza `king_square`, não trata captura/roque/en passant/promoção, não mexe nos relógios. E **chama `find_move`**, invertendo a fronteira de validação (ver §3 e `next_steps.md` §2 D4) |
| `unmake_move` | Esqueleto: move a peça de volta e restaura a captura. Não restaura lado, roque, en passant nem relógios |
| `MoveHistory` / `push_undo` / `pop_undo` | Existem, como **global em `movegen.c`**, e estão quebrados — `pop_undo` lê uma casa além do topo. Ver §5. Decisão de arquitetura tomada em 2026-09-10: a pilha global sai (§3) |
| Legalidade | Sem `is_square_attacked`, sem filtro de xeque. Todo lance gerado é pseudo-legal |
| Perft | Não existe. **Nada da geração está validado ainda** |
| Protocolo | Não existe. A interação é um `printf`/`fgets` no `main` |
| Testes | Nenhum automatizado |

---

## 3. Decisões arquiteturais firmadas

Estas são escolhas conscientes com trade-off avaliado. Mudá-las agora custa caro.

### Representação: mailbox de 64 casas

Um `Piece array[64]`, não bitboards. Bitboards são mais rápidos, mas mailbox é muito mais
fácil de acertar e de depurar — e correção vem antes de velocidade nesta fase.

Efeito colateral feliz, medido: `array[64]` são **64 bytes = exatamente uma linha de cache**.
Varrer o tabuleiro inteiro custa ~9,5 ns. Uma *piece list* (iterar só as ~30 casas ocupadas)
foi medida em ~12,3 ns — **mais lenta**, porque a indireção custa mais que a varredura que
economiza. Decisão registrada: não implementar piece lists.

### Indexação: `a1 = 0` (Little-Endian Rank-File)

```
sq = rank * 8 + file        rank 0 = 1ª fileira,  file 0 = coluna a
a1 = 0    h1 = 7    a8 = 56    h8 = 63
```

```
    a   b   c   d   e   f   g   h
8  56  57  58  59  60  61  62  63
7  48  49  50  51  52  53  54  55
6  40  41  42  43  44  45  46  47
5  32  33  34  35  36  37  38  39
4  24  25  26  27  28  29  30  31
3  16  17  18  19  20  21  22  23
2   8   9  10  11  12  13  14  15
1   0   1   2   3   4   5   6   7
```

É a convenção da Chess Programming Wiki, dos valores de perft publicados e das
piece-square tables prontas — evita traduzir mentalmente cada exemplo lido.

Consequências: peão branco avança `+8`, preto `-8`. A FEN é lida da fileira 8 para a 1,
então a **primeira** linha da FEN preenche os índices 56–63.

### Codificação da peça num `u8`

```c
MAKE_PIECE(color, type) = (color << 3) | type
TYPE_OF(piece)          = piece & 0x7
COLOR_OF(piece)         = (piece >> 3) & 0x1
```

Com `WHITE = 1`, `BLACK = 0`, `EMPTY = 0`. Peão branco = 9, peão preto = 1.

**Armadilha conhecida:** `COLOR_OF(EMPTY) == BLACK`. Casa vazia se parece com peça preta.
Sempre teste "está vazia" **antes** de testar a cor.

**Armadilha relacionada:** comparar o valor cru contra um `PieceType` (`if (array[sq] == PAWN)`)
funciona por acidente para as pretas e falha para as brancas, porque `BLACK = 0` faz o nibble
de cor sumir. Use sempre `TYPE_OF()`.

### Codificação do lance num `u32`

```
       origem    destino   promoção   flags
u32 = [00000000][00000000][00000000][00000000]
        <<24      <<16       <<8       <<0
```

8 bits por campo. É folgado — casas cabem em 6 bits e o padrão da literatura é um `u16`
com 6+6+4. A folga foi mantida deliberadamente por enquanto: cada campo cai num byte
alinhado, o que facilita a depuração. **Decisão em aberto** (ver §6).

`MoveDescription` é a forma desempacotada, devolvida por `decode_move`.

### Direções: tabela de distância até a borda

`SQ_TO_EDGE[64][8]` guarda, para cada casa e cada direção, quantas casas existem até a
borda. Resolve o problema de wrap-around sem 0x88 nem mailbox 10x12 — as três abordagens
resolvem o mesmo problema, e trocar depois custa mais do que ganha.

**A ordem do `enum Direction` é carga estrutural, não estética:**

```
índice:   0     1     2     3     4      5      6      7
        DIR_N DIR_S DIR_E DIR_W DIR_NE DIR_SW DIR_SE DIR_NW
        └──── ortogonais ────┘  └────── diagonais ──────┘

DIR_OFFSET = { 8,  -8,   1,   -1,    9,    -9,    -7,     7 }
```

Torre usa as direções 0–3, bispo 4–7, rainha 0–7. Isso permite **uma única função**
parametrizada por intervalo de direção, em vez de três. Reordenar o enum quebra isso em
silêncio.

Note a distinção entre **índice** (o valor do enum, usado para indexar as tabelas) e
**offset** (o delta somado ao índice da casa). Confundir os dois foi bug real neste projeto.

### Pseudo-legal primeiro, legalidade depois

Gerar todos os lances que respeitam o movimento da peça, e só depois filtrar os que deixam
o próprio rei em xeque. É mais fácil de acertar do que gerar apenas lances legais direto.

O filtro é: aplica o lance, pergunta se o rei do lado que jogou está atacado, desfaz.
    precompute_move_data();

### Apply/undo com pilha, não cópia do tabuleiro

Cada nó de busca aplica o lance sobre o `Board` existente e o desfaz depois, guardando o
que se perdeu numa struct `Undo` na pilha da recursão. **Não** copiar o `Board` inteiro
por nó.

O `Undo` precisa guardar exatamente o que **não é recalculável** a partir do tabuleiro
depois do lance:

```c
typedef struct {
    Piece captured;         /* o tabuleiro depois não sabe o que estava ali */
    u8    castling_rights;  /* mover a torre destrói o direito, sem deixar rastro */
    int   ep_square;
    int   halfmove_clock;
} Undo;
```

Esta decisão dói para trocar depois. Respeitar desde o início.

### Contrato de validação: fronteira valida, núcleo confia

Estabelecido nesta sessão e já implementado:

```
uci.c / main.c   →  recebe texto do usuário. VALIDA aqui, com find_move.
                 ↓
movegen.c        →  make_move(Board*, u32) confia na entrada. Não verifica nada.
```

`make_move` é uma **primitiva**, não um serviço — como `free(p)`, que não confere se `p`
veio de um `malloc`. A pré-condição é "este `u32` saiu do gerador para esta posição", e
ela é satisfeita por construção nos dois únicos chamadores:

1. **A busca / o filtro de legalidade** — itera a lista que o gerador acabou de produzir.
2. **A fronteira do protocolo** — chama `find_move`, que gera, acha, e devolve o lance.

`make_move` **não pode** validar, por dois motivos. O decisivo é que o filtro de legalidade
precisa aplicar lances que talvez sejam ilegais — é aplicando que se descobre. Um `make_move`
que recusa lances ilegais torna o filtro impossível de escrever, e fecha um ciclo de
dependência: geração → validação → make → geração. O segundo motivo é custo: na busca,
regenerar todos os lances dentro do `make_move` transformaria cada nó em O(n²).

`find_move` devolve **o lance como o gerador o produziu**, não um booleano. Quem digitou
`e5d6` não sabe se aquilo é captura comum ou en passant; o gerador sabe, porque foi ele
que marcou a flag. Devolver só `true` jogaria fora exatamente o que o `make_move` precisa.

### `Undo` é do chamador, não há pilha global de undo

**Decidido em 2026-09-10.** Assinaturas:

```c
void make_move  (Board *b, u32 move, Undo *u);
void unmake_move(Board *b, u32 move, const Undo *u);
```

O `Undo` guarda só o irrecuperável (`captured`, `castling_rights`, `ep_square`,
`halfmove_clock`); fora dele ficam o lance (quem desfaz tem o `u32` na mão) e o
`fullmove_number` (derivável). Perde o campo `Move`, porque o `score` é artefato de
ordenação: 20 → 12 bytes, medido.

O argumento decisivo: **a pilha da recursão já é a pilha de undo.** Um `MoveHistory`
explícito é uma segunda cópia de informação que o registro de ativação do C já mantém, e
duas cópias da mesma verdade podem dessincronizar. Com `Undo u;` local a dessincronização
é inexprimível, e o compilador passa a verificar o pareamento make/unmake — `unmake_move`
não compila sem o `u` no escopo.

Consequência: **três necessidades diferentes param de compartilhar uma estrutura.** O
filtro de legalidade quer 1 `Undo` por nó de recursão; a repetição tripla quer uma lista de
chaves `u64` (Zobrist), não de `Undo`s; o histórico de partida — para o menu de depuração e
para `position ... moves` — pertence à camada de protocolo:

```c
/* uci.h */
typedef struct { u32 move; Undo u; } GamePly;
typedef struct { Board board; GamePly ply[MAX_GAME_PLY]; int count; } Game;
```

E `MAX_PLY = 256` se divide em `MAX_SEARCH_PLY` (64) e `MAX_GAME_PLY` (1024) — a constante
única estava errada para um dos dois usos.

Justificativa completa (sete argumentos) em `next_steps.md` §2 D1. Nota de referência: o
Stockfish usa este desenho (`do_move(m, st)`), o TSCP usa o oposto (`hist_dat` global).

### Split de `board.c` em tres modulos

**Decidido em 2026-09-10.** `board.c` está em 721 linhas fazendo FEN e coordenadas.

```
types.h      vocabulário; só <stdbool.h> e <stdint.h>
square.h/c   sq_from_coord, coord_from_sq, SQ_TO_EDGE, KNIGHT_TARGETS, PAWN_ATTACKS
fen.h/c      parse_fen, board_to_fen
board.h/c    Board, print_board, board_check_invariants
movegen.h/c  Move, MoveList, MoveDescription, Undo + geração + make/unmake
uci.h/c      laço de comandos, Game, str_to_move
```

Agora e não depois porque as tabelas de cavalo e de peão nascem na próxima etapa: é mais
barato nascerem no lugar do que serem movidas. `square.c` tem responsabilidade coerente —
**geometria do tabuleiro**, independente de peça e de posição — em vez de ser saco de
utilitários.

### Testes como subcomando do binario

**Decidido em 2026-09-10.** `./main.out test` roda o round-trip de make/unmake sobre um
corpus de ~30 FENs; `./main.out perft N` conta. Zero infraestrutura nova, e já abre espaço
para os comandos de depuração (`d`, `perft`) que o protocolo vai precisar.

### `make_move` nao chama `find_move`

**Decidido em 2026-09-10** — reafirmação da fronteira que esta seção já descrevia, porque o
código atual a viola. Ver `next_steps.md` §2 D4.

### Motor stateless entre comandos

A posição chega sempre como FEN completa, não como histórico incremental. Qualquer posição
vira reproduzível isoladamente em teste, e cliente e motor não podem dessincronizar.

**Limite conhecido:** repetição tripla **não é detectável a partir de uma FEN** — ela é uma
fotografia sem histórico. A regra dos 50 lances funciona (vem do `halfmove_clock`), mas
repetição não. É por isso que o comando `position` do UCI aceita o sufixo `moves`:

```
position fen <fen> moves e2e4 e7e5 g1f3
```

O motor parte da FEN e reaplica os lances, construindo o histórico. Continua stateless —
tudo veio no comando — mas agora dá para detectar empate por repetição.

O invariante a defender: **protocolo stateless, processo stateful.** Nenhum comando *precisa*
do anterior; o processo pode cachear o que quiser.

---

## 4. Mapa dos módulos

```
types.h ──┬── board.h ──┬── utils.h ──┬── movegen.h
          │             │             │
          └── log.h ────┘             └── (main.c usa todos)
```

| Arquivo | Papel | Linhas |
|---|---|---|
| `types.h` | Vocabulário: `Board`, `Move`, `MoveList`, `Piece`, `Color`, `Direction`, e as macros de codificação | 98 |
| `log.h` / `log.c` | `LOG_ERROR(msg)` com `__FILE__`/`__LINE__`/`__func__` capturados **na macro**, não na função | 17 / 5 |
| `board.h` / `board.c` | FEN (parse, serialize, validação) **e** conversão de coordenadas | 18 / 324 |
| `movegen.h` / `movegen.c` | Direções, codificação de lance, geração, `find_move`, `make_move` | 20 / ~215 |
| `utils.h` / `utils.c` | Impressão de debug, leitura de stdin, `strslc` | 18 / ~70 |
| `main.c` | REPL provisório de teste | 66 |

**Problemas estruturais conhecidos:**

- `board.c` é dois módulos num arquivo só (FEN + coordenadas), e é o maior do projeto.
  Coordenada não é detalhe de FEN — o protocolo UCI (`e2e4`), a leitura de lances e a
  impressão de en passant vão todos precisar dela. Pertence ao vocabulário central.
- `types.h` arrasta seis headers da libc (`ctype`, `stdbool`, `stdint`, `stdio`, `stdlib`,
  `string`). Todo módulo ganha tudo de graça, e o compilador para de avisar quando surge
  acoplamento novo. Só `stdbool.h` e `stdint.h` são realmente necessários lá.
- `utils.h` define uma função `static` **dentro do header** (`clear_screen`). Isso duplica
  a função em cada unidade de tradução e gera aviso de "definida mas não usada" em todas.

---

## 5. Bugs abertos

Atualizado em 2026-09-10. **Cinco dos seis primeiros eram detectáveis em compilação** — dois
deles só com flags que o Makefile ainda não liga (`-Wconversion`, `-Wmissing-prototypes`).
Tabela completa, com as linhas exatas e as duas reproduções, em `next_steps.md` §3.

| Onde | Problema |
|---|---|
| `movegen.c:43` | **`pop_undo` lê uma casa além do topo.** `push_undo` grava em `undos[ply]` e *depois* incrementa; o topo e `ply - 1`. Reproduzido: após um lance, "Unmake Move" imprime `pop undo error` e o tabuleiro não volta. E o decremento acontece mesmo no caminho de falha, tornando `undos[0]` inalcançável |
| `movegen.c:269` | **`u32 *out_move;` não inicializado.** `find_move` escreve através dele (UB) e `push_undo(out_move, ...)` guarda o **ponteiro truncado a 32 bits** como lance. `-Wuninitialized` + `-Wint-conversion` |
| `movegen.c:281` | `push_undo` chamado **depois** de mutar o tabuleiro. Funciona por acidente porque `make_move` ainda não toca roque/ep/relógio; quebra no instante em que tocar |
| `movegen.c:25` | `u8 ep_square` no parâmetro trunca `SQ_NONE`: `-1` vira `255`. `-Wconversion` |
| `movegen.c:238` | **`find_move` aceita lance da cor errada.** Gera as duas cores, e a checagem de lado só faz `LOG_ERROR` sem `return false` — além de cair na armadilha `COLOR_OF(EMPTY) == BLACK`. Reproduzido |
| `movegen.c:299` | `printf("%s", PIECE_CHAR[...])` com um `char`. `-Wformat` |
| `movegen.c:68` | `add_move` usa `>` em vez de `>=`: escreve em `moves[GEN_MOVES_MAX]` |
| `movegen.c:144` | Duplo avanço do peão **em paralelo** ao simples, sem checar casa intermediária nem destino. `perft(1)` inicial passa (20) e quebra na profundidade 2–3 |
| `movegen.c:154` | Promoção gera só a dama; perft conta as quatro peças |
| `movegen.c:181` | `genenare_moves_from_direction` (typo) testa `board->side_to_move` enquanto o laço externo não filtra cor: peça inimiga gera "captura" das próprias peças |
| `movegen.c:313` | `str_to_move` não checa `sq_from_coord() == -1` (`z9z9` → origem 255), ignora o 5º caractere da promoção, e `char[6]` não cabe `"e7e8q\n"` + NUL |
| `movegen.c:13` | `g_history` tem linkage externo e **nenhuma declaração em header**: invisível em revisão, alcançável por `extern` de qualquer `.c`. `-Wmissing-prototypes` não cobre variáveis |
| `movegen.c:289` | Sentinela `move.move == 0` para "vazio": `0` é codificação válida de `a1a1` |
| `movegen.c:25` | `push_undo` sem limite superior; `pop_undo` em `ply == MAX_PLY` lê índice 256 de 256 |
| `movegen.c:37` | `pop_undo()` declarada com `()`. `-Wstrict-prototypes` |
| `types.h:106` | `enum Promotion` começa em 0, e 0 já significa "sem promoção": `encode_move(sq, t, 0, QUIET)` e `encode_move(sq, t, KNIGHT_PROMOTION, QUIET)` são o mesmo `u32`. Recomendação: o campo guarda o `PieceType` cru |
| `types.h:99` | `enum MoveFlag` não tem os roques. `make_move` precisa saber mover a torre; inferir de "o rei andou duas colunas" é um segundo mecanismo |
| `main.c:61` | `make_move(b, move)` usa o lance **digitado** (flags = 0), não o que `find_move` produziu — `out` é passado como `NULL` na linha 58. O `u32` que chega ao `make_move` não tem as flags do gerador |
| `main.c:52` | `char move_out[6]` não comporta `"e7e8q\n"` + NUL, que sao 7 bytes |

### Fechados em 2026-09-10

| Onde | Era |
|---|---|
| `log.h` | `LOG_ERROR` estava sob `#ifdef DEBUG` e o build padrão engolia todo erro. Agora é incondicional |
| `main.c` | `find_move(..., movedesc.flags)` passando um `u8` onde se espera `u32 *`; e `char move_out[5]` |

### Fechados em 2026-09-07 (ver `docs/fen.md` §4)

| Onde | Era |
|---|---|
| `board.c` | `for (char *ptr = fields[2][0]; ...)` — `char` atribuído a `char *`, o valor `'K'` (75) virava endereço |
| `board.c` | `split_fen_fields` devolvia o endereço de um vetor local, e os ponteiros dentro dele apontavam para outro local já morto |
| `board.c` | `coord_from_sq` sem `return` no ramo de erro: `FILE_OF(-1)` é `-1`, então a casa de en passant aparecia como `` `1 `` |
| `board.c` | `board_to_fen` gravava um espaço **depois** do `'\0'` e emitia só o campo das peças — o round-trip perdia lado, roque e en passant |
| `board.c` | Roque com caractere desconhecido era ignorado em silêncio (`KQxq` virava `KQq`) |
| `board.c` | Relógios com `strtol(..., NULL, 10)` sem checar: `"abc"` virava `0` |
| `main.c` | `MoveList l;` sem inicializar — `add_move` usa `count` como índice de escrita |
| `types.h` | `SQ_OFFBOARD(sq)` sem parênteses no parâmetro; `SQ_NONE` definido duas vezes |
| `utils.h` | `clear_screen` definida (não declarada) no header; `\e` não-ISO; `get_int(char[128])` disparando `-Wstringop-overflow`; `()` em vez de `(void)` |
| `Makefile` | `-Wall -g` sem `-O`, escondendo a maioria dos itens acima |

### Sobre o Makefile, especificamente

**Resolvido em 2026-09-07.** O texto abaixo fica como registro do raciocínio.

Das últimas seis sessões, **todo bug encontrado era detectável em tempo de compilação**.
A recomendação é trocar `-g` por `-Og -g` e ligar o conjunto completo:

```
-std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Og -g
-fsanitize=address,undefined
```

`-Og` é o nível de otimização feito para depuração: liga as análises que alimentam os avisos
sem tornar o código irrastreável no gdb. Note que **nenhum sanitizer pega leitura de memória
não inicializada** — ASan pega fora-dos-limites e use-after-free, UBSan pega UB aritmético.
Quem pega é o MemorySanitizer, que é só do clang e exige recompilar a libc. Na prática, o
aviso do compilador é a única defesa viável.

---

## 6. Decisões em aberto

| Decisão | Situação |
|---|---|
| ~~**Campos que faltam na `Board`**~~ | **Fechada em 2026-09-07.** Os quatro campos existem, `parse_fen` os preenche e `board_to_fen` os emite. `Undo` já está definida com o mesmo conteúdo |
| **Lance em 16 vs 32 bits** | Hoje 32. O padrão é 16 (6+6+4). Corta a `MoveList` pela metade. Momento barato de decidir: só `encode_move` e `decode_move` tocam o formato |
| **Sistema de build** | Makefile vs. CMake. O documento de arquitetura pede CMake por causa da integração com a equipe do cliente |
| ~~**Onde vive `side_to_move` na geração**~~ | **Fechada em 2026-09-10.** Laço externo, uma checagem só, com `is_own()` — que já está escrito em `movegen.c:22` e nunca é usado. Usar o helper torna a armadilha `COLOR_OF(EMPTY) == BLACK` inalcançável em vez de lembrada |
| ~~**Onde vive o `Undo`**~~ | **Fechada em 2026-09-10.** É do chamador; não há pilha global. Ver §3 |
| **Zobrist antes ou depois do perft** | Como depurador de `make/unmake` ele é imbatível (`assert(hash == recalculado)` dispara no lance exato). Mas adiciona uma coisa nova na etapa mais delicada. Decisão de sequenciamento, não de arquitetura |

---

## 7. Próximos passos, em ordem

Os primeiros itens são pré-requisitos reais dos seguintes.

1. **Fechar os bugs de `main.c`** — o `find_move` com `flags` no lugar de `&out`, e usar o
   lance devolvido no `make_move`.
2. **Completar a `Board` e o ciclo da FEN** — os quatro campos, `parse_fen` lendo os campos
   3 a 6, `board_to_fen` emitindo os seis. Enquanto o serializador só emite a posição, o
   round-trip não prova nada sobre roque e en passant.
3. **Fixar a assinatura de `generate_moves`** antes de gerar mais lances:
   `int generate_moves(const Board *board, MoveList *out)`. `const` porque geração não
   modifica nada; retorno é a contagem.
4. **Peças deslizantes** — uma função só, parametrizada por intervalo de direção (torre 0–3,
   bispo 4–7, rainha 0–7). Laço interno de três saídas: vazia → adiciona e continua;
   inimiga → adiciona e para; amiga → para.
5. **Cavalo e rei por tabela** pré-computada de destinos. Peão por último, é o que tem mais
   casos (dupla, duas capturas, en passant, promoção em quatro peças).
6. **`is_square_attacked` → `make_move`/`unmake_move` completos → filtro de legalidade.**
   Adicionar um `assert` de debug que recalcula `king_square` do zero e compara com o cache
   — campo denormalizado que dessincroniza é dos bugs mais chatos de rastrear.
7. **Perft, e perft divide.** Comparar com os valores publicados (inicial, Kiwipete).
   `perft divide` é a técnica de depuração: mostra a contagem por lance de raiz, você compara
   com o Stockfish (`go perft N`), acha o que diverge, entra nele e repete — busca binária
   dentro da árvore. **Nada de busca ou avaliação antes do perft bater.**
8. **Protocolo stdin/stdout.** Conjunto mínimo: `uci`, `isready`, `quit`, `position`,
   `legalmoves`, `go movetime`. Só depois disso, a IA.

### Armadilha do protocolo, para quando chegar lá

Quando a stdout é um terminal, a libc usa buffer de linha e cada `printf` sai na hora.
Quando é um **pipe** — exatamente o caso do cliente C++ rodando o motor como subprocesso —
ela muda para buffer de bloco de 4 KB. O `bestmove` fica preso, o cliente espera para
sempre, e não há sintoma para depurar. Duas linhas no começo do `main` resolvem:

```c
setvbuf(stdout, NULL, _IOLBF, 0);
setvbuf(stdin,  NULL, _IONBF, 0);
```

Junto: cuidado com `\r\n` se a equipe do cliente estiver no Windows.

---

## 8. Referências ativas

- **Chess Programming Wiki** — verbetes centrais para o que vem a seguir:
  [Encoding Moves](https://www.chessprogramming.org/Encoding_Moves),
  [Move List](https://www.chessprogramming.org/Move_List),
  [Pawn Push](https://www.chessprogramming.org/Pawn_Push),
  [Make Move](https://www.chessprogramming.org/Make_Move),
  [Square Attacked By](https://www.chessprogramming.org/Square_Attacked_By),
  [Perft](https://www.chessprogramming.org/Perft) e
  [Perft Results](https://www.chessprogramming.org/Perft_Results).
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas de texto puro, vale ler inteira.
- **TSCP** (Tom Kerrigan's Simple Chess Program) — engine mailbox completa em ~2000 linhas
  de C legível. Já está no repositório, em `tscp183b/`.
- `man gcc`, seção *Options to Request or Suppress Warnings* — em especial a nota sob
  `-Wmaybe-uninitialized` sobre a dependência do nível de otimização.

---

# Apêndice — Headers

A interface completa do projeto. Cinco headers, ~120 linhas de declaração descrevendo
~750 linhas de código.

## `src/types.h`

```c
#ifndef TYPES_H
#define TYPES_H

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int16_t i16;

#define INPUT_STR_SIZE 128
#define BOARD_SIZE 64
#define BOARD_WIDTH 8
#define GEN_MOVES_MAX 256

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define MemoryZero(addr, size) memset((addr), 0x0, (size))
#define MemoryZeroStruct(addr, st) MemoryZero((addr), sizeof(st))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define SQ_NONE (-1)
#define RANK_OF(sq) ((sq) / BOARD_WIDTH)
#define FILE_OF(sq) ((sq) % BOARD_WIDTH)
#define FILE_DIST(dest, orig) (abs(FILE_OF(dest) - FILE_OF(orig)))
#define SQ_FROM_RF(rank, file) ((rank) * BOARD_WIDTH + (file))
#define SQ_OFFBOARD(sq) (((sq < 0) || (sq >= 64)) ? 1 : 0)

#define MOVE_MASK 0xFFUL
#define MAX_FEN_STRING 256

typedef enum {
    WHITE = 1,
    BLACK = 0
} Color;

typedef enum {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
} PieceType;

typedef uint8_t Piece;

#define PIECE_TYPE_MASK 0x7
#define COLOR_MASK 0x1
#define MAKE_PIECE(color, type) \
    ((Piece)((((color) & COLOR_MASK) << 3) | ((type) & PIECE_TYPE_MASK)))
#define TYPE_OF(piece) ((PieceType)((piece) & PIECE_TYPE_MASK))
#define COLOR_OF(piece) ((Color)(((piece) >> 3) & COLOR_MASK))

typedef struct {
    Piece array[BOARD_SIZE];
    Color side_to_move;
    int king_square[2];
} Board;

typedef enum {
    DIR_N,
    DIR_S,
    DIR_E,
    DIR_W,
    DIR_NE,
    DIR_SW,
    DIR_SE,
    DIR_NW,
    DIR_COUNT
} Direction;

typedef struct {
    u8 origin_sq;
    u8 target_sq;
    u8 promotion;
    u8 flags;
} MoveDescription;

typedef struct {
    u32 move;
    int score;
} Move;

typedef struct {
    Move moves[GEN_MOVES_MAX];
    int count;
} MoveList;

#endif
```

## `src/log.h`

```c
#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdlib.h>

void log_msg(const char *msg, const char *file, int line, const char *func);

// #define LOG_ERROR(msg) log_msg((msg), __FILE__, __LINE__, __func__)

#ifdef DEBUG
    #define LOG_ERROR(msg) log_msg((msg), __FILE__, __LINE__, __func__)
#else
    #define LOG_ERROR(msg) ((void)0)
#endif

#endif
```

> `__FILE__`, `__LINE__` e `__func__` são capturados **na macro**, não dentro de `log_msg`.
> Se ficassem na função, ela reportaria a si mesma em toda chamada. Isso já foi bug aqui.

## `src/board.h`

```c
#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "log.h"

bool parse_fen(const char *fen_string, Board *out);
void board_to_fen(Board *board, char fen_out[MAX_FEN_STRING]);

int sq_from_coord(const char *coord);
void coord_from_sq(int sq, char out[3]);

extern const char PIECE_CHAR[17];

#endif
```

> `PIECE_CHAR` é `".pnbrqk..PNBRQK."` — indexado **diretamente pelo valor codificado da peça**.
> Peão branco = 9 → `'P'`; peão preto = 1 → `'p'`. Sem `if` nenhum.

## `src/movegen.h`

```c
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "types.h"
#include "utils.h"

void precompute_move_data();
void add_move(u32 move, MoveList *list);
void generate_pawn_moves(Board *board, MoveList *list);
void print_moves(MoveList *list);
void print_square_directions(char sq_str[3]);
bool find_move(Board *board, int origin_sq, int target_sq, int promo, u32 *out);
u32 encode_move(int origin_sq, int target_sq, int promo, int flags);
MoveDescription decode_move(u32 move);
u32 str_to_move(char move_in[5]);
void make_move(Board *b, u32 move);

#endif
```

> `precompute_move_data()` deveria ser `(void)` — em C, `()` declara "argumentos não
> especificados", não "nenhum argumento". `-Wstrict-prototypes` avisa.

## `src/utils.h`

```c
#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include "log.h"
#include "board.h"

void print_piece_chart();
void print_board(const Board *board);
void get_fen(char fen[MAX_FEN_STRING]);
int get_int(char msg[INPUT_STR_SIZE]);
void strslc(const char *src, char *dest, int start, int end);

static void clear_screen() {
    printf("\e[1;1H\e[2J");
    fflush(stdout);
}

#endif
```

> `clear_screen` é uma função `static` **definida dentro de um header** — cada unidade de
> tradução ganha a própria cópia, e todas avisam "definida mas não usada". Deveria ser
> `static inline`, ou ir para o `.c`. O `\e` também não é padrão C (é extensão GNU);
> a forma portável é `\x1b`.

---

*Este documento é escrito à mão — o apêndice de headers precisa ser atualizado manualmente
quando as interfaces mudarem.*
