#include "board.hpp"
#include "engine_bridge.hpp"
#include "piece.hpp"

#include <iostream>
#include <string>

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << "  [FAIL] " << __func__ << " (linha " << __LINE__     \
                      << "): " << (msg) << std::endl;                          \
            g_failed++;                                                        \
            return;                                                            \
        }                                                                      \
    } while (0)

#define RUN_TEST(fn)                                                           \
    do {                                                                       \
        int before = g_failed;                                                 \
        fn();                                                                  \
        if (g_failed == before) {                                              \
            g_passed++;                                                        \
            std::cout << "  [ OK ] " << #fn << std::endl;                      \
        }                                                                      \
    } while (0)

// 1. Teste de Unidade: Classe Piece
static void test_piece_unit() {
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
static void test_parse_square_basis_paths() {
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
static void test_pixel_to_square_boundaries() {
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
static void test_initial_position_and_bounds() {
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
static void test_apply_uci_move_basis_paths() {
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
static void test_process_uci_command() {
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
static void test_mouse_click_and_highlights() {
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
static void test_engine_bridge_with_stub() {
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

int main() {
    std::cout << "=== [UNIT TESTS] Interface (Caixa Branca, Fronteiras e Stubs) ===" << std::endl;

    RUN_TEST(test_piece_unit);
    RUN_TEST(test_parse_square_basis_paths);
    RUN_TEST(test_pixel_to_square_boundaries);
    RUN_TEST(test_initial_position_and_bounds);
    RUN_TEST(test_apply_uci_move_basis_paths);
    RUN_TEST(test_process_uci_command);
    RUN_TEST(test_mouse_click_and_highlights);
    RUN_TEST(test_engine_bridge_with_stub);

    std::cout << "=== Resultado Unidade: " << g_passed << " passaram, " << g_failed << " falharam ===" << std::endl;
    return (g_failed == 0) ? 0 : 1;
}
