#include <SFML/Graphics.hpp>
#include <iostream>
#include "board.hpp"
#include "engine_bridge.hpp"
#include "game_config.hpp"
#include "menu_screen.hpp"
#include "ui_helpers.hpp"

#define BOARD_SIDE_SIZE 800

int main()
{
    ChessBoard board(BOARD_SIDE_SIZE, DEFAULT_HUD_WIDTH);
    EngineBridge bridge;

#ifdef _WIN32
    const std::string enginePath = "../Backend/build/main.exe";
#else
    const std::string enginePath = "../Backend/build/main.out";
#endif

    if (bridge.start(enginePath)) {
        bridge.sendCommand("uci");
        for (int i = 0; i < 10; ++i) {
            if (bridge.readCommand() == "uciok") break;
        }
    } else {
        std::cerr << "[Aviso] Nao foi possivel iniciar " << enginePath << std::endl;
    }

    const float windowWidth = static_cast<float>(BOARD_SIDE_SIZE + DEFAULT_HUD_WIDTH);
    const float windowHeight = static_cast<float>(BOARD_SIDE_SIZE);

    sf::ContextSettings contextSettings;
    contextSettings.antialiasingLevel = 4;

    sf::RenderWindow window;
    bool isFullscreen = false;

    auto configureWindow = [&](bool fullscreen) {
        isFullscreen = fullscreen;
        if (isFullscreen) {
            window.create(sf::VideoMode::getDesktopMode(), "Xadrez - ES2",
                          sf::Style::Fullscreen, contextSettings);
        } else {
            window.create(
                sf::VideoMode(static_cast<unsigned int>(windowWidth), static_cast<unsigned int>(windowHeight)),
                "Xadrez - ES2",
                sf::Style::Default,
                contextSettings
            );
        }
        window.setFramerateLimit(60);
        sf::Vector2u sz = window.getSize();
        window.setView(ui::createLetterboxView(windowWidth, windowHeight, sz.x, sz.y));
    };

    configureWindow(false);

    board.setAnimationsEnabled(true);
    board.setAsyncAIEnabled(true);

    MenuScreen menu(windowWidth, windowHeight);
    AppScreen currentScreen = AppScreen::MAIN_MENU;
    GameConfig gameConfig;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::Resized) {
                window.setView(ui::createLetterboxView(
                    windowWidth, windowHeight, event.size.width, event.size.height));
            } else if (event.type == sf::Event::MouseMoved) {
                sf::Vector2f worldPos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseMove.x, event.mouseMove.y));
                menu.updateHover(worldPos.x, worldPos.y);
                board.setMousePosition(worldPos.x, worldPos.y);
            } else if (event.type == sf::Event::KeyPressed &&
                       event.key.code == sf::Keyboard::F11) {
                configureWindow(!isFullscreen);
            } else if (event.type == sf::Event::KeyPressed &&
                       event.key.code == sf::Keyboard::Escape &&
                       currentScreen == AppScreen::GAME_SETUP) {
                currentScreen = AppScreen::MAIN_MENU;
            } else if (event.type == sf::Event::MouseWheelScrolled &&
                       currentScreen == AppScreen::PLAYING) {
                board.scrollHistory(event.mouseWheelScroll.delta > 0 ? -1 : 1);
            } else if (event.type == sf::Event::MouseButtonPressed &&
                       event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f worldPos = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                float mx = worldPos.x;
                float my = worldPos.y;

                if (currentScreen == AppScreen::MAIN_MENU || currentScreen == AppScreen::GAME_SETUP) {
                    MenuAction action = menu.handleClick(currentScreen, mx, my, gameConfig);
                    if (action == MenuAction::GO_TO_SETUP) {
                        currentScreen = AppScreen::GAME_SETUP;
                    } else if (action == MenuAction::BACK_TO_MENU) {
                        currentScreen = AppScreen::MAIN_MENU;
                    } else if (action == MenuAction::START_GAME) {
                        board.startMatchWithEngine(gameConfig, bridge);
                        currentScreen = AppScreen::PLAYING;
                    } else if (action == MenuAction::EXIT_APP) {
                        window.close();
                    }
                } else if (currentScreen == AppScreen::PLAYING) {
                    if (board.isGameOver()) {
                        if (board.isAlertVisible() && board.getCloseButtonBounds().contains(mx, my)) {
                            board.dismissAlert();
                        } else {
                            PostGameAction postAction = board.checkPostGameButtonClick(mx, my);
                            if (postAction == PostGameAction::RESTART_SAME_CONFIG) {
                                board.startMatchWithEngine(gameConfig, bridge);
                            } else if (postAction == PostGameAction::OPEN_SETUP) {
                                board.waitForPendingAI();
                                currentScreen = AppScreen::GAME_SETUP;
                            } else if (postAction == PostGameAction::RETURN_TO_MENU) {
                                board.waitForPendingAI();
                                currentScreen = AppScreen::MAIN_MENU;
                            }
                        }
                    } else {
                        board.playTurnWithEngine(mx, my, bridge);
                    }
                }
            }
        }

        if (currentScreen == AppScreen::PLAYING) {
            board.updateAI(bridge);
        }

        window.clear(sf::Color(22, 26, 34));

        if (currentScreen == AppScreen::PLAYING) {
            board.draw(window);
        } else {
            menu.draw(window, currentScreen, gameConfig, board.getFont(), board.isFontLoaded());
        }

        window.display();

        if (currentScreen == AppScreen::PLAYING && board.needsAIMove()) {
            board.executeAIMove(bridge);
        }
    }

    board.waitForPendingAI();
    bridge.stop();
    return 0;
}
