#include "board.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

ChessBoard::ChessBoard(float boardSideSize)
    : squareSize(boardSideSize / 8.f),
      selectedCol(-1),
      selectedRow(-1),
      currentTurn(PieceColor::WHITE) {
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

void ChessBoard::clearSelection() {
    selectedCol = -1;
    selectedRow = -1;
    highlightedSquares.clear();
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
    clearSelection();
    moveHistory.clear();
    legalMoves.clear();
    currentTurn = PieceColor::WHITE;

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

std::string ChessBoard::squareToCoord(int col, int row) const {
    if (col < 0 || col > 7 || row < 0 || row > 7) {
        return "";
    }
    char fileChar = static_cast<char>('a' + col);
    char rankChar = static_cast<char>('8' - row);
    return std::string{fileChar, rankChar};
}

bool ChessBoard::pixelToSquare(float x, float y, int& outCol, int& outRow) const {
    float boardSize = squareSize * 8.f;
    if (x < 0.f || y < 0.f || x >= boardSize || y >= boardSize) {
        return false;
    }
    outCol = static_cast<int>(x / squareSize);
    outRow = static_cast<int>(y / squareSize);
    return (outCol >= 0 && outCol < 8 && outRow >= 0 && outRow < 8);
}

const Piece* ChessBoard::getPiece(int col, int row) const {
    if (col < 0 || col > 7 || row < 0 || row > 7) {
        return nullptr;
    }
    return pieces[col][row].get();
}

void ChessBoard::setLegalMoves(const std::string& legalMovesLine) {
    legalMoves.clear();
    std::stringstream ss(legalMovesLine);
    std::string token;
    while (ss >> token) {
        if (token != "legalmoves") {
            legalMoves.push_back(token);
        }
    }
    updateHighlightsForSelected();
}

bool ChessBoard::isMoveLegal(const std::string& moveStr) const {
    if (legalMoves.empty()) {
        return true;
    }
    return std::find(legalMoves.begin(), legalMoves.end(), moveStr) != legalMoves.end();
}

void ChessBoard::updateHighlightsForSelected() {
    highlightedSquares.clear();
    if (selectedCol < 0 || selectedRow < 0) {
        return;
    }
    std::string fromCoord = squareToCoord(selectedCol, selectedRow);
    for (const auto& m : legalMoves) {
        if (m.size() >= 4 && m.substr(0, 2) == fromCoord) {
            int toCol = -1, toRow = -1;
            if (parseSquare(m.substr(2, 2), toCol, toRow)) {
                std::pair<int, int> sq{toCol, toRow};
                if (std::find(highlightedSquares.begin(), highlightedSquares.end(), sq) == highlightedSquares.end()) {
                    highlightedSquares.push_back(sq);
                }
            }
        }
    }
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

    moveHistory.push_back(moveStr);
    currentTurn = (color == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
    clearSelection();

    return true;
}

void ChessBoard::processUCICommand(const std::string& command) {
    std::cout << "[UCI] Comando recebido: " << command << std::endl;

    std::stringstream ss(command);
    std::string token;
    if (!(ss >> token)) {
        return;
    }

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

    // Caso 3: Resposta do motor "bestmove <lance>"
    if (token == "bestmove") {
        std::string move;
        if (ss >> move && move != "0000") {
            applyUCIMove(move);
        }
        return;
    }

    // Caso 4: Lista de lances legais "legalmoves <l1> <l2> ..."
    if (token == "legalmoves") {
        setLegalMoves(command);
        return;
    }

    // Caso 5: Lance direto no formato UCI (ex: "e2e4", "e7e8q")
    if (token.length() >= 4 && token.length() <= 5) {
        applyUCIMove(token);
        return;
    }
}

std::string ChessBoard::handleMouseClick(float x, float y) {
    int col = -1, row = -1;
    if (!pixelToSquare(x, y, col, row)) {
        clearSelection();
        return "";
    }

    // 1o clique: nenhuma peca selecionada ainda
    if (selectedCol < 0 || selectedRow < 0) {
        const Piece* clicked = getPiece(col, row);
        if (clicked != nullptr && clicked->getColor() == currentTurn) {
            selectedCol = col;
            selectedRow = row;
            updateHighlightsForSelected();
        }
        return "";
    }

    // 2o clique na mesma casa: desseleciona
    if (col == selectedCol && row == selectedRow) {
        clearSelection();
        return "";
    }

    // 2o clique em outra peca da mesma cor: troca a peca selecionada
    const Piece* clicked = getPiece(col, row);
    if (clicked != nullptr && clicked->getColor() == currentTurn) {
        selectedCol = col;
        selectedRow = row;
        updateHighlightsForSelected();
        return "";
    }

    // Monta o lance candidato UCI
    std::string candidate = squareToCoord(selectedCol, selectedRow) + squareToCoord(col, row);
    const Piece* movingPiece = getPiece(selectedCol, selectedRow);
    if (movingPiece != nullptr && movingPiece->getType() == PieceType::PAWN && (row == 0 || row == 7)) {
        candidate += 'q';
    }

    if (!isMoveLegal(candidate)) {
        return "";
    }

    if (applyUCIMove(candidate)) {
        return candidate;
    }
    return "";
}

bool ChessBoard::playTurnWithEngine(float x, float y, EngineBridge& bridge) {
    if (legalMoves.empty()) {
        if (bridge.sendCommand("legalmoves")) {
            setLegalMoves(bridge.readCommand());
        }
    }

    std::string userMove = handleMouseClick(x, y);
    if (userMove.empty()) {
        return false;
    }

    auto syncPositionToEngine = [&]() {
        std::string posCmd = "position startpos moves";
        for (const auto& m : moveHistory) {
            posCmd += " " + m;
        }
        bridge.sendCommand(posCmd);
    };

    syncPositionToEngine();

    if (bridge.sendCommand("go")) {
        std::string bestLine = bridge.readCommand();
        processUCICommand(bestLine);
        syncPositionToEngine();
    }

    if (bridge.sendCommand("legalmoves")) {
        setLegalMoves(bridge.readCommand());
    }

    return true;
}

void ChessBoard::draw(sf::RenderWindow &window) {
    // Desenha as 64 casas do tabuleiro
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            window.draw(squares[i][j]);
        }
    }

    // Destaca a casa selecionada
    if (selectedCol >= 0 && selectedRow >= 0) {
        sf::RectangleShape selRect(sf::Vector2f(squareSize, squareSize));
        selRect.setPosition(selectedCol * squareSize, selectedRow * squareSize);
        selRect.setFillColor(sf::Color(246, 246, 105, 160));
        window.draw(selRect);
    }

    // Destaca as casas de destino válidas (Highlight - Issue #5)
    for (const auto& sq : highlightedSquares) {
        sf::RectangleShape hlRect(sf::Vector2f(squareSize, squareSize));
        hlRect.setPosition(sq.first * squareSize, sq.second * squareSize);
        hlRect.setFillColor(sf::Color(106, 190, 48, 130));
        window.draw(hlRect);
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
