#include "board.hpp"
#include <iostream>
#include <sstream>
#include <cmath>

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

bool ChessBoard::parseSquare(const std::string& sqStr, int& outCol, int& outRow) const {
    if (sqStr.length() != 2) return false;

    char fileChar = sqStr[0];
    char rankChar = sqStr[1];

    if (fileChar < 'a' || fileChar > 'h') return false;
    if (rankChar < '1' || rankChar > '8') return false;

    // Converte coluna ('a'..'h' -> 0..7)
    outCol = fileChar - 'a';

    // Converte linha ('1'..'8' -> 7..0, pois linha 0 é o topo e linha 7 é a base)
    outRow = 8 - (rankChar - '0');

    return true;
}

bool ChessBoard::applyUCIMove(const std::string& moveStr) {
    if (moveStr.length() < 4) {
        std::cerr << "[Erro UCI] Lance invalido (muito curto): " << moveStr << std::endl;
        return false;
    }

    std::string fromSquare = moveStr.substr(0, 2);
    std::string toSquare = moveStr.substr(2, 2);

    int fromCol, fromRow, toCol, toRow;
    if (!parseSquare(fromSquare, fromCol, fromRow) || !parseSquare(toSquare, toCol, toRow)) {
        std::cerr << "[Erro UCI] Coordenadas invalidas no lance: " << moveStr << std::endl;
        return false;
    }

    if (!pieces[fromCol][fromRow]) {
        std::cerr << "[Erro UCI] Nenhuma peca na casa de origem " << fromSquare << std::endl;
        return false;
    }

    PieceType type = pieces[fromCol][fromRow]->getType();
    PieceColor color = pieces[fromCol][fromRow]->getColor();

    // 1. Tratamento de Roque
    // No protocolo UCI, o roque e sinalizado pelo movimento do Rei de 2 casas
    if (type == PieceType::KING && std::abs(toCol - fromCol) == 2) {
        // Roque na ala do Rei (pequeno): o rei vai para a coluna g (col 6)
        if (toCol == 6) {
            // A torre esta na coluna h (col 7) e vai para f (col 5)
            if (pieces[7][fromRow]) {
                pieces[5][fromRow] = std::move(pieces[7][fromRow]);
                pieces[5][fromRow]->setGridPosition(5, fromRow, squareSize);
            }
        }
        // Roque na ala da Dama (grande): o rei vai para a coluna c (col 2)
        else if (toCol == 2) {
            // A torre esta na coluna a (col 0) e vai para d (col 3)
            if (pieces[0][fromRow]) {
                pieces[3][fromRow] = std::move(pieces[0][fromRow]);
                pieces[3][fromRow]->setGridPosition(3, fromRow, squareSize);
            }
        }
    }

    // 2. Tratamento de En Passant
    // Ocorre se um peao anda na diagonal para uma casa vazia
    if (type == PieceType::PAWN && fromCol != toCol && !pieces[toCol][toRow]) {
        // O peao capturado esta na mesma coluna de destino, mas na linha de origem
        pieces[toCol][fromRow].reset();
    }

    // 3. Movimento padrao e captura
    // Mover o ponteiro inteligente libera automaticamente qualquer peca capturada em pieces[toCol][toRow]
    pieces[toCol][toRow] = std::move(pieces[fromCol][fromRow]);
    pieces[toCol][toRow]->setGridPosition(toCol, toRow, squareSize);

    // 4. Tratamento de Promocao (ex: "e7e8q", "e2e1n")
    // O 5º caractere indica a peca promovida (q = dama, r = torre, b = bispo, n = cavalo)
    if (moveStr.length() == 5) {
        char promoChar = moveStr[4];
        PieceType promoType = PieceType::QUEEN; // padrao

        if (promoChar == 'r' || promoChar == 'R') promoType = PieceType::ROOK;
        else if (promoChar == 'b' || promoChar == 'B') promoType = PieceType::BISHOP;
        else if (promoChar == 'n' || promoChar == 'N') promoType = PieceType::KNIGHT;
        else if (promoChar == 'q' || promoChar == 'Q') promoType = PieceType::QUEEN;

        placePiece(promoType, color, toCol, toRow);
    }

    return true;
}

void ChessBoard::processUCICommand(const std::string& command) {
    std::cout << "[UCI] Comando recebido: " << command << std::endl;

    std::stringstream ss(command);
    std::string token;
    ss >> token;

    // Caso 1: Comando de posição "position startpos [moves ...]" ou "position fen ... [moves ...]"
    if (token == "position") {
        std::string type;
        ss >> type;

        if (type == "startpos") {
            setupInitialPosition();

            // Verifica se ha lances adicionais a seguir ("moves e2e4 e7e5...")
            std::string nextKeyword;
            if (ss >> nextKeyword && nextKeyword == "moves") {
                std::string move;
                while (ss >> move) {
                    applyUCIMove(move);
                }
            }
        }
        else if (type == "fen") {
            // Futuro: implementar o parser de FEN para preencher o tabuleiro
            std::cout << "[UCI] Configurando posicao FEN (a implementar)..." << std::endl;
        }
        return;
    }

    // Caso 2: Comando iniciando diretamente por "moves e2e4 e7e5..."
    if (token == "moves") {
        std::string move;
        while (ss >> move) {
            applyUCIMove(move);
        }
        return;
    }

    // Caso 3: Lance direto no formato UCI (ex: "e2e4", "e7e8q")
    if (token.length() >= 4 && token.length() <= 5) {
        applyUCIMove(token);
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
