#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "piece.hpp"
#include "engine_bridge.hpp"

#define DEFAULT_BOARD_SIDE_SIZE 800

class ChessBoard {
private:
    float squareSize;
    
    sf::RectangleShape squares[8][8];

    // Matriz de texturas [Cor: 0=WHITE, 1=BLACK][Tipo: 0=PAWN..5=KING]
    sf::Texture textures[2][6];

    // Matriz 8x8 de ponteiros inteligentes para as peças (nullptr se a casa estiver vazia)
    std::unique_ptr<Piece> pieces[8][8];

    int selectedCol;
    int selectedRow;
    PieceColor currentTurn;
    std::vector<std::string> moveHistory;
    std::vector<std::string> legalMoves;
    std::vector<std::pair<int, int>> highlightedSquares;

    void initSquares();
    void loadTextures();
    void placePiece(PieceType type, PieceColor color, int col, int row);
    void updateHighlightsForSelected();

public:
    explicit ChessBoard(float boardSideSize = DEFAULT_BOARD_SIDE_SIZE);

    float getSquareSize() const { return squareSize; }

    // Converte notação algébrica (ex: "e2") em coordenadas de matriz (col=4, row=6)
    bool parseSquare(const std::string& sqStr, int& outCol, int& outRow) const;

    // Converte coordenadas de matriz (col, row) em notação algébrica (ex: "e2")
    std::string squareToCoord(int col, int row) const;

    // Converte coordenadas em pixels (x, y) para casa do tabuleiro (col, row)
    bool pixelToSquare(float x, float y, int& outCol, int& outRow) const;

    // Retorna ponteiro constante para a peça na casa (col, row), ou nullptr se vazia/inválida
    const Piece* getPiece(int col, int row) const;

    PieceColor getCurrentTurn() const { return currentTurn; }
    int getSelectedCol() const { return selectedCol; }
    int getSelectedRow() const { return selectedRow; }
    const std::vector<std::string>& getMoveHistory() const { return moveHistory; }
    const std::vector<std::string>& getLegalMoves() const { return legalMoves; }
    const std::vector<std::pair<int, int>>& getHighlightedSquares() const { return highlightedSquares; }

    void clearSelection();

    // Recebe a linha "legalmoves e2e4 d2d4 ..." do motor e atualiza a lista de lances legais
    void setLegalMoves(const std::string& legalMovesLine);

    bool isMoveLegal(const std::string& moveStr) const;

    void clearBoard();

    // Posiciona as 32 peças na configuração inicial do xadrez
    void setupInitialPosition();

    // Aplica um lance individual no formato UCI (ex: "e2e4", "e7e8q")
    bool applyUCIMove(const std::string& moveStr);

    // Processa comandos completos no protocolo UCI (ex: "position startpos moves e2e4 e7e5", "bestmove e7e5")
    void processUCICommand(const std::string& command);

    // Processa um clique do mouse em pixels (x, y). Retorna o lance UCI executado (ex: "e2e4") ou "" se apenas selecionou/inválido
    std::string handleMouseClick(float x, float y);

    // Orquestra o clique do jogador integrado ao motor via pipe (valida com legalmoves, envia position + go e aplica bestmove)
    bool playTurnWithEngine(float x, float y, EngineBridge& bridge);

    void draw(sf::RenderWindow &window);
};
