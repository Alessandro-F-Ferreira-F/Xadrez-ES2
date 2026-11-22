from __future__ import annotations

import re
from dataclasses import dataclass

import pytest

from desktop_automation import (
    BoardVisionUncertain,
    DesktopAutomationTimeout,
    GameSession,
    ScreenCapture,
    changed_pixels,
    color_matches,
    is_game_board,
    is_main_menu,
    is_setup_screen,
    piece_at,
)

INITIAL_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
STATE_PATTERN = re.compile(
    r"^state check (\S+) result (\S+) reason (\S+) fen (.+)$"
)


@dataclass(frozen=True)
class BoardPosition:
    pieces: dict[str, str]
    turn: str


@dataclass(frozen=True)
class EngineState:
    raw: str
    check_square: str
    result: str
    reason: str
    fen: str
    position: BoardPosition


def _position_from_fen(fen: str) -> BoardPosition:
    fields = fen.split()
    if len(fields) != 6:
        raise ValueError(f"FEN com quantidade incorreta de campos: {fen}")

    pieces: dict[str, str] = {}
    ranks = fields[0].split("/")
    if len(ranks) != 8:
        raise ValueError(f"FEN deve conter oito fileiras: {fen}")
    for row, rank in enumerate(ranks):
        column = 0
        for token in rank:
            if token.isdigit():
                column += int(token)
            elif token in "pnbrqkPNBRQK" and column < 8:
                pieces[f"{chr(ord('a') + column)}{8 - row}"] = token
                column += 1
            else:
                raise ValueError(f"FEN contém peça ou fileira inválida: {fen}")
        if column != 8:
            raise ValueError(f"FEN contém fileira incompleta: {fen}")
    if fields[1] not in ("w", "b"):
        raise ValueError(f"FEN contém turno inválido: {fen}")
    return BoardPosition(pieces, fields[1])


def _parse_engine_state(raw: str) -> EngineState:
    match = STATE_PATTERN.fullmatch(raw)
    if match is None:
        raise ValueError(f"Resposta state inesperada no log: {raw}")
    check_square, result, reason, fen = match.groups()
    return EngineState(
        raw=raw,
        check_square=check_square,
        result=result,
        reason=reason,
        fen=fen,
        position=_position_from_fen(fen),
    )


def _engine_states(game: GameSession) -> list[EngineState]:
    return [
        _parse_engine_state(line)
        for line in game.logged_uci_responses()
        if line.startswith("state ")
    ]


def _bestmove_lines(game: GameSession) -> list[str]:
    return [
        line
        for line in game.logged_uci_responses()
        if line.startswith("bestmove ")
    ]


def _wait_for_engine_state(
    game: GameSession,
    previous_count: int,
    description: str,
    timeout: float = 4.0,
) -> EngineState:
    found: list[EngineState] = []

    def received(_: ScreenCapture) -> bool:
        states = _engine_states(game)
        if len(states) <= previous_count:
            return False
        found.append(states[previous_count])
        return True

    game.wait_for(description, received, timeout=timeout, interval=0.1)
    state = found[-1]
    game.record_protocol_evidence(state.raw)
    return state


def _wait_for_bestmove(
    game: GameSession, previous_count: int, timeout: float = 10.0
) -> str:
    found: list[str] = []

    def received(_: ScreenCapture) -> bool:
        moves = _bestmove_lines(game)
        if len(moves) <= previous_count:
            return False
        found.append(moves[previous_count])
        return True

    game.wait_for(
        "resposta bestmove do motor",
        received,
        timeout=timeout,
        interval=0.15,
    )
    fields = found[-1].split()
    if len(fields) != 2 or not re.fullmatch(r"[a-h][1-8][a-h][1-8][qrbn]?", fields[1]):
        raise AssertionError(f"Resposta bestmove inesperada: {found[-1]}")
    game.record_protocol_evidence(found[-1])
    game.record_observation(f"O motor respondeu com bestmove {fields[1]}.")
    return fields[1]


def _square_name(point: tuple[float, float]) -> str:
    column, row = int(point[0] // 100), int(point[1] // 100)
    if not (0 <= column < 8 and 0 <= row < 8):
        raise ValueError(f"Coordenada fora do tabuleiro: {point}")
    return f"{chr(ord('a') + column)}{8 - row}"


def _grid_square(name: str) -> tuple[int, int]:
    return ord(name[0]) - ord("a"), 8 - int(name[1])


def _position_after_move(position: BoardPosition, move: str) -> BoardPosition:
    if len(move) not in (4, 5):
        raise AssertionError(f"Lance UCI malformado: {move}")
    source, destination = move[:2], move[2:4]
    source_column = _grid_square(source)[0]
    target_column = _grid_square(destination)[0]
    pieces = dict(position.pieces)
    moving_piece = pieces.pop(source, None)
    if moving_piece is None:
        raise AssertionError(f"Não há peça em {source} antes de {move}.")

    if (
        moving_piece.lower() == "p"
        and source_column != target_column
        and destination not in pieces
    ):
        pieces.pop(f"{destination[0]}{source[1]}", None)

    if moving_piece.lower() == "k" and abs(target_column - source_column) == 2:
        if target_column > source_column:
            rook_source = f"h{source[1]}"
            rook_destination = f"f{source[1]}"
        else:
            rook_source = f"a{source[1]}"
            rook_destination = f"d{source[1]}"
        rook = pieces.pop(rook_source, None)
        if rook is None:
            raise AssertionError(f"Roque {move} sem torre em {rook_source}.")
        pieces[rook_destination] = rook

    if len(move) == 5:
        promoted = move[4].lower()
        if moving_piece.lower() != "p" or promoted not in "qrbn":
            raise AssertionError(f"Promoção UCI inválida: {move}")
        moving_piece = promoted.upper() if moving_piece.isupper() else promoted
    pieces[destination] = moving_piece
    return BoardPosition(pieces, "b" if position.turn == "w" else "w")


def _assert_move_state(
    game: GameSession,
    before: BoardPosition,
    state: EngineState,
    move: str,
) -> None:
    expected = _position_after_move(before, move)
    if state.position != expected:
        changed = sorted(
            square
            for square in set(expected.pieces) | set(state.position.pieces)
            if expected.pieces.get(square) != state.position.pieces.get(square)
        )
        details = {
            square: (expected.pieces.get(square), state.position.pieces.get(square))
            for square in changed
        }
        raise AssertionError(
            f"FEN do motor não corresponde ao lance {move}: "
            f"esperado={details}, FEN={state.fen}"
        )
    game.record_observation(
        f"Resposta state do motor confirma {move}; turno seguinte="
        f"{state.position.turn}, resultado={state.result}, motivo={state.reason}."
    )


def _assert_visual_squares(
    capture: ScreenCapture, expected: dict[str, str | None]
) -> None:
    observed = {
        square: piece_at(capture, *_grid_square(square))
        for square in expected
    }
    if observed != expected:
        raise AssertionError(
            f"Peças visíveis divergentes: esperado={expected}, observado={observed}."
        )


def _wait_for_visual_squares(
    game: GameSession,
    expected: dict[str, str | None],
    description: str,
    timeout: float = 4.0,
) -> ScreenCapture:
    last_observation: list[dict[str, str | None]] = []

    def matches(capture: ScreenCapture) -> bool:
        try:
            observed = {
                square: piece_at(capture, *_grid_square(square))
                for square in expected
            }
        except BoardVisionUncertain:
            return False
        last_observation[:] = [observed]
        return observed == expected

    try:
        capture = game.wait_for(
            description, matches, timeout=timeout, interval=0.1
        )
    except DesktopAutomationTimeout as exc:
        raise DesktopAutomationTimeout(
            f"{exc} Esperado={expected}; última classificação="
            f"{last_observation[-1] if last_observation else 'indisponível'}."
        ) from exc
    if len(expected) == 64:
        game.record_observation("As 64 casas visíveis correspondem à posição FEN esperada.")
    else:
        game.record_observation(f"Verificação visual de peças: {expected}.")
    return capture


def _wait_for_visual_position(
    game: GameSession, position: BoardPosition, description: str
) -> ScreenCapture:
    expected = {
        f"{chr(ord('a') + column)}{8 - row}": position.pieces.get(
            f"{chr(ord('a') + column)}{8 - row}"
        )
        for row in range(8)
        for column in range(8)
    }
    capture = _wait_for_visual_squares(
        game, expected, description, timeout=5.0
    )
    return capture


def _click_move(
    game: GameSession,
    source: tuple[float, float],
    destination: tuple[float, float],
    evidence_name: str,
) -> ScreenCapture:
    previous_states = _engine_states(game)
    before_position = (
        previous_states[-1].position
        if previous_states
        else _position_from_fen(INITIAL_FEN)
    )
    source_name = _square_name(source)
    destination_name = _square_name(destination)
    moving_piece = before_position.pieces.get(source_name)
    if moving_piece is None:
        raise AssertionError(f"A casa de origem {source_name} está vazia.")
    move = source_name + destination_name
    if moving_piece.lower() == "p" and destination_name[1] in ("1", "8"):
        move += "q"

    before = game.capture()
    game.click(*source)
    game.move_to_safe_area()
    source_col, source_row = int(source[0] // 100), int(source[1] // 100)
    target_col, target_row = int(destination[0] // 100), int(destination[1] // 100)
    affected_squares = (
        min(source_col, target_col) * 100,
        min(source_row, target_row) * 100,
        (max(source_col, target_col) + 1) * 100,
        (max(source_row, target_row) + 1) * 100,
    )
    selected = game.wait_for(
        "seleção da peça",
        lambda image: changed_pixels(before, image, affected_squares) > 100,
        timeout=3.0,
    )
    _assert_visual_squares(selected, {source_name: moving_piece})
    game.click(*destination)
    state = _wait_for_engine_state(
        game,
        len(previous_states),
        f"resposta state após o lance {move}",
    )
    _assert_move_state(game, before_position, state, move)
    visual_position = {
        source_name: None,
        destination_name: state.position.pieces.get(destination_name),
    }
    capture = _wait_for_visual_squares(
        game, visual_position, f"posição visual após {move}", timeout=6.0
    )
    game.save_capture(evidence_name, capture)
    return capture


def _open_setup(game: GameSession, name: str = "01-configuracao") -> ScreenCapture:
    game.click(560, 404)
    setup = game.wait_for(
        "tela de configuração da partida",
        is_setup_screen,
        timeout=3.0,
    )
    game.move_to_safe_area()
    game.save_capture(name, setup)
    return setup


def _start_local_game(game: GameSession) -> ScreenCapture:
    _open_setup(game)
    game.click(715, 198)
    local_mode = game.wait_for(
        "seleção visual do modo local",
        lambda image: color_matches(image, (587, 198), (46, 125, 75), 28),
        timeout=2.0,
    )
    game.move_to_safe_area()
    game.save_capture("02-modo-local-selecionado", local_mode)
    game.click(715, 633)
    game.wait_for("tabuleiro da partida", is_game_board, timeout=5.0)
    game.move_to_safe_area()
    initial = _wait_for_visual_position(
        game, _position_from_fen(INITIAL_FEN), "posição inicial completa"
    )
    game.save_capture("03-partida-local-iniciada", initial)
    return initial


@pytest.mark.acceptance(
    description="Abrir o menu, acionar Sair e confirmar o encerramento limpo do processo."
)
def test_menu_opens_and_exit_button_closes_game(game: GameSession) -> None:
    """O botão Sair encerra o jogo a partir do menu inicial."""
    assert game.last_capture is not None
    assert is_main_menu(game.last_capture), "A tela inicial não foi reconhecida."
    game.click(560, 478)
    exit_code = game.wait_for_exit(timeout=5.0)
    assert exit_code == 0, f"O processo encerrou com código {exit_code}."
    game.record_observation("A janela foi encerrada pelo botão Sair; processo retornou código 0.")
    game.record_limitation(
        "Este fluxo não solicita estado ao motor; a verificação é de navegação/encerramento."
    )


@pytest.mark.acceptance(
    description=(
        "Abrir a configuração, selecionar dificuldade, cor e relógio, "
        "e retornar ao menu usando Escape."
    )
)
def test_ai_configuration_and_escape_navigation(game: GameSession) -> None:
    """As opções de IA alteram o destaque visual e Escape volta ao menu."""
    _open_setup(game)
    game.click(354, 296)
    game.move_to_safe_area()
    easy_selected = game.wait_for(
        "dificuldade fácil selecionada",
        lambda image: color_matches(image, (276, 296), (46, 125, 75), 28),
        timeout=2.0,
    )
    game.save_capture("02-dificuldade-facil-selecionada", easy_selected)
    game.click(715, 392)
    game.move_to_safe_area()
    black_selected = game.wait_for(
        "cor preta selecionada",
        lambda image: color_matches(image, (600, 392), (46, 125, 75), 28),
        timeout=2.0,
    )
    game.save_capture("03-cor-preta-selecionada", black_selected)
    game.click(483, 493)
    game.move_to_safe_area()
    selected = game.wait_for(
        "opções selecionadas na configuração",
        lambda image: color_matches(image, (276, 296), (46, 125, 75), 28)
        and color_matches(image, (600, 392), (46, 125, 75), 28)
        and color_matches(image, (430, 493), (46, 125, 75), 28),
        timeout=3.0,
    )
    game.save_capture("04-ia-facil-pretas-5-minutos", selected)

    game.press("esc")
    main_menu = game.wait_for("retorno ao menu inicial", is_main_menu, timeout=3.0)
    game.save_capture("05-retorno-ao-menu", main_menu)
    game.record_observation(
        "Dificuldade Fácil, cor preta e opção de 5 minutos foram destacadas; Escape retornou ao menu."
    )
    game.record_limitation(
        "O cenário verifica a seleção visual, mas não inicia a partida nem observa a configuração aplicada ao motor."
    )


@pytest.mark.acceptance(
    description=(
        "Selecionar o modo local, iniciar uma partida, observar lances legais "
        "destacados, mover e2-e4 e e7-e5 e confirmar a alteração do tabuleiro e histórico."
    )
)
def test_local_game_moves_highlights_and_history(game: GameSession) -> None:
    """Dois lances pela interface atualizam a posição e a região do histórico."""
    initial = _start_local_game(game)
    initial_state_count = len(_engine_states(game))
    assert color_matches(initial, (350, 450), (181, 136, 99), 24), (
        "A área d4 não tem a cor esperada do tabuleiro em jogo."
    )

    game.click(450, 650)
    selection = game.wait_for(
        "destinos legais de e2",
        lambda image: color_matches(image, (450, 450), (172, 203, 113), 32),
        timeout=3.0,
    )
    game.save_capture("04-e2-selecionado-com-destinos", selection)

    previous_states = len(_engine_states(game))
    game.click(450, 450)
    white_state = _wait_for_engine_state(
        game, previous_states, "posição do motor após e2-e4"
    )
    _assert_move_state(
        game, _position_from_fen(INITIAL_FEN), white_state, "e2e4"
    )
    after_white = _wait_for_visual_squares(
        game,
        {"e2": None, "e4": "P"},
        "peão branco em e4 e casa e2 vazia",
    )
    game.save_capture("05-lance-branco-e2-e4", after_white)
    assert changed_pixels(initial, after_white, (400, 400, 500, 500)) > 250, (
        "O lance e2-e4 não alterou visualmente sua casa de destino."
    )

    after_black = _click_move(
        game, (450, 150), (450, 350), "06-lance-preto-e7-e5"
    )
    assert changed_pixels(after_white, after_black, (400, 300, 500, 400)) > 250, (
        "O lance e7-e5 não alterou visualmente sua casa de destino."
    )
    assert changed_pixels(initial, after_black, (830, 220, 1100, 270)) > 20, (
        "O histórico de lances não apresentou uma alteração visível."
    )
    states = _engine_states(game)[initial_state_count:]
    assert len(states) == 2, (
        f"Esperadas duas respostas state após os lances, recebidas {len(states)}."
    )
    game.record_observation(
        "As posições FEN consecutivas confirmaram e2e4 e e7e5; "
        "a região do painel de histórico também mudou."
    )
    game.record_limitation(
        "O texto UCI e a ordem renderizada no painel de histórico não são extraídos por OCR; "
        "a ordem dos lances é verificada pela sequência de posições FEN do motor."
    )


@pytest.mark.acceptance(
    description=(
        "Configurar uma partida contra a IA, executar e2-e4 e observar "
        "o motor responder com uma jogada no tabuleiro."
    )
)
def test_game_against_ai_produces_a_visible_reply(game: GameSession) -> None:
    """A resposta bestmove da IA atualiza legalmente a posição visível."""
    _open_setup(game)
    game.click(354, 296)
    game.move_to_safe_area()
    selected = game.wait_for(
        "dificuldade fácil selecionada",
        lambda image: color_matches(image, (276, 296), (46, 125, 75), 28),
        timeout=2.0,
    )
    game.save_capture("02-dificuldade-facil", selected)
    game.click(715, 633)
    game.wait_for(
        "início de partida contra IA", is_game_board, timeout=5.0
    )
    game.move_to_safe_area()
    initial_board = _wait_for_visual_position(
        game, _position_from_fen(INITIAL_FEN), "posição inicial contra a IA"
    )
    game.save_capture("03-partida-contra-ia", initial_board)

    game.click(450, 650)
    selection = game.wait_for(
        "destino legal e4",
        lambda image: color_matches(image, (450, 450), (172, 203, 113), 32),
        timeout=3.0,
    )
    _assert_visual_squares(selection, {"e2": "P"})
    previous_states = len(_engine_states(game))
    previous_bestmoves = len(_bestmove_lines(game))
    game.click(450, 450)
    human_state = _wait_for_engine_state(
        game, previous_states, "posição do motor após e2-e4"
    )
    _assert_move_state(
        game, _position_from_fen(INITIAL_FEN), human_state, "e2e4"
    )
    ai_previous_states = len(_engine_states(game))
    after_human = _wait_for_visual_squares(
        game,
        {"e2": None, "e4": "P"},
        "posição visual após o lance humano",
    )
    game.save_capture("04-jogador-joga-e2-e4", after_human)
    bestmove = _wait_for_bestmove(game, previous_bestmoves)
    source_piece = human_state.position.pieces.get(bestmove[:2])
    assert human_state.position.turn == "b" and source_piece is not None, (
        f"bestmove {bestmove} não parte de uma peça no turno preto."
    )
    assert source_piece.islower(), (
        f"bestmove {bestmove} parte de uma peça branca."
    )
    ai_state = _wait_for_engine_state(
        game,
        ai_previous_states,
        "posição do motor após a resposta da IA",
    )
    _assert_move_state(game, human_state.position, ai_state, bestmove)
    assert ai_state.position.turn == "w" and ai_state.result == "*", (
        f"A resposta da IA não deixou o turno/partida esperados: {ai_state.raw}"
    )
    reply_capture = _wait_for_visual_squares(
        game,
        {
            bestmove[:2]: None,
            bestmove[2:4]: ai_state.position.pieces.get(bestmove[2:4]),
        },
        f"resposta visual da IA {bestmove}",
    )
    game.save_capture("05-resposta-da-ia", reply_capture)
    game.record_limitation(
        "A lista legal completa não é registrada no log da interface; a resposta bestmove "
        "é validada contra a transição FEN subsequente e a peça/turno de origem."
    )


@pytest.mark.acceptance(
    description=(
        "Rejeitar e2-e5 sem alterar as casas/turno, aceitar e2-e4 e então "
        "confirmar desistência e retorno ao menu."
    )
)
def test_invalid_move_resign_and_return_to_menu(game: GameSession) -> None:
    """Lance ilegal mantém as casas e o turno antes de aceitar lance legal."""
    _start_local_game(game)
    previous_states = len(_engine_states(game))
    game.click(450, 650)
    game.move_to_safe_area()
    game.click(450, 350)
    invalid = game.wait_for(
        "alerta visual de movimento inválido",
        lambda image: color_matches(image, (200, 35), (52, 36, 37), 48)
        and color_matches(image, (450, 350), (215, 110, 59), 36),
        timeout=3.0,
    )
    game.save_capture("04-lance-ilegal-e2-e5", invalid)
    _assert_visual_squares(invalid, {"e2": "P", "e5": None})
    assert len(_engine_states(game)) == previous_states, (
        "A tentativa ilegal foi seguida por nova resposta state do motor."
    )
    game.record_observation(
        "Após e2-e5, e2 continuou com o peão, e5 permaneceu vazia e nenhuma nova resposta state foi registrada."
    )
    game.record_limitation(
        "A interface não consulta o motor para uma seleção ilegal; a rejeição/turno são "
        "confirmados visualmente e pela posição da continuação legal e2-e4."
    )

    game.click(593, 35)
    dismissed = game.wait_for(
        "fechamento do alerta de movimento inválido",
        lambda image: changed_pixels(invalid, image, (0, 0, 800, 80)) > 100,
        timeout=2.0,
    )
    game.save_capture("05-alerta-ilegal-fechado", dismissed)
    try:
        selected = game.wait_for(
            "destino legal e4 após fechar o alerta",
            lambda image: color_matches(image, (450, 450), (172, 203, 113), 32),
            timeout=0.5,
        )
    except DesktopAutomationTimeout:
        game.click(450, 650)
        selected = game.wait_for(
            "seleção de e2 após lance ilegal",
            lambda image: color_matches(image, (450, 450), (172, 203, 113), 32),
            timeout=2.0,
        )
    game.save_capture("05-e2-selecionado-apos-rejeicao", selected)
    game.click(450, 450)
    state = _wait_for_engine_state(
        game, previous_states, "estado após continuação legal e2-e4"
    )
    _assert_move_state(game, _position_from_fen(INITIAL_FEN), state, "e2e4")
    moved = _wait_for_visual_squares(
        game, {"e2": None, "e4": "P"}, "continuação legal e2-e4"
    )
    game.save_capture("06-continuação-legal-e2-e4", moved)

    game.click(960, 758)
    resigned = game.wait_for(
        "fim de partida por desistência",
        lambda image: color_matches(image, (160, 370), (52, 36, 37), 48),
        timeout=3.0,
    )
    game.save_capture("05-partida-encerrada-por-desistencia", resigned)
    assert color_matches(resigned, (160, 370), (52, 36, 37), 48), (
        "A janela visual de fim de partida não apareceu após desistir."
    )

    game.click(1034, 758)
    main_menu = game.wait_for(
        "retorno ao menu principal após desistência",
        is_main_menu,
        timeout=3.0,
    )
    game.save_capture("07-retorno-ao-menu", main_menu)


@pytest.mark.acceptance(
    description=(
        "Redimensionar a janela, recalcular a área útil e confirmar que o "
        "botão Jogar ainda leva à configuração."
    )
)
def test_resized_window_keeps_menu_interactive(game: GameSession) -> None:
    """A posição do clique acompanha a área do cliente após redimensionar."""
    initial_rect = game._client_rect()
    new_height = max(560, round(initial_rect.height * 0.82))
    new_width = round(new_height * 1.4)
    game.resize_client(new_width, new_height)
    resized = game.wait_for("menu após redimensionamento", is_main_menu, timeout=3.0)
    game.save_capture("01-menu-redimensionado", resized)
    assert resized.image.size == (new_width, new_height), (
        f"Captura da janela esperava {new_width}x{new_height}, "
        f"obteve {resized.image.width}x{resized.image.height}."
    )

    game.click(560, 404)
    setup = game.wait_for(
        "configuração após redimensionamento", is_setup_screen, timeout=3.0
    )
    game.save_capture("02-configuracao-apos-redimensionamento", setup)
    game.record_observation(
        f"Área cliente redimensionada para {new_width}x{new_height}; "
        "o clique calculado pela janela abriu a configuração."
    )
    game.record_limitation(
        "A verificação cobre uma dimensão/resolução por execução, não uma matriz de escalas DPI."
    )


@pytest.mark.acceptance(
    description=(
        "Executar o Mate do Louco por cliques, confirmar o estado visual de "
        "xeque-mate e iniciar uma nova partida pelo HUD."
    )
)
def test_fools_mate_and_restart_from_postgame_controls(game: GameSession) -> None:
    """Xeque-mate encerra a partida e o controle de nova partida reinicia."""
    _start_local_game(game)
    moves = [
        ((550, 650), (550, 550), "04-f2-f3"),
        ((450, 150), (450, 350), "05-e7-e5"),
        ((650, 650), (650, 450), "06-g2-g4"),
        ((350, 50), (750, 450), "07-d8-h4-mate"),
    ]
    for source, destination, evidence_name in moves:
        _click_move(game, source, destination, evidence_name)

    checkmate = game.wait_for(
        "alerta de fim de partida após xeque-mate",
        lambda image: color_matches(image, (160, 370), (52, 36, 37), 48),
        timeout=3.0,
    )
    game.save_capture("08-xeque-mate", checkmate)
    final_state = _engine_states(game)[-1]
    assert final_state.check_square == "e1", (
        f"O motor não identificou o rei branco em xeque: {final_state.raw}"
    )
    assert (final_state.result, final_state.reason) == ("0-1", "checkmate"), (
        f"O motor não declarou xeque-mate: {final_state.raw}"
    )
    game.record_protocol_evidence(final_state.raw)
    game.record_observation(
        "O estado protocolar identificou check=e1, result=0-1 e reason=checkmate."
    )

    game.click(960, 704)
    restarted = game.wait_for(
        "tabuleiro reiniciado pelo HUD", is_game_board, timeout=5.0
    )
    restarted = _wait_for_visual_position(
        game, _position_from_fen(INITIAL_FEN), "posição inicial após reinício"
    )
    game.save_capture("09-nova-partida", restarted)


@pytest.mark.acceptance(
    description=(
        "Executar e2-e4, a7-a6, e4-e5, d7-d5 e e5xd6 en passant; "
        "verificar visualmente a remoção do peão capturado."
    )
)
def test_en_passant_through_mouse_interaction(game: GameSession) -> None:
    """A captura en passant é acessível pelo fluxo normal de cliques."""
    _start_local_game(game)
    sequence = [
        ((450, 650), (450, 450), "04-e2-e4"),
        ((50, 150), (50, 250), "05-a7-a6"),
        ((450, 450), (450, 350), "06-e4-e5"),
        ((350, 150), (350, 350), "07-d7-d5"),
    ]
    before_capture = None
    for source, destination, name in sequence:
        before_capture = _click_move(game, source, destination, name)
    assert before_capture is not None

    after_capture = _click_move(
        game, (450, 350), (350, 250), "08-captura-en-passant-e5-d6"
    )
    final_state = _engine_states(game)[-1]
    assert final_state.position.pieces.get("d6") == "P"
    assert "d5" not in final_state.position.pieces
    assert "e5" not in final_state.position.pieces
    _assert_visual_squares(
        after_capture, {"d6": "P", "d5": None, "e5": None}
    )
    assert changed_pixels(
        before_capture, after_capture, (300, 200, 400, 300)
    ) > 250, "O peão não apareceu visualmente na casa d6."
    assert changed_pixels(
        before_capture, after_capture, (300, 300, 400, 400)
    ) > 250, "O peão capturado não foi removido visualmente de d5."
    game.record_observation(
        "Após e5xd6, o FEN e os sprites confirmam peão branco em d6 e casas d5/e5 vazias."
    )


@pytest.mark.acceptance(
    description=(
        "Executar lances legais que liberam o roque curto e verificar que "
        "o rei e a torre aparecem nas casas de destino."
    )
)
def test_castling_through_mouse_interaction(game: GameSession) -> None:
    """O roque curto pode ser executado por seleção e clique no destino."""
    _start_local_game(game)
    sequence = [
        ((650, 750), (550, 550), "04-cavalo-g1-f3"),
        ((50, 150), (50, 250), "05-peao-a7-a6"),
        ((450, 650), (450, 450), "06-peao-e2-e4"),
        ((50, 250), (50, 350), "07-peao-a6-a5"),
        ((550, 750), (250, 450), "08-bispo-f1-c4"),
        ((750, 150), (750, 250), "09-peao-h7-h6"),
    ]
    for source, destination, name in sequence:
        _click_move(game, source, destination, name)

    before_castling = game.capture()
    after_castling = _click_move(
        game, (450, 750), (650, 750), "10-roque-curto"
    )
    assert changed_pixels(
        before_castling, after_castling, (600, 700, 700, 800)
    ) > 250, "O rei não apareceu visualmente em g1 após o roque."
    assert changed_pixels(
        before_castling, after_castling, (500, 700, 600, 800)
    ) > 250, "A torre não apareceu visualmente em f1 após o roque."
    final_state = _engine_states(game)[-1]
    expected = {"e1": None, "f1": "R", "g1": "K", "h1": None}
    observed = {square: final_state.position.pieces.get(square) for square in expected}
    assert observed == expected, (
        f"Posição protocolar do roque incorreta: esperado={expected}, observado={observed}."
    )
    _assert_visual_squares(after_castling, expected)
    game.record_observation(
        "O FEN e a visão confirmam rei em g1, torre em f1 e casas e1/h1 vazias."
    )


@pytest.mark.acceptance(
    description=(
        "Executar lances com o mouse até a promoção de a7xb8 e confirmar "
        "a substituição do peão por uma dama em b8."
    )
)
def test_promotion_through_mouse_interaction(game: GameSession) -> None:
    """O peão que alcança b8 é promovido automaticamente a dama."""
    _start_local_game(game)
    sequence = [
        ((50, 650), (50, 450), "04-a2-a4"),
        ((150, 150), (150, 350), "05-b7-b5"),
        ((50, 450), (150, 350), "06-a4xb5"),
        ((50, 150), (50, 250), "07-a7-a6"),
        ((150, 350), (50, 250), "08-b5xa6"),
        ((650, 50), (550, 250), "09-cavalo-g8-f6"),
        ((50, 250), (50, 150), "10-a6-a7"),
        ((250, 150), (250, 250), "11-c7-c6"),
    ]
    for source, destination, name in sequence:
        _click_move(game, source, destination, name)

    before_promotion = game.capture()
    after_promotion = _click_move(
        game, (50, 150), (150, 50), "12-promocao-a7xb8"
    )
    final_state = _engine_states(game)[-1]
    assert final_state.position.pieces.get("b8") == "Q", (
        f"A peça em b8 não é dama branca: {final_state.raw}"
    )
    assert "a7" not in final_state.position.pieces, (
        f"O peão não saiu de a7 após a promoção: {final_state.raw}"
    )
    _assert_visual_squares(after_promotion, {"a7": None, "b8": "Q"})
    assert changed_pixels(
        before_promotion, after_promotion, (100, 0, 200, 100)
    ) > 250, "A promoção não alterou visualmente a casa b8."
    game.record_observation(
        "O estado protocolar e a textura visível confirmam dama branca em b8 e a7 vazia."
    )
    game.record_limitation(
        "O teste assume a promoção automática para dama observada na implementação atual; "
        "não valida escolha entre diferentes peças."
    )
