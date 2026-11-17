#include "hud_panel.hpp"
#include "ui_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

HudPanel::HudPanel(float posX, float width, float height)
    : hudX(posX),
      hudWidth(width),
      hudHeight(height),
      remainingTimeSec{DEFAULT_CLOCK_SECONDS, DEFAULT_CLOCK_SECONDS},
      historyScrollOffset(0) {
    float btnW = (hudWidth - 42.f) * 0.5f;
    float btnH = 44.f;
    float btnY = hudHeight - btnH - 20.f;
    resignButtonBounds = sf::FloatRect(hudX + 16.f, btnY, btnW, btnH);
    newGameButtonBounds = sf::FloatRect(hudX + 26.f + btnW, btnY, btnW, btnH);
    reset();
}

void HudPanel::reset() {
    remainingTimeSec[0] = DEFAULT_CLOCK_SECONDS;
    remainingTimeSec[1] = DEFAULT_CLOCK_SECONDS;
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
                    const std::vector<std::string>& moveHistory) const {
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

    sf::CircleShape turnDot(8.f);
    turnDot.setPosition(hudX + pad + 16.f, 36.f);
    if (isGameOver) {
        turnDot.setFillColor(sf::Color(225, 80, 80));
    } else if (currentTurn == PieceColor::WHITE) {
        turnDot.setFillColor(sf::Color(245, 245, 245));
    } else {
        turnDot.setFillColor(sf::Color(60, 65, 75));
        turnDot.setOutlineThickness(2.f);
        turnDot.setOutlineColor(sf::Color(200, 205, 215));
    }
    window.draw(turnDot);

    if (fontLoaded) {
        sf::Text turnTxt(getTurnIndicatorText(currentTurn, isGameOver), font, 18);
        turnTxt.setFillColor(sf::Color::White);
        turnTxt.setPosition(hudX + pad + 42.f, 32.f);
        window.draw(turnTxt);
    }

    // 2. Relogios da Partida (RF07)
    const float clkW = (innerW - 12.f) * 0.5f;
    const float clkH = 74.f;
    const float clkY = 88.f;
    bool whiteActive = !isGameOver && (currentTurn == PieceColor::WHITE);
    bool blackActive = !isGameOver && (currentTurn == PieceColor::BLACK);

    ui::drawRoundedRect(window, hudX + pad, clkY, clkW, clkH, 10.f,
                        sf::Color(32, 38, 48), 2.f,
                        whiteActive ? sf::Color(106, 190, 48) : sf::Color(60, 68, 82));
    ui::drawRoundedRect(window, hudX + pad + clkW + 12.f, clkY, clkW, clkH, 10.f,
                        sf::Color(32, 38, 48), 2.f,
                        blackActive ? sf::Color(106, 190, 48) : sf::Color(60, 68, 82));

    if (fontLoaded) {
        sf::Text wLbl("BRANCAS", font, 12);
        wLbl.setFillColor(sf::Color(180, 190, 205));
        wLbl.setPosition(hudX + pad + 14.f, clkY + 10.f);
        window.draw(wLbl);

        sf::Text wTime(formatClock(remainingTimeSec[0]), font, 24);
        wTime.setFillColor(sf::Color::White);
        wTime.setPosition(hudX + pad + 14.f, clkY + 32.f);
        window.draw(wTime);

        sf::Text bLbl("PRETAS", font, 12);
        bLbl.setFillColor(sf::Color(180, 190, 205));
        bLbl.setPosition(hudX + pad + clkW + 26.f, clkY + 10.f);
        window.draw(bLbl);

        sf::Text bTime(formatClock(remainingTimeSec[1]), font, 24);
        bTime.setFillColor(sf::Color::White);
        bTime.setPosition(hudX + pad + clkW + 26.f, clkY + 32.f);
        window.draw(bTime);
    }

    // 3. Historico de Lances (RF20)
    const float histY = 178.f;
    const float histH = resignButtonBounds.top - histY - 16.f;
    ui::drawRoundedRect(window, hudX + pad, histY, innerW, histH, 10.f,
                        sf::Color(28, 33, 43), 1.5f, sf::Color(55, 65, 82));

    if (fontLoaded) {
        sf::Text histTitle("Historico de Lances", font, 15);
        histTitle.setFillColor(sf::Color(190, 200, 220));
        histTitle.setPosition(hudX + pad + 14.f, histY + 12.f);
        window.draw(histTitle);

        sf::RectangleShape hLine(sf::Vector2f(innerW - 24.f, 1.f));
        hLine.setPosition(hudX + pad + 12.f, histY + 38.f);
        hLine.setFillColor(sf::Color(55, 65, 82));
        window.draw(hLine);

        auto lines = getFormattedMoveHistory(moveHistory);
        int startRow = std::max(0, std::min(historyScrollOffset, static_cast<int>(lines.size())));
        int endRow = std::min(static_cast<int>(lines.size()), startRow + HUD_VISIBLE_HISTORY_ROWS);

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

    // 4. Botoes de Controle: Desistir e Nova Partida
    ui::drawRoundedRect(window,
                        resignButtonBounds.left, resignButtonBounds.top,
                        resignButtonBounds.width, resignButtonBounds.height, 9.f,
                        isGameOver ? sf::Color(70, 50, 50) : sf::Color(175, 52, 52),
                        1.5f, sf::Color(225, 95, 95));

    ui::drawRoundedRect(window,
                        newGameButtonBounds.left, newGameButtonBounds.top,
                        newGameButtonBounds.width, newGameButtonBounds.height, 9.f,
                        sf::Color(46, 125, 75),
                        1.5f, sf::Color(95, 195, 130));

    if (fontLoaded) {
        sf::Text resTxt("Desistir", font, 15);
        resTxt.setFillColor(sf::Color::White);
        sf::FloatRect rb = resTxt.getLocalBounds();
        resTxt.setPosition(
            resignButtonBounds.left + (resignButtonBounds.width - rb.width) * 0.5f - rb.left,
            resignButtonBounds.top + (resignButtonBounds.height - rb.height) * 0.5f - rb.top
        );
        window.draw(resTxt);

        sf::Text newTxt("Nova Partida", font, 15);
        newTxt.setFillColor(sf::Color::White);
        sf::FloatRect nb = newTxt.getLocalBounds();
        newTxt.setPosition(
            newGameButtonBounds.left + (newGameButtonBounds.width - nb.width) * 0.5f - nb.left,
            newGameButtonBounds.top + (newGameButtonBounds.height - nb.height) * 0.5f - nb.top
        );
        window.draw(newTxt);
    }
}
