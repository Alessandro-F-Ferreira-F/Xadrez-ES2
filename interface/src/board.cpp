#include "board.hpp"
#include <iostream>

ChessBoard::ChessBoard(float boardSideSize) {
    squareSize = boardSideSize / 8.f;

    initSquares();
    loadTextures();
    setupInitialPosition();
}

void ChessBoard::initSquares() {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            squares[i][j].setSize(sf::Vector2f(squareSize, squareSize));

            // Cores estilo xadrez de madeira (claras e escuras)
            // para que peças pretas fiquem bem visíveis nas casas escuras
            if ((i + j) % 2 == 0) {
                squares[i][j].setFillColor(sf::Color(240, 217, 181)); // Casa clara
            } else {
                squares[i][j].setFillColor(sf::Color(181, 136, 99));  // Casa escura
            }

            squares[i][j].setPosition(i * squareSize, j * squareSize);
        }
    }
}

void ChessBoard::loadTextures() {
    const std::string basePath = "./assets/pieces/";

    const std::string typeNames[6] = {
        "pawn", "knight", "bishop", "rook", "queen", "king"
    };

    // 0 = WHITE ("w-"), 1 = BLACK ("b-")
    const std::string colorPrefix[2] = { "w-", "b-" };

    for (int c = 0; c < 2; c++) {
        for (int t = 0; t < 6; t++) {
            std::string filePath = basePath + colorPrefix[c] + typeNames[t] + ".png";
            if (!textures[c][t].loadFromFile(filePath)) {
                std::cerr << "[Erro] Nao foi possivel carregar a textura: " << filePath << std::endl;
            } else {
                textures[c][t].setSmooth(true);
            }
        }
    }
}

void ChessBoard::placePiece(PieceType type, PieceColor color, int col, int row) {
    int cIdx = static_cast<int>(color);
    int tIdx = static_cast<int>(type);
    
    // std::make_unique aloca e gerencia a memória da peça de forma automática
    pieces[col][row] = std::make_unique<Piece>(type, color, textures[cIdx][tIdx], col, row, squareSize);
}

void ChessBoard::clearBoard() {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            // .reset() libera a memória da peça anterior e define o ponteiro como nullptr
            pieces[i][j].reset();
        }
    }
}

void ChessBoard::setupInitialPosition() {
    clearBoard();

    // --- PEÇAS PRETAS (Linhas 0 e 1, no topo da tela) ---
    // Peões pretos na linha 1
    for (int col = 0; col < 8; col++) {
        placePiece(PieceType::PAWN, PieceColor::BLACK, col, 1);
    }
    // Peças maiores pretas na linha 0
    placePiece(PieceType::ROOK,   PieceColor::BLACK, 0, 0);
    placePiece(PieceType::KNIGHT, PieceColor::BLACK, 1, 0);
    placePiece(PieceType::BISHOP, PieceColor::BLACK, 2, 0);
    placePiece(PieceType::QUEEN,  PieceColor::BLACK, 3, 0);
    placePiece(PieceType::KING,   PieceColor::BLACK, 4, 0);
    placePiece(PieceType::BISHOP, PieceColor::BLACK, 5, 0);
    placePiece(PieceType::KNIGHT, PieceColor::BLACK, 6, 0);
    placePiece(PieceType::ROOK,   PieceColor::BLACK, 7, 0);

    // --- PEÇAS BRANCAS (Linhas 6 e 7, na base da tela) ---
    // Peões brancos na linha 6
    for (int col = 0; col < 8; col++) {
        placePiece(PieceType::PAWN, PieceColor::WHITE, col, 6);
    }
    // Peças maiores brancas na linha 7
    placePiece(PieceType::ROOK,   PieceColor::WHITE, 0, 7);
    placePiece(PieceType::KNIGHT, PieceColor::WHITE, 1, 7);
    placePiece(PieceType::BISHOP, PieceColor::WHITE, 2, 7);
    placePiece(PieceType::QUEEN,  PieceColor::WHITE, 3, 7); // d1
    placePiece(PieceType::KING,   PieceColor::WHITE, 4, 7); // e1
    placePiece(PieceType::BISHOP, PieceColor::WHITE, 5, 7);
    placePiece(PieceType::KNIGHT, PieceColor::WHITE, 6, 7);
    placePiece(PieceType::ROOK,   PieceColor::WHITE, 7, 7);
}

void ChessBoard::processUCICommand(const std::string& command) {
    std::cout << "[UCI] Comando recebido: " << command << std::endl;

    // Caso 1: Comando de posição inicial "position startpos"
    if (command.rfind("position startpos", 0) == 0) {
        setupInitialPosition();
        // Futuro: parsear "moves e2e4 e7e5..." se existirem lances adicionais
        return;
    }

    // Caso 2: Comando de posição personalizada via FEN "position fen ..."
    if (command.rfind("position fen", 0) == 0) {
        // Futuro: implementar o parser de FEN para preencher o tabuleiro
        std::cout << "[UCI] Configurando posicao FEN (a implementar)..." << std::endl;
        return;
    }

    // Caso 3: Lance direto no formato UCI (ex: "e2e4", "e7e8q")
    if (command.length() >= 4 && command.length() <= 5) {
        // Futuro: converter coordenadas notação -> grid (ex: 'e'-'a'=4, '2'-'1'=1)
        // e aplicar o movimento na matriz pieces[col][row]
        std::cout << "[UCI] Lance direto detectado: " << command << " (a implementar)..." << std::endl;
        return;
    }
}

void ChessBoard::draw(sf::RenderWindow &window) {
    // Desenha as 64 casas do tabuleiro
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            window.draw(squares[i][j]);
        }
    }

    // Desenha todas as peças que estiverem vivas no tabuleiro
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (pieces[i][j]) {
                pieces[i][j]->draw(window);
            }
        }
    }
}
