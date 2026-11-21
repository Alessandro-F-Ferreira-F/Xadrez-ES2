#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <string>
#include <vector>
#include "piece.hpp"

#define DEFAULT_HUD_WIDTH 320
#define DEFAULT_CLOCK_SECONDS 600.f
#define HUD_VISIBLE_HISTORY_ROWS 12

class HudPanel {
private:
    float hudX;
    float hudWidth;
    float hudHeight;

    float initialTimeSec;
    float remainingTimeSec[2];
    bool timerEnabled[2];
    std::string playerLabels[2];
    std::string untimedText[2];
    std::chrono::steady_clock::time_point lastClockTick;
    int historyScrollOffset;

    sf::FloatRect resignButtonBounds;
    sf::FloatRect newGameButtonBounds;
    sf::FloatRect setupButtonBounds;
    sf::FloatRect menuButtonBounds;

public:
    HudPanel(float posX, float width, float height);

    void configureTimers(bool whiteTimed,
                         bool blackTimed,
                         float seconds,
                         const std::string& whiteLabel,
                         const std::string& blackLabel,
                         const std::string& whiteUntimedText = "Sem tempo",
                         const std::string& blackUntimedText = "Sem tempo");

    void reset();
    float getWidth() const { return hudWidth; }

    float getRemainingTimeSec(PieceColor color) const;
    void setRemainingTimeSec(PieceColor color, float seconds);

    // Desconta deltaSec do jogador da vez. Retorna true se o tempo acabou.
    bool advanceClock(PieceColor currentTurn, bool isGameOver, float deltaSec);
    bool tickClock(PieceColor currentTurn, bool isGameOver);
    void onMovePlayed(size_t totalHalfMoves);

    static std::string formatClock(float seconds);
    static std::string getTurnIndicatorText(PieceColor currentTurn, bool isGameOver);
    static std::vector<std::string> getFormattedMoveHistory(const std::vector<std::string>& moveHistory);

    void scrollHistory(int deltaLines, size_t totalHalfMoves);
    int getHistoryScrollOffset() const { return historyScrollOffset; }

    bool isResignClicked(float x, float y) const { return resignButtonBounds.contains(x, y); }
    bool isNewGameClicked(float x, float y) const { return newGameButtonBounds.contains(x, y); }
    bool isSetupClicked(float x, float y) const { return setupButtonBounds.contains(x, y); }
    bool isMenuClicked(float x, float y) const { return menuButtonBounds.contains(x, y); }
    const sf::FloatRect& getResignButtonBounds() const { return resignButtonBounds; }
    const sf::FloatRect& getNewGameButtonBounds() const { return newGameButtonBounds; }
    const sf::FloatRect& getSetupButtonBounds() const { return setupButtonBounds; }
    const sf::FloatRect& getMenuButtonBounds() const { return menuButtonBounds; }

    void draw(sf::RenderWindow& window,
              const sf::Font& font,
              bool fontLoaded,
              PieceColor currentTurn,
              bool isGameOver,
              const std::vector<std::string>& moveHistory,
              sf::Vector2f mousePos = sf::Vector2f(-1.f, -1.f)) const;
};
