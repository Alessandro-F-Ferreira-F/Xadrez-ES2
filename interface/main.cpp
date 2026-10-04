#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.hpp"

#define BOARD_SIDE_SIZE 800

int main()
{
    sf::RenderWindow window(sf::VideoMode(BOARD_SIDE_SIZE, BOARD_SIDE_SIZE), "Xadrez");

    ChessBoard board(BOARD_SIDE_SIZE);

    // Teste simulando o comando UCI com os lances do Gambito do Rei Aceito (1. e4 e5 2. f4 exf4):
    board.processUCICommand("position startpos moves e2e4 e7e5 f2f4 e5f4");

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);

        board.draw(window);

        window.display();
    }

    return 0;
}
