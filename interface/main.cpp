#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.hpp"

#define BOARD_SIDE_SIZE 800

int main()
{
    sf::RenderWindow window(sf::VideoMode(BOARD_SIDE_SIZE, BOARD_SIDE_SIZE), "Xadrez");

    ChessBoard board(BOARD_SIDE_SIZE);

    // board.processUCICommand("position startpos");

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
