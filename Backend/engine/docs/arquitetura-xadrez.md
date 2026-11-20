# Arquitetura do Sistema de Xadrez

> Documento de design técnico — motor de regras, IA e cliente desktop (o **MVP**); backend
> e cliente web entram só como **extensão de multiplayer online**, se sobrar tempo.
>
> Revisado em 13 de setembro de 2026, para corrigir uma premissa que não é mais válida: a
> versão anterior deste documento assumia que o cliente nunca fala diretamente com o C, e
> que existe sempre um backend Node/TS no meio, mesmo em modo local. **Isso não é o que foi
> decidido.** Ver `project_context.md` para o estado e as decisões internas do motor —
> este documento cobre a integração motor ↔ cliente ↔ (futuramente) servidor.

## Princípio arquitetural central

**MVP: o cliente desktop (C++) fala diretamente com o motor (C), via subprocesso e
stdin/stdout. Não há servidor no caminho.** O motor é um binário local, spawnado pelo
cliente, que recebe uma posição e devolve lances — exatamente como um motor UCI real
(Stockfish etc.) conversa com uma GUI local.

Isso resolve "jogar contra a IA offline" e "multiplayer local no mesmo dispositivo" **de
graça, por construção** — não existe estado remoto, não existe rede, não existe nada de
servidor no caminho crítico. Não é uma simplificação temporária: é a arquitetura de
produto para o escopo que o grupo se comprometeu a entregar.

**Extensão, somente se sobrar tempo: multiplayer online.** Se isso entrar, um servidor
(Node/TypeScript) é **adicionado por cima**, não substitui o caminho direto. Ele passa a
existir como uma segunda forma de falar com um motor (agora rodando no servidor, um
subprocesso por partida online) para os casos que exigem autoridade de terceiro — dois
jogadores em máquinas diferentes não podem cada um confiar no motor local do outro. Jogo
local contra IA e multiplayer local continuam usando o caminho direto, mesmo depois de o
servidor existir.

## 1. Visão geral da arquitetura

### MVP (o que será entregue)

```
Cliente Desktop (C++)
        |
        | spawna como subprocesso
        | stdin / stdout
        | (protocolo texto, estilo UCI)
        v
Motor de regras + IA (C)
- Geração e validação de lances
- Busca (minimax + alfa-beta)
```

Duas camadas, uma fronteira só:

- **Motor + IA (C, este repositório, `engine/`)**: um binário que sabe jogar xadrez. Não
  sabe nada sobre interface gráfica, processos remotos ou múltiplas partidas — recebe uma
  posição por texto, devolve lances legais ou o melhor lance. Ver `project_context.md` para
  o desenho interno (representação, geração, make/unmake) — não duplicado aqui.
- **Cliente Desktop (C++)**: interface gráfica, captura de input, e o lado "gerente de
  processo" da integração — sobe o motor, escreve comandos, lê respostas, trata o motor
  travar ou crashar. Nenhuma lógica de regras: todo lance, mesmo em partida local
  humano-vs-humano, é validado pelo motor antes de ser aceito na tela.

### Extensão futura — multiplayer online (não é MVP)

```
Cliente Desktop        Cliente Web (futuro, se sobrar tempo)
      |                          |
      +------------+-------------+
                   |
        HTTP (REST) + WebSocket
                   |
                   v
        Servidor (Node / TypeScript)
        - API REST + WebSocket
        - Sessões de partida online (estado em memória)
        - Spawn e comunicação com uma instância do motor por partida
                   |
                   | stdin / stdout — mesmo protocolo do MVP
                   v
        Motor de regras + IA (C)
```

O ponto importante deste segundo diagrama: **o protocolo motor↔processo-pai não muda**.
O servidor, quando existir, é só mais um processo que fala a mesma língua que o cliente
desktop já fala hoje. É por isso que dá pra adiar essa camada inteira sem reescrever nada
do motor nem do cliente.

## 2. Motor de regras + IA (C)

Já em desenvolvimento neste repositório (`engine/`). As decisões de representação,
codificação de peça/lance, geração de lances, apply/undo e o roadmap de fases estão
documentadas em detalhe em `project_context.md` e `roadmap-motor.md` — aqui só o resumo
necessário para entender a integração:

- **Representação**: mailbox de 64 casas (`Piece array[64]`), não bitboards — decisão
  fechada, com medição (varredura completa ~9,5 ns vs. ~12,3 ns de uma piece list).
- **Indexação**: `a1 = 0` (Little-Endian Rank-File), `sq = rank * 8 + file`.
- **Codificação de peça**: um `u8` (`Piece`), com `PIECE_MAKE`/`PIECE_TYPE`/`PIECE_COLOR`.
- **Codificação de lance**: um `u16` — 6 bits de origem, 6 de destino, 4 de tipo — com o
  tipo sendo uma enumeração de 16 valores **mutuamente exclusivos** (quieto, avanço duplo,
  os dois roques, captura, captura en passant, 4 promoções, 4 promoções-com-captura), como
  a Chess Programming Wiki recomenda:

  ```c
  typedef u16 Move;   /* [tipo:4][destino:6][origem:6] */

  typedef struct {
      Piece array[64];
      Color side_to_move;
      int   king_square[2];
      u8    castling_rights;  /* bitmask KQkq */
      int   ep_square;        /* -1 se não houver */
      int   halfmove_clock;
      int   fullmove_number;
  } Board;
  ```

  (Ver `board.h`, `piece.h`, `move.h` para a definição exata — o acima é fiel a eles, só
  sem os comentários e macros auxiliares.)

- **Geração de lances**: pseudo-legal primeiro (ignora se deixa o próprio rei em xeque),
  filtro de legalidade separado depois (por `is_square_attacked` com busca reversa a partir
  do rei). Mesma justificativa da versão anterior deste documento: mais simples de acertar
  do que gerar só lances legais direto.
- **Apply/undo**: pilha da recursão, não histórico global — `make_move(Board*, Move,
  Undo*)` / `unmake_move(Board*, Move, const Undo*)`, com `Undo` alocado pelo chamador. A
  pilha de chamadas de C já *é* a pilha de undo.
- **FEN**: parser e serializador — o formato de posição que circula entre motor e cliente
  (e, na extensão futura, também entre motor e servidor).
- **Motor stateless entre comandos, processo stateful**: cada comando `position` recebe o
  FEN completo (mais um sufixo opcional `moves ...`, que o motor reaplica). Isso evita
  qualquer ambiguidade de estado entre um comando e o próximo, ao custo de precisar do
  histórico completo para detectar repetição tripla (limitação aceita, ver `project_context.md`).
- **IA integrada no mesmo binário**: a busca chama a geração de lances milhares de vezes
  por lance pensado — se isso cruzasse qualquer fronteira de processo a cada nó, seria
  ordens de magnitude mais lento. A única fronteira externa é a de entrada/saída do
  protocolo: uma posição entra, um lance sai.
- **Fora de escopo agora** (deliberado, ver `project_context.md`/`CLAUDE.md`): bitboards,
  magic bitboards, transposition table. Entram só se sobrar tempo depois do MVP funcional.
- **Perft** como validação de geração de lances (padrão-ouro da área) e **testes como
  subcomando do próprio binário** (`./main.out test`, `./main.out perft N`) — decidido, ver
  `project_context.md` §3.

## 3. Comunicação Motor ↔ Cliente Desktop — stdin/stdout direto

Esta é a seção que substitui a antiga "Integração C ↔ Node/TS": **não existe FFI nem
servidor aqui.** O cliente desktop spawna o binário do motor como subprocesso e conversa
com ele por texto, uma linha por comando/resposta, num protocolo inspirado no UCI:

```
> position startpos
> go movetime 2000
< bestmove e2e4

> position fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2
> legalmoves
< moves g1f3 b1c3 f1c4 f1b5 d1h5 d1g4 ...
```
*(`>` = enviado pelo cliente desktop, `<` = recebido do motor)*

**Ciclo de vida do processo**: um subprocesso persistente **por partida ativa** — spawnado
quando a partida começa, mantido vivo até o fim — não um spawn a cada lance. Evita overhead
repetido e permite ao motor manter estado interno de busca entre lances.

**A armadilha que mais importa aqui, e que não tinha como aparecer na versão anterior deste
documento (que nunca chegava a especificar o lado do pipe do motor):** quando `stdout` é um
terminal, a libc usa buffer de linha; quando é um **pipe** — exatamente o caso de um
subprocesso — ela troca para buffer de bloco de 4 KB. Sem tratar isso, o `bestmove` fica
preso no buffer do motor e o cliente espera para sempre, sem nenhum sintoma visível. O
motor já está ciente disso (`roadmap-motor.md`, Fase 6a) e vai chamar `setvbuf(stdout, NULL,
_IOLBF, 0)` logo no início do `main`. **O lado do cliente também precisa ler de forma que
não fique bloqueado esperando um buffer que nunca enche** — isso é responsabilidade de
quem escrever a camada de processo no C++ (thread de leitura dedicada, ou I/O assíncrono).

**Multiplataforma**: a API de spawn de subprocesso e pipes difere entre POSIX (`fork`/`exec`
+ `pipe`) e Windows (`CreateProcess` + `CreatePipe`, ou uma lib que abstraia isso). Dois
detalhes concretos de que o cliente precisa cuidar: o executável do motor precisa da
extensão `.exe` no Windows, e o fim de linha pode chegar como `\r\n` — tratar os dois casos
ao parsear.

**Isolamento de falha**: um crash do motor (bug de memória em C, por exemplo) não pode
derrubar o cliente. O cliente deve detectar o subprocesso morto (pipe fechado / processo
terminado) e reportar isso como estado recuperável, não deixar a UI travada esperando uma
resposta que nunca vai chegar.

## 4. Cliente Desktop (C++)

Decidido — não é mais uma escolha entre stacks: **cliente nativo em C++, com SFML** para
renderização (ver `monitoramento_controle.md`, Iteração 3). A tabuleiro-cliente já nasce na
mesma linguagem de sistema que o motor, o que simplifica a integração de processo (sem
FFI, sem binding — só `stdin`/`stdout`/pipes, nativos da linguagem).

**Responsabilidades do cliente:**

- Renderizar tabuleiro e peças, capturar clique/drag-and-drop.
- Subir o motor como subprocesso no início da partida, falar o protocolo da seção 3.
- Traduzir clique de peça → notação de lance (`e2e4`), mandar pro motor, aplicar a
  resposta na tela — nunca decidir sozinho se um lance é legal.
- **Multiplayer local (hot-seat) já está no MVP**: dois jogadores humanos revezando no
  mesmo dispositivo, mesmo cliente, mesmo motor — não precisa de rede nem de servidor,
  só de alternar de quem é a vez de clicar. É o requisito RF01 do documento de requisitos
  do grupo, e ele não depende de nenhum trabalho de multiplayer online.
- Indicação de lances legais, xeque, fim de jogo — tudo **derivado das respostas do motor**
  (`legalmoves`, o próprio `bestmove`, e futuramente algum sinal de fim de partida no
  protocolo), nunca recalculado no cliente.

## 5. Extensão — multiplayer online (somente se sobrar tempo)

Tudo abaixo **não faz parte do MVP**. Existe aqui para não perder o desenho caso o grupo
decida investir tempo depois de o MVP (motor + cliente desktop, jogo completo contra IA e
multiplayer local) estar de pé e demonstrável.

### 5.1 Por que precisa de servidor aqui e não no MVP

No MVP, o único motor "de verdade" é o que roda na máquina do próprio jogador — não tem
com quem trapacear. Em multiplayer online, dois jogadores estão em máquinas diferentes;
nenhum dos dois pode ser a autoridade sobre se o próprio lance é legal. É isso, e só isso,
que justifica introduzir um terceiro processo: **validação e estado de partida
server-side**, o requisito RNF02 do documento de requisitos do grupo.

### 5.2 Integração Servidor (Node/TS) ↔ Motor (C)

Mesma decisão de sempre — **subprocesso, não FFI** — e pela mesma razão: uma chamada FFI
síncrona bloquearia a thread única do Node enquanto uma IA pensa, travando todas as outras
partidas simultâneas. Subprocesso isola cada partida no seu próprio processo, sem
concorrência manual em JS ou C, e ainda ganha o isolamento de falha de graça (um crash do
motor de uma partida não derruba as outras nem o servidor).

```javascript
// Pseudocódigo ilustrativo — gerenciamento do subprocesso do motor no servidor
const engine = spawn('./chess_engine' + (process.platform === 'win32' ? '.exe' : ''));

engine.stdin.write(`position fen ${currentFen}\n`);
engine.stdin.write('go movetime 2000\n');

engine.stdout.on('data', (chunk) => {
  const line = chunk.toString().trim();
  if (line.startsWith('bestmove')) {
    const move = line.split(' ')[1];
    // validar, aplicar, atualizar a sessão, notificar os clientes via WS
  }
});
```

### 5.3 Endpoints e estado de sessão

| Método | Rota | O que faz |
|---|---|---|
| `POST` | `/games` | Cria uma partida online nova |
| `GET` | `/games/:id` | Estado atual: FEN, histórico, status, de quem é a vez |
| `POST` | `/games/:id/moves` | Submete um lance; validado pelo motor antes de aplicar |
| `GET` | `/games/:id/legal-moves` | *(opcional)* lances legais da posição, pra UI destacar |
| `POST` | `/games/:id/resign` | Encerra por desistência |
| `WS` | `/games/:id/ws` | Notifica lances, "oponente pensando", fim de jogo |

```json
// Servidor → Cliente, via WebSocket
{ "type": "move_made", "by": "opponent", "move": "e7e5", "fen": "...", "inCheck": false }
{ "type": "game_over", "result": "checkmate", "winner": "white" }
{ "type": "illegal_move", "move": "e2e5", "reason": "movimento inválido para peão" }
```

- **Sessões em memória** (`gameId → GameSession`) bastam para esse escopo; banco de dados
  só entra se reconexão persistente virar requisito de verdade.
- Cada sessão modela dois "slots" de jogador: `{ type: 'ai' }`, `{ type: 'local_human' }`
  (caso um cliente desktop queira oferecer "jogar online contra alguém que joga localmente
  do outro lado", cenário raro) ou `{ type: 'remote_human', connectionId }`.
- **Validação sempre server-autoritativa** — o mesmo motor que valida localmente no MVP é
  reaproveitado aqui, só que rodando no servidor em vez de na máquina do jogador.

### 5.4 Cliente Web (futuro)

Só entra em cena junto com o servidor — não tem função sem ele, já que não haveria com quem
o cliente web falaria diretamente (motor em C não é exposto à web). Reaproveitar tanto
quanto possível do desenho visual do cliente desktop, mas como um projeto à parte: nenhum
código é automaticamente compartilhável entre um cliente C++/SFML e um cliente web.

## 6. Requisitos

### MVP

| ID | Descrição |
|---|---|
| RF-M1 | Partida completa contra a IA, localmente, sem internet e sem servidor |
| RF-M2 | Multiplayer local (hot-seat): dois jogadores no mesmo dispositivo, mesmo cliente |
| RF-M3 | O motor deve validar a legalidade de todos os lances, incluindo roque, en passant e promoção |
| RF-M4 | Detecção de xeque-mate, afogamento, e empates por regra dos 50 lances / material insuficiente |
| RF-M5 | A IA deve retornar um lance válido dentro de um tempo/profundidade configurável |
| RF-M6 | A interface deve exibir o tabuleiro, aceitar input de lances e refletir o estado da partida |
| RF-M7 | O sistema deve manter o histórico de lances de uma partida |

| ID | Descrição |
|---|---|
| RNF-M1 | Desempenho: a IA deve responder em tempo jogável (segundos, não minutos) na profundidade padrão |
| RNF-M2 | Isolamento de falhas: um crash do subprocesso do motor não pode derrubar nem travar o cliente |
| RNF-M3 | Testabilidade: o motor deve ser testável isoladamente, sem depender do cliente (via subcomandos `test`/`perft` e o menu interativo) |
| RNF-M4 | Portabilidade: motor e cliente compilam e rodam nos SOs-alvo definidos pelo grupo |
| RNF-M5 | Usabilidade: feedback claro sobre lances ilegais, de quem é a vez, e estado de xeque |

### Extensão — multiplayer online (se sobrar tempo)

Correspondem a RF14–RF19 do documento de requisitos do grupo (autenticação, perfil,
convite direto, matchmaking, comunicação em tempo real, tratamento de desconexão):

| ID | Descrição |
|---|---|
| RF-X1 | O servidor deve expor uma API (REST + WS) para criar partidas online, submeter lances e notificar atualizações |
| RF-X2 | A arquitetura de sessões deve suportar múltiplas partidas online simultâneas |
| RF-X3 | Autenticação e perfil de jogador (apelido + ID) |
| RF-X4 | Convite direto por apelido/ID e/ou matchmaking automático |
| RF-X5 | Tratamento de desconexão temporária com janela de reconexão |

| ID | Descrição |
|---|---|
| RNF-X1 | Extensibilidade: adicionar esta camada não deve exigir reescrever o motor nem o cliente desktop — só é possível porque o protocolo motor↔processo-pai (§3) não muda |
| RNF-X2 | Segurança: validação de lance sempre server-side; senhas armazenadas com hash |
| RNF-X3 | Disponibilidade/escalabilidade do servidor (uptime, sessões assíncronas) |

## 7. Roadmap de implementação

A ordem é deliberada: primeiro o que é isolável e testável sozinho (motor via terminal,
sem precisar de mais nada), depois a integração com o cliente, e só então multiplayer —
que fica de fora do caminho crítico inteiramente.

### Etapa 1 — Motor de regras + IA (C), isolado

Coberto em detalhe por `roadmap-motor.md` (Fases 0–8 daquele documento: fundação de
qualidade, vocabulário completo, geometria, geração pseudo-legal, make/unmake, filtro de
legalidade, protocolo mínimo + IA aleatória, perft, IA incremental, robustez). Não
duplicado aqui — esse é o plano de execução autoritativo do motor.
**Testar:** perft nas posições de referência; corpus de FEN; o próprio motor via seu menu
interativo/subcomandos, sem precisar de nenhum cliente.

### Etapa 2 — Integração Motor ↔ Cliente Desktop

**Implementar:** no cliente C++, a camada de processo (spawn, pipes, leitura não-bloqueante)
descrita na seção 3; tradução clique/drag-and-drop → notação de lance; parsing das
respostas do motor (`bestmove`, `legalmoves`).
**Testar:** jogar uma partida inteira do início ao fim pela UI, contra o motor real —
substitui a etapa de "cliente CLI mínimo" que fazia sentido quando havia uma API HTTP no
meio para validar; aqui a integração já é simples o bastante pra testar direto com a UI.

### Etapa 3 — Cliente desktop completo

**Implementar:** tabuleiro com input de lances, destaque de lances legais, indicadores de
xeque/fim de jogo, histórico de lances na tela; multiplayer local (hot-seat).
**Testar:** partida completa via UI, incluindo casos especiais (roque, en passant,
promoção, xeque-mate, afogamento); rodar em pelo menos duas máquinas/SOs diferentes do
grupo; matar o processo do motor manualmente e confirmar que o cliente não trava.

### Etapa 4 (extensão, se sobrar tempo) — Servidor + multiplayer online

**Implementar:** servidor Node/TS (seção 5.2–5.3); autenticação e perfil; convite/matchmaking;
cliente web, se o grupo decidir por ele.
**Testar:** via curl/Postman antes de qualquer UI; partida completa entre dois clientes
remotos; matar o subprocesso do motor no servidor e confirmar que só aquela partida cai.

## 8. Decisões em aberto para o grupo

- ~~Cliente fala direto com o motor, ou sempre via backend?~~ **Fechada por este documento:
  direto, via subprocesso, para o MVP.** (Isso resolve a divergência que existia entre este
  documento e `project_context.md`, registrada como pendência em `roadmap-motor.md` §7.)
- **SOs-alvo**: quais sistemas operacionais o motor e o cliente C++/SFML precisam suportar
  — afeta build (Makefile/CMake) e empacotamento.
- **Cronograma da extensão**: só faz sentido mapear em cima do calendário real depois que o
  MVP estiver com data de conclusão mais confiável — não antes.
- **Stack do servidor**, caso a extensão avance: Node/TypeScript é a escolha default deste
  documento, mas não foi validada com o grupo ainda.
- **Bitboards e transposition table**: seguem fora de escopo do MVP (ver `project_context.md`);
  reavaliar só se sobrar tempo depois da IA v2 (`roadmap-motor.md` Fase 7).
- **Persistência**: sessões em memória bastam para a extensão de multiplayer nesse escopo;
  banco de dados só entra se reconexão persistente entre sessões do servidor virar requisito
  de verdade.
