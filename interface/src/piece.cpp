#include "piece.hpp"

Piece::Piece(PieceType type, PieceColor color, const sf::Texture& texture, int col, int row, float squareSize)
    : type(type), color(color), col(col), row(row) {
    
    sprite.setTexture(texture);

    // Ajusta a escala proporcionalmente para caber exatamente em uma casa
    float scaleX = squareSize / texture.getSize().x;
    float scaleY = squareSize / texture.getSize().y;
    sprite.setScale(scaleX, scaleY);

    setGridPosition(col, row, squareSize);
}

void Piece::setGridPosition(int newCol, int newRow, float squareSize) {
    col = newCol;
    row = newRow;
    // Converte a coordenada do grid (col, row) para pixels na tela
    sprite.setPosition(col * squareSize, row * squareSize);
}

void Piece::draw(sf::RenderWindow& window) const {
    window.draw(sprite);
}
