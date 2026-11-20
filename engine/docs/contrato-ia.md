# Contrato motor–IA até a apresentação de 23/11/2026

## Cabeçalhos congelados

Até a apresentação, mudanças incompatíveis exigem aviso e alinhamento prévio com a
equipe da IA. Preservar assinaturas, codificação e semântica de:

- `board.h`, `piece.h`, `move.h`, `makemove.h`, `movegen.h`, `square.h` e `perft.h`;
- `generate_legal_moves`, `make_move`, `unmake_move`, `move_is_capture`,
  `move_is_ep_capture`, `move_from`, `move_to`, `move_to_str`, `movelist_find`,
  `is_square_attacked` e `perft`;
- `SQ_TO_EDGE`, `DIR_OFFSET`, `KNIGHT_ATTACKS`, `PAWN_ATTACKS`, `PAWN_PUSH`,
  `RANK_OF` e `FILE_OF`, incluindo índices e valores de sentinela.

`a1 = 0`; destinos fora do tabuleiro usam `SQ_NONE`. `KING_TARGETS[sq][dir]`
agora contém o vizinho na direção `dir` ou `SQ_NONE`, inicializado por
`init_square_tables()`. Inicializar as tabelas antes da geração/busca; `fen_parse`
também as inicializa para sua própria validação.

## Consultas públicas de regras

`attack.h/c` reúne `is_square_attacked(b, sq, defender)` e `in_check(b)`.
O terceiro argumento é **o defensor**, nunca o atacante. A declaração continua
acessível por `movegen.h` para compatibilidade com os consumidores existentes.

`draw.h/c` oferece `DrawReason draw_reason(const Board *b)`: `DRAW_NONE = 0`,
`DRAW_50`, `DRAW_MATERIAL` e `DRAW_REPETITION`. A consulta não gera lances, não
modifica o tabuleiro e percorre no máximo 64 casas. Implementa 50 lances e
material insuficiente: K×K, K+B×K, K+N×K e apenas reis e bispos todos na mesma
cor de casa. K+NN×K, bispos em cores opostas e bispo+cavalo não são declarados
empate. Não tenta reconhecer todas as posições mortas possíveis.

Mate/afogamento devem ser verificados antes das regras de empate. `state` e a
raiz da IA respeitam isso; na busca, um empate por regra em posição de xeque
também verifica se há mate. Os 50 lances encerram automaticamente nesta versão.

`IA/src/regras.c` adapta os motivos para `testeEmpateRegra`, preservando o
contrato da IA. `stubs.c` é exclusivo dos testes isolados e não entra no motor.
Os resultados reais de empate já são mapeados para `JOGADA.resultado`.

Repetição tripla fica somente na partida, no protocolo. O histórico é reconstruído
por `position ... moves ...`, com até 1025 posições, e substituído atomicamente.
Uma FEN isolada não informa posições anteriores. A chave compara peças, lado,
roque e en passant **com captura legal disponível**, sem depender dos relógios.
Não há chaves da partida repassadas à árvore da IA; isso fica para uma futura
integração de histórico/Zobrist.

## Busca e dificuldade

- Nível: `setoption name Nivel value N`, com 1=fácil, 2=médio, 3=difícil.
  Padrão médio; a opção persiste em `ucinewgame` e `position`.
- Sem limites explícitos, `go` usa exatamente `parametrosNivel`: fácil com
  profundidade 2, 180 ms e sem quiescência; médio com profundidade 3, 180 ms e
  quiescência; difícil com profundidade 64, 950 ms e quiescência.
- `go depth N` aceita 1–64 e desliga o prazo do perfil.
- `go movetime T` aceita 0–INT_MAX ms e permite aprofundar até 64.
  Zero desliga o relógio, inclusive quando não há `depth`.
- Com os dois parâmetros, ambos valem; a busca termina no primeiro limite.
  Quiescência e sorteio continuam conforme o nível. Os limites não alteram o perfil.
- A busca usa uma cópia do `Board`. O protocolo confere o lance completo na lista
  legal. Em partida terminada ou resultado da IA diferente de sucesso, retorna
  `bestmove 0000`; em sucesso, envia `info depth ... score cp|mate ... nodes ...`
  antes do `bestmove`.
- A semente permanece fixa e é reiniciada por `ucinewgame`. O protocolo sorteia
  entre `CANDIDATOS` usando seu xorshift, independente do `rand()` da libc.
  Busca sem relógio é reproduzível; com prazo, velocidade da máquina e carga
  podem mudar a iteração concluída e, consequentemente, os candidatos.

RNF01: resposta abaixo de 200 ms no fácil/médio e de 1 s no difícil. Os perfis
reservam 20/50 ms de margem. O runner mede comando/resposta com pipe aberto,
excluindo a criação do processo, em startpos e kiwipete. Medições locais no
Windows/MinGW com `-Og`: fácil abaixo de 1 ms, médio cerca de 182 ms no kiwipete,
difícil cerca de 951 ms. São medições dessas posições/máquina, não garantia de
latência sob qualquer carga. Limites explícitos substituem o orçamento RNF01.

## Validação

Na raiz: `make -C engine`, `python3 engine/test/protocol/run.py` e
`make -C engine test`. No Windows, usar `mingw32-make` e `py`; o executável é
`engine/build/main.exe`. Para a suíte isolada da IA, compilar explicitamente
`IA/src/teste.c`, `ia.c`, `avaliar.c`, `ordenar.c`, `niveis.c` e `stubs.c`, junto
com os fontes de regras necessários (sem `main.c`, `uci.c` ou `regras.c`).
