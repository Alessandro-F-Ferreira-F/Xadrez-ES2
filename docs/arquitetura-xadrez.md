# Arquitetura do Sistema de Xadrez

> Documento de design técnico — motor de regras, IA, backend e cliente.

Este documento define como as partes do sistema (motor de regras, IA, backend e cliente) devem se integrar, levanta os requisitos do projeto, e propõe um roadmap de implementação em etapas testáveis.

## Princípio arquitetural central

A ideia que amarra toda a arquitetura: **o cliente nunca fala diretamente com o código em C.** Ele sempre fala com o backend (Node/TS) através de uma API HTTP + WebSocket — seja esse backend rodando em `localhost` (modo offline, contra a IA) ou em um servidor remoto (multiplayer, no futuro).

Isso significa que "jogar offline" não é um modo especial com um caminho de código separado: é só o backend rodando localmente em vez de remotamente. O cliente é sempre um consumidor da mesma API — é isso que a torna "universal", e é isso que resolve o requisito de jogar sem internet sem precisar de nenhuma lógica extra no cliente.

## 1. Visão geral da arquitetura

```
Cliente Desktop          Cliente Web (futuro)
      |                          |
      +------------+-------------+
                   |
        HTTP (REST) + WebSocket
                   |
                   v
        Backend (Node / TypeScript)
        - API REST + WebSocket
        - Sessões de partida (estado em memória)
        - Spawn e comunicação com o motor
                   |
                   | stdin / stdout
                   | (protocolo texto, estilo UCI)
                   v
        Motor de regras + IA (C)
        - Geração e validação de lances
        - Busca (minimax + alfa-beta)
```

Três camadas, cada uma com uma responsabilidade clara:

- **Motor + IA (C)**: um binário que sabe jogar xadrez. Não sabe nada sobre HTTP, sessões, ou múltiplas partidas — recebe uma posição, devolve lances legais ou o melhor lance.
- **Backend (Node/TS)**: orquestra. Gerencia sessões de partida, expõe a API, decide quando chamar o motor, e notifica os clientes.
- **Cliente(s)**: só sabe falar com a API. Não tem lógica de xadrez nenhuma além de UX otimista (destacar lances legais, etc.).

## 2. Motor de regras (C)

**Sim, deve ser em C.** A justificativa não é só "porque já decidimos usar C no backend" — é que geração de lances é chamada em volume altíssimo pela IA durante a busca (milhares de vezes por lance pensado), então é exatamente o tipo de código que se beneficia de performance bruta. Além disso, as regras do xadrez são um problema fechado e bem especificado — não vão mudar — então não existe o benefício de "iterar rápido" que outras linguagens ofereceriam aqui.

**Componentes:**

- **Representação de tabuleiro**: recomendo começar com array 8x8 (mailbox), não bitboards. Bitboards são mais rápidos, mas bem mais complexos de implementar e depurar corretamente. Pra um projeto com prazo, array 8x8 com alfa-beta bem implementado já alcança profundidade de busca boa o suficiente. Bitboards ficam como otimização de etapa avançada, se sobrar tempo.
- **Geração de lances**: pseudo-legais primeiro (ignora se deixa o próprio rei em xeque), depois um filtro de legalidade separado. É mais simples de implementar corretamente do que gerar só lances totalmente legais direto.
- **Casos especiais**: roque, en passant, promoção — são a parte que mais gera bugs sutis em motores caseiros.
- **Aplicar/desfazer lance**: usem apply/undo com uma pilha (guardando peça capturada, direitos de roque, alvo de en passant antes do lance) em vez de copiar o tabuleiro inteiro a cada nó de busca. Isso importa de verdade pra performance da busca, e é uma decisão que dói para trocar depois — vale decidir certo desde o início.
- **FEN**: parser e serializador. É o formato de posição que vai circular por todo o sistema (motor, backend, e possivelmente até o cliente).
- **Detecção de fim de jogo**: xeque, xeque-mate, afogamento, empate por regra dos 50 lances / material insuficiente.

```c
// Esboço ilustrativo da representação — não é a interface final
typedef enum { EMPTY, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING } PieceType;

typedef struct {
    PieceType squares[64];       // representação mailbox (8x8 linear)
    Color     colors[64];
    Color     side_to_move;
    int       castling_rights;   // bitmask: KQkq
    int       en_passant_square; // -1 se não houver
    int       halfmove_clock;
    int       fullmove_number;
} Board;
```

**Teste recomendado — perft**: é o padrão-ouro pra validar geração de lances em motores de xadrez. Consiste em contar o número de nós (posições) alcançáveis em profundidades conhecidas a partir de posições de referência, e comparar com valores publicados. Pega bugs sutis (en passant errado, roque através de casa atacada, promoção mal tratada) que teste manual dificilmente encontra.

## 3. IA (C): arquitetura e transferência de dados

**A IA integra diretamente com o motor — mesmo binário, chamadas de função C comuns.** Isso é importante: durante a busca, a IA chama a função de geração de lances do motor milhares de vezes. Se isso passasse por qualquer fronteira (FFI, subprocess, rede) a cada nó da árvore de busca, a IA seria ordens de magnitude mais lenta. A fronteira com o "mundo de fora" (o backend em Node) deve existir só na borda externa: *"aqui está uma posição, busque até profundidade/tempo X, devolva o melhor lance"* — uma chamada de entrada, uma de saída. Tudo dentro disso é C chamando C.

**Camadas dentro do C:**

- **Avaliação**: função estática que dá uma pontuação pra uma posição — material + piece-square tables (tabelas de valor por casa/tipo de peça) pra começar. Pode ser refinada depois (segurança do rei, estrutura de peões, mobilidade).
- **Busca**: minimax + poda alfa-beta, com iterative deepening (buscar profundidade 1, depois 2, depois 3... dentro de um orçamento de tempo — assim sempre existe um "melhor lance até agora" pronto, mesmo se o tempo acabar no meio da busca de uma profundidade maior). Ordenação de lances (testar capturas primeiro, usando heurística tipo MVV-LVA — most valuable victim, least valuable attacker) torna a poda alfa-beta bem mais eficaz.
- **Transposition table** (opcional, etapa avançada): cache de posições já buscadas, indexado por hash Zobrist, pra evitar rebuscar a mesma posição alcançada por ordens de lance diferentes.
- **Interface**: a camada fina que lê comandos de texto (via stdin) e traduz pra chamadas nas camadas acima, formatando a resposta de volta.

**Um detalhe que vale reaproveitar**: como a IA já precisa de acesso total ao motor de regras internamente, o mesmo binário pode expor tanto um comando `go` (buscar melhor lance) quanto um comando `legalmoves`/`checkmove` (só validar/gerar lances, sem busca). Isso significa que o **mesmo processo C serve tanto como "IA" quanto como "validador de regras"** para o backend — sem precisar reimplementar nada em JS.

### Transferência de dados

Protocolo texto, linha a linha, inspirado no UCI (Universal Chess Interface — o protocolo que engines reais como Stockfish usam):

```
> position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1
> go movetime 2000
< bestmove e2e4

> position fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2
> legalmoves
< moves g1f3 b1c3 f1c4 f1b5 d1h5 d1g4 ...
```
*(`>` = enviado pelo backend, `<` = recebido do motor)*

- **Entrada**: sempre o FEN completo da posição atual, não o histórico incremental. Isso deixa o motor stateless entre chamadas — mais simples de implementar e muito mais fácil de testar/depurar (dá pra reproduzir qualquer posição isoladamente).
- **Limitação a ter em mente**: FEN sozinho não é suficiente pra detectar empate por repetição tripla, porque não carrega o histórico completo da partida. Se quiserem essa regra, o backend precisa rastrear o histórico de posições e informar isso separadamente — ou deixar como um item de escopo futuro.
- **Saída**: o lance em notação longa (`e2e4`, `e7e8q` pra promoção), e opcionalmente avaliação/profundidade/nós buscados, úteis pra debug e pra UI mostrar "pensando... profundidade 6, avaliação +0.3".

**Ciclo de vida do processo**: um subprocesso persistente por partida ativa (spawn quando a partida começa, mantido vivo até o fim), não um spawn a cada lance. Isso evita overhead repetido de spawn, permite manter estado interno (transposition table) entre lances da mesma partida, e segue o mesmo modelo mental do UCI de verdade.

## 4. Integração C ↔ Node/TS

Duas opções realistas pra Node chamar o binário C: **FFI** (carregar o C como shared library, ex: com a lib `koffi`) ou **subprocess** (o C roda como processo separado, comunicação por stdin/stdout).

**Recomendação: subprocess.** A razão específica pro contexto de vocês (API universal, backend pensado pra suportar múltiplas partidas simultâneas — base pro multiplayer futuro): por padrão, uma chamada FFI síncrona bloqueia a thread principal do Node enquanto a IA pensa. Como o Node é single-threaded, isso travaria o processamento de **todas as outras partidas simultâneas** enquanto uma IA calcula um lance. Dá pra contornar isso (threads dedicadas, padrões assíncronos), mas isso adiciona complexidade que o modelo de subprocess já resolve de forma natural — cada processo roda independente, paralelizado pelo próprio sistema operacional, sem nenhum código de concorrência manual em JS ou C.

Vantagens adicionais do subprocess: um crash de bug de memória no C derruba só aquele processo, não o backend inteiro; dá pra testar o motor sozinho no terminal antes de escrever qualquer código de integração; e é literalmente o modelo que engines de xadrez reais usam (UCI), então há bastante prática consolidada em cima disso.

```javascript
// Pseudocódigo ilustrativo — gerenciamento do subprocess do motor
const engine = spawn('./chess_engine' + (process.platform === 'win32' ? '.exe' : ''));

engine.stdin.write(`position fen ${currentFen}\n`);
engine.stdin.write('go movetime 2000\n');

engine.stdout.on('data', (chunk) => {
  const line = chunk.toString().trim();
  if (line.startsWith('bestmove')) {
    const move = line.split(' ')[1];
    // aplicar o lance, atualizar o estado da sessão, notificar clientes via WS
  }
});
```

**É multiplataforma?** Sim, com alguns cuidados:

- Escrevam o C em padrão portável (evitem APIs específicas de SO na lógica principal do motor).
- Usem CMake em vez de Makefile puro — generaliza melhor entre compiladores/SOs diferentes.
- Não existe binário único que rode em qualquer SO: precisa compilar separadamente por SO-alvo (cada membro do grupo compila localmente, ou configuram CI pra gerar os artefatos).
- `child_process.spawn` do Node é multiplataforma na API, mas fiquem atentos a dois detalhes: o executável precisa da extensão `.exe` no Windows, e a saída do processo pode vir com `\r\n` em vez de `\n` — tratem os dois casos ao parsear linha por linha.

## 5. Backend: endpoints e gerenciamento de estado

| Método | Rota | O que faz |
|---|---|---|
| `POST` | `/games` | Cria uma partida nova (vs. IA, com profundidade/tempo configurável) |
| `GET` | `/games/:id` | Retorna o estado atual: FEN, histórico de lances, status, de quem é a vez |
| `POST` | `/games/:id/moves` | Submete um lance; validado pelo motor antes de ser aplicado |
| `GET` | `/games/:id/legal-moves` | *(opcional)* lista os lances legais da posição atual, pra UI destacar |
| `POST` | `/games/:id/resign` | Encerra a partida por desistência |
| `GET` | `/health` | Verifica se o backend está de pé — essencial no modo local |
| `WS` | `/games/:id/ws` | Canal em tempo real: notifica lances, "IA pensando", fim de jogo |

A divisão faz sentido assim: **REST pra ações de comando** (criar/consultar/encerrar partida — request/response natural) e **WebSocket pro fluxo de jogo em tempo real** (lances e notificações — importante desde já porque é exatamente o canal que o multiplayer vai reaproveitar sem mudança).

```json
// POST /games — corpo da requisição
{
  "mode": "vs_ai",
  "aiConfig": { "depth": 6, "movetimeMs": 2000 },
  "playerColor": "white"
}
```

```json
// Servidor → Cliente, via WebSocket
{ "type": "move_made", "by": "ai", "move": "e7e5", "fen": "...", "inCheck": false }
{ "type": "ai_thinking" }
{ "type": "game_over", "result": "checkmate", "winner": "white" }
{ "type": "illegal_move", "move": "e2e5", "reason": "movimento inválido para peão" }
```

**Outros pontos importantes de design:**

- **Sessões em memória**: um mapa `gameId → GameSession` é suficiente pra essa fase (sem banco de dados). Encapsulem o acesso a esse mapa atrás de um módulo/interface pequeno, pra facilitar trocar por persistência real quando o multiplayer com reconexão for implementado de verdade.
- **IA assíncrona**: quando for a vez da IA, respondam à requisição do lance do humano imediatamente, computem a IA em background (via o subprocess), e empurrem o resultado por WebSocket quando pronto. Nunca bloqueiem a resposta HTTP esperando a IA pensar.
- **Validação sempre server-autoritativa**: nunca confiem em legalidade calculada no cliente. Pra UX responsiva sem duplicar as regras em JS, uma boa estratégia é buscar os lances legais da posição uma vez por turno via `/legal-moves` e cachear no cliente só pra destacar visualmente — sem reimplementar validação nenhuma ali.
- **Isolamento de falha**: um crash do subprocesso do motor não pode derrubar o backend. Tratem a comunicação com try/catch, tenham timeout, e reportem como erro recuperável ao cliente.
- **Health-check**: crucial especificamente pro modo local — o cliente desktop precisa saber quando o backend local terminou de subir antes de tentar conectar.

## 6. Estrutura para multiplayer (preparando o terreno, sem implementar ainda)

Não implementem multiplayer agora, mas desenhem a estrutura de forma que adicioná-lo depois não exija reescrever a lógica de jogo:

- Modelem cada sessão de partida com dois "slots" de jogador (branco/preto), onde cada slot pode ser `{ type: 'ai' }`, `{ type: 'local_human' }` ou, no futuro, `{ type: 'remote_human', connectionId: ... }`.
- A lógica de broadcast via WebSocket já deve ser escrita de forma genérica ("notificar todos os clientes inscritos nesse `gameId`"), que funciona igual com uma conexão (single-player, só observando os lances da IA) ou duas (multiplayer).

Com isso, adicionar multiplayer de verdade vira "adicionar um novo tipo de slot + lógica de matchmaking/conexão", não uma reescrita.

## 7. Cliente: opções e requisito offline

Com a arquitetura acima, o requisito "jogar contra a IA localmente sem internet" já está resolvido por construção: o cliente nunca fala com o C diretamente, então "offline" é só o backend (Node/TS + subprocess do motor) rodando em `localhost` em vez de remoto. O fluxo de inicialização fica assim: o app desktop sobe o processo do backend local → aguarda `/health` responder → conecta a UI normalmente, exatamente como se estivesse falando com um servidor remoto.

Dado que o grupo decidiu por cliente desktop, restam duas famílias de opção:

- **Tauri ou Electron (JS/TS)**: reaproveita quase todo o código de UI com um futuro cliente web — se usarem algo como React, boa parte pode literalmente ser compartilhada. Mesmo skillset do time, `fetch`/`WebSocket` nativos, sem lib de rede extra. Tauri é mais leve (usa o webview nativo do SO + uma casca fina em Rust) que Electron (empacota Chromium + Node inteiros), mas ambos evitam introduzir mais uma linguagem no projeto.
- **C++ nativo (Qt)**: mais "genuinamente desktop", e a transição vindo de C é mais suave do que pra outras linguagens. Mas introduz uma terceira frente de linguagem simultânea no projeto (depois de C no motor e JS/TS no backend), compilação mais lenta que o ciclo de um app web, zero reaproveitamento de código com um cliente web futuro, e rede em C++ exige lib extra (Boost.Beast, Qt Network).

**Recomendação**: Tauri (ou Electron, se preferirem simplicidade/maturidade em troca de peso maior). Como o grupo já vai lidar com C (motor+IA) e JS/TS (backend) ao mesmo tempo, manter o frontend também em JS/TS reduz o número de trocas de contexto de linguagem simultâneas — o que pesa bastante com poucas horas semanais disponíveis — sem abrir mão de nada "impressionante" no projeto, já que isso está garantido pelo motor+IA em C. Se o grupo tiver tempo sobrando e quiser Qt/C++ como parte do aprendizado, é uma escolha válida — só entrem cientes do trade-off de tempo.

## 8. Levantamento de requisitos

### Requisitos funcionais (RF)

| ID | Descrição |
|---|---|
| RF01 | O sistema deve permitir uma partida completa contra a IA, localmente, sem internet |
| RF02 | O motor deve validar a legalidade de todos os lances, incluindo roque, en passant e promoção |
| RF03 | O sistema deve detectar xeque-mate, afogamento, e empates por regra dos 50 lances / material insuficiente |
| RF04 | A IA deve retornar um lance válido dentro de um tempo/profundidade configurável |
| RF05 | A interface deve exibir o tabuleiro e aceitar input de lances, refletindo o estado da partida |
| RF06 | O backend deve expor uma API (REST + WS) para criar partidas, submeter lances e notificar atualizações |
| RF07 | A arquitetura de sessões deve suportar, em estrutura, múltiplas partidas simultâneas — base para multiplayer |
| RF08 | O sistema deve manter o histórico de lances de uma partida |

### Requisitos não funcionais (RNF)

| ID | Descrição |
|---|---|
| RNF01 | Portabilidade: rodar nos SOs-alvo definidos pelo grupo, com apenas recompilação do C onde necessário |
| RNF02 | Desempenho: a IA deve responder em tempo jogável (ordem de segundos, não minutos) na profundidade padrão |
| RNF03 | Isolamento de falhas: um crash do subprocesso do motor não pode derrubar o backend nem travar o cliente |
| RNF04 | Testabilidade: o motor deve ser testável isoladamente, sem depender do backend ou da interface |
| RNF05 | Extensibilidade: adicionar multiplayer no futuro não deve exigir reescrever a lógica de jogo já implementada |
| RNF06 | Usabilidade: feedback claro sobre lances ilegais, de quem é a vez, e estado de xeque |

## 9. Roadmap de implementação

A ordem abaixo é deliberada: primeiro as peças mais isoláveis e testáveis sozinhas (motor, depois IA — ambas testáveis via terminal, sem precisar de mais nada), depois a integração (backend), depois um cliente mínimo só pra validar o pipeline inteiro, e só então a interface gráfica de verdade. Essa sequência evita descobrir um bug de regra do xadrez depois de já ter investido tempo em UI.

### Etapa 1 — Setup e definição de contratos
**Implementar:** estrutura do repositório (`engine/`, `backend/`, `client/`); toolchain de build do C (CMake); esqueleto do backend com `/health`; rascunho do protocolo texto entre backend e motor (formato de FEN e de lances).
**Testar:** build do C compila e roda; `/health` responde; protocolo revisado pelo grupo.

### Etapa 2 — Motor de regras (C), isolado
**Implementar:** representação de tabuleiro + FEN; geração de lances pseudo-legais + filtro de legalidade; roque, en passant, promoção; apply/undo com pilha; detecção de xeque/xeque-mate/afogamento.
**Testar:** testes unitários com FENs conhecidos; **perft** em profundidades de referência; CLI de debug (lê FEN, lista lances legais).

### Etapa 3 — IA (C), ainda isolada
**Implementar:** avaliação (material + piece-square tables); minimax + poda alfa-beta; iterative deepening; ordenação de lances (MVV-LVA); interface de comandos via stdin/stdout.
**Testar:** manualmente via terminal; IA nunca crasha e nunca devolve lance ilegal, em posições variadas; medir nós/segundo e profundidade alcançável em tempo fixo, como baseline.

### Etapa 4 — Backend: integração + API
**Implementar:** gerenciamento do subprocess (spawn, I/O, parsing); sessões em memória; endpoints REST; WebSocket por partida; tratamento de erro/crash do subprocesso.
**Testar:** via curl/Postman, sem UI; simular uma partida inteira via chamadas manuais de API; matar o subprocesso manualmente e confirmar que o backend não cai.

### Etapa 5 — Cliente mínimo (CLI), pra validar o pipeline
**Implementar:** um cliente de linha de comando (não faz parte do produto final) que imprime o tabuleiro em texto e aceita lances digitados, falando com o backend via HTTP/WS.
**Testar:** jogar uma partida inteira do início ao fim, só em texto — confirma que todo o pipeline (cliente → API → backend → subprocess → motor/IA → volta) funciona antes de investir em UI gráfica.

### Etapa 6 — Cliente desktop real
**Implementar:** stack final (Tauri/Electron ou Qt); tabuleiro com input de lances, destaque de lances legais, indicação de xeque/fim de jogo; lógica de subir o backend local automaticamente e aguardar `/health`; empacotamento básico.
**Testar:** partida completa via UI; fluxo offline (desconectar da rede e confirmar que funciona — valida RF01); rodar em pelo menos duas máquinas/SOs diferentes do grupo.

### Etapa 7 — Polimento e preparação para multiplayer
**Implementar:** transposition table (hash Zobrist), melhor ordenação, avaliação mais refinada; histórico visível, exportação PGN, relógio (se decidido); prova de conceito com dois slots humanos na mesma sessão.
**Testar:** regressão (rodar perft de novo, garantir que nada quebrou); teste de conceito multiplayer local (dois clientes, mesma partida, mesma máquina).

## 10. Decisões em aberto para o grupo

- **SOs-alvo**: quais sistemas operacionais o projeto precisa suportar? Afeta o setup de build e o empacotamento do cliente.
- **Tauri vs. Electron vs. Qt**: decisão final — depende de quanto o grupo quer investir em aprender uma stack nova vs. reaproveitar JS/TS.
- **Cronograma**: este documento não define prazos em semanas porque depende da duração total do projeto e da carga horária do grupo — vale mapear as etapas acima em cima do calendário real de entregas.
- **Bitboards e transposition table**: tratados aqui como otimizações opcionais da Etapa 7 — vale decidir cedo se isso é meta do grupo, já que impacta o tempo total.
- **Persistência**: sessões em memória bastam pra este escopo; banco de dados só entra em cena se multiplayer com reconexão for implementado de verdade.
