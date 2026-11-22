# Testes de aceitação desktop — Xadrez ES2

Esta suíte automatiza a aplicação SFML como um usuário: inicia o executável,
localiza sua janela, envia cliques/teclas, redimensiona a janela e verifica as
peças nas capturas da área de cliente. Para os cenários que produzem uma
resposta de estado, também analisa passivamente as linhas UCI que a própria
interface já registrou em seu stdout. Não modifica o código, os testes ou a
configuração de build do jogo. A POC `poc_window.py` e suas capturas existentes
são preservadas.

## Requisitos

- Windows com sessão gráfica interativa desbloqueada (não funciona em sessão
  Windows sem desktop interativo, como alguns serviços/CI sem desktop).
- Python 3.10 ou superior.
- Jogo compilado em `../interface/build/xadrez.exe`.
- Motor compilado em `../Backend/build/main.exe`.
- Diretório de trabalho da aplicação `../interface`, necessário para carregar
  `assets/pieces/` e localizar `../Backend/build/main.exe`.
- Escala de exibição e resolução suficientes para a janela e os elementos da
  interface. A automação reduz a janela mantendo o aspecto 1120×800 e falha
  explicitamente se a área disponível ficar abaixo do mínimo.

## Instalação

No PowerShell, a partir de `acceptance-tests/`:

```powershell
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

Se o ambiente virtual já existir, a segunda linha basta. Não é necessário
OpenCV nem OCR: Pillow compara as casas do tabuleiro com as texturas PNG das
peças incluídas pela própria aplicação. As verificações de navegação e
realces de controles ainda usam cores/pontos visuais do layout.

## Execução

Executar todos os cenários e gerar relatórios:

```powershell
.\.venv\Scripts\python.exe .\run_tests.py
```

Executar um cenário por nome:

```powershell
.\.venv\Scripts\python.exe .\run_tests.py tests\test_acceptance_flows.py::test_local_game_moves_highlights_and_history
```

Também é possível usar `pytest` diretamente dentro do ambiente. Nesse caso os
relatórios JSON/Markdown são criados pelo plugin local; `run_tests.py` é o
comando recomendado porque também configura automaticamente a saída JUnit.

Para apontar para outro binário Windows do jogo, defina `CHESS_GAME_EXE` antes
de executar. O diretório de trabalho permanece `interface/`, pois o jogo usa
caminhos relativos para assets e motor.

## Evidências e relatórios

Cada execução cria um diretório exclusivo em `reports/`, por exemplo:

```text
reports/20261122-143000-123456/
  junit.xml
  report.json
  report.md
  evidence/<cenario>/*.png
  logs/<cenario>.log
```

- `report.md`: resultado PASS/FAIL/SKIP, comportamento esperado e observado,
  evidência UCI disponível, limitações, motivo de falha, duração e links para
  capturas/log.
- `report.json`: mesmos resultados em formato estruturado, incluindo o código de
  saída do pytest e listas separadas de evidências visuais, observações e
  limitações.
- `junit.xml`: compatível com ferramentas de CI.
- Capturas: recortadas à janela do jogo para não incluir o restante da área de
  trabalho.

Um clique só conta como aprovado quando as verificações do cenário confirmam o
resultado: o harness compara o FEN registrado pela interface após lances,
confere a transição de casas/turno e, nos lances críticos, confirma as peças
visíveis por textura. Falhas de inicialização, foco, timeout ou divergências
reprovam o cenário. Cenários não executáveis fora de Windows são registrados
como SKIP, nunca como PASS.

## Cenários e limitações

Os cenários cobrem menu/saída, navegação por Escape, opções visuais de modo,
dificuldade/cor/tempo, redimensionamento, início de jogo local e contra IA,
destaques de lances legais, movimentos e histórico, lance inválido, desistência,
xeque-mate/reinício, en passant, roque e promoção.

### Observação do protocolo e limites

A aplicação já registra no log respostas que recebeu normalmente do motor,
incluindo `state ... fen ...` e, na partida contra IA, `bestmove ...`. O harness
somente lê esse arquivo de log; não abre os pipes do motor, não consome nem
duplica mensagens, não envia comandos UCI e não usa a opção `--trace`. Assim, o
fluxo de comunicação original permanece inalterado. Se a interface não registrar
a informação necessária, a checagem de protocolo falha/permanece limitada em
vez de consultar o motor separadamente.

Os cenários validam os dois lances locais pela sequência de FEN; uma tentativa
ilegal pela permanência visual das peças, ausência de novo `state` e continuação
legal; xeque-mate por `check=e1`, resultado `0-1` e motivo `checkmate`; en
passant e roque pelas casas finais; promoção pela dama em b8; e a jogada da IA
pelo `bestmove` e pelo estado subsequente. A resposta da IA é confrontada com o
FEN resultante e com a peça/turno de origem; a lista completa de lances legais
não é registrada pela interface.

O histórico gráfico não é submetido a OCR. O teste de dois lances verifica a
ordem das posições transmitidas pelo motor e uma alteração na região do painel,
mas não afirma que o texto renderizado do histórico corresponde aos lances.
Verificações de menu, configuração e destaques continuam dependendo de cores e
coordenadas lógicas do layout atual; mudanças visuais podem exigir atualização
dos pontos de amostragem. O matcher por sprites também pode reportar incerteza
se animações/alertas encobrirem uma peça.

A suíte não cobre funcionalidades ausentes (desfazer, arrastar-e-soltar,
multiplayer online, PGN ou áudio). Os dez cenários compartilham desktop, mouse e
teclado, portanto devem ser executados serialmente, como faz o comando
`run_tests.py`; não habilite execução paralela.
