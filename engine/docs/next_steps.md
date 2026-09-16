# Próximos Passos — de `unmake_move` até o protocolo

> Guia de implementação e arquitetura. **Documento vivo**: cada etapa tem critério de saída
> objetivo; marque conforme avança e registre aqui o que mudou de opinião no caminho.
>
> Estado base: **16 de setembro de 2026** (commit `f79d50a` + árvore de trabalho).
> Contexto e estado bug-a-bug: `project_context.md`. Fases de longo prazo: `roadmap-motor.md`.
>
> **Esta é a terceira versão deste documento.** A anterior (10/09) foi escrita contra o
> `movegen.c` monolítico e planejava um `make_move` que não existia. Ele existe agora, está
> verificado, e o plano de lá foi cumprido quase linha a linha — inclusive as duas armadilhas
> (en passant e `CASTLE_MASK` no destino). O que não foi cumprido é o espelho.
>
> Escopo deste documento: `unmake_move`, o teste-portão, o cavalo, a limpeza do build, e o
> caminho até o UCI mínimo. **O filtro de legalidade fica de fora de propósito** — ele depende
> de tudo isto estar correto primeiro.

---

## 1. Estado em 2026-09-16

O que mudou desde a revisão de 13/09 do `project_context.md`:

| Área | Estado |
|---|---|
| `make_move` | ✅ **Completo e verificado.** Undo, captura, roque, en passant, promoção, `castle_mask` por origem **e** destino, relógios, `king_square` |
| `board_check_invariants` | ✅ Implementada — 4 das 6 checagens planejadas |
| `board_find_king` | ✅ Implementada |
| Rei | ✅ Gerado pelas 8 direções de `SQ_TO_EDGE`, como decidido |
| Roque | ✅ Gerado (direito + casas vazias). Xeque e casa atacada ficam para o filtro, de propósito |
| Peão | ✅ Completo: push, duplo aninhado, captura, promoção nas 4 peças, en passant. Sem o parâmetro `side` |
| `generate_all_moves` | ✅ Entrada única |
| REPL de `main.c` | ✅ Consertado — usa o índice de `movelist_find` e aplica o lance real |
| Predicados de `MoveType` | ✅ Roque e EP comparam por igualdade, não por máscara |
| **`unmake_move`** | ❌ **Sem corpo.** Erro de link, reproduzido |
| **Cavalo** | ❌ Não gerado. Posição inicial dá 16 lances, não 20 |
| `is_square_attacked`, legalidade, perft, UCI | ❌ Não existem |
| Testes automatizados | ❌ Não existem. `test/test.c` é um REPL manual sem assertions |

Build atual: **8 avisos**, zero erros. Três deles são bugs reais (`project_context.md` §5).

---

## 2. A etapa 1 é `unmake_move`, e não há discussão

Tudo o que vem depois — filtro de legalidade, perft, busca, e o próprio teste que prova que
`make_move` continua certo amanhã — passa por ela. Hoje `makemove.c:130` é:

```c
void unmake_move(Board *b, Move move, const Undo *u);
```

Uma declaração repetida, terminada em `;`. O compilador não reclama (é uma redeclaração
válida) e o link só quebra se alguém chamar. Ninguém chama. Por isso o build passa.

### 2.1 A ordem, que é o espelho da de `make_move`

```
1. inverter side_to_move                                       ← PRIMEIRO
2. decodificar o lance (from, to, type)
3. desfazer o caso especial:
     promoção → array[from] recebe um PEÃO, não a peça promovida
     roque    → devolver a torre para a casa de origem
     ep       → recolocar o peão adversário em to - PAWN_PUSH[side]
4. mover a peça de volta: array[from] = array[to]
5. restaurar a casa de destino:
     ep       → array[to] = EMPTY   (a captura não estava lá)
     senão    → array[to] = u->captured
6. restaurar do Undo: castling_rights, ep_square, halfmove_clock
7. fullmove_number: -1 se quem tinha jogado era BLACK
8. king_square, se a peça era rei (inclui o roque)
```

**O passo 1 vem primeiro, e é a assimetria que importa.** Todo o resto depende de saber de
quem era a vez: de que lado o peão veio, onde estava o peão capturado no en passant, qual
torre volta no roque. Fazer por último significa calcular tudo com a cor errada — e o
sintoma disso não é um crash, é um perft errado três níveis abaixo.

### 2.2 Armadilha — en passant, e por que o `Undo` sozinho não basta

```
Peão branco em e5, pretas acabaram de jogar d7-d5.  ep_square = d6.

  Lance:           e5xd6 en passant
  Peça capturada:  o peão preto em d5, não em d6
  Casa a limpar:   ep_square - PAWN_PUSH[WHITE]  =  d6 - 8  =  d5
```

`make_move` já faz isso certo (verificado: `e5f6` na FEN de teste devolve o tabuleiro
correto). O que `unmake_move` encontra é a consequência: **`u->captured` vale `EMPTY`**,
porque a casa de destino estava vazia quando o `Undo` foi gravado. O peão capturado não pode
ser restaurado a partir do `Undo`.

Ele tem de ser **deduzido**: `MV_EP_CAPTURE` já diz que é um peão, e `side` já diz de que
cor. Não é defeito do `Undo` — é precisamente o motivo pelo qual o lance volta como parâmetro
em vez de morar dentro do `Undo`. Se você se pegar querendo adicionar um campo ao `Undo` para
resolver isso, a resposta é: o `MoveType` já é esse campo.

No `unmake`, restaurar a peça em `to` faz **um peão sumir e outro aparecer** — e o tabuleiro
continua com o número certo de peças, então nenhuma checagem de contagem pega. Quem pega é o
round-trip de FEN (§3).

### 2.3 Armadilha — promoção

O que volta para `array[from]` é um **peão**, não a peça promovida. Parece óbvio escrito
assim, e é o erro mais comum aqui, porque o passo 4 ("mover a peça de volta") lê
`array[to]` — que é a dama.

E o caso duplo: `MV_PROMO_CAP_*` é promoção **e** captura. Os dois tratamentos se aplicam.

### 2.4 Armadilha — roque

`make_move` move o rei pelo caminho normal e a torre pelo `switch (to)`. `unmake_move` tem de
desfazer os dois. As quatro casas são as mesmas quatro (`SQ_C1`, `SQ_G1`, `SQ_C8`, `SQ_G8`),
e a torre volta para `SQ_A1`/`SQ_H1`/`SQ_A8`/`SQ_H8`.

Os direitos de roque **não** precisam ser recalculados: eles vêm do `Undo`. É exatamente o
que o `Undo` existe para guardar, porque são irrecuperáveis do tabuleiro pós-lance.

### 2.5 Enquanto estiver no arquivo, três coisas baratas

Regra acordada no `CLAUDE.md` §3: nada de passe de refatoração separado; cada limpeza entra
quando o arquivo já vai ser aberto. `makemove.c` vai ser aberto agora.

- **`king_square` por varredura completa** (`makemove.c:72-73`). `make_move` chama
  `board_find_king` **duas vezes por lance** — 128 leituras por nó de busca, para um campo
  que existe justamente para evitar varredura. Atualizar só quando `PIECE_TYPE(p) == KING`.
  Não é otimização prematura: é o cache voltar a ser cache.
- **`fill_sq(b, "d1", …)`** (`makemove.c:87`). Parsing de string no caminho mais quente do
  motor, com o `bool` de retorno descartado. Acrescente `SQ_D1`, `SQ_F1`, `SQ_D8`, `SQ_F8` ao
  enum de `square.h` e escreva direto em `b->array[…]`.
- **`check_rook_squares` mente no nome** (`makemove.c:8`): ela também trata `SQ_E1`/`SQ_E8`,
  que são casas de rei. `update_castle_rights_for_square` ou a tabela `CASTLE_MASK[64]` que o
  plano de 10/09 sugeria — a tabela faz as duas metades (origem e destino) caírem na mesma
  linha, e é difícil lembrar de uma e esquecer da outra.

### Critério de saída

`unmake_move` linka e desfaz um lance simples. Isso é só o pré-requisito da Etapa 2, que é o
portão de verdade.

---

## 3. Etapa 2 — O teste-portão 🚧

**Esforço:** 1 sessão. **É o item de maior retorno do projeto inteiro, e já era antes.**

### 3.1 O teste

```
para cada FEN do corpus:
    fen_parse(fen, &b)
    assert(board_check_invariants(&b))
    fen_antes = fen_write(&b)

    generate_all_moves(&b, &lista)
    para cada lance m da lista:
        Undo u;
        make_move(&b, m, &u)
        unmake_move(&b, m, &u)
        assert(fen_write(&b) == fen_antes)
        assert(board_check_invariants(&b))
```

~30 posições × ~35 lances = **~1000 verificações**, e roda em milissegundos. Ele pega
praticamente todo bug de make/unmake — incluindo §2.2, §2.3 e §2.4 — **antes** que o perft
precise entrar em cena.

E o motivo pelo qual ele funciona tão bem é que **o round-trip da FEN já está verificado**:
você está comparando contra um serializador em que confia. Foi essa maturidade do módulo
`fen` que permitiu mover o teste de corpus da Fase 1 para cá, no `roadmap-motor.md` — ele
não desapareceu, ele virou o oráculo deste teste.

### 3.2 Onde ele mora

Subcomando do binário, como decidido em 10/09:

```
./build/main.out            REPL (o de hoje)
./build/main.out test       este teste
./build/main.out perft N    contagem, quando existir
```

Sem framework, sem infraestrutura nova: assertions e um contador de passa/falha. E `make`
roda `test` — um portão que ninguém executa não é portão.

**Nota sobre `test/test.c`:** ele existe e o Makefile tem um alvo `test`, mas hoje é um
segundo REPL manual (lê lance do stdin, imprime o tabuleiro, nenhuma assertion). Ou ele vira
este teste, ou vira `repl.c` e o teste nasce em outro arquivo. O que não pode continuar é
um alvo chamado `test` que não testa — é pior que não ter, porque parece cobertura.

### 3.3 O corpus

Num `.c` próprio, como `const char *CORPUS[]`. **Não** um arquivo texto lido em runtime: sem
I/O, sem caminho relativo, sem caso de erro para tratar, e ele compila junto.

Comece com estas, que já cobrem os caminhos que existem hoje:

```c
"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"              /* inicial       */
"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"  /* Kiwipete      */
"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"                             /* Perft pos 3   */
"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"      /* Perft pos 4   */
"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"                                  /* 4 roques vivos*/
"r3k2r/8/8/8/8/8/8/R3K2R b kq - 0 1"                                    /* roque parcial */
"rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3"         /* en passant    */
"rn5k/1P2P3/8/8/8/8/8/7K w - - 0 1"                                     /* promoção x2   */
```

As quatro primeiras são as posições de referência dos *Perft Results* — usá-las aqui
significa que, quando o perft chegar, o corpus já é o certo.

### 3.4 Um cuidado no `board_check_invariants` antes de rodar 1000 vezes

`fail_msg` acumula num log global que **nunca é zerado** (`utils.c:14`). Em ~1000 iterações
ele bate no teto de 128 e passa a emitir `LOG_ERROR("too many fail messages")` por cima da
saída do teste. Ou o log ganha um `fail_log_reset()`, ou o teste chama `print_fail_log()` e
para na primeira falha. A segunda opção é melhor: numa suíte de round-trip, a **primeira**
falha é a informativa; as 999 seguintes são consequência.

### Critério de saída 🚧

`./build/main.out test` verde nas ~30 posições, com invariantes passando. **Não avance sem
isso** — e depois de cada etapa seguinte, rode de novo. De portão único ele vira teste de
regressão permanente.

---

## 4. Etapa 3 — Cavalo

**Esforço:** meia sessão. Trivial, e é a última peça que falta para o gerador ficar completo.

`KNIGHT_TARGETS[64][8]` já está **declarada e alocada** em `square.c:15` — e zerada, porque
`init_square_tables()` só popula `SQ_TO_EDGE` e `PAWN_ATTACKS`.

### 4.1 Construir a tabela iterando rank/file, não índice de casa

Para cada `(rank, file)` e cada um dos 8 deltas `(dr, df)`, testar
`0 <= rank+dr < 8 && 0 <= file+df < 8` e **só então** calcular `SQ_AT(r, f)`. A função
`offset_square` em `square.c:23` já faz exatamente isso e já é usada por
`init_pawn_attacks` — reaproveite, não escreva uma segunda.

A forma rank/file torna o wraparound **impossível por construção**. Testar
`abs(FILE_OF(a) - FILE_OF(b)) > 2` em runtime também funciona, mas conviveria com
`SQ_TO_EDGE` no mesmo arquivo, e aí o gerador passa a ter duas formas de estar errado na
borda.

Guarde `KNIGHT_COUNT[64]` junto, ou preencha os slots vazios com `SQ_NONE` como
`PAWN_ATTACKS` faz — o importante é escolher **uma** convenção e ser consistente com a que
já existe no arquivo.

### 4.2 `KING_TARGETS` deve ser apagada

Ela está alocada (`square.c:16`), zerada, e `generate_king_moves` **não a usa** — usa
`SQ_TO_EDGE`, que foi a decisão de 10/09 e é a certa. Tabela alocada que ninguém preenche e
ninguém lê é um convite para alguém preenchê-la e criar o segundo mecanismo de borda que a
decisão evitou de propósito.

### 4.3 Verificação

| Verificação | Esperado |
|---|---|
| Cavalo em a1 | 2 destinos |
| Cavalo em b1 | 3 destinos |
| Cavalo em c3 | 8 destinos |
| Cavalo em h8 | 2 destinos |
| Soma sobre as 64 casas | **336** |

A soma é a forma mais barata de verificar a tabela inteira de uma vez.

### Critério de saída

`generate_all_moves` na posição inicial devolve **20** (hoje devolve 16). Kiwipete devolve
**48** (hoje devolve 37). Nessas duas posições as contagens pseudo-legais coincidem com as
legais, então elas servem de checagem antecipada antes do filtro existir.

**E rode o teste da Etapa 2 de novo.** Agora ele exercita caminhos de `make_move` que não
existiam quando passou.

---

## 5. Etapa 4 — Fechar o build

**Esforço:** meia sessão. Pode ser feita a qualquer momento, e três dos oito avisos **são
bugs**.

### 5.1 Os oito avisos

| Aviso | Onde | O que fazer |
|---|---|---|
| `-Wmissing-prototypes` × 5 | `ui`, `print_move`, `check_pawn_promotion`, `genenare_moves_from_direction`, `check_allowed_castles` | Cada uma é **ou** helper interno que devia ser `static`, **ou** API que devia estar no header. A flag não conserta; ela força a decisão, que hoje está sendo tomada por omissão |
| `-Wenum-conversion` | `move.c:112` | `promotion_type = EMPTY;` atribui `PieceType` a `MoveType`. Funciona porque os dois zeros coincidem — que é exatamente a confusão que matou o `enum Promotion` antigo. Use `MV_QUIET` |
| `useless storage class` | `movegen.c:159` | `static const enum {LEFT, RIGHT};` → `enum { LEFT, RIGHT };` |
| `-Wunused-const-variable` | `square.c:8` | `DIR_CHARMAP` sobrou de uma função de debug que não existe mais. Apagar |

Enquanto estiver em `movegen.c`, o typo `genenare_moves_from_direction` vira `generate_`.

### 5.2 As flags que faltam, medidas contra esta árvore

| Flag | Custo hoje | Decisão |
|---|---|---|
| `-Wshadow` | **0** | Ligar |
| `-Wcast-qual` | **0** | Ligar |
| `-Wwrite-strings` | +6 | Ligar. Os seis são `fail_msg(char *)` recebendo literal — mude para `const char *` e os seis somem juntos |
| `-Wconversion` | +8 | **Adiar.** Todos os oito são `*castling_rights &= ~(CASTLE_*)` em `u8` (`makemove.c:12-27`), benignos. Ligar agora significaria silenciá-los com cast, e essa é a flag que historicamente mais pagou aqui — não vale gastá-la em ruído. Ligue junto com a tabela `CASTLE_MASK[64]` do §2.5, que resolve os oito de uma vez |

Mover também `-Wframe-larger-than=16384` de `DBGFLAGS` para `CFLAGS`.

### Critério de saída

`make` limpo, zero avisos, com `-Wshadow -Wcast-qual -Wwrite-strings` somadas ao conjunto
atual. `gcc -std=c11 -fsyntax-only -x c src/<cada>.h` passa nos 11 headers.

---

## 6. Etapa 5 — Completar `board_check_invariants`

**Esforço:** uma hora. Ela hoje cobre 4 das 6 checagens de `docs/guides.md`.

Existem:

- nenhum código de peça inválido
- exatamente um rei de cada cor
- `king_square[cor]` bate com a varredura do zero
- nenhum peão na 1ª ou 8ª fileira

Faltam:

- **`ep_square`, se definido, na fileira coerente com `side_to_move`** — fileira 6 (índice 5)
  quando são as brancas a jogar, fileira 3 (índice 2) quando são as pretas, com o peão
  adversário na casa imediatamente atrás.
- **Direitos de roque coerentes com rei e torres nas casas de origem.** Esta **já está
  escrita**: `castling_matches_board`, hoje `static` em `fen.c:385`. Expor é a maior parte do
  trabalho — é a mesma verificação que `fen_parse` faz na entrada, e usá-la também na saída de
  `make_move` é o que fecha o ciclo que a decisão "FEN inconsistente é rejeitada, não
  corrigida" abriu de propósito (`docs/fen.md`).

O retorno é desproporcional e já foi medido em outros projetos: praticamente todo bug de
make/unmake se manifesta como quebra de invariante **uma ou duas jogadas antes** de causar
sintoma visível. A diferença entre depurar na causa e depurar no sintoma, dentro de uma
árvore de profundidade 6, é de horas.

---

## 7. Etapa 6 — Filtro de legalidade 🚧

**Esforço:** 1–2 sessões. Só comece com a Etapa 2 verde e a Etapa 3 fechada.

### 7.1 `is_square_attacked(const Board *b, int sq, Color by)`

Busca **reversa**: em vez de gerar todos os lances do adversário e ver se algum chega em
`sq`, pergunte de `sq` para fora — "andando como torre a partir daqui, encontro uma torre ou
dama inimiga antes de encontrar qualquer outra coisa?". Mesma pergunta para bispo/dama, para
cavalo (`KNIGHT_TARGETS`, que a Etapa 3 acabou de popular), para rei, e para peão.

É aqui que `PAWN_ATTACKS` se paga pela segunda vez: "de que casas um peão branco ataca `sq`?"
é a consulta **invertida** de "que casas um peão branco em `sq` ataca". Ter a tabela torna a
inversão um índice de cor, em vez de um raciocínio refeito toda vez.

### 7.2 O `const` é a metade do par

```c
int generate_pseudo_legal(const Board *b, MoveList *out);  /* const  */
int generate_legal       (Board *b,       MoveList *out);  /* NAO const */
```

O filtro **não pode** ser `const`, porque ele muta o tabuleiro para descobrir: aplica o
lance, pergunta se o próprio rei ficou atacado, desfaz. Isso não é defeito da assinatura, é
a natureza do filtro — e é a razão pela qual `unmake_move` vinha antes de tudo.

### 7.3 O bug que esta etapa vai acordar, e que já está reproduzido

`generate_king_moves` (`movegen.c:191`) lê `SQ_TO_EDGE[b->king_square[side]][dir]` **sem
checar `SQ_NONE`**. Reproduzido com ASan hoje:

```
FEN "R6k/8/8/8/8/8/8/K7 w - - 0 1"
  gerador emite a8h8 (captura do rei — pseudo-legal, e legítimo nesta camada)
  make_move grava king_square[BLACK] = -1
  generate_all_moves seguinte:
    movegen.c:191 runtime error: index -1 out of bounds for type 'int [64][8]'
    AddressSanitizer: global-buffer-overflow
```

Hoje o sintoma é silencioso, porque o vizinho na memória é `KNIGHT_TARGETS`, que está zerada
— o `continue` da linha seguinte salva por acidente. **Isso deixa de valer no instante em que
a Etapa 3 popular `KNIGHT_TARGETS`**, e o filtro de legalidade é justamente o código que
aplica lances de captura de rei aos milhares.

Duas saídas, e elas não são equivalentes:

1. O filtro nunca deixa a posição chegar nesse estado — mas isso é uma pré-condição
   documentada, e pré-condição documentada é a mesma categoria de "armadilha lembrada" que
   este projeto já decidiu não aceitar.
2. `generate_king_moves` verifica `SQ_NONE` e retorna sem gerar nada. Duas linhas, e o bug
   fica **inexprimível**.

Faça as duas, e ponha o `assert` também.

### 7.4 O que o filtro ganha de graça

Xeque, mate e afogamento saem do mesmo mecanismo: `in_check(b, side)` é
`is_square_attacked(b, b->king_square[side], !side)`; mate é xeque com zero lances legais;
afogamento é zero lances legais sem xeque. Nenhum dos três precisa de código próprio.

E é aqui — não antes — que entram as três condições de roque que `check_castle_side`
deliberadamente **não** verifica: rei não está em xeque, não atravessa casa atacada, não
chega em casa atacada. Elas estão documentadas como fora do gerador no `project_context.md`
§3 exatamente para ninguém reimplementar metade nos dois lugares.

### Critério de saída 🚧

`perft(1)` = 20 e `perft(2)` = 400 na posição inicial; `perft(1)` = 48 na Kiwipete. E o teste
da Etapa 2 de novo.

---

## 8. Etapa 7 — UCI mínimo

**Esforço:** 1 sessão. **Pode ser feita em paralelo com tudo acima** — é o que desbloqueia a
equipe do cliente, que hoje não tem alvo de integração.

### 8.1 A linha mais importante do arquivo

```c
setvbuf(stdout, NULL, _IOLBF, 0);
```

Quando a stdout é um terminal, a libc usa buffer de linha e cada `printf` sai na hora. Quando
é um **pipe** — exatamente o caso do cliente rodando o motor como subprocesso — ela troca
para buffer de bloco de 4 KB. A resposta fica presa, o cliente espera para sempre, e **não há
sintoma para depurar**: o motor está vivo, correto, e mudo. Se aparecer na apresentação, não
tem depuração possível ao vivo.

### 8.2 Leitura de linha

- Buffer de **8192**. `position startpos moves …` passa de 1 KB em partida longa.
- `line[strcspn(line, "\r\n")] = '\0'` trata `\n` e `\r\n` de uma vez — a spec avisa que o
  terminador varia por sistema, e a equipe do cliente pode estar no Windows. (`utils.c` já
  usa esse idioma em `get_fen`; reaproveite.)

### 8.3 Conjunto mínimo de comandos

| Comando | Resposta |
|---|---|
| `uci` | `id name …`, `id author …`, `uciok` |
| `isready` | `readyok` |
| `position startpos [moves …]` | — |
| `position fen <FEN> [moves …]` | — |
| `d` | tabuleiro + FEN *(não-UCI, depuração)* |
| `perft N` | contagem *(não-UCI, depuração)* |
| `quit` | encerra |
| desconhecido | **ignora em silêncio** (a spec manda) |

`go` fica para depois, junto com a IA.

`position … moves` é o consumidor natural de `movelist_find`: os lances chegam em notação de
coordenada (`e2e4`, `e7e8q`) e é o **gerador** que sabe se `e5d6` é captura comum ou en
passant, porque foi ele que marcou a flag. Suporte o sufixo `moves` desde já — ele depende só
do `make_move`, que já está pronto, e é o que permite plugar o motor no Cute Chess ou Arena.

### 8.4 `main.c` encolhe

```
./main.out            laço UCI  (o default, porque é o que o cliente executa)
./main.out test       o teste da Etapa 2
./main.out perft N    contagem
./main.out repl       o menu interativo de hoje
```

O menu atual é genuinamente útil para depurar à mão — mas não pode mais ser o default, porque
o default é o que o cliente vai executar. Mova para `repl.c`. E a FEN de partida sai do
`#define` hardcoded em `main.c:95`.

### 8.5 Dívidas conscientes — anotar, não deixar implícitas

- A spec exige processar stdin **enquanto pensa**, para atender `stop`. Um laço bloqueante
  não faz isso. Para busca rasa é irrelevante; resolve-se depois com thread de input.
- **Divergência a resolver com a equipe**: `arquitetura-xadrez.md` diz que o cliente nunca
  fala direto com o motor; o cliente C++ em `interface/` roda o motor como subprocesso. Isso
  decide se o protocolo precisa de `legalmoves` (para a UI destacar lances) ou se outra
  camada monta isso. Conversa de 15 minutos que evita implementar um comando que ninguém
  chama — ou descobrir na integração que falta um.

### Critério de saída

O cliente (ou Cute Chess/Arena) joga uma partida inteira contra o motor sem travar. Antes de
emitir `bestmove`, verificar que o lance está na lista legal: custa uma varredura e garante
que o motor **nunca** devolve lance ilegal, que é requisito funcional do projeto.

---

## 9. Boas práticas para este projeto

### 9.1 Onde vive uma variável global

| Categoria | Onde | Exemplo |
|---|---|---|
| Tabela read-only depois do init | `static` no `.c` dono, ou `extern` no header dele; escrita **só** pela função de init | `SQ_TO_EDGE`, `KNIGHT_TARGETS` |
| Estado mutável | **Nunca global.** Mora numa struct passada por ponteiro | `Board`, `Game` |
| Constante | `static const` no `.c`, ou macro no header | `DIR_OFFSET`, `PAWN_PUSH` |

O `FailLog` de `utils.c` é a exceção que entrou nesta janela, e ela foi paga: por ser global
e não ter reset, ele vira ruído dentro do teste da Etapa 2 (§3.4). Não é motivo para
reescrevê-lo agora — é motivo para saber que a regra existe porque casos assim aparecem.

### 9.2 Onde vive um tipo

No header do **módulo dono dos seus invariantes**. Se um módulo não pode quebrar o invariante,
não devia ver o tipo. `types.h` é para vocabulário que todo mundo precisa de verdade — e é
incluído por todo `.c`, então cada edição nele recompila o projeto inteiro.

### 9.3 Toda função não exportada é `static`

E a flag força a escolha, em vez de confiar na memória. Hoje cinco funções têm linkage
externo por omissão — nenhuma delas de propósito (§5.1).

### 9.4 Assert como executável, não como comentário

Todo campo denormalizado merece um assert que o recalcula do zero e compara.
`board_check_invariants` já é isso para `king_square`, e é por isso que ela vale o que vale.
Se o Zobrist entrar, `assert(b->hash == compute_hash_from_scratch(b))` no fim de make/unmake
é o detector de bug mais sensível que existe para esta fase: um resumo de 64 bits de *tudo*
que `make_move` deveria ter alterado, disparando no lance exato em vez de três níveis de
busca depois.

### 9.5 A regra de ouro daqui

**Se o bug é detectável em compilação, ligue a flag em vez de lembrar dele.** Continua
valendo, e continua sendo confirmada: três dos oito avisos de hoje são bugs reais, e os seis
avisos de `-Wwrite-strings` apontam para um único defeito de assinatura.

Corolário: **nenhum sanitizer pega leitura de memória não inicializada.** ASan pega
fora-dos-limites e use-after-free (foi ele que pegou o §7.3); UBSan pega UB aritmético. Quem
pegaria memória não inicializada é o MemorySanitizer, que é só do clang e exige recompilar a
libc. O aviso do compilador é a única defesa.

### 9.6 `LOG_ERROR` não é tratamento de erro

Logar e seguir é **pior** do que não logar: dá a impressão de que o caso foi tratado. A
regra: logar **e** devolver status. `movelist_add` é o caso atual (§5, Bug #14) — e o acordo
já era `assert`.

### 9.7 Uma função, uma camada

A camada é: protocolo valida, núcleo confia. `make_move` cumpre isso hoje — ele não valida, e
é por isso que ele pode ser chamado pelo filtro de legalidade com um lance que captura o rei.
Toda vez que uma função do núcleo sentir vontade de validar a entrada, a pergunta certa é
"quem deveria ter validado isso antes?".

### 9.8 Commits

Pequenos, um por etapa, em português (convenção do grupo). **Refatoração que move código
nunca muda comportamento no mesmo commit** — um commit que faz as duas coisas não é revisável
nem bissetável, e perft existe para bissetar.

---

## 10. Ordem, portões e progresso

```
1 ─── 2 🚧 ─── 3 ─── 5 ─── 6 🚧 ─── (IA)
unmake  teste   cavalo inv.  legalidade
              │
              4 (build)  ─── pode entrar em qualquer ponto
              7 (UCI)    ─── pode ir em paralelo, e deve
```

| Etapa | Critério de saída | Feito |
|---|---|---|
| 1 — `unmake_move` | Linka e desfaz um lance simples | ☐ |
| 2 — Teste-portão 🚧 | `./main.out test` verde em ~30 posições, invariantes passando | ☐ |
| 3 — Cavalo | Soma de destinos = 336; inicial gera 20; Kiwipete gera 48 | ☐ |
| 4 — Build | `make` com zero avisos e três flags novas | ☐ |
| 5 — Invariantes | As 6 checagens de `guides.md` | ☐ |
| 6 — Legalidade 🚧 | `perft(1)` = 20, `perft(2)` = 400, Kiwipete `perft(1)` = 48 | ☐ |
| 7 — UCI | Partida inteira contra Cute Chess/Arena sem travar | ☐ |

**A Etapa 2 é portão duplo:** passa uma vez no fim de 1, e volta a rodar como regressão
depois de 3 e de 6 — quando há cavalo e filtro para exercitar, ela testa muito mais do que
testava. É o que vai dizer qual commit quebrou a geração quando mais gente estiver em
`movegen.c`.

Depois da Etapa 6, o próximo documento é sobre perft e busca. **Nada de avaliação ou IA antes
do perft bater.**

---

## 11. Referências para estas etapas

- **Chess Programming Wiki** — *Unmake Move* e *Make Move* (Etapa 1), *Knight Pattern*
  (Etapa 3), *Square Attacked By* e *Legal Move* (Etapa 6), *Perft* e *Perft Results*
- **TSCP** — `makemove`/`takeback` em mailbox legível, e é *agora* a hora de ler. Note que
  ele usa o desenho **oposto** ao nosso (pilha global `hist_dat`); vale justamente pelo
  contraste
- **Especificação UCI** (Stefan Meyer-Kahlen) — ~6 páginas, ler inteira antes da Etapa 7
- `man gcc`, *Options to Request or Suppress Warnings* — em especial a nota sob
  `-Wmaybe-uninitialized` sobre a dependência do nível de otimização
