#include "hud_panel.hpp"
#include "ui_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

HudPanel::HudPanel(float posX, float width, float height)
    : hudX(posX),
      hudWidth(width),
      hudHeight(height),
      initialTimeSec(DEFAULT_CLOCK_SECONDS),
      remainingTimeSec{DEFAULT_CLOCK_SECONDS, DEFAULT_CLOCK_SECONDS},
      timerEnabled{true, true},
      playerLabels{"BRANCAS", "PRETAS"},
      untimedText{"Sem tempo", "Sem tempo"},
      historyScrollOffset(0) {
    const float pad = 16.f;
    const float innerW = hudWidth - 2.f * pad;
    const float btnH = 44.f;
    const float bottomY = hudHeight - btnH - 20.f;
    const float halfW = (innerW - 10.f) * 0.5f;

    // Durante a partida: apenas Desistir ocupa a faixa inferior
    resignButtonBounds = sf::FloatRect(hudX + pad, bottomY, innerW, btnH);

    // Pos-jogo: Jogar Novamente em cima; Configurar e Menu Principal embaixo
    newGameButtonBounds = sf::FloatRect(hudX + pad, bottomY - btnH - 10.f, innerW, btnH);
    setupButtonBounds = sf::FloatRect(hudX + pad, bottomY, halfW, btnH);
    menuButtonBounds = sf::FloatRect(hudX + pad + halfW + 10.f, bottomY, halfW, btnH);

    reset();
}

void HudPanel::configureTimers(bool whiteTimed,
                               bool blackTimed,
                               float seconds,
                               const std::string& whiteLabel,
                               const std::string& blackLabel,
                               const std::string& whiteUntimedText,
                               const std::string& blackUntimedText) {
    timerEnabled[0] = whiteTimed;
    timerEnabled[1] = blackTimed;
    initialTimeSec = std::max(0.f, seconds);
    playerLabels[0] = whiteLabel;
    playerLabels[1] = blackLabel;
    untimedText[0] = whiteUntimedText;
    untimedText[1] = blackUntimedText;
    reset();
}

void HudPanel::reset() {
    remainingTimeSec[0] = initialTimeSec;
    remainingTimeSec[1] = initialTimeSec;
    lastClockTick = std::chrono::steady_clock::now();
    historyScrollOffset = 0;
}

float HudPanel::getRemainingTimeSec(PieceColor color) const {
    int idx = static_cast<int>(color);
    return (idx >= 0 && idx < 2) ? remainingTimeSec[idx] : 0.f;
}

void HudPanel::setRemainingTimeSec(PieceColor color, float seconds) {
    int idx = static_cast<int>(color);
    if (idx >= 0 && idx < 2) {
        remainingTimeSec[idx] = std::max(0.f, seconds);
    }
}

bool HudPanel::advanceClock(PieceColor currentTurn, bool isGameOver, float deltaSec) {
    if (isGameOver || deltaSec <= 0.f) {
        return false;
    }
    int idx = static_cast<int>(currentTurn);
    if (idx < 0 || idx > 1 || !timerEnabled[idx]) {
        return false;
    }
    remainingTimeSec[idx] -= deltaSec;
    if (remainingTimeSec[idx] <= 0.f) {
        remainingTimeSec[idx] = 0.f;
        return true;
    }
    return false;
}

bool HudPanel::tickClock(PieceColor currentTurn, bool isGameOver) {
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - lastClockTick).count();
    lastClockTick = now;
    return advanceClock(currentTurn, isGameOver, dt);
}

void HudPanel::onMovePlayed(size_t totalHalfMoves) {
    int totalRows = static_cast<int>((totalHalfMoves + 1) / 2);
    historyScrollOffset = std::max(0, totalRows - HUD_VISIBLE_HISTORY_ROWS);
    lastClockTick = std::chrono::steady_clock::now();
}

std::string HudPanel::formatClock(float seconds) {
    float clamped = std::max(0.f, seconds);
    int totalSec = static_cast<int>(std::floor(clamped));
    int mins = totalSec / 60;
    int secs = totalSec % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", mins, secs);
    return std::string(buf);
}

std::string HudPanel::getTurnIndicatorText(PieceColor currentTurn, bool isGameOver) {
    if (isGameOver) {
        return "Partida Encerrada";
    }
    return (currentTurn == PieceColor::WHITE) ? "Vez das Brancas" : "Vez das Pretas";
}

std::vector<std::string> HudPanel::getFormattedMoveHistory(const std::vector<std::string>& moveHistory) {
    std::vector<std::string> lines;
    for (size_t i = 0; i < moveHistory.size(); i += 2) {
        int turnNum = static_cast<int>(i / 2) + 1;
        char buf[64];
        if (i + 1 < moveHistory.size()) {
            std::snprintf(buf, sizeof(buf), "%2d.  %-7s  %-7s",
                          turnNum, moveHistory[i].c_str(), moveHistory[i + 1].c_str());
        } else {
            std::snprintf(buf, sizeof(buf), "%2d.  %-7s",
                          turnNum, moveHistory[i].c_str());
        }
        lines.emplace_back(buf);
    }
    return lines;
}

void HudPanel::scrollHistory(int deltaLines, size_t totalHalfMoves) {
    int totalRows = static_cast<int>((totalHalfMoves + 1) / 2);
    int maxOffset = std::max(0, totalRows - HUD_VISIBLE_HISTORY_ROWS);
    historyScrollOffset = std::max(0, std::min(maxOffset, historyScrollOffset + deltaLines));
}

void HudPanel::draw(sf::RenderWindow& window,
                    const sf::Font& font,
                    bool fontLoaded,
                    PieceColor currentTurn,
                    bool isGameOver,
                    const std::vector<std::string>& moveHistory,
                    sf::Vector2f mousePos,
                    const std::string& turnTextOverride) const {
    sf::RectangleShape hudBg(sf::Vector2f(hudWidth, hudHeight));
    hudBg.setPosition(hudX, 0.f);
    hudBg.setFillColor(sf::Color(22, 26, 34));
    window.draw(hudBg);

    sf::RectangleShape sep(sf::Vector2f(2.f, hudHeight));
    sep.setPosition(hudX, 0.f);
    sep.setFillColor(sf::Color(48, 56, 70));
    window.draw(sep);

    const float pad = 16.f;
    const float innerW = hudWidth - 2.f * pad;

    // 1. Indicador de Turno (RF13)
    ui::drawRoundedRect(window, hudX + pad, 16.f, innerW, 56.f, 10.f,
                        sf::Color(34, 40, 52), 1.5f, sf::Color(70, 82, 104));

    const bool hasOverride = (!isGameOver && !turnTextOverride.empty());
    sf::CircleShape turnDot(8.f);
    turnDot.setPosition(hudX + pad + 16.f, 36.f);
    if (isGameOver) {
        turnDot.setFillColor(sf::Color(225, 80, 80));
    } else if (hasOverride) {
        turnDot.setFillColor(sf::Color(110, 175, 240));
    } else if (currentTurn == PieceColor::WHITE) {
        turnDot.setFillColor(sf::Color(245, 245, 245));
    } else {
        turnDot.setFillColor(sf::Color(60, 65, 75));
        turnDot.setOutlineThickness(2.f);
        turnDot.setOutlineColor(sf::Color(200, 205, 215));
    }
    window.draw(turnDot);

    if (fontLoaded) {
        std::string turnLabel = hasOverride
            ? turnTextOverride
            : getTurnIndicatorText(currentTurn, isGameOver);
        sf::Text turnTxt(ui::utf8(turnLabel), font, 18);
        turnTxt.setFillColor(sf::Color::White);
        turnTxt.setPosition(hudX + pad + 42.f, 32.f);
        window.draw(turnTxt);
    }

    // 2. Relogios / Cards dos Jogadores (RF07)
    const float clkW = (innerW - 12.f) * 0.5f;
    const float clkH = 74.f;
    const float clkY = 88.f;
    bool activeSide[2] = {
        !isGameOver && (currentTurn == PieceColor::WHITE),
        !isGameOver && (currentTurn == PieceColor::BLACK)
    };

    for (int i = 0; i < 2; ++i) {
        float cardX = hudX + pad + (i == 1 ? clkW + 12.f : 0.f);
        ui::drawRoundedRect(window, cardX, clkY, clkW, clkH, 10.f,
                            sf::Color(32, 38, 48), 2.f,
                            activeSide[i] ? sf::Color(106, 190, 48) : sf::Color(60, 68, 82));

        if (fontLoaded) {
            sf::Text lbl(ui::utf8(playerLabels[i]), font, 11);
            lbl.setFillColor(sf::Color(180, 190, 205));
            lbl.setPosition(cardX + 12.f, clkY + 10.f);
            window.draw(lbl);

            std::string mainVal = timerEnabled[i] ? formatClock(remainingTimeSec[i]) : untimedText[i];
            unsigned int valSize = timerEnabled[i] ? 24 : 15;
            float valY = timerEnabled[i] ? (clkY + 32.f) : (clkY + 37.f);
            sf::Text valTxt(ui::utf8(mainVal), font, valSize);
            valTxt.setFillColor(timerEnabled[i] ? sf::Color::White : sf::Color(210, 218, 230));
            valTxt.setPosition(cardX + 12.f, valY);
            window.draw(valTxt);
        }
    }

    // 3. Historico de Lances (RF20)
    const float histY = 178.f;
    const float controlsTop = isGameOver ? newGameButtonBounds.top : resignButtonBounds.top;
    const float histH = controlsTop - histY - 16.f;
    ui::drawRoundedRect(window, hudX + pad, histY, innerW, histH, 10.f,
                        sf::Color(28, 33, 43), 1.5f, sf::Color(55, 65, 82));

    if (fontLoaded) {
        sf::Text histTitle(ui::utf8("Histórico de Lances"), font, 15);
        histTitle.setFillColor(sf::Color(190, 200, 220));
        histTitle.setPosition(hudX + pad + 14.f, histY + 12.f);
        window.draw(histTitle);

        sf::RectangleShape hLine(sf::Vector2f(innerW - 24.f, 1.f));
        hLine.setPosition(hudX + pad + 12.f, histY + 38.f);
        hLine.setFillColor(sf::Color(55, 65, 82));
        window.draw(hLine);

        auto lines = getFormattedMoveHistory(moveHistory);
        int visibleRows = std::max(1, static_cast<int>((histH - 50.f) / 28.f));
        int startRow = std::max(0, std::min(historyScrollOffset, static_cast<int>(lines.size())));
        int endRow = std::min(static_cast<int>(lines.size()), startRow + visibleRows);

        for (int r = startRow; r < endRow; ++r) {
            float rowY = histY + 46.f + (r - startRow) * 28.f;
            if ((r % 2) == 1) {
                sf::RectangleShape stripe(sf::Vector2f(innerW - 16.f, 26.f));
                stripe.setPosition(hudX + pad + 8.f, rowY);
                stripe.setFillColor(sf::Color(36, 42, 54, 150));
                window.draw(stripe);
            }
            sf::Text lineTxt(lines[r], font, 15);
            lineTxt.setFillColor(sf::Color(230, 235, 245));
            lineTxt.setPosition(hudX + pad + 16.f, rowY + 3.f);
            window.draw(lineTxt);
        }
    }

    // 4. Botoes de Controle
    if (!isGameOver) {
        bool hoverResign = resignButtonBounds.contains(mousePos);
        ui::drawRoundedRect(window,
                            resignButtonBounds.left, resignButtonBounds.top,
                            resignButtonBounds.width, resignButtonBounds.height, 9.f,
                            hoverResign ? sf::Color(198, 64, 64) : sf::Color(175, 52, 52),
                            1.5f, sf::Color(225, 95, 95));
        if (fontLoaded) {
            ui::drawCenteredText(window, "Desistir", font, 16, sf::Color::White, resignButtonBounds);
        }
    } else {
        bool hoverNew = newGameButtonBounds.contains(mousePos);
        bool hoverSetup = setupButtonBounds.contains(mousePos);
        bool hoverMenu = menuButtonBounds.contains(mousePos);

        ui::drawRoundedRect(window,
                            newGameButtonBounds.left, newGameButtonBounds.top,
                            newGameButtonBounds.width, newGameButtonBounds.height, 9.f,
                            hoverNew ? sf::Color(58, 148, 90) : sf::Color(46, 125, 75),
                            1.5f, sf::Color(95, 195, 130));

        ui::drawRoundedRect(window,
                            setupButtonBounds.left, setupButtonBounds.top,
                            setupButtonBounds.width, setupButtonBounds.height, 9.f,
                            hoverSetup ? sf::Color(52, 62, 82) : sf::Color(38, 46, 62),
                            1.5f, sf::Color(92, 108, 136));

        ui::drawRoundedRect(window,
                            menuButtonBounds.left, menuButtonBounds.top,
                            menuButtonBounds.width, menuButtonBounds.height, 9.f,
                            hoverMenu ? sf::Color(52, 62, 82) : sf::Color(38, 46, 62),
                            1.5f, sf::Color(92, 108, 136));

        if (fontLoaded) {
            ui::drawCenteredText(window, "Jogar Novamente", font, 15, sf::Color::White, newGameButtonBounds);
            ui::drawCenteredText(window, "Configurar", font, 14, sf::Color::White, setupButtonBounds);
            ui::drawCenteredText(window, "Menu Principal", font, 14, sf::Color::White, menuButtonBounds);
        }
    }
}
