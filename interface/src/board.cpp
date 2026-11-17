#include "board.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <sstream>

namespace {
void drawRoundedRect(sf::RenderWindow& window,
                     float x, float y, float w, float h, float radius,
                     const sf::Color& fill,
                     float outlineThickness = 0.f,
                     const sf::Color& outline = sf::Color::Transparent) {
    const int cornerPoints = 8;
    float r = std::min(radius, std::min(w, h) * 0.5f);
    sf::ConvexShape shape(cornerPoints * 4);

    const float cx[4] = { x + w - r, x + r,     x + r,     x + w - r };
    const float cy[4] = { y + r,     y + r,     y + h - r, y + h - r };
    const float startAngle[4] = { -1.5707963f, 3.1415926f, 1.5707963f, 0.f };
    // Ordem horária: topo-dir (270..360), baixo-dir (0..90), baixo-esq (90..180), topo-esq (180..270)
    const int order[4] = { 0, 3, 2, 1 };
    const float angles[4] = { -1.5707963f, 0.f, 1.5707963f, 3.1415926f };
    (void)startAngle;

    int idx = 0;
    for (int c = 0; c < 4; ++c) {
        int corner = order[c];
        for (int i = 0; i < cornerPoints; ++i) {
            float a = angles[c] + (1.5707963f * i) / (cornerPoints - 1);
            shape.setPoint(idx++, sf::Vector2f(cx[corner] + r * std::cos(a),
                                               cy[corner] + r * std::sin(a)));
        }
    }

    shape.setFillColor(fill);
    shape.setOutlineThickness(outlineThickness);
    shape.setOutlineColor(outline);
    window.draw(shape);
}

sf::Uint8 scaleAlpha(sf::Uint8 baseAlpha, float factor) {
    float clamped = std::max(0.f, std::min(1.f, factor));
    return static_cast<sf::Uint8>(baseAlpha * clamped);
}
} // namespace

ChessBoard::ChessBoard(float boardSideSize, float hudSideWidth)
    : squareSize(boardSideSize / 8.f),
      hudWidth(hudSideWidth),
      selectedCol(-1),
      selectedRow(-1),
      currentTurn(PieceColor::WHITE),
      checkCol(-1),
      checkRow(-1),
      invalidMoveCol(-1),
      invalidMoveRow(-1),
      gameResult("*"),
      gameReason("none"),
      alertVisible(false),
      alertIsCritical(false),
      alertDurationSec(0.f),
      fontLoaded(false),
      remainingTimeSec{DEFAULT_CLOCK_SECONDS, DEFAULT_CLOCK_SECONDS},
      historyScrollOffset(0) {
    initSquares();
    initHudBounds();
    loadTextures();
    loadAlertFont();
    setupInitialPosition();
}

void ChessBoard::initHudBounds() {
    float boardSize = squareSize * 8.f;
    float hudX = boardSize;
    float btnW = (hudWidth - 42.f) * 0.5f;
    float btnH = 44.f;
    float btnY = boardSize - btnH - 20.f;
    resignButtonBounds = sf::FloatRect(hudX + 16.f, btnY, btnW, btnH);
    newGameButtonBounds = sf::FloatRect(hudX + 26.f + btnW, btnY, btnW, btnH);
}

void ChessBoard::loadAlertFont() {
    const char* candidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "./assets/font.ttf"
    };
    for (const char* path : candidates) {
        if (alertFont.loadFromFile(path)) {
            fontLoaded = true;
            return;
        }
    }
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

void ChessBoard::resetGame() {
    setupInitialPosition();
}

void ChessBoard::dismissAlert() {
    alertVisible = false;
    invalidMoveCol = -1;
    invalidMoveRow = -1;
}

void ChessBoard::updateCloseButtonBounds() {
    float boardSize = squareSize * 8.f;
    float btnSize = 26.f;
    if (isGameOver()) {
        float cardW = boardSize * 0.68f;
        float cardH = 120.f;
        float cardX = (boardSize - cardW) * 0.5f;
        float cardY = (boardSize - cardH) * 0.5f;
        closeButtonBounds = sf::FloatRect(cardX + cardW - btnSize - 10.f, cardY + 10.f, btnSize, btnSize);
    } else {
        float cardW = boardSize * 0.54f;
        float cardH = 46.f;
        float cardX = (boardSize - cardW) * 0.5f;
        float cardY = 12.f;
        closeButtonBounds = sf::FloatRect(cardX + cardW - btnSize - 10.f, cardY + (cardH - btnSize) * 0.5f, btnSize, btnSize);
    }
}

void ChessBoard::showAlert(const std::string& message, float durationSec, bool critical) {
    alertMessage = message;
    alertDurationSec = durationSec;
    alertIsCritical = critical;
    alertVisible = true;
    alertStart = std::chrono::steady_clock::now();
    updateCloseButtonBounds();
}

void ChessBoard::updateAlert() {
    if (!alertVisible || alertDurationSec <= 0.f) {
        return;
    }
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - alertStart).count();
    if (elapsed >= alertDurationSec) {
        dismissAlert();
    }
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
    checkCol = -1;
    checkRow = -1;
    invalidMoveCol = -1;
    invalidMoveRow = -1;
    gameResult = "*";
    gameReason = "none";
    alertMessage.clear();
    alertVisible = false;
    alertIsCritical = false;
    alertDurationSec = 0.f;
    remainingTimeSec[0] = DEFAULT_CLOCK_SECONDS;
    remainingTimeSec[1] = DEFAULT_CLOCK_SECONDS;
    lastClockTick = std::chrono::steady_clock::now();
    historyScrollOffset = 0;

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

float ChessBoard::getRemainingTimeSec(PieceColor color) const {
    int idx = static_cast<int>(color);
    return (idx >= 0 && idx < 2) ? remainingTimeSec[idx] : 0.f;
}

void ChessBoard::setRemainingTimeSec(PieceColor color, float seconds) {
    int idx = static_cast<int>(color);
    if (idx >= 0 && idx < 2) {
        remainingTimeSec[idx] = std::max(0.f, seconds);
    }
}

void ChessBoard::advanceClock(float deltaSec) {
    if (isGameOver() || deltaSec <= 0.f) {
        return;
    }
    int idx = static_cast<int>(currentTurn);
    remainingTimeSec[idx] -= deltaSec;
    if (remainingTimeSec[idx] <= 0.f) {
        remainingTimeSec[idx] = 0.f;
        gameResult = (currentTurn == PieceColor::WHITE) ? "0-1" : "1-0";
        gameReason = "timeout";
        clearSelection();
        std::string winner = (currentTurn == PieceColor::WHITE) ? "Pretas vencem" : "Brancas vencem";
        showAlert("Tempo esgotado! " + winner, -1.f, true);
    }
}

void ChessBoard::tickClock() {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastClockTick).count();
    lastClockTick = now;
    advanceClock(dt);
}

std::string ChessBoard::formatClock(float seconds) {
    float clamped = std::max(0.f, seconds);
    int totalSec = static_cast<int>(std::floor(clamped));
    int mins = totalSec / 60;
    int secs = totalSec % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    return std::string(buf);
}

std::string ChessBoard::getTurnIndicatorText() const {
    if (isGameOver()) {
        return "Partida Encerrada";
    }
    return (currentTurn == PieceColor::WHITE) ? "Vez das Brancas" : "Vez das Pretas";
}

std::vector<std::string> ChessBoard::getFormattedMoveHistory() const {
    std::vector<std::string> lines;
    for (size_t i = 0; i < moveHistory.size(); i += 2) {
        int turnNum = static_cast<int>(i / 2) + 1;
        char buf[64];
        if (i + 1 < moveHistory.size()) {
            std::snprintf(buf, sizeof(buf), "%2d.  %-7s  %-7s",
                          turnNum, moveHistory[i].c_str(), moveHistory[i + 1].c_str());
        } else {
            std::snprintf(buf, sizeof(buf), "%2d.  %-7s",
                          turnNum, moveHistory[i].c_str());
        }
        lines.emplace_back(buf);
    }
    return lines;
}

void ChessBoard::scrollHistory(int deltaLines) {
    int totalRows = static_cast<int>((moveHistory.size() + 1) / 2);
    int maxOffset = std::max(0, totalRows - HUD_VISIBLE_HISTORY_ROWS);
    historyScrollOffset = std::max(0, std::min(maxOffset, historyScrollOffset + deltaLines));
}

void ChessBoard::resign() {
    if (isGameOver()) {
        return;
    }
    gameResult = (currentTurn == PieceColor::WHITE) ? "0-1" : "1-0";
    gameReason = "resign";
    clearSelection();
    std::string winner = (currentTurn == PieceColor::WHITE) ? "Pretas vencem" : "Brancas vencem";
    showAlert("Desistencia! " + winner, -1.f, true);
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
    bool isCapture = (pieces[toCol][toRow] != nullptr) ||
                     (type == PieceType::PAWN && fromCol != toCol && !pieces[toCol][toRow]);

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
    int totalRows = static_cast<int>((moveHistory.size() + 1) / 2);
    historyScrollOffset = std::max(0, totalRows - HUD_VISIBLE_HISTORY_ROWS);
    lastClockTick = std::chrono::steady_clock::now();
    currentTurn = (color == PieceColor::WHITE) ? PieceColor::BLACK : PieceColor::WHITE;
    clearSelection();
    invalidMoveCol = -1;
    invalidMoveRow = -1;
    if (isCapture) {
        showAlert("Captura!", 3.0f, false);
    }

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

    // Caso 5: Estado da partida "state check <casa|-> result <res> reason <mot> fen ..."
    if (token == "state") {
        std::string key, val;
        while (ss >> key) {
            if (key == "check" && (ss >> val)) {
                if (val == "-") {
                    checkCol = -1;
                    checkRow = -1;
                } else {
                    parseSquare(val, checkCol, checkRow);
                }
            } else if (key == "result" && (ss >> val)) {
                gameResult = val;
            } else if (key == "reason" && (ss >> val)) {
                gameReason = val;
            } else if (key == "fen") {
                break;
            }
        }

        if (isGameOver()) {
            if (gameReason == "checkmate") {
                std::string winner = (gameResult == "1-0") ? "Brancas vencem" : "Pretas vencem";
                showAlert("Xeque-mate! " + winner, -1.f, true);
            } else if (gameReason == "stalemate") {
                showAlert("Afogamento - Empate", -1.f, true);
            } else if (gameReason == "fifty-moves") {
                showAlert("Empate - Regra dos 50 lances", -1.f, true);
            } else {
                showAlert("Fim de jogo: " + gameResult, -1.f, true);
            }
        } else if (isInCheck()) {
            showAlert("Xeque!", 3.0f, true);
        }
        return;
    }

    // Caso 6: Lance direto no formato UCI (ex: "e2e4", "e7e8q")
    if (token.length() >= 4 && token.length() <= 5) {
        applyUCIMove(token);
        return;
    }
}

std::string ChessBoard::handleMouseClick(float x, float y) {
    if (alertVisible && closeButtonBounds.contains(x, y)) {
        dismissAlert();
        return "";
    }

    if (newGameButtonBounds.contains(x, y)) {
        resetGame();
        return "";
    }

    if (resignButtonBounds.contains(x, y)) {
        resign();
        return "";
    }

    if (isGameOver()) {
        clearSelection();
        return "";
    }

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
        invalidMoveCol = col;
        invalidMoveRow = row;
        showAlert("Movimento invalido!", 3.0f, true);
        return "";
    }

    if (applyUCIMove(candidate)) {
        return candidate;
    }
    return "";
}

bool ChessBoard::playTurnWithEngine(float x, float y, EngineBridge& bridge) {
    if (alertVisible && closeButtonBounds.contains(x, y)) {
        dismissAlert();
        return false;
    }

    if (newGameButtonBounds.contains(x, y)) {
        resetGame();
        bridge.sendCommand("ucinewgame");
        bridge.sendCommand("position startpos");
        if (bridge.sendCommand("legalmoves")) {
            setLegalMoves(bridge.readCommand());
        }
        return false;
    }

    if (resignButtonBounds.contains(x, y)) {
        resign();
        setLegalMoves("legalmoves");
        return false;
    }

    if (isGameOver()) {
        return false;
    }

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

    if (bridge.sendCommand("state")) {
        processUCICommand(bridge.readCommand());
    }

    if (isGameOver()) {
        setLegalMoves("legalmoves");
        return true;
    }

    if (bridge.sendCommand("go")) {
        std::string bestLine = bridge.readCommand();
        processUCICommand(bestLine);
        syncPositionToEngine();
        if (bridge.sendCommand("state")) {
            processUCICommand(bridge.readCommand());
        }
    }

    if (bridge.sendCommand("legalmoves")) {
        setLegalMoves(bridge.readCommand());
    }

    return true;
}

void ChessBoard::drawHud(sf::RenderWindow& window) {
    float boardSize = squareSize * 8.f;
    float hudX = boardSize;

    // Fundo do painel lateral
    sf::RectangleShape hudBg(sf::Vector2f(hudWidth, boardSize));
    hudBg.setPosition(hudX, 0.f);
    hudBg.setFillColor(sf::Color(22, 26, 34));
    window.draw(hudBg);

    // Linha divisoria sutil
    sf::RectangleShape sep(sf::Vector2f(2.f, boardSize));
    sep.setPosition(hudX, 0.f);
    sep.setFillColor(sf::Color(48, 56, 70));
    window.draw(sep);

    // 1. Card de Indicador de Turno (RF13)
    float pad = 16.f;
    float innerW = hudWidth - 2.f * pad;
    drawRoundedRect(window, hudX + pad, 16.f, innerW, 56.f, 10.f,
                    sf::Color(34, 40, 52), 1.5f, sf::Color(70, 82, 104));

    sf::CircleShape turnDot(8.f);
    turnDot.setPosition(hudX + pad + 16.f, 36.f);
    if (isGameOver()) {
        turnDot.setFillColor(sf::Color(225, 80, 80));
    } else if (currentTurn == PieceColor::WHITE) {
        turnDot.setFillColor(sf::Color(245, 245, 245));
    } else {
        turnDot.setFillColor(sf::Color(60, 65, 75));
        turnDot.setOutlineThickness(2.f);
        turnDot.setOutlineColor(sf::Color(200, 205, 215));
    }
    window.draw(turnDot);

    if (fontLoaded) {
        sf::Text turnTxt(getTurnIndicatorText(), alertFont, 18);
        turnTxt.setFillColor(sf::Color::White);
        turnTxt.setPosition(hudX + pad + 42.f, 32.f);
        window.draw(turnTxt);
    }

    // 2. Relogios da Partida (RF07)
    float clkW = (innerW - 12.f) * 0.5f;
    float clkH = 74.f;
    float clkY = 88.f;
    bool whiteActive = !isGameOver() && (currentTurn == PieceColor::WHITE);
    bool blackActive = !isGameOver() && (currentTurn == PieceColor::BLACK);

    drawRoundedRect(window, hudX + pad, clkY, clkW, clkH, 10.f,
                    sf::Color(32, 38, 48), 2.f,
                    whiteActive ? sf::Color(106, 190, 48) : sf::Color(60, 68, 82));
    drawRoundedRect(window, hudX + pad + clkW + 12.f, clkY, clkW, clkH, 10.f,
                    sf::Color(32, 38, 48), 2.f,
                    blackActive ? sf::Color(106, 190, 48) : sf::Color(60, 68, 82));

    if (fontLoaded) {
        sf::Text wLbl("BRANCAS", alertFont, 12);
        wLbl.setFillColor(sf::Color(180, 190, 205));
        wLbl.setPosition(hudX + pad + 14.f, clkY + 10.f);
        window.draw(wLbl);

        sf::Text wTime(formatClock(remainingTimeSec[0]), alertFont, 24);
        wTime.setFillColor(sf::Color::White);
        wTime.setPosition(hudX + pad + 14.f, clkY + 32.f);
        window.draw(wTime);

        sf::Text bLbl("PRETAS", alertFont, 12);
        bLbl.setFillColor(sf::Color(180, 190, 205));
        bLbl.setPosition(hudX + pad + clkW + 26.f, clkY + 10.f);
        window.draw(bLbl);

        sf::Text bTime(formatClock(remainingTimeSec[1]), alertFont, 24);
        bTime.setFillColor(sf::Color::White);
        bTime.setPosition(hudX + pad + clkW + 26.f, clkY + 32.f);
        window.draw(bTime);
    }

    // 3. Historico de Lances (RF20)
    float histY = 178.f;
    float histH = resignButtonBounds.top - histY - 16.f;
    drawRoundedRect(window, hudX + pad, histY, innerW, histH, 10.f,
                    sf::Color(28, 33, 43), 1.5f, sf::Color(55, 65, 82));

    if (fontLoaded) {
        sf::Text histTitle("Historico de Lances", alertFont, 15);
        histTitle.setFillColor(sf::Color(190, 200, 220));
        histTitle.setPosition(hudX + pad + 14.f, histY + 12.f);
        window.draw(histTitle);

        sf::RectangleShape hLine(sf::Vector2f(innerW - 24.f, 1.f));
        hLine.setPosition(hudX + pad + 12.f, histY + 38.f);
        hLine.setFillColor(sf::Color(55, 65, 82));
        window.draw(hLine);

        auto lines = getFormattedMoveHistory();
        int startRow = std::max(0, std::min(historyScrollOffset, static_cast<int>(lines.size())));
        int endRow = std::min(static_cast<int>(lines.size()), startRow + HUD_VISIBLE_HISTORY_ROWS);

        for (int r = startRow; r < endRow; ++r) {
            float rowY = histY + 46.f + (r - startRow) * 28.f;
            if ((r % 2) == 1) {
                sf::RectangleShape stripe(sf::Vector2f(innerW - 16.f, 26.f));
                stripe.setPosition(hudX + pad + 8.f, rowY);
                stripe.setFillColor(sf::Color(36, 42, 54, 150));
                window.draw(stripe);
            }
            sf::Text lineTxt(lines[r], alertFont, 15);
            lineTxt.setFillColor(sf::Color(230, 235, 245));
            lineTxt.setPosition(hudX + pad + 16.f, rowY + 3.f);
            window.draw(lineTxt);
        }
    }

    // 4. Botoes de Controle: Desistir e Nova Partida
    drawRoundedRect(window,
                    resignButtonBounds.left, resignButtonBounds.top,
                    resignButtonBounds.width, resignButtonBounds.height, 9.f,
                    isGameOver() ? sf::Color(70, 50, 50) : sf::Color(175, 52, 52),
                    1.5f, sf::Color(225, 95, 95));

    drawRoundedRect(window,
                    newGameButtonBounds.left, newGameButtonBounds.top,
                    newGameButtonBounds.width, newGameButtonBounds.height, 9.f,
                    sf::Color(46, 125, 75),
                    1.5f, sf::Color(95, 195, 130));

    if (fontLoaded) {
        sf::Text resTxt("Desistir", alertFont, 15);
        resTxt.setFillColor(sf::Color::White);
        sf::FloatRect rb = resTxt.getLocalBounds();
        resTxt.setPosition(
            resignButtonBounds.left + (resignButtonBounds.width - rb.width) * 0.5f - rb.left,
            resignButtonBounds.top + (resignButtonBounds.height - rb.height) * 0.5f - rb.top
        );
        window.draw(resTxt);

        sf::Text newTxt("Nova Partida", alertFont, 15);
        newTxt.setFillColor(sf::Color::White);
        sf::FloatRect nb = newTxt.getLocalBounds();
        newTxt.setPosition(
            newGameButtonBounds.left + (newGameButtonBounds.width - nb.width) * 0.5f - nb.left,
            newGameButtonBounds.top + (newGameButtonBounds.height - nb.height) * 0.5f - nb.top
        );
        window.draw(newTxt);
    }
}

void ChessBoard::draw(sf::RenderWindow &window) {
    tickClock();
    updateAlert();

    // Desenha as 64 casas do tabuleiro
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            window.draw(squares[i][j]);
        }
    }

    // Destaca rei em xeque (vermelho persistente enquanto estiver em xeque)
    if (isInCheck()) {
        sf::RectangleShape checkRect(sf::Vector2f(squareSize, squareSize));
        checkRect.setPosition(checkCol * squareSize, checkRow * squareSize);
        checkRect.setFillColor(sf::Color(220, 45, 45, 175));
        window.draw(checkRect);
    }

    // Destaca casa de tentativa de movimento invalido enquanto o pop-up estiver ativo
    if (invalidMoveCol >= 0 && invalidMoveRow >= 0) {
        sf::RectangleShape invRect(sf::Vector2f(squareSize, squareSize));
        invRect.setPosition(invalidMoveCol * squareSize, invalidMoveRow * squareSize);
        invRect.setFillColor(sf::Color(235, 95, 35, 160));
        window.draw(invRect);
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

    // Desenha o pop-up moderno (bordas arredondadas + sombra + fade-in/fade-out + botao [X])
    if (alertVisible && !alertMessage.empty()) {
        float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - alertStart).count();
        const float fadeInSec = 0.22f;
        const float fadeOutSec = 0.40f;
        float alphaFactor = 1.0f;
        if (elapsed < fadeInSec) {
            alphaFactor = elapsed / fadeInSec;
        } else if (alertDurationSec > 0.f && elapsed > alertDurationSec - fadeOutSec) {
            alphaFactor = (alertDurationSec - elapsed) / fadeOutSec;
        }
        alphaFactor = std::max(0.f, std::min(1.f, alphaFactor));

        float boardSize = squareSize * 8.f;
        bool gameOver = isGameOver();
        float cardW = gameOver ? boardSize * 0.68f : boardSize * 0.54f;
        float cardH = gameOver ? 120.f : 46.f;
        float slideY = gameOver ? 0.f : (1.0f - alphaFactor) * -10.f;
        float cardX = (boardSize - cardW) * 0.5f;
        float cardY = (gameOver ? (boardSize - cardH) * 0.5f : 12.f) + slideY;

        // Sombra suave
        drawRoundedRect(window, cardX + 3.f, cardY + 5.f, cardW, cardH, 14.f,
                        sf::Color(0, 0, 0, scaleAlpha(110, alphaFactor)));

        // Card principal arredondado
        sf::Color bg = alertIsCritical
            ? sf::Color(36, 20, 24, scaleAlpha(235, alphaFactor))
            : sf::Color(24, 30, 40, scaleAlpha(232, alphaFactor));
        sf::Color border = alertIsCritical
            ? sf::Color(230, 80, 80, scaleAlpha(255, alphaFactor))
            : sf::Color(110, 175, 240, scaleAlpha(255, alphaFactor));
        drawRoundedRect(window, cardX, cardY, cardW, cardH, 14.f, bg, 2.f, border);

        // Botao [X] arredondado
        drawRoundedRect(window,
                        closeButtonBounds.left, closeButtonBounds.top + slideY,
                        closeButtonBounds.width, closeButtonBounds.height, 7.f,
                        sf::Color(185, 60, 60, scaleAlpha(225, alphaFactor)));

        if (fontLoaded) {
            sf::Color txtWhite(255, 255, 255, scaleAlpha(255, alphaFactor));
            sf::Color txtSub(210, 215, 225, scaleAlpha(255, alphaFactor));

            sf::Text title(alertMessage, alertFont, gameOver ? 24 : 18);
            title.setFillColor(txtWhite);
            sf::FloatRect tb = title.getLocalBounds();
            title.setPosition(
                cardX + (cardW - closeButtonBounds.width - 16.f - tb.width) * 0.5f - tb.left,
                gameOver ? (cardY + 26.f) : (cardY + (cardH - tb.height) * 0.5f - tb.top)
            );
            window.draw(title);

            if (gameOver) {
                sf::Text sub("Pressione R para nova partida", alertFont, 15);
                sub.setFillColor(txtSub);
                sf::FloatRect sb = sub.getLocalBounds();
                sub.setPosition(cardX + (cardW - sb.width) * 0.5f - sb.left, cardY + 74.f);
                window.draw(sub);
            }

            sf::Text xTxt("X", alertFont, 15);
            xTxt.setFillColor(txtWhite);
            sf::FloatRect xb = xTxt.getLocalBounds();
            xTxt.setPosition(
                closeButtonBounds.left + (closeButtonBounds.width - xb.width) * 0.5f - xb.left,
                closeButtonBounds.top + slideY + (closeButtonBounds.height - xb.height) * 0.5f - xb.top
            );
            window.draw(xTxt);
        }
    }

    drawHud(window);
}
