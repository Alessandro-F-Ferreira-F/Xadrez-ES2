# Medição de desempenho — motor e IA

Como medir a velocidade do motor (e, depois, da IA), como ler os números e como achar onde
o tempo vai, com `gprof`, `perf` e flamegraph. As regras e a ordem das etapas de otimização
estão em `next_steps.md` Parte II (O1–O6). Este guia explica **como** executá-las.

Os exemplos foram medidos em 07/10/2026, commit `a93ddc7`, num i5-14600K sob WSL2. Todos os
comandos foram rodados nessa máquina, exceto os que pedem `sudo` e a abertura do SVG pelo
`explorer.exe`.

---

## 1. Três perguntas, três ferramentas

| Pergunta | Ferramenta | Resposta |
|---|---|---|
| Ficou mais rápido? | `bench`: perft das seis posições | Mnós/s |
| Onde está o tempo? | `perf` (ou `gprof`) + flamegraph | % do tempo por função |
| Quanto custa esta função? | microbenchmark (`src/bench/`) | ns por chamada |

**Só a primeira decide se um commit entra.** As outras duas dizem onde mexer e explicam o
resultado. Uma função que ficou 30% mais rápida sem mover o `bench` não justifica o código a
mais.

---

## 2. O que medir

### 2.1 Motor: velocidade de geração

A métrica principal são os **Mnós/s no perft das seis posições da CPW**, em profundidade fixa:

| Posição | Prof. | Folhas | Mnós/s (07/10) |
|---|---|---|---|
| inicial | 5 | 4 865 609 | 20,3 |
| Kiwipete | 4 | 4 085 603 | 21,9 |
| posição 3 | 6 | 11 030 083 | **15,4** |
| posição 4 | 4 | 422 333 | 20,7 |
| posição 5 | 4 | 2 103 487 | 20,9 |
| posição 6 | 4 | 3 894 594 | 23,2 |
| **total** | | **26 401 709** | **18,4** (≈ 1,4 s) |

O perft é a carga certa por três motivos: é determinístico, exercita exatamente o código
quente (geração, filtro de legalidade, make/unmake) e **confere a correção na mesma
rodada**. Daí as três regras:

1. **A contagem de nós faz parte da medição.** Se a contagem mudou, a mudança não é
   otimização, é bug. O `bench` compara cada contagem com a tabela e falha se ela divergir. O
   Stockfish faz o mesmo: todo commit dele leva `Bench: <nós>` na mensagem, como assinatura de
   comportamento (`~/development/Stockfish/src/benchmark.cpp`).
2. **Reporte por posição, não só o total.** O total esconde onde está o problema. A posição 3 é
   um final com 7 peças: ela leva metade do tempo da suíte e tem o pior Mnós/s, porque os
   custos fixos por nó (as varreduras das 64 casas) se diluem em poucos lances. Uma otimização
   pode ajudar só um tipo de posição, e só a tabela por posição mostra isso.
3. **A definição de "nó" não muda no meio da série.** O `perft` de hoje gera, aplica e desfaz
   até a última folha. Se um dia entrar o *bulk counting* (devolver `list.count` na
   profundidade 1), o número de Mnós/s salta 2–3× sem que nada tenha ficado mais rápido. Se
   isso acontecer, comece uma série nova de medições e anote o motivo.

Métrica secundária: o **custo por folha**, que é o número de chamadas dividido pelo número de
folhas. Ele vem da coluna `calls` do `gprof` (§6, passo 4). Hoje: `is_square_attacked` **1,18** por
folha e `make_move` **2,17** por folha, porque cada folha paga um par make/unmake no filtro de
legalidade e outro no perft. É a métrica de saída da etapa O3.

### 2.2 IA: qualidade e velocidade da busca *(quando a IA estiver ligada ao `go`)*

Na busca, o Mnós/s deixa de ser a métrica principal: uma busca melhor **visita menos nós**.
Meça sempre nas mesmas posições, na mesma profundidade e com `SORTEIO_DESLIGADO`, para a
medição ser repetível.

| Métrica | Como obter | Leitura |
|---|---|---|
| **Nós até a profundidade N** | Um contador incrementado a cada nó visitado (em `minimax` e em `podaAlfaBeta`) | Menos nós = ordenação e poda melhores. É a métrica da O4 |
| Tempo até a profundidade N | Relógio em volta de `escolherJogada` | O que o usuário sente |
| NPS da busca | nós ÷ tempo | Custo por nó, avaliação incluída. Compare com o Mnós/s do perft: a diferença é o preço da avaliação |
| **Fator de ramificação efetivo (EBF)** | √(N(d) ÷ N(d−2)) | Quanto cada nível a mais multiplica o trabalho. Minimax no meio-jogo ≈ 35; alfa-beta bem ordenado ≈ 6 (≈ √35); motores fortes, com podas agressivas, chegam perto de 2. A comparação é com d−2, e não com d−1, porque o alfa-beta oscila entre profundidade par e ímpar |
| Corte no 1º lance | Entre os nós que terminaram em corte beta, a fração em que o corte veio do primeiro lance | Qualidade da ordenação. A regra prática da literatura é ficar acima de ~90% |
| Profundidade em tempo fixo | Aprofundamento iterativo com `go movetime` | O objetivo final das etapas O4/O5 |

O invariante que vale como teste: **com o sorteio desligado e a mesma profundidade, o
alfa-beta devolve o mesmo placar que o minimax** (`next_steps.md` O4). A razão
`nós(minimax) ÷ nós(alfa-beta)` mede diretamente quanto a poda está rendendo.

**Força de jogo** só se mede com partidas: versão nova contra versão antiga, centenas de jogos,
mesmo controle de tempo e aberturas variadas. Ferramentas: `cutechess-cli` ou `fastchess`,
com o teste estatístico SPRT para decidir quando parar (CPW, *Engine Testing*, *SPRT*). Uma
partida isolada não prova nada. Isso fica para quando o motor jogar pelo UCI completo.

### 2.3 O que não medir

- O build de desenvolvimento (`make`, `-Og -g`) ou o de debug (ASan): eles medem o depurador.
- O build `-pg` do gprof: ele roda **3,3× mais lento** (§5.3). Serve só para o perfil.
- Uma execução só, ou a média de várias (§3.3).
- Cargas curtas (menos de ~0,5 s), em que o ruído do sistema pesa tanto quanto o código.

---

## 3. Como medir

### 3.1 O build

Use **`-O2 -DNDEBUG -flto`** (o `make release`), sempre com as mesmas flags antes e depois da
mudança.

- O `-flto` permite expandir funções pequenas de outros arquivos (os acessores de `Move`, por
  exemplo). Ele rendeu +57% na linha de base de 04/10.
- **Não use `-march=native` nas medições oficiais.** O binário precisa rodar nas máquinas do
  grupo e na apresentação.

### 3.2 O relógio

- Use um relógio **monotônico** (`clock_gettime(CLOCK_MONOTONIC)`, que é o `timer_now_ns()` de
  `src/bench/timer.h`). Ele não volta nem salta se o horário do sistema for ajustado.
- Meça só o trabalho: a chamada do `perft`, sem a leitura da FEN e sem impressão.
- Cada medição deve durar **≥ 1 s**. Assim a resolução do relógio fica irrelevante e as
  interrupções se diluem.

Enquanto o subcomando `bench` (etapa O1) não existe, dá para medir pelo protocolo:

```sh
printf 'position startpos\ngo perft 5\nquit\n' | /usr/bin/time -f "%e s" ./build/main.out
# Mnós/s = "Nodes searched" ÷ segundos ÷ 10^6. A partida do processo custa poucos ms.
```

### 3.3 Repetição e ruído

Uma execução não basta. Rode **10 vezes, intercalando o antes e o depois**, e compare as
**medianas**:

```sh
for i in $(seq 10); do
  /usr/bin/time -f "antes  %e s" ./antes.out  < carga.txt > /dev/null
  /usr/bin/time -f "depois %e s" ./depois.out < carga.txt > /dev/null
done 2>&1 | sort -k1,1 -k2,2n
```

O `sort` separa e ordena os dois lados; a mediana de cada lado é a média entre a 5ª e a 6ª
linha do grupo. **Intercalar** importa: se a máquina mudar de comportamento no meio da série,
os dois lados sofrem igual e a comparação continua justa.

O motivo de tanto cuidado é que, nesta máquina, o tempo tem **dois modos**. Dez execuções do
perft 6 (release):

```
5,81  5,84  5,82  4,74  5,81  5,91  5,85  5,94  4,68  5,84  s
```

Oito ficaram entre 5,81 e 5,94 s (±1%), e duas ficaram ~20% mais rápidas. A explicação
provável é a CPU híbrida sob o WSL2 (§3.4): o Windows decide em que núcleo físico, P ou E, a
CPU virtual roda, e isso muda de uma execução para outra. Às vezes uma série inteira cai no
mesmo modo: em outra sessão, 8 execuções seguidas deram 18,56–18,68 Mnós/s (±0,3%).

Com 2 execuções em 10 fora da curva, a mediana não se mexe. A média, sim, e o mínimo pegaria
justamente a execução rara. Por isso, use sempre a mediana (nunca a média nem o mínimo) e
**olhe a lista inteira**. Se os dois modos aparecerem em quantidades parecidas (4 ou 5 em 10),
a mediana fica instável: repita a série.

A regra de decisão:

| Diferença entre as medianas | Conclusão |
|---|---|
| < 1% | Empate: a mudança não fez diferença |
| 1–2% | Repita a série antes de concluir |
| > 2% | É real |

### 3.4 A máquina

- Feche o que for pesado: navegador com vídeo, compilação, indexação do editor.
- **Não fixe CPU no WSL2.** Medido: fixando com `sched_setaffinity`, os números ficaram entre
  18,37 e 18,72, mais espalhados do que sem fixar. O WSL fixa uma CPU *virtual*, e quem escolhe
  o núcleo físico é o Windows; o 14600K ainda mistura núcleos P e E, e essa é a causa provável
  dos dois modos da §3.3. Num Linux nativo, fixe por fora, sem código:
  `taskset -c 2 ./build/main.out ...`.
- **Só compare números da mesma máquina.** As medições de cada membro do grupo são séries
  separadas.

### 3.5 Microbenchmarks

A biblioteca em `src/bench/` (`bench.h`) serve para medir **uma função isolada**. Ela repete a
função muitas vezes e devolve a mediana em ns por chamada. Exemplos medidos:
`generate_legal_moves` na Kiwipete custa **1,65 µs**, um par make+unmake **7,4 ns**, e o
próprio laço de medição **0,2 ns**.

As regras de uso:

- O setup (ler a FEN, montar a lista) fica **fora** do laço medido.
- O corpo do laço precisa ser **idempotente**: um `make` seguido de `unmake`, por exemplo, para
  cada volta medir o mesmo estado.
- Consuma o resultado com `bench_keep`, ou o `-O2` apaga a chamada inteira. Passe estruturas
  locais por `bench_escape`, ou o compilador pode tirar o trabalho de dentro do laço.
- Não saia do laço antes do fim (`break`, `return`): um laço interrompido não é medido.

O microbenchmark confirma que a mudança fez **localmente** o que você esperava. Quem decide
se ela entra é o `bench`.

### 3.6 Registrar

Uma linha por medição em `docs/benchmarks.md`:

```
| Data       | Commit  | Build              | Mnós/s (mediana de 10) | Mudança             |
|------------|---------|--------------------|-----------------------|----------------------|
| 2026-10-07 | a93ddc7 | -O2 -DNDEBUG -flto | 18,4                  | linha de base        |
```

Na mensagem do commit de otimização vão o antes, o depois e o portão:
`perf(makemove): castling rook por tabela — 18,4 → 19,1 Mnós/s (+3,8%), perft exato`.

---

## 4. Como interpretar

### 4.1 Ganho

`ganho = novo ÷ antigo − 1`. De 18,4 para 19,1 Mnós/s o ganho é de 3,8%. Se a medida for
tempo em vez de Mnós/s, a razão se inverte: `antigo ÷ novo − 1`.

### 4.2 Lei de Amdahl: o teto antes de começar

Se uma função ocupa a fração **p** do tempo e você a deixa **s** vezes mais rápida, o programa
inteiro acelera:

```
speedup = 1 / ((1 − p) + p / s)
```

Exemplo: `is_square_attacked` ocupa p ≈ 0,32.

- Deixá-la 2× mais rápida dá `1 / (0,68 + 0,16)` = 1,19, ou seja, **+19%**.
- Mesmo que ela passasse a custar zero, o ganho máximo seria `1 / 0,68` = **+47%**.

Faça essa conta antes de escolher o alvo. Uma função com 3% do tempo nunca vai render mais do
que 3%.

### 4.3 Perfil: tempo *self* e tempo *total*

- **Self** (exclusivo) é o tempo gasto no código da própria função. Ele mostra onde a CPU
  queima.
- **Total** (inclusivo, *children*) é a função mais tudo o que ela chama. Ele mostra qual
  **subsistema** custa caro.

O exemplo mais útil do motor hoje: `generate_legal_moves` tem **2,3% de self e 70% de total**.
Ou seja, o filtro de legalidade inteiro, com seus `is_square_attacked` e o par make/unmake
extra, é 70% do tempo. É o argumento da etapa O3 em um número.

### 4.4 Com e sem `-flto`

O `-flto` deixa o compilador expandir funções pequenas dentro de quem as chama, mesmo entre
arquivos. No perfil, o tempo da função expandida passa para a função que a recebeu. Medido na
posição inicial, prof. 6 (119 milhões de folhas):

| Função | Sem `-flto` (self / total) | Com `-flto` (self / total) |
|---|---|---|
| `is_square_attacked` | 26,2% | 40,6% |
| `generate_legal_moves` | 2,0% / 69,1% | 24,0% / 77,7% |
| Acessores de `Move`, geradores por peça | Aparecem separados (`move_is_castle` 5,5%, `generate_pawn_moves` 4,4%…) | **Somem**: estão dentro de `generate_legal_moves` |

Use o perfil sem LTO para entender a estrutura (quem chama quem) e o com LTO para confirmar
onde o binário release gasta. Quais funções somem depende do binário: num programa de teste
que só chamava o `perft`, foi a própria `generate_legal_moves` que sumiu dentro dele.

**Porcentagem é fatia, não tempo.** Com LTO, a fatia de `is_square_attacked` subiu de 26% para
41%, mas o tempo gasto nela **caiu** 5% (`perf diff -c ratio`, §7.3, passo 8). A fatia subiu
porque o resto encolheu mais. Para comparar tempo entre dois builds, use o `bench` ou o
`perf diff -c ratio`, nunca as porcentagens de dois relatórios.

---

## 5. perf × gprof: o que cada um faz

Os dois respondem "onde está o tempo?", mas por mecanismos diferentes, e por isso erram em
lugares diferentes.

### 5.1 gprof: o compilador instrumenta, o programa se mede

Com `-pg`, o compilador insere uma chamada a `mcount` na entrada de **toda** função. A cada
chamada, o `mcount` anota o par "quem chamou → quem foi chamado" e soma 1 nesse contador. Em
paralelo, o programa se interrompe 100 vezes por segundo (timer `SIGPROF`) e anota em que
função estava. Na saída normal do programa, tudo isso vai para o `gmon.out`; o `gprof` lê
esse arquivo junto com o executável e monta o relatório.

O que isso implica:

- As **contagens de chamadas são exatas**, e o grafo de chamadas é montado a partir delas.
- O tempo por função é **amostrado**, a 100 Hz, e o tempo dos "filhos" de cada função é
  **estimado** a partir das contagens.
- **O programa medido muda**: o `mcount` custa caro (§6, limitações).
- Ele só vê o código compilado com `-pg`: nem a libc, nem o que foi expandido inline.

### 5.2 perf: o kernel observa o programa de fora

O `perf` é a ferramenta de linha de comando do subsistema `perf_events` do kernel Linux. Ele
tem dois modos:

- **Contagem** (`perf stat`): o kernel conta eventos durante a execução inteira e devolve
  totais. Os eventos são tempo de CPU, trocas de contexto e faltas de página; onde há suporte
  de hardware, também ciclos, instruções e desvios mal previstos.
- **Amostragem** (`perf record`): o kernel interrompe o programa ~1000 vezes por segundo e,
  a cada interrupção, grava o endereço da instrução que estava executando e, com `-g`, a
  **pilha de chamadas inteira** até o `main`. Tudo vai para o `perf.data`. Depois, o
  `perf report` traduz os endereços em funções e linhas (pelos símbolos e pelo `-g` do
  binário) e soma: a fração de amostras em cada função é a fração do tempo.

O que isso implica:

- O programa **não muda**: o binário perfilado é o mesmo que você mede. O custo do
  `perf record` não foi mensurável, ficou dentro da variação entre execuções.
- A resolução vai até a **instrução** e a **linha de código**, não só até a função.
- Cada amostra traz a **pilha real**. Por isso o tempo total (inclusivo) é medido, não
  estimado, e é isso que alimenta o flamegraph.
- Ele vê tudo: a libc, e o código expandido inline (atribuído à função que o recebeu).
- **Não conta chamadas.**

### 5.3 Comparação

| | gprof | perf |
|---|---|---|
| Mecanismo | Instrumentação (`-pg`) + amostragem a 100 Hz | Amostragem pelo kernel, ~1000 Hz (ajustável) |
| Recompilar? | Sim, com `-pg` | Não (mas `-g -fno-omit-frame-pointer` melhoram o resultado) |
| Efeito sobre o programa | **3,3× mais lento**, 88% do tempo dentro do `mcount` (medido) | Não mensurável |
| Contagem de chamadas | **Exata** | Não tem |
| Tempo dos filhos | Estimado pelas contagens | Medido pela pilha |
| Resolução | Função | Instrução / linha |
| Funções inline | Invisíveis | Tempo vai para quem as recebeu |
| Quando grava | Só na saída normal do programa | Durante a execução; pode se anexar a um processo que já está rodando |
| Onde roda | Qualquer lugar com GCC, inclusive MinGW no Windows | Só Linux (no WSL2, sem contadores de hardware) |

### 5.4 Qual usar

- **perf** para saber onde está o tempo. É o padrão.
- **gprof** para contar chamadas (o custo por folha, §2.1), ou em máquina sem `perf`, como
  quem compila no Windows.

No essencial os dois concordam: na mesma carga, os dois põem `is_square_attacked` em
primeiro, com cerca de 30%.

---

## 6. gprof, passo a passo

Rode tudo de dentro de `build/`: o `gmon.out` é gravado no diretório atual, e `build/` já
está no `.gitignore`.

**Passo 1. Compilar com `-pg`.** Sem `-flto`, que esconde funções do perfil.

```sh
cd engine
gcc -std=c11 -O2 -DNDEBUG -pg src/*.c test/test.c -o build/prof.out
cd build
```

**Passo 2. Preparar a carga.** É um arquivo com os comandos do protocolo, e precisa terminar
em `quit`:

```sh
printf 'position startpos\ngo perft 6\nquit\n' > carga.txt
```

O `gmon.out` **só é gravado se o programa terminar normalmente** (`quit`, EOF ou `return` do
`main`). Se o processo for morto com Ctrl-C, o perfil se perde.

**Passo 3. Rodar.**

```sh
./prof.out < carga.txt      # gera gmon.out
```

**Passo 4. Perfil plano: onde está o tempo.**

```sh
gprof -b -p prof.out gmon.out | head -20
```

```
  %   cumulative   self              self     total
 time   seconds   seconds    calls  ms/call  ms/call  name
 31.76      0.27     0.27 31142789     0.00     0.00  is_square_attacked
 15.29      0.40     0.13 57382588     0.00     0.00  make_move
 10.59      0.49     0.09 57382588     0.00     0.00  unmake_move
```

- `% time` e `self seconds` dão o tempo *self* (§4.3).
- `calls` é **exato**. É o que o gprof tem de mais valioso: `calls ÷ folhas` é o custo por
  folha (§2.1).
- `cumulative seconds` na **última linha** é quanto tempo de amostra você tem. Esse número é o
  tamanho do sinal.

**Passo 5. Grafo de chamadas: quem chama quem.**

```sh
gprof -b -q prof.out gmon.out | less               # todas as funções
gprof -b -qis_square_attacked prof.out gmon.out    # só uma (sem espaço depois do -q)
```

```
index % time    self  children    called     name
                0.17    0.00 15882573/15882573     generate_legal_moves (4)
[5]     27.4    0.17    0.00 15882573         is_square_attacked [5]
```

A linha com `[N]` é a função. **Acima** dela vêm os pais (quem a chama) e **abaixo** os
filhos (o que ela chama). `15882573/15882573` quer dizer "chamadas vindas deste pai / total de
chamadas da função": aqui, todas as chamadas a `is_square_attacked` vêm de
`generate_legal_moves`. A coluna `children` é o tempo estimado dos filhos.

**Passo 6 (opcional). Somar rodadas,** para ter mais amostras e menos ruído:

```sh
for i in 1 2 3 4 5; do GMON_OUT_PREFIX=gmon.out ./prof.out < carga.txt; done   # gera gmon.out.<pid>
gprof -s prof.out gmon.out.*               # soma tudo em gmon.sum
gprof -b -p prof.out gmon.sum | head -20
```

**Limitações (medidas):**

1. **O `-pg` distorce o programa.** O build instrumentado levou 7,65 s contra 2,29 s sem
   instrumentação, e o `perf` mostrou 88% do tempo dentro do `mcount` (libc), que o gprof não
   enxerga. As porcentagens são relativas ao seu código, e os segundos do relatório não
   somam o tempo real da execução.
2. **Poucas amostras, muito ruído.** Cada amostra vale 10 ms. Com ~1 s de amostras, a mesma
   carga deu `is_square_attacked` com 31,8% numa rodada e 23,3% na outra. Junte **≥ 5 s** na
   coluna `cumulative`, com uma carga maior ou somando rodadas (passo 6).
3. Funções expandidas (`static inline`, ou o que o `-flto` expandir) não aparecem.
4. O tempo de uma função é dividido entre os pais na proporção das chamadas, como se toda
   chamada custasse igual. Isso é estimado, não medido (seção *Inaccuracy of gprof Output*
   do manual).

---

## 7. perf, passo a passo

### 7.1 Instalar e conferir

```sh
sudo apt install linux-tools-generic
perf --version
```

Se aparecer `WARNING: perf not found for kernel 6.6.114.1-microsoft`, a instalação funcionou e
falta um passo. O pacote instala um `/usr/bin/perf` que é só um wrapper: ele procura o binário
de verdade em `/usr/lib/linux-tools/$(uname -r)/`. O kernel do WSL não tem pacote próprio, então
essa pasta não existe e o wrapper desiste. O binário `-generic` que veio no pacote funciona no
kernel do WSL (perf 6.8.12 testado no kernel 6.6.114.1); basta colocá-lo **antes** do wrapper
no `PATH`.

Sem sudo, com um script em `~/.local/bin`. Essa pasta precisa vir antes de `/usr/bin` no
`PATH` (confira com `echo $PATH`). O script continua funcionando quando o pacote for
atualizado:

```sh
cat > ~/.local/bin/perf <<'EOF'
#!/bin/sh
exec "$(ls -d /usr/lib/linux-tools/*-generic | sort -V | tail -1)/perf" "$@"
EOF
chmod +x ~/.local/bin/perf
hash -r && perf --version      # perf version 6.8.12
```

Ou, com sudo, um link para todos os usuários (refaça depois de atualizar o pacote):
`sudo ln -sf /usr/lib/linux-tools/<versão>-generic/perf /usr/local/bin/perf`.

Não é preciso mexer no `perf_event_paranoid`. O padrão (2) já permite perfilar os próprios
processos em espaço de usuário, que é tudo o que interessa aqui. A única coisa que ele bloqueia
é o `perf top` do sistema inteiro; use `perf top -p <PID>` (§7.4).

### 7.2 O que o WSL2 não tem

**Contadores de hardware.** Não existe PMU `cpu` em `/sys/bus/event_source/devices/`, então
`cycles`, `instructions`, `branch-misses` e `cache-misses` aparecem como `<not supported>`. O
`perf record` cai sozinho para o evento de software `cpu-clock` (um timer), o que basta para
fazer perfil.

Num Linux nativo esses contadores existem e dão duas métricas úteis para a O2/O3: **IPC**
(instruções por ciclo) e a **taxa de desvios mal previstos**. São opcionais neste projeto.

### 7.3 Passo a passo

Rode de dentro de `build/`, pelo mesmo motivo do gprof: o `perf.data` vai para o diretório
atual.

**Passo 1. Compilar para perfil.**

```sh
cd engine
gcc -std=c11 -O2 -g -fno-omit-frame-pointer -DNDEBUG src/*.c test/test.c -o build/perf.out
cd build
```

- `-g` grava símbolos e linhas do código-fonte. Não muda o código gerado.
- `-fno-omit-frame-pointer` permite ao perf reconstruir a pilha de forma barata. A
  alternativa, `--call-graph dwarf`, funciona sem essa flag mas gera arquivos ~60× maiores
  (12 MB contra 0,2 MB na mesma execução).
- `-flto` é opcional (§4.4).

**Passo 2. Preparar a carga.** É o mesmo `carga.txt` do gprof (§6, passo 2).

**Passo 3. `perf stat`: quanto tempo, sem perfil.**

```sh
perf stat -e task-clock ./perf.out < carga.txt
```

```
           2199.34 msec task-clock:u          #    0.953 CPUs utilized
```

`task-clock` é o tempo de CPU do processo. "CPUs utilized" perto de 1 significa uma thread
ocupada o tempo todo, que é o esperado para o motor. Passe sempre `-e task-clock`: sem isso, o
perf tenta métricas de hardware, imprime um erro sobre `TOPDOWN.SLOTS` e mostra uma série de
`<not supported>`.

**Passo 4. `perf record`: gravar as amostras.**

```sh
perf record -F 999 -g -- ./perf.out < carga.txt
```

```
[ perf record: Captured and wrote 1.346 MB perf.data (9443 samples) ]
```

- `-F 999`: 999 amostras por segundo. Usa-se 999 em vez de 1000 para não amostrar no mesmo
  compasso de algum evento periódico.
- `-g`: grava a pilha de chamadas em cada amostra.
- `--`: separa as opções do perf do comando a medir.
- Para ter sinal, junte **alguns milhares de amostras**, ou seja, alguns segundos de execução.

**Passo 5. `perf report`: onde está o tempo.**

```sh
perf report --no-children      # interativo, ordenado por tempo self
perf report --children         # interativo, ordenado por tempo total (inclusivo)
```

Cada linha é uma função: `Overhead` (a fatia do tempo), o comando, o binário (`Shared Object`)
e o símbolo. `[.]` indica código de usuário e `[k]`, do kernel. As teclas:

| Tecla | Faz |
|---|---|
| `↑` `↓` | Navega entre as funções |
| `+` | Expande ou recolhe um nível da pilha da função selecionada |
| `E` / `C` | Expande / recolhe todas as pilhas |
| `a` | Anota a função selecionada (passo 6) |
| `Enter` | Menu da função (anotar, filtrar por binário ou thread) |
| `/` | Filtra por nome |
| `h` ou `?` | Ajuda com todas as teclas |
| `q` / `Esc` | Volta / sai |

As mesmas visões em texto, para colar num relatório ou num PR:

```sh
perf report --stdio --no-children --sort symbol -g none | head -20   # tempo self
perf report --stdio --children   --sort symbol -g none | head -20    # tempo total
perf report --stdio --no-children -S is_square_attacked              # de onde ela é chamada
```

```
    26.22%  [.] is_square_attacked
    13.82%  [.] make_move
    12.60%  [.] unmake_move
     6.34%  [.] check_rook_squares
     5.54%  [.] move_is_castle
```

No modo `--no-children`, a pilha embaixo de cada função mostra **por onde se chegou nela**:
`is_square_attacked ← generate_legal_moves ← perft ← perft …`. Na prática, porém, o flamegraph
(§8) mostra quem chama quem com mais clareza que a pilha em texto, que fica longa por causa da
recursão do `perft`.

**Passo 6. `perf annotate`: onde, dentro da função.**

```sh
perf annotate is_square_attacked                   # interativo
perf annotate --stdio is_square_attacked | less    # texto
```

O assembly da função aparece com as linhas do `.c` intercaladas (graças ao `-g`). À esquerda
fica a % das amostras daquela função que caíram em cada instrução. As teclas:

| Tecla | Faz |
|---|---|
| `H` | Vai para a instrução mais quente |
| `Tab` / `Shift+Tab` | Percorre as instruções quentes em ordem |
| `s` | Liga/desliga o código-fonte intercalado |
| `o` | Alterna entre a saída crua e a simplificada do disassembler |
| `t` | Alterna a coluna entre %, período e número de amostras |
| `/` | Busca texto |
| `q` | Volta |

A amostra costuma cair na instrução **seguinte** à que realmente demorou (o efeito *skid*:
uma leitura lenta da memória "aparece" na instrução depois dela). Leia a vizinhança da linha
quente, não a instrução exata.

**Passo 7. Tempo por linha de código.**

```sh
perf report --stdio --no-children --sort srcline -g none | head -20
```

```
     4.96%  makemove.c:9
     4.79%  makemove.c:174
     3.70%  movegen.c:353
```

É a visão mais direta para escolher o que reescrever. Pode levar alguns segundos, porque o
perf traduz cada endereço em arquivo:linha.

**Passo 8. `perf diff`: antes × depois.**

Grave a mesma carga com os dois binários. **O nome do executável tem de ser o mesmo nas duas
gravações**, porque o `perf diff` casa as funções pelo nome do binário e do símbolo. Com nomes
diferentes, ele não pareia nada. Por isso, use duas pastas:

```sh
(cd antes  && perf record -F 999 -g -o ../antes.data  -- ./perf.out < ../carga.txt)
(cd depois && perf record -F 999 -g -o ../depois.data -- ./perf.out < ../carga.txt)
perf diff antes.data depois.data             # variação da FATIA de cada função
perf diff -c ratio antes.data depois.data    # razão do TEMPO de cada função (depois ÷ antes)
```

Exemplo real, sem LTO (antes) contra com LTO (depois), perft 6:

```
perf diff                                   perf diff -c ratio
 2.01%  +21.96%  generate_legal_moves        2.01%  7.305263  generate_legal_moves
26.22%  +14.33%  is_square_attacked         26.22%  0.948304  is_square_attacked
12.60%   -3.95%  unmake_move
```

A primeira coluna é a fatia no *antes*. No `perf diff` simples, a segunda é quantos pontos a
fatia ganhou ou perdeu. No `-c ratio`, é a razão do tempo absoluto. As duas contam histórias
diferentes:

- `is_square_attacked` ganhou 14 pontos de fatia, mas a razão 0,95 diz que o tempo **caiu** 5%.
- `generate_legal_moves` ficou 7,3× mais "pesada" porque absorveu as funções expandidas pelo LTO
  (§4.4), não porque ficou mais lenta.

Para decidir, use o `-c ratio`.

### 7.4 Perfilar o motor rodando dentro da interface

O perf pode se anexar a um processo que **já está rodando**. Isso serve para medir o motor
enquanto a interface o usa, com a carga real de uma partida:

```sh
pgrep -a main.out                              # acha o PID do motor
perf record -F 999 -g -p <PID> -- sleep 10     # grava 10 s do processo, depois solta
perf report --no-children
perf top -p <PID>                              # ao vivo; atualiza a cada 2 s, q sai
```

Para ter nomes de função e pilhas, o motor que a interface lança precisa ter sido compilado
com `-g -fno-omit-frame-pointer`, ou pelo menos sem `strip`.

---

## 8. Flamegraph

### 8.1 Como ler

Cada caixa é uma função.

- A **largura** é a fração das amostras em que a função estava na pilha, ou seja, tempo.
- A **altura** é a profundidade da pilha: quem chama fica embaixo, quem é chamado em cima.
- A ordem horizontal é **alfabética**. O eixo x não é o tempo.
- As cores são só estética.

Procure duas coisas: **platôs largos no topo** (funções com muito tempo self) e **torres
largas** (subsistemas caros, como os 70% de `generate_legal_moves`). O SVG é interativo no
navegador: clique numa caixa para dar zoom, e `Ctrl+F` busca e soma a porcentagem de uma
função em todas as torres.

### 8.2 Gerar

Os scripts são do Brendan Gregg, em Perl (que já está instalado):

```sh
git clone --depth 1 https://github.com/brendangregg/FlameGraph ~/FlameGraph
```

A partir de um `perf.data` gravado com `-g` (§7.3, passo 4):

```sh
perf script > out.perf
~/FlameGraph/stackcollapse-perf.pl out.perf > out.folded
~/FlameGraph/stackcollapse-recursive.pl out.folded > out.rec.folded
~/FlameGraph/flamegraph.pl --title "perft inicial d6" out.rec.folded > flame.svg
```

O passo `stackcollapse-recursive.pl` importa aqui porque o `perft` é recursivo. Sem ele, cada
nível de recursão vira uma pilha diferente (`perft;perft;perft;…;is_square_attacked`), e a
mesma função aparece picada em seis torres finas em vez de uma larga.

Para abrir o SVG a partir do WSL: `explorer.exe flame.svg` costuma abrir no navegador padrão.
Se não abrir, copie o arquivo para `/mnt/c/Users/<você>/Downloads/`.

### 8.3 Antes × depois (flamegraph diferencial)

Para ver o que mudou com uma otimização, grave um `.folded` antes e outro depois:

```sh
~/FlameGraph/difffolded.pl -n antes.folded depois.folded | ~/FlameGraph/flamegraph.pl > diff.svg
```

O formato é o do *depois*. **Vermelho** é o que cresceu e **azul** o que encolheu. O `-n`
normaliza as duas contagens, para que execuções de duração diferente fiquem comparáveis.

---

## 9. Receita: uma otimização do começo ao fim

1. Guarde o binário atual como `antes.out`.
2. **`perf record` + flamegraph**: escolha o alvo e faça a conta de Amdahl (§4.2).
3. Mude **uma** coisa.
4. **Perft das seis posições exato.** Se a contagem mudou, é bug: volte.
5. Compile o `depois.out` e meça os dois **intercalados, 10 vezes** (§3.3). Se o ganho ficou
   abaixo de 1%, descarte a mudança: é complexidade sem ganho.
6. Faça o commit com os números na mensagem (§3.6) e acrescente a linha em
   `docs/benchmarks.md`.

---

## 10. Referências

- Brendan Gregg — [CPU Flame Graphs](https://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html)
  e [perf Examples](https://www.brendangregg.com/perf.html). São a referência prática de
  `perf` + flamegraph.
- [perf wiki — Tutorial](https://perf.wiki.kernel.org/index.php/Tutorial); `man perf-record`,
  `man perf-report`, `man perf-annotate`.
- [GNU gprof manual](https://sourceware.org/binutils/docs/gprof/), em especial a seção
  *Inaccuracy of gprof Output*.
- Chess Programming Wiki: *Perft*, *Branching Factor*, *Move Ordering*, *Engine Testing*,
  *SPRT*.
- Stockfish, `src/benchmark.cpp` (cópia local em `~/development/Stockfish`): o conceito de
  `bench` como assinatura e como velocidade.
- Chandler Carruth, *Tuning C++: Benchmarks, and CPUs, and Compilers! Oh My!* (CppCon 2015):
  a técnica de `escape`/`clobber` usada nos microbenchmarks.
