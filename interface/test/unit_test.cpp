#include "board.hpp"
#include "engine_bridge.hpp"
#include "piece.hpp"
#include "../../tools/ctest/test_api.h"

#include <iostream>
#include <string>

#define CHECK(cond, msg) ASSERT_MSG((cond), "%s", (msg))

// 1. Teste de Unidade: Classe Piece
TEST(piece_unit) {
    sf::Texture tex;
    tex.create(100, 100);

    Piece p(PieceType::KNIGHT, PieceColor::WHITE, tex, 1, 7, 100.f);
    CHECK(p.getType() == PieceType::KNIGHT, "Tipo inicial deve ser KNIGHT");
    CHECK(p.getColor() == PieceColor::WHITE, "Cor inicial deve ser WHITE");
    CHECK(p.getCol() == 1, "Coluna inicial deve ser 1");
    CHECK(p.getRow() == 7, "Linha inicial deve ser 7");

    p.setGridPosition(2, 5, 100.f);
    CHECK(p.getCol() == 2, "Coluna atualizada deve ser 2");
    CHECK(p.getRow() == 5, "Linha atualizada deve ser 5");
}

// 2. Teste de Unidade (Caixa Branca / Conjunto Basico): parseSquare e squareToCoord
// V(G) = 6 caminhos independentes + fronteiras
TEST(parse_square_basis_paths) {
    ChessBoard board(800.f);
    int col = -1, row = -1;

    // Caminho 1: tamanho != 2
    CHECK(!board.parseSquare("", col, row), "String vazia deve falhar");
    CHECK(!board.parseSquare("e", col, row), "String de 1 char deve falhar");
    CHECK(!board.parseSquare("e2e4", col, row), "String de 4 chars deve falhar");

    // Caminho 2: fileChar < 'a'
    CHECK(!board.parseSquare("`1", col, row), "Coluna antes de 'a' deve falhar");

    // Caminho 3: fileChar > 'h'
    CHECK(!board.parseSquare("i1", col, row), "Coluna depois de 'h' deve falhar");

    // Caminho 4: rankChar < '1'
    CHECK(!board.parseSquare("a0", col, row), "Linha antes de '1' deve falhar");

    // Caminho 5: rankChar > '8'
    CHECK(!board.parseSquare("a9", col, row), "Linha depois de '8' deve falhar");

    // Caminho 6: fronteiras validas ('a1', 'h8', 'e4')
    CHECK(board.parseSquare("a1", col, row) && col == 0 && row == 7, "a1 -> (0, 7)");
    CHECK(board.parseSquare("h8", col, row) && col == 7 && row == 0, "h8 -> (7, 0)");
    CHECK(board.parseSquare("e4", col, row) && col == 4 && row == 4, "e4 -> (4, 4)");

    // squareToCoord: validos e fronteiras invalidas
    CHECK(board.squareToCoord(0, 7) == "a1", "(0, 7) -> a1");
    CHECK(board.squareToCoord(7, 0) == "h8", "(7, 0) -> h8");
    CHECK(board.squareToCoord(-1, 0).empty(), "col < 0 -> vazio");
    CHECK(board.squareToCoord(8, 0).empty(), "col > 7 -> vazio");
    CHECK(board.squareToCoord(0, -1).empty(), "row < 0 -> vazio");
    CHECK(board.squareToCoord(0, 8).empty(), "row > 7 -> vazio");
}

// 3. Teste de Unidade (Fronteiras): pixelToSquare
TEST(pixel_to_square_boundaries) {
    ChessBoard board(800.f);
    int col = -1, row = -1;

    CHECK(board.getSquareSize() == 100.f, "squareSize deve ser 100");
    CHECK(!board.pixelToSquare(-0.1f, 50.f, col, row), "x < 0 deve falhar");
    CHECK(!board.pixelToSquare(50.f, -1.f, col, row), "y < 0 deve falhar");
    CHECK(!board.pixelToSquare(800.f, 50.f, col, row), "x == 800 deve falhar");
    CHECK(!board.pixelToSquare(50.f, 800.f, col, row), "y == 800 deve falhar");

    CHECK(board.pixelToSquare(0.f, 0.f, col, row) && col == 0 && row == 0, "(0,0) -> (0,0)");
    CHECK(board.pixelToSquare(799.9f, 799.9f, col, row) && col == 7 && row == 7, "(799.9,799.9) -> (7,7)");
    CHECK(board.pixelToSquare(450.f, 650.f, col, row) && col == 4 && row == 6, "(450,650) -> e2 (4,6)");
}

// 4. Teste de Unidade: Posicao Inicial e Fronteiras de getPiece
TEST(initial_position_and_bounds) {
    ChessBoard board(800.f);

    CHECK(board.getPiece(-1, 0) == nullptr, "getPiece(-1, 0) deve ser nullptr");
    CHECK(board.getPiece(8, 0) == nullptr, "getPiece(8, 0) deve ser nullptr");
    CHECK(board.getPiece(0, -1) == nullptr, "getPiece(0, -1) deve ser nullptr");
    CHECK(board.getPiece(0, 8) == nullptr, "getPiece(0, 8) deve ser nullptr");

    const Piece* wKing = board.getPiece(4, 7);
    CHECK(wKing != nullptr && wKing->getType() == PieceType::KING && wKing->getColor() == PieceColor::WHITE,
          "Rei branco em e1 (4, 7)");

    const Piece* bQueen = board.getPiece(3, 0);
    CHECK(bQueen != nullptr && bQueen->getType() == PieceType::QUEEN && bQueen->getColor() == PieceColor::BLACK,
          "Dama preta em d8 (3, 0)");

    for (int col = 0; col < 8; ++col) {
        CHECK(board.getPiece(col, 1) != nullptr && board.getPiece(col, 1)->getType() == PieceType::PAWN,
              "Peao preto na fileira 7");
        CHECK(board.getPiece(col, 6) != nullptr && board.getPiece(col, 6)->getType() == PieceType::PAWN,
              "Peao branco na fileira 2");
        for (int row = 2; row <= 5; ++row) {
            CHECK(board.getPiece(col, row) == nullptr, "Casas centrais vazias no inicio");
        }
    }
}

// 5. Teste de Unidade (Caixa Branca / Conjunto Basico): applyUCIMove
// Cobre erros de entrada, lance normal, captura, roque curto, roque longo, en passant e 4 promocoes
TEST(apply_uci_move_basis_paths) {
    ChessBoard board(800.f);

    // Caminhos de erro
    CHECK(!board.applyUCIMove("e2"), "Lance curto (<4) deve retornar false");
    CHECK(!board.applyUCIMove("z2e4"), "Origem invalida deve retornar false");
    CHECK(!board.applyUCIMove("e2z4"), "Destino invalido deve retornar false");
    CHECK(!board.applyUCIMove("e4e5"), "Casa de origem vazia deve retornar false");

    // Lance normal + Captura
    CHECK(board.applyUCIMove("e2e4"), "Deve aplicar e2e4");
    CHECK(board.getPiece(4, 6) == nullptr && board.getPiece(4, 4) != nullptr, "Peao moveu de e2 para e4");
    CHECK(board.applyUCIMove("d7d5"), "Deve aplicar d7d5");
    CHECK(board.applyUCIMove("e4d5"), "Deve capturar em d5");
    CHECK(board.getPiece(3, 3) != nullptr && board.getPiece(3, 3)->getColor() == PieceColor::WHITE,
          "Peao branco ocupou d5");

    // Roque Pequeno (e1g1): abre espaco em f1 e g1
    board.setupInitialPosition();
    CHECK(board.applyUCIMove("g1f3"), "Sai cavalo g1f3");
    CHECK(board.applyUCIMove("e2e4"), "Abre bispo");
    CHECK(board.applyUCIMove("f1c4"), "Sai bispo f1c4");
    CHECK(board.applyUCIMove("e1g1"), "Roque curto e1g1");
    CHECK(board.getPiece(6, 7) != nullptr && board.getPiece(6, 7)->getType() == PieceType::KING,
          "Rei branco em g1 apos roque curto");
    CHECK(board.getPiece(5, 7) != nullptr && board.getPiece(5, 7)->getType() == PieceType::ROOK,
          "Torre branca em f1 apos roque curto");

    // Roque Grande (e1c1): abre espaco em b1, c1, d1
    board.setupInitialPosition();
    CHECK(board.applyUCIMove("b1c3"), "Sai cavalo b1c3");
    CHECK(board.applyUCIMove("d2d4"), "Abre bispo e dama");
    CHECK(board.applyUCIMove("c1f4"), "Sai bispo c1f4");
    CHECK(board.applyUCIMove("d1d2"), "Sai dama d1d2");
    CHECK(board.applyUCIMove("e1c1"), "Roque longo e1c1");
    CHECK(board.getPiece(2, 7) != nullptr && board.getPiece(2, 7)->getType() == PieceType::KING,
          "Rei branco em c1 apos roque longo");
    CHECK(board.getPiece(3, 7) != nullptr && board.getPiece(3, 7)->getType() == PieceType::ROOK,
          "Torre branca em d1 apos roque longo");

    // En Passant: e2e4, a7a6, e4e5, d7d5, e5d6
    board.setupInitialPosition();
    board.processUCICommand("position startpos moves e2e4 a7a6 e4e5 d7d5");
    CHECK(board.applyUCIMove("e5d6"), "Captura en passant e5d6");
    CHECK(board.getPiece(3, 2) != nullptr && board.getPiece(3, 2)->getType() == PieceType::PAWN,
          "Peao branco em d6");
    CHECK(board.getPiece(3, 3) == nullptr, "Peao preto capturado em d5 foi removido");

    // Promocoes nas 4 pecas (q, r, b, n)
    board.setupInitialPosition();
    board.processUCICommand("position startpos moves a2a4 b7b5 a4b5 a7a6 b5a6 b8c6 a6a7");
    CHECK(board.applyUCIMove("a7b8q") && board.getPiece(1, 0)->getType() == PieceType::QUEEN, "Promocao para Queen");

    board.setupInitialPosition();
    board.processUCICommand("position startpos moves a2a4 b7b5 a4b5 a7a6 b5a6 b8c6 a6a7");
    CHECK(board.applyUCIMove("a7b8r") && board.getPiece(1, 0)->getType() == PieceType::ROOK, "Promocao para Rook");

    board.setupInitialPosition();
    board.processUCICommand("position startpos moves a2a4 b7b5 a4b5 a7a6 b5a6 b8c6 a6a7");
    CHECK(board.applyUCIMove("a7b8b") && board.getPiece(1, 0)->getType() == PieceType::BISHOP, "Promocao para Bishop");

    board.setupInitialPosition();
    board.processUCICommand("position startpos moves a2a4 b7b5 a4b5 a7a6 b5a6 b8c6 a6a7");
    CHECK(board.applyUCIMove("a7b8n") && board.getPiece(1, 0)->getType() == PieceType::KNIGHT, "Promocao para Knight");
}

// 6. Teste de Unidade: processUCICommand (position, moves, bestmove, lance direto)
TEST(process_uci_command) {
    ChessBoard board(800.f);

    board.processUCICommand("");
    board.processUCICommand("position startpos moves e2e4 e7e5");
    CHECK(board.getPiece(4, 4) != nullptr && board.getPiece(4, 3) != nullptr, "e4 e e5 ocupados");
    CHECK(board.getMoveHistory().size() == 2, "Historico com 2 lances");

    board.processUCICommand("moves g1f3 b8c6");
    CHECK(board.getPiece(5, 5) != nullptr && board.getPiece(2, 2) != nullptr, "f3 e c6 ocupados");

    board.processUCICommand("bestmove f1c4");
    CHECK(board.getPiece(2, 4) != nullptr && board.getPiece(2, 4)->getType() == PieceType::BISHOP,
          "bestmove f1c4 aplicou bispo em c4");

    board.processUCICommand("bestmove 0000");
    board.processUCICommand("position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    board.processUCICommand("d7d6");
    CHECK(board.getPiece(3, 2) != nullptr, "Lance direto d7d6 aplicado");
}

// 7. Teste de Unidade: Interacao por Clique do Mouse e Highlight de Movimentos Validos
TEST(mouse_click_and_highlights) {
    ChessBoard board(800.f);
    // Com legalMoves vazia, qualquer lance estruturalmente valido e considerado legal
    CHECK(board.isMoveLegal("e2e4"), "Com lista vazia, isMoveLegal retorna true");

    board.setLegalMoves("legalmoves e2e3 e2e4 g1f3 g1h3");
    CHECK(board.getLegalMoves().size() == 4, "4 lances legais carregados");
    CHECK(board.isMoveLegal("e2e4"), "e2e4 deve ser legal");
    CHECK(!board.isMoveLegal("e2e5"), "e2e5 nao deve ser legal");

    // Clique fora do tabuleiro
    CHECK(board.handleMouseClick(-5.f, 100.f).empty(), "Clique fora nao gera lance");

    // Clique em casa vazia sem selecao
    CHECK(board.handleMouseClick(450.f, 450.f).empty(), "Clique em e4 vazio nao seleciona");
    CHECK(board.getSelectedCol() == -1, "Nenhuma casa selecionada");

    // Clique em peca adversaria (preta em e7 = col 4, row 1) no turno das brancas
    CHECK(board.handleMouseClick(450.f, 150.f).empty(), "Nao pode selecionar peca adversaria");
    CHECK(board.getSelectedCol() == -1, "Continua sem selecao");

    // 1o clique valido: peao branco em e2 (col 4, row 6 -> x=450, y=650)
    CHECK(board.handleMouseClick(450.f, 650.f).empty(), "1o clique apenas seleciona");
    CHECK(board.getSelectedCol() == 4 && board.getSelectedRow() == 6, "e2 selecionado");
    CHECK(board.getHighlightedSquares().size() == 2, "Destaca 2 casas (e3 e e4)");

    // Clique na mesma casa e2: desseleciona
    CHECK(board.handleMouseClick(450.f, 650.f).empty(), "Clicar na mesma casa desseleciona");
    CHECK(board.getSelectedCol() == -1 && board.getHighlightedSquares().empty(), "Selecao limpa");

    // Seleciona e2 e depois clica em outra peca branca (g1 = col 6, row 7 -> x=650, y=750)
    board.handleMouseClick(450.f, 650.f);
    CHECK(board.handleMouseClick(650.f, 750.f).empty(), "Troca selecao para cavalo em g1");
    CHECK(board.getSelectedCol() == 6 && board.getSelectedRow() == 7, "g1 selecionado");
    CHECK(board.getHighlightedSquares().size() == 2, "Destaca f3 e h3");

    // Tenta clicar em casa ilegal para o cavalo (g3 = col 6, row 5 -> x=650, y=550)
    CHECK(board.handleMouseClick(650.f, 550.f).empty(), "Destino ilegal g1g3 deve ser rejeitado");
    CHECK(board.getPiece(6, 7) != nullptr, "Cavalo continua em g1");

    // Clica em destino legal f3 (col 5, row 5 -> x=550, y=550)
    std::string played = board.handleMouseClick(550.f, 550.f);
    CHECK(played == "g1f3", "Deve retornar lance g1f3 executado");
    CHECK(board.getPiece(5, 5) != nullptr && board.getPiece(5, 5)->getType() == PieceType::KNIGHT,
          "Cavalo moveu para f3");
    CHECK(board.getCurrentTurn() == PieceColor::BLACK, "Turno mudou para BLACK");
    CHECK(board.getSelectedCol() == -1 && board.getHighlightedSquares().empty(), "Selecao limpa apos lance");

    // Promocao via clique do mouse (peao branco em a7 = col 0, row 1 clicando em b8 = col 1, row 0)
    board.processUCICommand("position startpos moves a2a4 b7b5 a4b5 a7a6 b5a6 b8c6 a6a7 c6a5");
    board.setLegalMoves("legalmoves a7b8q a7b8r a7b8b a7b8n");
    CHECK(board.handleMouseClick(50.f, 150.f).empty(), "Seleciona peao branco em a7");
    CHECK(board.getHighlightedSquares().size() == 1, "4 promocoes para b8 geram 1 unico highlight em b8");
    std::string promoMove = board.handleMouseClick(150.f, 50.f);
    CHECK(promoMove == "a7b8q", "Clique na ultima fileira promove para dama por padrao (a7b8q)");
    CHECK(board.getPiece(1, 0) != nullptr && board.getPiece(1, 0)->getType() == PieceType::QUEEN,
          "Peca em b8 virou QUEEN");
}

// 8. Teste de Unidade com Stub (Slide 26: Driver -> EngineBridge -> Stub /bin/cat)
TEST(engine_bridge_with_stub) {
    EngineBridge bridge;

    // Estado inicial desligado
    CHECK(bridge.readCommand().empty(), "readCommand sem processo deve retornar vazio");
    CHECK(!bridge.sendCommand("noop"), "sendCommand sem processo deve retornar false");

    // Usa /bin/cat como Stub de eco para testar os pipes isolados do motor de xadrez
    CHECK(bridge.start("/bin/cat"), "Deve iniciar stub /bin/cat");
    // Reiniciar com processo ativo deve parar o anterior automaticamente
    CHECK(bridge.start("/bin/cat"), "Re-start deve encerrar o anterior e abrir novo stub");
    CHECK(bridge.sendCommand("ping_stub_1\r\nping_stub_2"), "Envia 2 linhas (com CRLF) ao stub");
    CHECK(bridge.readCommand() == "ping_stub_1", "Le 1a linha removendo CR do buffer do pipe");
    CHECK(bridge.readCommand() == "ping_stub_2", "Le 2a linha do buffer do pipe");
    CHECK(bridge.stop(), "Encerra o stub com sucesso");
}

// 9. Teste de Unidade (Issue #7): Alertas de Estado de Jogo (Xeque, Mate, Afogamento, 50 lances e Bloqueio)
TEST(game_state_alerts_and_lock) {
    ChessBoard board(800.f);

    CHECK(!board.isGameOver(), "Partida nao deve iniciar encerrada");
    CHECK(!board.isInCheck(), "Partida nao deve iniciar em xeque");
    CHECK(board.getCheckCol() == -1 && board.getCheckRow() == -1, "checkCol/Row iniciais devem ser -1");
    CHECK(board.getGameResult() == "*", "Resultado inicial deve ser '*'");
    CHECK(board.getGameReason() == "none", "Motivo inicial deve ser 'none'");
    CHECK(board.getAlertMessage().empty(), "Sem alerta inicial");

    // Xeque simples em e1
    board.processUCICommand("state check e1 result * reason none fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    CHECK(board.isInCheck(), "Deve detectar xeque em e1");
    CHECK(board.getCheckCol() == 4 && board.getCheckRow() == 7, "Casa do rei em xeque deve ser e1 (4, 7)");
    CHECK(!board.isGameOver(), "Xeque simples nao encerra partida");
    CHECK(board.getAlertMessage().find("Xeque") != std::string::npos, "Mensagem deve alertar Xeque");

    // Saida do xeque
    board.processUCICommand("state check - result * reason none fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    CHECK(!board.isInCheck(), "Xeque deve limpar com 'check -'");
    CHECK(board.getCheckCol() == -1 && board.getCheckRow() == -1, "checkCol/Row voltam para -1");

    // Xeque-mate (0-1)
    board.processUCICommand("state check e1 result 0-1 reason checkmate fen rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
    CHECK(board.isGameOver(), "Xeque-mate deve encerrar a partida");
    CHECK(board.isInCheck(), "Rei em e1 continua destacado em xeque no mate");
    CHECK(board.getGameResult() == "0-1", "Resultado deve ser 0-1");
    CHECK(board.getGameReason() == "checkmate", "Motivo deve ser checkmate");
    CHECK(board.getAlertMessage().find("Xeque-mate") != std::string::npos, "Mensagem deve alertar Xeque-mate");

    // Bloqueio de cliques apos fim de jogo
    CHECK(board.handleMouseClick(450.f, 650.f).empty(), "Nao deve selecionar nem mover apos fim de jogo");
    CHECK(board.getSelectedCol() == -1, "Selecao bloqueada apos fim de jogo");

    // Reinicio da partida
    board.resetGame();
    CHECK(!board.isGameOver(), "resetGame deve limpar fim de jogo");
    CHECK(!board.isInCheck(), "resetGame deve limpar xeque");
    CHECK(board.getGameResult() == "*", "resetGame restaura resultado '*'");
    CHECK(board.getAlertMessage().empty(), "resetGame limpa alerta");

    // Afogamento (stalemate)
    board.processUCICommand("state check - result 1/2-1/2 reason stalemate fen 7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    CHECK(board.isGameOver(), "Afogamento deve encerrar partida");
    CHECK(board.getGameReason() == "stalemate", "Motivo deve ser stalemate");
    CHECK(board.getAlertMessage().find("Afogamento") != std::string::npos, "Mensagem deve alertar Afogamento");

    // Regra dos 50 lances (fifty-moves)
    board.processUCICommand("state check - result 1/2-1/2 reason fifty-moves fen 8/8/8/3K2k1/8/8/8/8 b - - 100 211");
    CHECK(board.isGameOver(), "Empate de 50 lances deve encerrar partida");
    CHECK(board.getGameReason() == "fifty-moves", "Motivo deve ser fifty-moves");
    CHECK(board.getAlertMessage().find("50 lances") != std::string::npos, "Mensagem deve alertar 50 lances");
}

// 10. Teste de Unidade (Issue #7): Alertas de Movimento Invalido e Captura
TEST(capture_and_invalid_move_alerts) {
    ChessBoard board(800.f);
    board.setLegalMoves("legalmoves g1f3");

    // Seleciona cavalo em g1 (col 6, row 7 -> x=650, y=750) e clica em casa ilegal g3 (col 6, row 5 -> x=650, y=550)
    board.handleMouseClick(650.f, 750.f);
    CHECK(board.handleMouseClick(650.f, 550.f).empty(), "Lance ilegal deve ser bloqueado");
    CHECK(board.getInvalidMoveCol() == 6 && board.getInvalidMoveRow() == 5,
          "Casa do clique ilegal (g3) deve ser marcada para alerta visual");
    CHECK(board.getAlertMessage().find("invalido") != std::string::npos,
          "Mensagem deve alertar movimento invalido");

    // Captura normal (1. e4 d5 2. exd5)
    board.resetGame();
    board.processUCICommand("position startpos moves e2e4 d7d5");
    CHECK(board.applyUCIMove("e4d5"), "Captura e4d5 deve ser aplicada");
    CHECK(board.getAlertMessage().find("Captura") != std::string::npos,
          "Mensagem deve alertar captura de peca");

    // Captura en passant (1. e4 a6 2. e5 d5 3. exd6)
    board.resetGame();
    board.processUCICommand("position startpos moves e2e4 a7a6 e4e5 d7d5");
    CHECK(board.applyUCIMove("e5d6"), "Captura en passant e5d6 deve ser aplicada");
    CHECK(board.getAlertMessage().find("Captura") != std::string::npos,
          "Mensagem deve alertar captura en passant");
    CHECK(board.isAlertVisible(), "Pop-up de alerta deve ficar visivel apos captura");

    // Clicar no botao [X] do pop-up fecha o alerta sem selecionar pecas
    const sf::FloatRect& closeRect = board.getCloseButtonBounds();
    CHECK(board.handleMouseClick(closeRect.left + 5.f, closeRect.top + 5.f).empty(),
          "Clique no botao [X] fecha pop-up sem mover peca");
    CHECK(!board.isAlertVisible(), "Pop-up deve desaparecer apos clicar no [X]");
    CHECK(board.getSelectedCol() == -1, "Clique no [X] nao deve selecionar peca por baixo");
}

// 11. Teste de Unidade (Issue #8): Relogios da Partida (RF07), Formatacao MM:SS e Timeout
TEST(hud_clocks_and_timeout) {
    ChessBoard board(800.f);

    // Estado inicial: 10:00 (600s) para cada lado
    CHECK(board.getRemainingTimeSec(PieceColor::WHITE) == 600.f, "Brancas iniciam com 600s");
    CHECK(board.getRemainingTimeSec(PieceColor::BLACK) == 600.f, "Pretas iniciam com 600s");
    CHECK(ChessBoard::formatClock(600.f) == "10:00", "600s -> 10:00");
    CHECK(ChessBoard::formatClock(65.f) == "01:05", "65s -> 01:05");
    CHECK(ChessBoard::formatClock(0.f) == "00:00", "0s -> 00:00");
    CHECK(ChessBoard::formatClock(-5.f) == "00:00", "Negativo -> 00:00");

    // Desconta tempo apenas do jogador da vez (WHITE)
    board.advanceClock(15.f);
    CHECK(board.getRemainingTimeSec(PieceColor::WHITE) == 585.f, "Relogio branco descontou 15s");
    CHECK(board.getRemainingTimeSec(PieceColor::BLACK) == 600.f, "Relogio preto permaneceu parado");

    // Apos lance das brancas, passa a descontar das pretas
    board.applyUCIMove("e2e4");
    board.advanceClock(20.f);
    CHECK(board.getRemainingTimeSec(PieceColor::WHITE) == 585.f, "Relogio branco parou na vez das pretas");
    CHECK(board.getRemainingTimeSec(PieceColor::BLACK) == 580.f, "Relogio preto descontou 20s");

    // Estouro de tempo (timeout) das pretas
    board.setRemainingTimeSec(PieceColor::BLACK, 2.f);
    board.advanceClock(5.f);
    CHECK(board.getRemainingTimeSec(PieceColor::BLACK) == 0.f, "Relogio preto trava em 0s");
    CHECK(board.isGameOver(), "Timeout deve encerrar a partida");
    CHECK(board.getGameResult() == "1-0", "Timeout das pretas da vitoria 1-0 para as brancas");
    CHECK(board.getGameReason() == "timeout", "Motivo deve ser timeout");
    CHECK(board.getAlertMessage().find("Tempo esgotado") != std::string::npos,
          "Pop-up deve anunciar Tempo esgotado");
}

// 12. Teste de Unidade (Issue #8): Indicador de Turno, Historico Formatado, Scroll e Botoes (Desistir / Nova Partida)
TEST(hud_turn_history_and_buttons) {
    ChessBoard board(800.f);

    CHECK(board.getTurnIndicatorText() == "Vez das Brancas", "Indicador inicial deve ser Vez das Brancas");
    CHECK(board.getFormattedMoveHistory().empty(), "Historico formatado inicia vazio");

    board.processUCICommand("position startpos moves e2e4 e7e5 g1f3");
    CHECK(board.getTurnIndicatorText() == "Vez das Pretas", "Apos 3 meios-lances, vez das Pretas");

    auto lines = board.getFormattedMoveHistory();
    CHECK(lines.size() == 2, "3 meios-lances geram 2 linhas numeradas");
    CHECK(lines[0].find("1.") != std::string::npos &&
          lines[0].find("e2e4") != std::string::npos &&
          lines[0].find("e7e5") != std::string::npos,
          "Linha 1 deve conter '1. e2e4 ... e7e5'");
    CHECK(lines[1].find("2.") != std::string::npos &&
          lines[1].find("g1f3") != std::string::npos,
          "Linha 2 deve conter '2. g1f3'");

    // Teste de scroll do historico
    for (int i = 0; i < 15; ++i) {
        board.setLegalMoves("");
    }
    board.scrollHistory(-2);
    CHECK(board.getHistoryScrollOffset() >= 0, "Scroll offset nunca fica negativo");

    // Clique no botao Desistir do HUD lateral
    const sf::FloatRect& resignRect = board.getResignButtonBounds();
    CHECK(board.handleMouseClick(resignRect.left + 10.f, resignRect.top + 10.f).empty(),
          "Clique em Desistir nao retorna lance UCI");
    CHECK(board.isGameOver(), "Desistir encerra a partida");
    CHECK(board.getGameResult() == "1-0", "Desistencia na vez das Pretas da vitoria 1-0 para as Brancas");
    CHECK(board.getGameReason() == "resign", "Motivo deve ser resign");
    CHECK(board.getTurnIndicatorText() == "Partida Encerrada", "Indicador mostra Partida Encerrada");
    CHECK(board.getAlertMessage().find("Desistencia") != std::string::npos, "Pop-up anuncia Desistencia");

    // Clique no botao Nova Partida do HUD lateral (funciona mesmo com partida encerrada)
    const sf::FloatRect& newGameRect = board.getNewGameButtonBounds();
    board.handleMouseClick(newGameRect.left + 10.f, newGameRect.top + 10.f);
    CHECK(!board.isGameOver(), "Botao Nova Partida reinicia o jogo");
    CHECK(board.getMoveHistory().empty(), "Historico limpo apos Nova Partida");
    CHECK(board.getRemainingTimeSec(PieceColor::WHITE) == 600.f, "Relogio branco resetado para 600s");
    CHECK(board.getTurnIndicatorText() == "Vez das Brancas", "Turno volta para Brancas");
}

int main() {
    return test_run_all();
}
