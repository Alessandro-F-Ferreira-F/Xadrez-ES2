#pragma once

#include "piece.hpp"
#include <string>

enum class AppScreen {
    MAIN_MENU,
    GAME_SETUP,
    PLAYING
};

enum class GameMode {
    VS_AI,
    LOCAL_PVP
};

enum class AIDifficulty {
    EASY = 1,
    MEDIUM = 2,
    HARD = 3
};

struct GameConfig {
    GameMode mode = GameMode::VS_AI;
    AIDifficulty difficulty = AIDifficulty::MEDIUM;
    PieceColor playerColor = PieceColor::WHITE;
    int timeLimitMinutes = 10; // 0 = Sem tempo

    bool hasTimer() const {
        return timeLimitMinutes > 0;
    }

    float timeLimitSeconds() const {
        return static_cast<float>(timeLimitMinutes) * 60.f;
    }

    bool isBoardFlipped() const {
        return mode == GameMode::VS_AI && playerColor == PieceColor::BLACK;
    }

    int uciLevel() const {
        return static_cast<int>(difficulty);
    }

    std::string difficultyLabel() const {
        switch (difficulty) {
            case AIDifficulty::EASY:   return "Fácil";
            case AIDifficulty::MEDIUM: return "Médio";
            case AIDifficulty::HARD:   return "Difícil";
        }
        return "Médio";
    }
};
