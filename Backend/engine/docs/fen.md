# FEN: leitura, validação e escrita

Documento de referência do módulo `src/fen.c` (antes de 12/09, esta lógica vivia em
`src/board.c` — o split de módulos moveu o código, não o comportamento). Cobre o formato,
o desenho do parser, cada validação feita (e por quê), os bugs que foram corrigidos e o que
ainda falta.

Escrito em 2026-09-07, junto com a expansão do `parse_fen` (hoje `fen_parse`) para os seis
campos.

---

## 1. O formato

Uma FEN descreve uma posição completa em uma linha, com seis campos separados
por espaço:

```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
└──────────────── 1 ────────────────┘ │ │    │ │ │
                                      2 3    4 5 6
```

| # | Campo | Conteúdo |
|---|-------|----------|
| 1 | Peças | As 8 fileiras, da **8 para a 1**, separadas por `/`. Dentro de cada fileira, da coluna `a` para a `h`. Letra maiúscula = branca, minúscula = preta. Dígito de 1 a 8 = sequência de casas vazias |
| 2 | Lado a jogar | `w` ou `b` |
| 3 | Roque | Subconjunto de `KQkq`, **nesta ordem**, ou `-` |
| 4 | En passant | Casa alvo (`e3`, `d6`), ou `-` |
| 5 | Halfmove clock | Lances desde a última captura ou avanço de peão. É o contador da regra dos 50 lances |
| 6 | Fullmove number | Número do lance completo. Começa em 1 e incrementa depois de cada lance das pretas |

### Notas originais deste arquivo

Conteúdo que já estava aqui antes desta revisão, preservado — o diagrama é a
melhor explicação visual da indexação que existe no repositório:

<img src="game_example.png" width="500" Alt="Description">

O formato FEN desse jogo seria: `rnbqk1nr/pppp1ppp/8/2b5/3pP3/5N2/PPP2PPP/RNBQKB1R`

```
#define START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
                     |      |                           |      |
                     56     63                          0      7
board[64] =
                [0, 1, 2, 3, ..., 61, 62, 63]

                0  -->  a1
                7  -->  h1
                56 -->  a8
                63 -->  h8

posição 56: rank = 7, file = 0;
posição 63: rank = 7, file = 7;
posição 48: rank = 6, file 0  ->  6 * 8 + 0 = 48
```

### A pegadinha da ordem das fileiras

O projeto indexa as casas com **`a1 = 0`** (Little-Endian Rank-File):
`sq = rank * 8 + file`, `h8 = 63`.

A FEN é escrita **de trás para frente** em relação a isso: a primeira coisa que
aparece na string é a fileira 8, que ocupa os índices **56 a 63**. Por isso o
laço de leitura começa em `rank = 7` e decrementa, enquanto o de escrita
(`board_to_fen`) faz `for (rank = 7; rank >= 0; rank--)`.

Errar isso produz um tabuleiro espelhado verticalmente, que *parece* válido e
só quebra quando um peão começa a andar para o lado errado.

### O campo de en passant não é onde o peão está

É a casa **atrás** do peão que acabou de dar o avanço duplo — a casa onde o
capturador vai parar. Se as pretas jogam `d7d5`, o peão está em **d5** e o
campo da FEN diz **d6**.

Consequência direta, usada na validação: a fileira do campo é determinada por
quem joga.

| Lado a jogar | Último lance foi | Casa de en passant na fileira | Índice de `RANK_OF` |
|---|---|---|---|
| Brancas | preto, de 7 para 5 | 6 | 5 |
| Pretas | branco, de 2 para 4 | 3 | 2 |

---

## 2. Desenho do parser

`parse_fen()` deixou de ser um bloco único e virou um despachante que chama uma
função por campo. Cada uma valida o que é seu, registra o próprio erro e devolve
`bool`:

```c
static int  split_fen_fields(char *fen_mut, char *fields[], int max_fields);

static bool parse_placement   (const char *field, Board *b);   /* campo 1 */
static bool parse_side_to_move(const char *field, Board *b);   /* campo 2 */
static bool parse_castling    (const char *field, Board *b);   /* campo 3 */
static bool castling_matches_board(const Board *b);            /* coerência */
static bool parse_ep_square   (const char *field, Board *b);   /* campo 4 */
static bool parse_uint_field  (const char *field, int min, int max, int *out);
```

Três propriedades que esse desenho garante:

**Todas são `static`.** Não aparecem em `board.h`, então o compilador impede
que outro `.c` dependa delas. A fronteira do módulo deixa de ser convenção e
vira regra verificada na compilação — é o mesmo raciocínio da conversa sobre
camadas: transformar a arquitetura em algo que o compilador checa.

**A ordem das chamadas não é arbitrária.** Ela é uma cadeia de dependências
reais:

```
1. peças ─────────┬──────────────► 3. roque      (precisa do rei e da torre no tabuleiro)
                  │
2. lado ──────────┼──────────────► 4. en passant (precisa do tabuleiro E do lado)
                  │
                  └──────────────► 5/6. relógios (independentes, por último)
```

**Ou tudo, ou nada.** A montagem acontece num `Board b = {0}` local; `*out` só
é escrito na última linha, depois que todas as validações passaram:

```c
    *out = b;
    return true;
```

Isso importa na prática: no menu, quando o usuário digita uma FEN inválida, o
tabuleiro que já estava lá sobrevive intacto. Não existe estado meio-preenchido.

### `split_fen_fields` e a regra de não alocar

A função devolve os campos **num vetor fornecido pelo chamador**:

```c
char *fields[FEN_MAX_FIELDS];
int   field_count = split_fen_fields(copy, fields, FEN_MAX_FIELDS);
```

A versão anterior tentava devolver o vetor por valor e por isso não funcionava
de jeito nenhum — ver a seção 4.

Dois detalhes que valem lembrar ao mexer nela:

- Os ponteiros em `fields[]` apontam **para dentro de `copy`**. Se `copy` sair
  de escopo, viram ponteiros pendentes. Por isso `copy` é declarado em
  `parse_fen` e vive até o fim dela.
- `strtok` guarda estado numa variável estática interna: não é reentrante e não
  pode ser chamada intercalada com outro laço de `strtok`. A engine é single
  thread e ninguém mais a usa, então está seguro. Se isso mudar, a substituta é
  `strtok_r` (POSIX) ou uma varredura manual.

### Quantos campos são aceitos

Aceita de **4 a 6**. Os relógios são opcionais porque bancos de posições e
strings EPD costumam omiti-los; quando faltam, valem `0` e `1`. Os quatro
primeiros são obrigatórios: sem eles não dá para saber de quem é a vez nem se o
roque ainda é possível.

Sete campos ou mais é erro — `split_fen_fields` devolve `-1` quando sobra token
depois de encher o vetor.

---

## 3. Cada validação, e o que ela evita

### Campo 1 — peças

| Verificação | O que evita |
|---|---|
| `/` só quando a fileira somou 8 colunas | Fileira curta ou longa |
| `rank < 0` depois de decrementar | **Escrita fora dos limites.** Com 9 ou mais fileiras (`8/8/8/8/8/8/8/8/8`), `rank` chega a −1 e `sq = rank*8 + file` fica negativo. O teste vem antes de qualquer escrita em `array[]` |
| Dígito entre `1` e `8` | `0` e `9` |
| `file + n > 8` | Sequência de vazios estourando a fileira |
| `file >= 8` antes de colocar peça | Peça além da coluna `h` |
| Terminar com `rank == 0 && file == 8` | Placement que não cobre 64 casas |
| Exatamente um rei de cada cor | Posição sem rei — quebraria `king_square[]` e o filtro de legalidade |
| No máximo 8 peões por cor | Contagem impossível |
| No máximo 16 peças por cor | Idem |
| Peão nas fileiras 1 ou 8 | Posição impossível (o peão teria promovido) |

Sobre a pergunta que estava no código (`// claude, é necessário essa verificação?`
no teste `(rank >= 8) || (rank < 0)`): **metade é necessária, metade é morta.**
`rank` começa em 7 e só decrementa, então `rank >= 8` nunca acontece — foi
removido. Já `rank < 0` é indispensável e é a única coisa entre a FEN e uma
escrita em `board->array[-1]`. Ficou:

```c
if (rank < 0) {
    LOG_ERROR("too many ranks in board placement");
    return false;
}
```

### Campo 3 — roque

Duas camadas.

**Sintaxe** (`parse_castling`): `-`, ou até 4 letras de `KQkq`, cada uma no
máximo uma vez. A mudança importante em relação à versão anterior é que
**caractere desconhecido agora é erro**. Antes, o `switch` tinha
`default: break;` e engolia em silêncio: `KQxq` virava `KQq` sem nenhum aviso, e
uma FEN corrompida só apareceria como uma divergência de perft muito depois.

**Coerência com o tabuleiro** (`castling_matches_board`): um direito de roque só
faz sentido se o rei e a torre correspondentes ainda estiverem nas casas de
origem.

| Direito | Exige rei em | Exige torre em |
|---|---|---|
| `K` | e1 | h1 |
| `Q` | e1 | a1 |
| `k` | e8 | h8 |
| `q` | e8 | a8 |

`KQkq` num tabuleiro sem torre em h1 é uma FEN inconsistente, e aceitá-la faria
o gerador produzir um roque com uma torre que não existe.

> **Decisão consciente: rejeitar, não corrigir em silêncio.** Algumas engines
> apenas apagam o bit inválido. Aqui a FEN é recusada. O motivo é de depuração:
> quando `board_to_fen()` reescrever a posição, um `make_move` que esqueceu de
> limpar o direito ao mover a torre aparece **na hora**, em vez de virar uma
> divergência de perft na profundidade 5. Se um dia for preciso engolir FENs de
> bancos externos mal formados, trocar os `return false` por
> `b->castling_rights &= ~need[i].bit`.

Assume roque padrão, não Chess960.

### Campo 4 — en passant

```c
int expected_rank = (b->side_to_move == WHITE) ? 5 : 2;
```

Além da fileira, as três casas envolvidas são checadas — custa quase nada e
evita gerar uma captura en passant fantasma:

- a própria casa de en passant tem que estar **vazia**;
- a casa de onde o peão saiu tem que estar **vazia**;
- o peão que avançou tem que estar de fato lá, e ser **do adversário**.

Repare na ordem do último teste no código:

```c
if ((TYPE_OF(b->array[pawn_sq]) != PAWN) ||
    (COLOR_OF(b->array[pawn_sq]) != pusher)) {
```

`TYPE_OF` vem antes de `COLOR_OF` de propósito. `COLOR_OF(EMPTY)` devolve
`BLACK` — a armadilha registrada no `CLAUDE.md` §5 — então uma casa vazia se
disfarçaria de peão preto. Como `TYPE_OF(EMPTY) == EMPTY != PAWN`, testar o
tipo primeiro já barra o caso.

### Campos 5 e 6 — relógios

A versão anterior era `strtol(fields[4], NULL, 10)` sem nenhuma checagem, o que
aceitava qualquer coisa: `"abc"` virava `0` silenciosamente. Agora
`parse_uint_field` exige campo não vazio, **só dígitos**, e resultado dentro da
faixa, com `errno == ERANGE` cobrindo números absurdos.

| Campo | Faixa | Constante |
|---|---|---|
| Halfmove clock | 0 … 1000 | `FEN_HALFMOVE_MAX` |
| Fullmove number | 1 … 9999 | `FEN_FULLMOVE_MAX` |

Os máximos não vêm da especificação (que não põe teto). São generosos o
bastante para qualquer partida real e apertados o bastante para pegar digitação
errada e estouro de `int`.

---

## 4. Bugs corrigidos

### 4.1 O erro de compilação — `board.c:188`

```c
for (char *ptr = fields[2][0]; *ptr != '\0'; ptr++) {
```

`fields` é `char *[6]`, então `fields[2]` é a string e `fields[2][0]` é o
**primeiro caractere dela**. Atribuir um `char` a um `char *` é
`-Wint-conversion`: o valor do caractere (`'K'` = 75) vira endereço, e o
primeiro `*ptr` é uma leitura em `(char*)75`. Era `fields[2]`.

O parser novo não tem essa forma: `parse_castling` recebe o campo já como
`const char *`.

### 4.2 `split_fen_fields` devolvia um vetor local

```c
char** split_fen_fields(char fen_string[MAX_FEN_STRING]) {
    char copy[MAX_FEN_STRING];      /* local */
    char *fields[FEN_PARSE_FIELDS]; /* local */
    ...
    if (token == NULL) { ...; return false; }   /* 'false' num char** */
    ...
    return fields;                  /* endereço de local: -Wreturn-local-addr */
}
```

Três problemas de uma vez:

1. Devolve o endereço de um vetor automático, que deixa de existir no `return`.
2. Mesmo que devolvesse, os ponteiros dentro dele apontam para `copy`, que
   também morre ali.
3. `return false` numa função que devolve `char **` — `false` é `0`, então
   compila como ponteiro nulo, mas quem chama não tem como distinguir isso de
   um resultado válido.

Corrigido invertendo a direção: o chamador fornece o buffer e o vetor, e a
função devolve **quantos** campos achou.

### 4.3 `coord_from_sq` sem `return` — e o `` `1 `` no `print_board`

```c
void coord_from_sq(int sq, char out[3]) {
    if (sq == SQ_NONE) {
        out[0] = 'X'; out[1] = 'X'; out[2] = '\0';
    }                       /* <- faltava o return */
    out[0] = 'a' + FILE_OF(sq);
    out[1] = '1' + RANK_OF(sq);
    out[2] = '\0';
}
```

Sem o `return`, o ramo de erro escrevia `"XX"` e caía direto no código de baixo,
que sobrescrevia tudo. Com `sq == -1`: em C a divisão trunca para zero, então
`FILE_OF(-1)` é `-1` e `out[0]` virava `'a' - 1 == '`'`. O resultado era visível
no `print_board` — a casa de en passant aparecia como `` `1 `` em toda posição
sem en passant.

Agora usa `SQ_OFFBOARD(sq)`, que cobre `SQ_NONE` e qualquer outro valor fora de
0–63, escreve `"--"` (a mesma convenção do `-` da FEN) e **retorna**.

### 4.4 `board_to_fen` emitia só o campo das peças

```c
    fen_out[pos++] = '\0';
    fen_out[pos]   = ' ';   /* espaço DEPOIS do terminador */
```

O espaço era gravado numa posição que ninguém lê. O efeito prático é que o
round-trip mentia:

```
parse_fen("… w KQkq d6 0 3")  ->  board_to_fen  ->  "…"   (sem os 5 campos)
                                                     └─> parse_fen falha
```

Perder lado a jogar, roque e en passant no caminho é exatamente o tipo de coisa
que o perft acusa e ninguém consegue localizar. Como o servidor JS vai receber
essa string, ela precisa carregar a posição inteira.

Agora emite os seis campos. A ordem `KQkq` no campo 3 é obrigatória pela
especificação — `qkQK` descreve a mesma posição mas não é FEN válida.

### 4.5 `MoveList l;` sem inicializar — `main.c`

```c
MoveList l;                       /* count com lixo */
generate_pawn_moves(b, &l, WHITE);
```

`add_move` usa `list->count` como índice de escrita. Com lixo lá, a primeira
geração grava fora do vetor. O `MemoryZeroStruct` que existia acontecia *depois*
de imprimir, então a primeira iteração do menu sempre corria com lixo.

Este é o bug que o `CLAUDE.md` §7 descreve: **`-Wall -g` sozinho não o pega.**
O GCC só faz análise de fluxo de dados com otimização ligada. Foi por isso que
o Makefile passou a usar `-Og`.

### 4.6 Higiene que estava escondendo warnings

| Onde | Problema | Correção |
|---|---|---|
| `utils.h` | `static void clear_screen()` **definida dentro do header** — recompilada em cada TU e "defined but not used" em todas | Declaração no header, definição em `utils.c` |
| `utils.h` | `"\e[1;1H\e[2J"` — `\e` é extensão do GCC, `-Wpedantic` reclama | `"\033[1;1H\033[2J"` |
| `utils.h` | `int get_int(char msg[INPUT_STR_SIZE])` promete vetor de 128 bytes; passar `"Insert option: "` (16) disparava `-Wstringop-overflow` | `int get_int(const char *msg)` |
| `utils.h`, `movegen.h` | `void f()` significa "argumentos não especificados", não "nenhum argumento" — `-Wstrict-prototypes` | `void f(void)` |
| `utils.c` | retorno de `fgets` ignorado: em EOF (um Ctrl-D no menu) o buffer ficava com lixo | Testa `NULL`, devolve string vazia |
| `types.h` | `SQ_NONE` definido duas vezes | Removida a segunda |
| `types.h` | `SQ_OFFBOARD(sq)` sem parênteses no parâmetro (`CLAUDE.md` §7) e com `64` literal | `(((sq) < 0) \|\| ((sq) >= BOARD_SIZE))` |
| `types.h` | `Board.castling` vs `Undo.castling_rights` | Ambos `castling_rights` |
| `board.c` | `print_board` definida aqui mas declarada em `utils.h` | Declaração movida para `board.h` |

Depois disso o build fica **limpo** com o conjunto completo do `CLAUDE.md` §7:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic -Wstrict-prototypes -Og -g src/*.c
```

E o Makefile passou a usar esse conjunto — antes era só `-Wall -g`, que
escondia a maioria dos itens desta tabela.

---

## 5. Como isto foi verificado

29 casos, rodando sob `-fsanitize=address,undefined`: 8 FENs válidas
(inicial, Kiwipete, posições 3 e 4 do perft, en passant real, EPD de 4 e 5
campos, só reis), 16 inválidas (campos a mais e a menos, lado inválido, letra
inválida e duplicada no roque, roque sem torre, en passant na fileira errada,
sem peão e com coordenada inválida, halfmove não numérico, fullmove zero e
estourando, 9 fileiras, sem rei, dígito 9, peão na fileira 8) e 5 round-trips
`FEN -> Board -> FEN` conferindo string idêntica.

O arquivo de teste está no diretório temporário da sessão, não no repositório.
**Vale trazê-lo para cá como `tests/test_fen.c`** — é a base do portão de CI
que o `CLAUDE.md` §3 pede, e é barato agora que o parser está estável.

---

## 6. O que ainda falta

| Item | Nota |
|---|---|
| **Rei adversário em xeque** | Uma FEN onde o lado que *não* joga está em xeque é ilegal (o lance anterior seria impossível). Precisa de `is_square_attacked`, que ainda não existe. É a última validação que falta para `parse_fen` estar completa |
| **`str_to_move` não checa `sq_from_coord`** | `movegen.c:37` — a função devolve `SQ_NONE` para coordenada inválida, e `encode_move(-1, …)` corrompe o lance. Digitar `zz99` no menu passa direto |
| **`print_board` escreve em stdout** | Vai ser o canal do UCI. Quando `uci.c` existir, isto tem que ir para stderr ou ficar atrás de um comando explícito de depuração, senão o cliente JS recebe o desenho do tabuleiro no meio das respostas do protocolo |
| **`LOG_ERROR` some fora do `-DDEBUG`** | `CLAUDE.md` §8. "Sua FEN está errada" é condição que o cliente precisa ver sempre. O menu já contorna com um `printf` no caso 1, mas a solução geral continua em aberto |
| **Halfmove clock não é usado** | O campo é lido e guardado, mas a regra dos 50 lances ainda não existe |

---

## 7. Referências

- [Forsyth-Edwards Notation](https://www.chessprogramming.org/Forsyth-Edwards_Notation) — CPW, a definição canônica dos seis campos.
- [En passant](https://www.chessprogramming.org/En_passant) — em especial a distinção entre a casa alvo e a casa do peão.
- [Perft Results](https://www.chessprogramming.org/Perft_Results) — as FENs de Kiwipete e das posições 3 a 6 usadas nos testes acima.
- `man 3 strtok` — a nota sobre a variável estática interna e a reentrância.
