#include <SFML/Graphics.hpp>
#include <iostream>
#include <filesystem>
#include <vector>
#include "board.hpp"
#include "engine_bridge.hpp"

#define BOARD_SIDE_SIZE 800

static std::string findEngineExecutable(int argc, char **argv) {
#ifdef _WIN32
    const char *relativeName = "main.exe";
#else
    const char *relativeName = "main.out";
#endif

    namespace fs = std::filesystem;
    std::vector<fs::path> candidates;
    candidates.emplace_back(fs::path("engine") / "build" / relativeName);
    candidates.emplace_back(fs::path("..") / "engine" / "build" / relativeName);
    candidates.emplace_back(fs::path("..") / ".." / "engine" / "build" / relativeName);

    if (std::filesystem::exists(fs::current_path() / candidates[0])) {
        return (fs::current_path() / candidates[0]).string();
    }

    std::error_code ec;
    fs::path exePath = argc > 0 ? fs::absolute(fs::path(argv[0]), ec) : fs::absolute(fs::path(), ec);
    fs::path exeDir = exePath.parent_path();
    candidates.clear();
    candidates.emplace_back(exeDir / "engine" / "build" / relativeName);
    candidates.emplace_back(exeDir / ".." / "engine" / "build" / relativeName);
    candidates.emplace_back(exeDir / ".." / ".." / "engine" / "build" / relativeName);
    candidates.emplace_back(exeDir / ".." / ".." / ".." / "engine" / "build" / relativeName);

    for (const fs::path &candidate : candidates) {
        if (fs::exists(candidate, ec)) {
            return fs::absolute(candidate, ec).string();
        }
    }

    return candidates[1].string();
}

int main(int argc, char **argv)
{
    sf::RenderWindow window(
        sf::VideoMode(BOARD_SIDE_SIZE + DEFAULT_HUD_WIDTH, BOARD_SIDE_SIZE),
        "Xadrez - ES2"
    );

    ChessBoard board(BOARD_SIDE_SIZE, DEFAULT_HUD_WIDTH);
    EngineBridge bridge;
    std::string enginePath = findEngineExecutable(argc, argv);

    if (bridge.start(enginePath)) {
        bridge.sendCommand("uci");
        for (int i = 0; i < 10; ++i) {
            if (bridge.readCommand() == "uciok") break;
        }
        bridge.sendCommand("position startpos");
        bridge.sendCommand("legalmoves");
        board.setLegalMoves(bridge.readCommand());
    } else {
        std::cerr << "[Aviso] Nao foi possivel iniciar o motor: " << enginePath << std::endl;
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
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                board.scrollHistory(event.mouseWheelScroll.delta > 0 ? -1 : 1);
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
