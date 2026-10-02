#pragma once

#include <SFML/Graphics.hpp>

enum class PieceType {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5
};

enum class PieceColor {
    WHITE = 0,
    BLACK = 1
};

class Piece {
private:
    PieceType type;
    PieceColor color;
    int col; // 0 a 7
    int row; // 0 a 7
    sf::Sprite sprite;

public:
    Piece(PieceType type, PieceColor color, const sf::Texture& texture, int col, int row, float squareSize);

    void setGridPosition(int col, int row, float squareSize);

    int getCol() const { return col; }
    int getRow() const { return row; }
    PieceType getType() const { return type; }
    PieceColor getColor() const { return color; }

    void draw(sf::RenderWindow& window) const;
};
