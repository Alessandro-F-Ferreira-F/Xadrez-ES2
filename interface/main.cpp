#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.hpp"
#include "engine_bridge.hpp"

#define BOARD_SIDE_SIZE 800

int main()
{
    ChessBoard board(BOARD_SIDE_SIZE, DEFAULT_HUD_WIDTH);
    EngineBridge bridge;

    if (bridge.start("../Backend/build/main.out")) {
        bridge.sendCommand("uci");
        for (int i = 0; i < 10; ++i) {
            if (bridge.readCommand() == "uciok") break;
        }
        bridge.sendCommand("position startpos");
        bridge.sendCommand("legalmoves");
        board.setLegalMoves(bridge.readCommand());
    } else {
        std::cerr << "[Aviso] Nao foi possivel iniciar ../Backend/build/main.out" << std::endl;
    }

    sf::RenderWindow window(
        sf::VideoMode(BOARD_SIDE_SIZE + DEFAULT_HUD_WIDTH, BOARD_SIDE_SIZE),
        "Xadrez - ES2"
    );

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::MouseButtonPressed &&
                       event.mouseButton.button == sf::Mouse::Left) {
                // Nao bloqueia: so envia o go; o bestmove chega em pollEngine
                board.startTurnWithEngine(
                    static_cast<float>(event.mouseButton.x),
                    static_cast<float>(event.mouseButton.y),
                    bridge
                );
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                board.scrollHistory(event.mouseWheelScroll.delta > 0 ? -1 : 1);
            } else if (event.type == sf::Event::KeyPressed &&
                       event.key.code == sf::Keyboard::R &&
                       !board.isAwaitingEngine()) {
                board.resetGame();
                bridge.sendCommand("ucinewgame");
                bridge.sendCommand("position startpos");
                bridge.sendCommand("legalmoves");
                board.setLegalMoves(bridge.readCommand());
            }
        }

        // Aplica o lance da IA quando o bestmove chegar, sem travar a janela
        board.pollEngine(bridge);

        window.clear(sf::Color::Black);

        board.draw(window);

        window.display();
    }

    bridge.stop();
    return 0;
}
