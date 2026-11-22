# Relatório de Testes de Aceitação

- Gerado: 2026-11-22T03:40:15.387563-03:00
- Código de saída do pytest: 0
- Resumo: 10 PASS, 0 FAIL, 0 SKIP (10 cenários)

## test_menu_opens_and_exit_button_closes_game — PASS

- Esperado: Abrir o menu, acionar Sair e confirmar o encerramento limpo do processo.
- Duração: 3.068s
- Comportamento observado:
  - A janela foi encerrada pelo botão Sair; processo retornou código 0.
- Limitações da verificação:
  - Este fluxo não solicita estado ao motor; a verificação é de navegação/encerramento.
- Evidências:
  - [00-menu-inicial.png](evidence/test_menu_opens_and_exit_button_closes_game/00-menu-inicial.png)
- Log da aplicação: [test_menu_opens_and_exit_button_closes_game.log](logs/test_menu_opens_and_exit_button_closes_game.log)

## test_ai_configuration_and_escape_navigation — PASS

- Esperado: Abrir a configuração, selecionar dificuldade, cor e relógio, e retornar ao menu usando Escape.
- Duração: 2.104s
- Comportamento observado:
  - Dificuldade Fácil, cor preta e opção de 5 minutos foram destacadas; Escape retornou ao menu.
- Limitações da verificação:
  - O cenário verifica a seleção visual, mas não inicia a partida nem observa a configuração aplicada ao motor.
- Evidências:
  - [00-menu-inicial.png](evidence/test_ai_configuration_and_escape_navigation/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_ai_configuration_and_escape_navigation/01-configuracao.png)
  - [02-dificuldade-facil-selecionada.png](evidence/test_ai_configuration_and_escape_navigation/02-dificuldade-facil-selecionada.png)
  - [03-cor-preta-selecionada.png](evidence/test_ai_configuration_and_escape_navigation/03-cor-preta-selecionada.png)
  - [04-ia-facil-pretas-5-minutos.png](evidence/test_ai_configuration_and_escape_navigation/04-ia-facil-pretas-5-minutos.png)
  - [05-retorno-ao-menu.png](evidence/test_ai_configuration_and_escape_navigation/05-retorno-ao-menu.png)
- Log da aplicação: [test_ai_configuration_and_escape_navigation.log](logs/test_ai_configuration_and_escape_navigation.log)

## test_local_game_moves_highlights_and_history — PASS

- Esperado: Selecionar o modo local, iniciar uma partida, observar lances legais destacados, mover e2-e4 e e7-e5 e confirmar a alteração do tabuleiro e histórico.
- Duração: 3.048s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Resposta state do motor confirma e2e4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e2': None, 'e4': 'P'}.
  - Resposta state do motor confirma e7e5; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'e7': None, 'e5': 'p'}.
  - As posições FEN consecutivas confirmaram e2e4 e e7e5; a região do painel de histórico também mudou.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1`
  - `state check - result * reason none fen rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2`
- Limitações da verificação:
  - O texto UCI e a ordem renderizada no painel de histórico não são extraídos por OCR; a ordem dos lances é verificada pela sequência de posições FEN do motor.
- Evidências:
  - [00-menu-inicial.png](evidence/test_local_game_moves_highlights_and_history/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_local_game_moves_highlights_and_history/01-configuracao.png)
  - [02-modo-local-selecionado.png](evidence/test_local_game_moves_highlights_and_history/02-modo-local-selecionado.png)
  - [03-partida-local-iniciada.png](evidence/test_local_game_moves_highlights_and_history/03-partida-local-iniciada.png)
  - [04-e2-selecionado-com-destinos.png](evidence/test_local_game_moves_highlights_and_history/04-e2-selecionado-com-destinos.png)
  - [05-lance-branco-e2-e4.png](evidence/test_local_game_moves_highlights_and_history/05-lance-branco-e2-e4.png)
  - [06-lance-preto-e7-e5.png](evidence/test_local_game_moves_highlights_and_history/06-lance-preto-e7-e5.png)
- Log da aplicação: [test_local_game_moves_highlights_and_history.log](logs/test_local_game_moves_highlights_and_history.log)

## test_game_against_ai_produces_a_visible_reply — PASS

- Esperado: Configurar uma partida contra a IA, executar e2-e4 e observar o motor responder com uma jogada no tabuleiro.
- Duração: 3.713s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Resposta state do motor confirma e2e4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e2': None, 'e4': 'P'}.
  - O motor respondeu com bestmove b8c6.
  - Resposta state do motor confirma b8c6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'b8': None, 'c6': 'n'}.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1`
  - `bestmove b8c6`
  - `state check - result * reason none fen r1bqkbnr/pppppppp/2n5/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 1 2`
- Limitações da verificação:
  - A lista legal completa não é registrada no log da interface; a resposta bestmove é validada contra a transição FEN subsequente e a peça/turno de origem.
- Evidências:
  - [00-menu-inicial.png](evidence/test_game_against_ai_produces_a_visible_reply/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_game_against_ai_produces_a_visible_reply/01-configuracao.png)
  - [02-dificuldade-facil.png](evidence/test_game_against_ai_produces_a_visible_reply/02-dificuldade-facil.png)
  - [03-partida-contra-ia.png](evidence/test_game_against_ai_produces_a_visible_reply/03-partida-contra-ia.png)
  - [04-jogador-joga-e2-e4.png](evidence/test_game_against_ai_produces_a_visible_reply/04-jogador-joga-e2-e4.png)
  - [05-resposta-da-ia.png](evidence/test_game_against_ai_produces_a_visible_reply/05-resposta-da-ia.png)
- Log da aplicação: [test_game_against_ai_produces_a_visible_reply.log](logs/test_game_against_ai_produces_a_visible_reply.log)

## test_invalid_move_resign_and_return_to_menu — PASS

- Esperado: Rejeitar e2-e5 sem alterar as casas/turno, aceitar e2-e4 e então confirmar desistência e retorno ao menu.
- Duração: 3.244s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Após e2-e5, e2 continuou com o peão, e5 permaneceu vazia e nenhuma nova resposta state foi registrada.
  - Resposta state do motor confirma e2e4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e2': None, 'e4': 'P'}.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1`
- Limitações da verificação:
  - A interface não consulta o motor para uma seleção ilegal; a rejeição/turno são confirmados visualmente e pela posição da continuação legal e2-e4.
- Evidências:
  - [00-menu-inicial.png](evidence/test_invalid_move_resign_and_return_to_menu/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_invalid_move_resign_and_return_to_menu/01-configuracao.png)
  - [02-modo-local-selecionado.png](evidence/test_invalid_move_resign_and_return_to_menu/02-modo-local-selecionado.png)
  - [03-partida-local-iniciada.png](evidence/test_invalid_move_resign_and_return_to_menu/03-partida-local-iniciada.png)
  - [04-lance-ilegal-e2-e5.png](evidence/test_invalid_move_resign_and_return_to_menu/04-lance-ilegal-e2-e5.png)
  - [05-alerta-ilegal-fechado.png](evidence/test_invalid_move_resign_and_return_to_menu/05-alerta-ilegal-fechado.png)
  - [05-e2-selecionado-apos-rejeicao.png](evidence/test_invalid_move_resign_and_return_to_menu/05-e2-selecionado-apos-rejeicao.png)
  - [06-continua-o-legal-e2-e4.png](evidence/test_invalid_move_resign_and_return_to_menu/06-continua-o-legal-e2-e4.png)
  - [05-partida-encerrada-por-desistencia.png](evidence/test_invalid_move_resign_and_return_to_menu/05-partida-encerrada-por-desistencia.png)
  - [07-retorno-ao-menu.png](evidence/test_invalid_move_resign_and_return_to_menu/07-retorno-ao-menu.png)
- Log da aplicação: [test_invalid_move_resign_and_return_to_menu.log](logs/test_invalid_move_resign_and_return_to_menu.log)

## test_resized_window_keeps_menu_interactive — PASS

- Esperado: Redimensionar a janela, recalcular a área útil e confirmar que o botão Jogar ainda leva à configuração.
- Duração: 1.759s
- Comportamento observado:
  - Área cliente redimensionada para 784x560; o clique calculado pela janela abriu a configuração.
- Limitações da verificação:
  - A verificação cobre uma dimensão/resolução por execução, não uma matriz de escalas DPI.
- Evidências:
  - [00-menu-inicial.png](evidence/test_resized_window_keeps_menu_interactive/00-menu-inicial.png)
  - [01-menu-redimensionado.png](evidence/test_resized_window_keeps_menu_interactive/01-menu-redimensionado.png)
  - [02-configuracao-apos-redimensionamento.png](evidence/test_resized_window_keeps_menu_interactive/02-configuracao-apos-redimensionamento.png)
- Log da aplicação: [test_resized_window_keeps_menu_interactive.log](logs/test_resized_window_keeps_menu_interactive.log)

## test_fools_mate_and_restart_from_postgame_controls — PASS

- Esperado: Executar o Mate do Louco por cliques, confirmar o estado visual de xeque-mate e iniciar uma nova partida pelo HUD.
- Duração: 4.126s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Resposta state do motor confirma f2f3; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'f2': None, 'f3': 'P'}.
  - Resposta state do motor confirma e7e5; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'e7': None, 'e5': 'p'}.
  - Resposta state do motor confirma g2g4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'g2': None, 'g4': 'P'}.
  - Resposta state do motor confirma d8h4; turno seguinte=w, resultado=0-1, motivo=checkmate.
  - Verificação visual de peças: {'d8': None, 'h4': 'q'}.
  - O estado protocolar identificou check=e1, result=0-1 e reason=checkmate.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/8/5P2/PPPPP1PP/RNBQKBNR b KQkq - 0 1`
  - `state check - result * reason none fen rnbqkbnr/pppp1ppp/8/4p3/8/5P2/PPPPP1PP/RNBQKBNR w KQkq e6 0 2`
  - `state check - result * reason none fen rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2`
  - `state check e1 result 0-1 reason checkmate fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3`
- Evidências:
  - [00-menu-inicial.png](evidence/test_fools_mate_and_restart_from_postgame_controls/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_fools_mate_and_restart_from_postgame_controls/01-configuracao.png)
  - [02-modo-local-selecionado.png](evidence/test_fools_mate_and_restart_from_postgame_controls/02-modo-local-selecionado.png)
  - [03-partida-local-iniciada.png](evidence/test_fools_mate_and_restart_from_postgame_controls/03-partida-local-iniciada.png)
  - [04-f2-f3.png](evidence/test_fools_mate_and_restart_from_postgame_controls/04-f2-f3.png)
  - [05-e7-e5.png](evidence/test_fools_mate_and_restart_from_postgame_controls/05-e7-e5.png)
  - [06-g2-g4.png](evidence/test_fools_mate_and_restart_from_postgame_controls/06-g2-g4.png)
  - [07-d8-h4-mate.png](evidence/test_fools_mate_and_restart_from_postgame_controls/07-d8-h4-mate.png)
  - [08-xeque-mate.png](evidence/test_fools_mate_and_restart_from_postgame_controls/08-xeque-mate.png)
  - [09-nova-partida.png](evidence/test_fools_mate_and_restart_from_postgame_controls/09-nova-partida.png)
- Log da aplicação: [test_fools_mate_and_restart_from_postgame_controls.log](logs/test_fools_mate_and_restart_from_postgame_controls.log)

## test_en_passant_through_mouse_interaction — PASS

- Esperado: Executar e2-e4, a7-a6, e4-e5, d7-d5 e e5xd6 en passant; verificar visualmente a remoção do peão capturado.
- Duração: 4.445s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Resposta state do motor confirma e2e4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e2': None, 'e4': 'P'}.
  - Resposta state do motor confirma a7a6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'a7': None, 'a6': 'p'}.
  - Resposta state do motor confirma e4e5; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e4': None, 'e5': 'P'}.
  - Resposta state do motor confirma d7d5; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'d7': None, 'd5': 'p'}.
  - Resposta state do motor confirma e5d6; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e5': None, 'd6': 'P'}.
  - Após e5xd6, o FEN e os sprites confirmam peão branco em d6 e casas d5/e5 vazias.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1`
  - `state check - result * reason none fen rnbqkbnr/1ppppppp/p7/8/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2`
  - `state check - result * reason none fen rnbqkbnr/1ppppppp/p7/4P3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2`
  - `state check - result * reason none fen rnbqkbnr/1pp1pppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3`
  - `state check - result * reason none fen rnbqkbnr/1pp1pppp/p2P4/8/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3`
- Evidências:
  - [00-menu-inicial.png](evidence/test_en_passant_through_mouse_interaction/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_en_passant_through_mouse_interaction/01-configuracao.png)
  - [02-modo-local-selecionado.png](evidence/test_en_passant_through_mouse_interaction/02-modo-local-selecionado.png)
  - [03-partida-local-iniciada.png](evidence/test_en_passant_through_mouse_interaction/03-partida-local-iniciada.png)
  - [04-e2-e4.png](evidence/test_en_passant_through_mouse_interaction/04-e2-e4.png)
  - [05-a7-a6.png](evidence/test_en_passant_through_mouse_interaction/05-a7-a6.png)
  - [06-e4-e5.png](evidence/test_en_passant_through_mouse_interaction/06-e4-e5.png)
  - [07-d7-d5.png](evidence/test_en_passant_through_mouse_interaction/07-d7-d5.png)
  - [08-captura-en-passant-e5-d6.png](evidence/test_en_passant_through_mouse_interaction/08-captura-en-passant-e5-d6.png)
- Log da aplicação: [test_en_passant_through_mouse_interaction.log](logs/test_en_passant_through_mouse_interaction.log)

## test_castling_through_mouse_interaction — PASS

- Esperado: Executar lances legais que liberam o roque curto e verificar que o rei e a torre aparecem nas casas de destino.
- Duração: 5.268s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Resposta state do motor confirma g1f3; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'g1': None, 'f3': 'N'}.
  - Resposta state do motor confirma a7a6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'a7': None, 'a6': 'p'}.
  - Resposta state do motor confirma e2e4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e2': None, 'e4': 'P'}.
  - Resposta state do motor confirma a6a5; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'a6': None, 'a5': 'p'}.
  - Resposta state do motor confirma f1c4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'f1': None, 'c4': 'B'}.
  - Resposta state do motor confirma h7h6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'h7': None, 'h6': 'p'}.
  - Resposta state do motor confirma e1g1; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'e1': None, 'g1': 'K'}.
  - O FEN e a visão confirmam rei em g1, torre em f1 e casas e1/h1 vazias.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1`
  - `state check - result * reason none fen rnbqkbnr/1ppppppp/p7/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 2`
  - `state check - result * reason none fen rnbqkbnr/1ppppppp/p7/8/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq e3 0 2`
  - `state check - result * reason none fen rnbqkbnr/1ppppppp/8/p7/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 0 3`
  - `state check - result * reason none fen rnbqkbnr/1ppppppp/8/p7/2B1P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 1 3`
  - `state check - result * reason none fen rnbqkbnr/1pppppp1/7p/p7/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 4`
  - `state check - result * reason none fen rnbqkbnr/1pppppp1/7p/p7/2B1P3/5N2/PPPP1PPP/RNBQ1RK1 b kq - 1 4`
- Evidências:
  - [00-menu-inicial.png](evidence/test_castling_through_mouse_interaction/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_castling_through_mouse_interaction/01-configuracao.png)
  - [02-modo-local-selecionado.png](evidence/test_castling_through_mouse_interaction/02-modo-local-selecionado.png)
  - [03-partida-local-iniciada.png](evidence/test_castling_through_mouse_interaction/03-partida-local-iniciada.png)
  - [04-cavalo-g1-f3.png](evidence/test_castling_through_mouse_interaction/04-cavalo-g1-f3.png)
  - [05-peao-a7-a6.png](evidence/test_castling_through_mouse_interaction/05-peao-a7-a6.png)
  - [06-peao-e2-e4.png](evidence/test_castling_through_mouse_interaction/06-peao-e2-e4.png)
  - [07-peao-a6-a5.png](evidence/test_castling_through_mouse_interaction/07-peao-a6-a5.png)
  - [08-bispo-f1-c4.png](evidence/test_castling_through_mouse_interaction/08-bispo-f1-c4.png)
  - [09-peao-h7-h6.png](evidence/test_castling_through_mouse_interaction/09-peao-h7-h6.png)
  - [10-roque-curto.png](evidence/test_castling_through_mouse_interaction/10-roque-curto.png)
- Log da aplicação: [test_castling_through_mouse_interaction.log](logs/test_castling_through_mouse_interaction.log)

## test_promotion_through_mouse_interaction — PASS

- Esperado: Executar lances com o mouse até a promoção de a7xb8 e confirmar a substituição do peão por uma dama em b8.
- Duração: 6.408s
- Comportamento observado:
  - As 64 casas visíveis correspondem à posição FEN esperada.
  - Resposta state do motor confirma a2a4; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'a2': None, 'a4': 'P'}.
  - Resposta state do motor confirma b7b5; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'b7': None, 'b5': 'p'}.
  - Resposta state do motor confirma a4b5; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'a4': None, 'b5': 'P'}.
  - Resposta state do motor confirma a7a6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'a7': None, 'a6': 'p'}.
  - Resposta state do motor confirma b5a6; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'b5': None, 'a6': 'P'}.
  - Resposta state do motor confirma g8f6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'g8': None, 'f6': 'n'}.
  - Resposta state do motor confirma a6a7; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'a6': None, 'a7': 'P'}.
  - Resposta state do motor confirma c7c6; turno seguinte=w, resultado=*, motivo=none.
  - Verificação visual de peças: {'c7': None, 'c6': 'p'}.
  - Resposta state do motor confirma a7b8q; turno seguinte=b, resultado=*, motivo=none.
  - Verificação visual de peças: {'a7': None, 'b8': 'Q'}.
  - O estado protocolar e a textura visível confirmam dama branca em b8 e a7 vazia.
- Evidência protocolar (respostas já registradas pela interface):
  - `state check - result * reason none fen rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq a3 0 1`
  - `state check - result * reason none fen rnbqkbnr/p1pppppp/8/1p6/P7/8/1PPPPPPP/RNBQKBNR w KQkq b6 0 2`
  - `state check - result * reason none fen rnbqkbnr/p1pppppp/8/1P6/8/8/1PPPPPPP/RNBQKBNR b KQkq - 0 2`
  - `state check - result * reason none fen rnbqkbnr/2pppppp/p7/1P6/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 3`
  - `state check - result * reason none fen rnbqkbnr/2pppppp/P7/8/8/8/1PPPPPPP/RNBQKBNR b KQkq - 0 3`
  - `state check - result * reason none fen rnbqkb1r/2pppppp/P4n2/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 1 4`
  - `state check - result * reason none fen rnbqkb1r/P1pppppp/5n2/8/8/8/1PPPPPPP/RNBQKBNR b KQkq - 0 4`
  - `state check - result * reason none fen rnbqkb1r/P2ppppp/2p2n2/8/8/8/1PPPPPPP/RNBQKBNR w KQkq - 0 5`
  - `state check - result * reason none fen rQbqkb1r/3ppppp/2p2n2/8/8/8/1PPPPPPP/RNBQKBNR b KQkq - 0 5`
- Limitações da verificação:
  - O teste assume a promoção automática para dama observada na implementação atual; não valida escolha entre diferentes peças.
- Evidências:
  - [00-menu-inicial.png](evidence/test_promotion_through_mouse_interaction/00-menu-inicial.png)
  - [01-configuracao.png](evidence/test_promotion_through_mouse_interaction/01-configuracao.png)
  - [02-modo-local-selecionado.png](evidence/test_promotion_through_mouse_interaction/02-modo-local-selecionado.png)
  - [03-partida-local-iniciada.png](evidence/test_promotion_through_mouse_interaction/03-partida-local-iniciada.png)
  - [04-a2-a4.png](evidence/test_promotion_through_mouse_interaction/04-a2-a4.png)
  - [05-b7-b5.png](evidence/test_promotion_through_mouse_interaction/05-b7-b5.png)
  - [06-a4xb5.png](evidence/test_promotion_through_mouse_interaction/06-a4xb5.png)
  - [07-a7-a6.png](evidence/test_promotion_through_mouse_interaction/07-a7-a6.png)
  - [08-b5xa6.png](evidence/test_promotion_through_mouse_interaction/08-b5xa6.png)
  - [09-cavalo-g8-f6.png](evidence/test_promotion_through_mouse_interaction/09-cavalo-g8-f6.png)
  - [10-a6-a7.png](evidence/test_promotion_through_mouse_interaction/10-a6-a7.png)
  - [11-c7-c6.png](evidence/test_promotion_through_mouse_interaction/11-c7-c6.png)
  - [12-promocao-a7xb8.png](evidence/test_promotion_through_mouse_interaction/12-promocao-a7xb8.png)
- Log da aplicação: [test_promotion_through_mouse_interaction.log](logs/test_promotion_through_mouse_interaction.log)
