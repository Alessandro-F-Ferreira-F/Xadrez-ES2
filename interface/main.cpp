#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.hpp"
#include "engine_bridge.hpp"

#define BOARD_SIDE_SIZE 800

int main()
{
    sf::RenderWindow window(sf::VideoMode(BOARD_SIDE_SIZE, BOARD_SIDE_SIZE), "Xadrez - ES2");

    ChessBoard board(BOARD_SIDE_SIZE);
    EngineBridge bridge;

    if (bridge.start("../engine/build/main.out")) {
        bridge.sendCommand("uci");
        for (int i = 0; i < 10; ++i) {
            if (bridge.readCommand() == "uciok") break;
        }
        bridge.sendCommand("position startpos");
        bridge.sendCommand("legalmoves");
        board.setLegalMoves(bridge.readCommand());
    } else {
        std::cerr << "[Aviso] Nao foi possivel iniciar ../engine/build/main.out" << std::endl;
    }

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::MouseButtonPressed &&
                       event.mouseButton.button == sf::Mouse::Left) {
                board.playTurnWithEngine(
                    static_cast<float>(event.mouseButton.x),
                    static_cast<float>(event.mouseButton.y),
                    bridge
                );
            } else if (event.type == sf::Event::KeyPressed &&
                       event.key.code == sf::Keyboard::R) {
                board.resetGame();
                bridge.sendCommand("ucinewgame");
                bridge.sendCommand("position startpos");
                bridge.sendCommand("legalmoves");
                board.setLegalMoves(bridge.readCommand());
            }
        }

        window.clear(sf::Color::Black);

        board.draw(window);

        window.display();
    }

    bridge.stop();
    return 0;
}
