#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "piece.hpp"

#define DEFAULT_BOARD_SIDE_SIZE 800

class ChessBoard {
private:
    float squareSize;
    
    sf::RectangleShape squares[8][8];

    // Matriz de texturas [Cor: 0=WHITE, 1=BLACK][Tipo: 0=PAWN..5=KING]
    sf::Texture textures[2][6];

    // Matriz 8x8 de ponteiros inteligentes para as peças (nullptr se a casa estiver vazia)
    std::unique_ptr<Piece> pieces[8][8];

    void initSquares();
    void loadTextures();
    void placePiece(PieceType type, PieceColor color, int col, int row);

    // Converte notação algébrica (ex: "e2") em coordenadas de matriz (col=4, row=6)
    bool parseSquare(const std::string& sqStr, int& outCol, int& outRow) const;

public:
    explicit ChessBoard(float boardSideSize = DEFAULT_BOARD_SIDE_SIZE);

    float getSquareSize() const { return squareSize; }

    void clearBoard();

    // Posiciona as 32 peças na configuração inicial do xadrez
    void setupInitialPosition();

    // Aplica um lance individual no formato UCI (ex: "e2e4", "e7e8q")
    bool applyUCIMove(const std::string& moveStr);

    // Processa comandos completos no protocolo UCI (ex: "position startpos moves e2e4 e7e5")
    void processUCICommand(const std::string& command);

    void draw(sf::RenderWindow &window);
};
