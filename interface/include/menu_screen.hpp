#pragma once

#include <SFML/Graphics.hpp>
#include "game_config.hpp"

enum class MenuAction {
    NONE,
    GO_TO_SETUP,
    BACK_TO_MENU,
    START_GAME,
    EXIT_APP
};

class MenuScreen {
private:
    float windowWidth;
    float windowHeight;
    sf::Vector2f mousePos;

    // Menu Principal
    sf::FloatRect playButtonBounds;
    sf::FloatRect exitButtonBounds;

    // Configuracao da Partida
    sf::FloatRect modeAiBounds;
    sf::FloatRect modeLocalBounds;

    sf::FloatRect diffEasyBounds;
    sf::FloatRect diffMediumBounds;
    sf::FloatRect diffHardBounds;

    sf::FloatRect colorWhiteBounds;
    sf::FloatRect colorBlackBounds;

    sf::FloatRect timeBounds[7];
    static constexpr int TIME_OPTIONS[7] = { 3, 5, 10, 15, 20, 30, 0 };

    sf::FloatRect backButtonBounds;
    sf::FloatRect startGameButtonBounds;

    void initLayout();
    void drawMainMenu(sf::RenderWindow& window,
                      const sf::Font& font,
                      bool fontLoaded) const;
    void drawGameSetup(sf::RenderWindow& window,
                       const GameConfig& config,
                       const sf::Font& font,
                       bool fontLoaded) const;
    void drawOptionButton(sf::RenderWindow& window,
                          const sf::FloatRect& bounds,
                          const std::string& label,
                          bool selected,
                          const sf::Font& font,
                          bool fontLoaded,
                          unsigned int fontSize = 16) const;

public:
    MenuScreen(float windowWidth, float windowHeight);

    void updateHover(float x, float y);
    MenuAction handleClick(AppScreen screen, float x, float y, GameConfig& config) const;
    void draw(sf::RenderWindow& window,
              AppScreen screen,
              const GameConfig& config,
              const sf::Font& font,
              bool fontLoaded) const;
};
