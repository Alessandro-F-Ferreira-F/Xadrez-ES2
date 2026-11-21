#pragma once

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <string>

namespace ui {

inline sf::String utf8(const std::string& str) {
    return sf::String::fromUtf8(str.begin(), str.end());
}

inline bool loadSystemFont(sf::Font& font) {
    const char* candidates[] = {
#ifdef _WIN32
        "C:/Windows/Fonts/segoeuib.ttf",
        "C:/Windows/Fonts/arialbd.ttf",
        "C:/Windows/Fonts/tahomabd.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
#endif
        "./assets/font.ttf"
    };
    for (const char* path : candidates) {
        if (font.loadFromFile(path)) {
            return true;
        }
    }
    return false;
}

inline sf::Uint8 scaleAlpha(sf::Uint8 baseAlpha, float factor) {
    float clamped = std::max(0.f, std::min(1.f, factor));
    return static_cast<sf::Uint8>(baseAlpha * clamped);
}

inline sf::View createLetterboxView(float logicalWidth,
                                    float logicalHeight,
                                    unsigned int windowWidth,
                                    unsigned int windowHeight) {
    sf::View view(sf::FloatRect(0.f, 0.f, logicalWidth, logicalHeight));
    if (windowWidth == 0 || windowHeight == 0 || logicalWidth <= 0.f || logicalHeight <= 0.f) {
        return view;
    }

    float windowRatio = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float viewRatio = logicalWidth / logicalHeight;
    float posX = 0.f;
    float posY = 0.f;
    float sizeX = 1.f;
    float sizeY = 1.f;

    if (windowRatio > viewRatio) {
        sizeX = viewRatio / windowRatio;
        posX = (1.f - sizeX) * 0.5f;
    } else {
        sizeY = windowRatio / viewRatio;
        posY = (1.f - sizeY) * 0.5f;
    }

    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
    return view;
}

inline void drawRoundedRect(sf::RenderWindow& window,
                            float x, float y, float w, float h, float radius,
                            const sf::Color& fill,
                            float outlineThickness = 0.f,
                            const sf::Color& outline = sf::Color::Transparent) {
    const int cornerPoints = 8;
    float r = std::min(radius, std::min(w, h) * 0.5f);
    sf::ConvexShape shape(cornerPoints * 4);

    const float cx[4] = { x + w - r, x + r,     x + r,     x + w - r };
    const float cy[4] = { y + r,     y + r,     y + h - r, y + h - r };
    const int order[4] = { 0, 3, 2, 1 };
    const float angles[4] = { -1.5707963f, 0.f, 1.5707963f, 3.1415926f };

    int idx = 0;
    for (int c = 0; c < 4; ++c) {
        int corner = order[c];
        for (int i = 0; i < cornerPoints; ++i) {
            float a = angles[c] + (1.5707963f * i) / (cornerPoints - 1);
            shape.setPoint(idx++, sf::Vector2f(cx[corner] + r * std::cos(a),
                                               cy[corner] + r * std::sin(a)));
        }
    }

    shape.setFillColor(fill);
    shape.setOutlineThickness(outlineThickness);
    shape.setOutlineColor(outline);
    window.draw(shape);
}

inline void drawCenteredText(sf::RenderWindow& window,
                             const std::string& utf8Text,
                             const sf::Font& font,
                             unsigned int charSize,
                             const sf::Color& color,
                             const sf::FloatRect& bounds) {
    sf::Text text(utf8(utf8Text), font, charSize);
    text.setFillColor(color);
    sf::FloatRect tb = text.getLocalBounds();
    text.setPosition(
        bounds.left + (bounds.width - tb.width) * 0.5f - tb.left,
        bounds.top + (bounds.height - tb.height) * 0.5f - tb.top
    );
    window.draw(text);
}

} // namespace ui
