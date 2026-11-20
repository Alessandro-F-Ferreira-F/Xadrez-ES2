#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <string>
#include <vector>
#include "piece.hpp"

#define DEFAULT_HUD_WIDTH 320
#define DEFAULT_CLOCK_SECONDS 600.f
#define HUD_VISIBLE_HISTORY_ROWS 12

// Profundidade pedida ao motor (go depth N), ajustada pelos botoes -/+ da HUD. O
// motor para no teto de 10 s mesmo que a profundidade nao termine; acima de ~7 e
// quase sempre o teto que decide.
#define AI_DEPTH_MIN 1
#define AI_DEPTH_MAX 12
#define AI_DEPTH_DEFAULT 3

class HudPanel {
private:
    float hudX;
    float hudWidth;
    float hudHeight;

    float remainingTimeSec[2];
    std::chrono::steady_clock::time_point lastClockTick;
    int historyScrollOffset;

    sf::FloatRect resignButtonBounds;
    sf::FloatRect newGameButtonBounds;

    // Controle de profundidade da IA (nao volta ao padrao em reset: e preferencia do jogador)
    int aiDepth;
    float depthPanelY;
    float depthPanelH;
    sf::FloatRect depthDownButtonBounds;
    sf::FloatRect depthUpButtonBounds;

public:
    HudPanel(float posX, float width, float height);

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
    const sf::FloatRect& getResignButtonBounds() const { return resignButtonBounds; }
    const sf::FloatRect& getNewGameButtonBounds() const { return newGameButtonBounds; }

    int getAiDepth() const { return aiDepth; }
    // Limita a [AI_DEPTH_MIN, AI_DEPTH_MAX]
    void setAiDepth(int depth);
    bool isDepthDownClicked(float x, float y) const { return depthDownButtonBounds.contains(x, y); }
    bool isDepthUpClicked(float x, float y) const { return depthUpButtonBounds.contains(x, y); }
    const sf::FloatRect& getDepthDownButtonBounds() const { return depthDownButtonBounds; }
    const sf::FloatRect& getDepthUpButtonBounds() const { return depthUpButtonBounds; }

    void draw(sf::RenderWindow& window,
              const sf::Font& font,
              bool fontLoaded,
              PieceColor currentTurn,
              bool isGameOver,
              const std::vector<std::string>& moveHistory,
              bool engineThinking = false) const;
};
