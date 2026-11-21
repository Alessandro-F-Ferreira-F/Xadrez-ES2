#include "menu_screen.hpp"
#include "ui_helpers.hpp"

#include <string>

constexpr int MenuScreen::TIME_OPTIONS[7];

MenuScreen::MenuScreen(float windowWidth, float windowHeight)
    : windowWidth(windowWidth),
      windowHeight(windowHeight),
      mousePos(-1.f, -1.f) {
    initLayout();
}

void MenuScreen::initLayout() {
    // Menu Principal
    const float menuBtnW = 320.f;
    const float menuBtnH = 56.f;
    const float menuBtnX = (windowWidth - menuBtnW) * 0.5f;
    playButtonBounds = sf::FloatRect(menuBtnX, 376.f, menuBtnW, menuBtnH);
    exitButtonBounds = sf::FloatRect(menuBtnX, 450.f, menuBtnW, menuBtnH);

    // Configuracao da Partida
    const float cardW = 680.f;
    const float cardX = (windowWidth - cardW) * 0.5f;
    const float innerX = cardX + 36.f;
    const float innerW = cardW - 72.f;

    // 1. Modo de Jogo (2 botoes)
    const float modeW = (innerW - 14.f) * 0.5f;
    modeAiBounds = sf::FloatRect(innerX, 174.f, modeW, 48.f);
    modeLocalBounds = sf::FloatRect(innerX + modeW + 14.f, 174.f, modeW, 48.f);

    // 2. Dificuldade da IA (3 botoes)
    const float diffW = (innerW - 24.f) / 3.f;
    diffEasyBounds = sf::FloatRect(innerX, 274.f, diffW, 44.f);
    diffMediumBounds = sf::FloatRect(innerX + diffW + 12.f, 274.f, diffW, 44.f);
    diffHardBounds = sf::FloatRect(innerX + (diffW + 12.f) * 2.f, 274.f, diffW, 44.f);

    // 3. Cor do Jogador (2 botoes)
    const float colW = (innerW - 14.f) * 0.5f;
    colorWhiteBounds = sf::FloatRect(innerX, 370.f, colW, 44.f);
    colorBlackBounds = sf::FloatRect(innerX + colW + 14.f, 370.f, colW, 44.f);

    // 4. Tempo de Partida (4 botoes na linha 1 + 3 botoes na linha 2)
    const float timeW4 = (innerW - 36.f) / 4.f;
    for (int i = 0; i < 4; ++i) {
        timeBounds[i] = sf::FloatRect(innerX + i * (timeW4 + 12.f), 472.f, timeW4, 42.f);
    }
    const float timeW3 = (innerW - 24.f) / 3.f;
    for (int i = 0; i < 3; ++i) {
        timeBounds[4 + i] = sf::FloatRect(innerX + i * (timeW3 + 12.f), 524.f, timeW3, 42.f);
    }

    // 5. Rodape (Voltar / Iniciar Partida)
    const float footW = (innerW - 16.f) * 0.5f;
    backButtonBounds = sf::FloatRect(innerX, 608.f, footW, 50.f);
    startGameButtonBounds = sf::FloatRect(innerX + footW + 16.f, 608.f, footW, 50.f);
}

void MenuScreen::updateHover(float x, float y) {
    mousePos = sf::Vector2f(x, y);
}

MenuAction MenuScreen::handleClick(AppScreen screen, float x, float y, GameConfig& config) const {
    if (screen == AppScreen::MAIN_MENU) {
        if (playButtonBounds.contains(x, y)) {
            return MenuAction::GO_TO_SETUP;
        }
        if (exitButtonBounds.contains(x, y)) {
            return MenuAction::EXIT_APP;
        }
        return MenuAction::NONE;
    }

    if (screen == AppScreen::GAME_SETUP) {
        if (modeAiBounds.contains(x, y)) {
            config.mode = GameMode::VS_AI;
            return MenuAction::NONE;
        }
        if (modeLocalBounds.contains(x, y)) {
            config.mode = GameMode::LOCAL_PVP;
            return MenuAction::NONE;
        }

        if (config.mode == GameMode::VS_AI) {
            if (diffEasyBounds.contains(x, y)) {
                config.difficulty = AIDifficulty::EASY;
                return MenuAction::NONE;
            }
            if (diffMediumBounds.contains(x, y)) {
                config.difficulty = AIDifficulty::MEDIUM;
                return MenuAction::NONE;
            }
            if (diffHardBounds.contains(x, y)) {
                config.difficulty = AIDifficulty::HARD;
                return MenuAction::NONE;
            }
            if (colorWhiteBounds.contains(x, y)) {
                config.playerColor = PieceColor::WHITE;
                return MenuAction::NONE;
            }
            if (colorBlackBounds.contains(x, y)) {
                config.playerColor = PieceColor::BLACK;
                return MenuAction::NONE;
            }
        }

        for (int i = 0; i < 7; ++i) {
            if (timeBounds[i].contains(x, y)) {
                config.timeLimitMinutes = TIME_OPTIONS[i];
                return MenuAction::NONE;
            }
        }

        if (backButtonBounds.contains(x, y)) {
            return MenuAction::BACK_TO_MENU;
        }
        if (startGameButtonBounds.contains(x, y)) {
            return MenuAction::START_GAME;
        }
    }

    return MenuAction::NONE;
}

void MenuScreen::drawOptionButton(sf::RenderWindow& window,
                                  const sf::FloatRect& bounds,
                                  const std::string& label,
                                  bool selected,
                                  const sf::Font& font,
                                  bool fontLoaded,
                                  unsigned int fontSize) const {
    bool hovered = bounds.contains(mousePos);
    sf::Color fill = selected
        ? (hovered ? sf::Color(56, 146, 88) : sf::Color(46, 125, 75))
        : (hovered ? sf::Color(50, 59, 76) : sf::Color(36, 42, 54));
    sf::Color border = selected
        ? sf::Color(115, 210, 145)
        : (hovered ? sf::Color(115, 130, 158) : sf::Color(68, 78, 98));

    ui::drawRoundedRect(window, bounds.left, bounds.top, bounds.width, bounds.height,
                        10.f, fill, selected ? 2.f : 1.5f, border);

    if (fontLoaded) {
        ui::drawCenteredText(window, label, font, fontSize, sf::Color::White, bounds);
    }
}

void MenuScreen::drawMainMenu(sf::RenderWindow& window,
                              const sf::Font& font,
                              bool fontLoaded) const {
    const float cardW = 500.f;
    const float cardH = 340.f;
    const float cardX = (windowWidth - cardW) * 0.5f;
    const float cardY = (windowHeight - cardH) * 0.5f;

    // Sombra suave e card central
    ui::drawRoundedRect(window, cardX + 4.f, cardY + 6.f, cardW, cardH, 18.f, sf::Color(0, 0, 0, 115));
    ui::drawRoundedRect(window, cardX, cardY, cardW, cardH, 18.f,
                        sf::Color(28, 33, 44), 2.f, sf::Color(64, 75, 96));

    if (fontLoaded) {
        ui::drawCenteredText(window, "Xadrez", font, 64, sf::Color(245, 247, 250),
                             sf::FloatRect(cardX, cardY + 40.f, cardW, 82.f));
    }

    bool hoverPlay = playButtonBounds.contains(mousePos);
    bool hoverExit = exitButtonBounds.contains(mousePos);

    ui::drawRoundedRect(window,
                        playButtonBounds.left, playButtonBounds.top,
                        playButtonBounds.width, playButtonBounds.height, 12.f,
                        hoverPlay ? sf::Color(58, 150, 92) : sf::Color(46, 125, 75),
                        2.f, sf::Color(106, 200, 138));

    ui::drawRoundedRect(window,
                        exitButtonBounds.left, exitButtonBounds.top,
                        exitButtonBounds.width, exitButtonBounds.height, 12.f,
                        hoverExit ? sf::Color(195, 62, 62) : sf::Color(160, 48, 48),
                        1.5f, sf::Color(220, 95, 95));

    if (fontLoaded) {
        ui::drawCenteredText(window, "Jogar", font, 22, sf::Color::White, playButtonBounds);
        ui::drawCenteredText(window, "Sair", font, 20, sf::Color::White, exitButtonBounds);
    }
}

void MenuScreen::drawGameSetup(sf::RenderWindow& window,
                               const GameConfig& config,
                               const sf::Font& font,
                               bool fontLoaded) const {
    const float cardW = 680.f;
    const float cardH = 630.f;
    const float cardX = (windowWidth - cardW) * 0.5f;
    const float cardY = (windowHeight - cardH) * 0.5f;
    const float innerX = cardX + 36.f;
    const float innerW = cardW - 72.f;

    ui::drawRoundedRect(window, cardX + 4.f, cardY + 6.f, cardW, cardH, 18.f, sf::Color(0, 0, 0, 115));
    ui::drawRoundedRect(window, cardX, cardY, cardW, cardH, 18.f,
                        sf::Color(28, 33, 44), 2.f, sf::Color(64, 75, 96));

    if (fontLoaded) {
        ui::drawCenteredText(window, "Configurar Partida", font, 30, sf::Color::White,
                             sf::FloatRect(cardX, cardY + 18.f, cardW, 42.f));

        sf::Text modeLbl(ui::utf8("Modo de Jogo"), font, 15);
        modeLbl.setFillColor(sf::Color(190, 202, 222));
        modeLbl.setPosition(innerX, 148.f);
        window.draw(modeLbl);
    }

    drawOptionButton(window, modeAiBounds, "Contra IA", config.mode == GameMode::VS_AI, font, fontLoaded, 16);
    drawOptionButton(window, modeLocalBounds, "2 Jogadores (Local)", config.mode == GameMode::LOCAL_PVP, font, fontLoaded, 16);

    if (config.mode == GameMode::VS_AI) {
        if (fontLoaded) {
            sf::Text diffLbl(ui::utf8("Dificuldade da IA"), font, 15);
            diffLbl.setFillColor(sf::Color(190, 202, 222));
            diffLbl.setPosition(innerX, 248.f);
            window.draw(diffLbl);

            sf::Text colLbl(ui::utf8("Sua Cor (Pretas invertem o tabuleiro)"), font, 15);
            colLbl.setFillColor(sf::Color(190, 202, 222));
            colLbl.setPosition(innerX, 344.f);
            window.draw(colLbl);
        }

        drawOptionButton(window, diffEasyBounds, "Fácil", config.difficulty == AIDifficulty::EASY, font, fontLoaded, 15);
        drawOptionButton(window, diffMediumBounds, "Médio", config.difficulty == AIDifficulty::MEDIUM, font, fontLoaded, 15);
        drawOptionButton(window, diffHardBounds, "Difícil", config.difficulty == AIDifficulty::HARD, font, fontLoaded, 15);

        drawOptionButton(window, colorWhiteBounds, "Jogar de Brancas", config.playerColor == PieceColor::WHITE, font, fontLoaded, 15);
        drawOptionButton(window, colorBlackBounds, "Jogar de Pretas", config.playerColor == PieceColor::BLACK, font, fontLoaded, 15);
    } else {
        ui::drawRoundedRect(window, innerX, 260.f, innerW, 148.f, 12.f,
                            sf::Color(32, 38, 50), 1.5f, sf::Color(58, 68, 88));
        if (fontLoaded) {
            ui::drawCenteredText(window,
                                 "Modo Local (2 Jogadores no mesmo computador)",
                                 font, 17, sf::Color(220, 228, 240),
                                 sf::FloatRect(innerX, 286.f, innerW, 34.f));
            ui::drawCenteredText(window,
                                 "Todas as regras oficiais e lances legais são validados pelo motor.",
                                 font, 14, sf::Color(168, 178, 198),
                                 sf::FloatRect(innerX, 326.f, innerW, 32.f));
        }
    }

    if (fontLoaded) {
        sf::Text timeLbl(ui::utf8(config.mode == GameMode::VS_AI
                                      ? "Seu Tempo de Jogo (a IA não possui relógio)"
                                      : "Tempo de Jogo por Jogador"),
                         font, 15);
        timeLbl.setFillColor(sf::Color(190, 202, 222));
        timeLbl.setPosition(innerX, 444.f);
        window.draw(timeLbl);
    }

    const std::string timeLabels[7] = {
        "3 min", "5 min", "10 min", "15 min", "20 min", "30 min", "Sem tempo"
    };
    for (int i = 0; i < 7; ++i) {
        drawOptionButton(window, timeBounds[i], timeLabels[i],
                         config.timeLimitMinutes == TIME_OPTIONS[i],
                         font, fontLoaded, 15);
    }

    bool hoverBack = backButtonBounds.contains(mousePos);
    bool hoverStart = startGameButtonBounds.contains(mousePos);

    ui::drawRoundedRect(window,
                        backButtonBounds.left, backButtonBounds.top,
                        backButtonBounds.width, backButtonBounds.height, 10.f,
                        hoverBack ? sf::Color(58, 66, 84) : sf::Color(44, 50, 64),
                        1.5f, sf::Color(92, 106, 132));

    ui::drawRoundedRect(window,
                        startGameButtonBounds.left, startGameButtonBounds.top,
                        startGameButtonBounds.width, startGameButtonBounds.height, 10.f,
                        hoverStart ? sf::Color(58, 150, 92) : sf::Color(46, 125, 75),
                        2.f, sf::Color(106, 200, 138));

    if (fontLoaded) {
        ui::drawCenteredText(window, "Voltar", font, 17, sf::Color::White, backButtonBounds);
        ui::drawCenteredText(window, "Iniciar Partida", font, 18, sf::Color::White, startGameButtonBounds);
    }
}

void MenuScreen::draw(sf::RenderWindow& window,
                      AppScreen screen,
                      const GameConfig& config,
                      const sf::Font& font,
                      bool fontLoaded) const {
    // Fundo sutil estilo tabuleiro escuro
    sf::RectangleShape bg(sf::Vector2f(windowWidth, windowHeight));
    bg.setPosition(0.f, 0.f);
    bg.setFillColor(sf::Color(22, 26, 34));
    window.draw(bg);

    if (screen == AppScreen::MAIN_MENU) {
        drawMainMenu(window, font, fontLoaded);
    } else if (screen == AppScreen::GAME_SETUP) {
        drawGameSetup(window, config, font, fontLoaded);
    }
}
