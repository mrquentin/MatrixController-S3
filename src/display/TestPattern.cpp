#include "display/TestPattern.h"

void drawTestPattern(IDisplay& display) {
    const int16_t w = static_cast<int16_t>(display.width());
    const int16_t h = static_cast<int16_t>(display.height());

    display.clear();

    // Top two thirds: one bar per primary plus white, inside the border
    constexpr Color bars[] = {Colors::Red, Colors::Green, Colors::Blue, Colors::White};
    constexpr int16_t barCount = sizeof(bars) / sizeof(bars[0]);
    const int16_t innerWidth = static_cast<int16_t>(w - 2);
    const int16_t barHeight = static_cast<int16_t>((h - 2) * 2 / 3);
    for (int16_t i = 0; i < barCount; ++i) {
        const int16_t x0 = static_cast<int16_t>(1 + innerWidth * i / barCount);
        const int16_t x1 = static_cast<int16_t>(1 + innerWidth * (i + 1) / barCount);
        display.fillRect(x0, 1, static_cast<int16_t>(x1 - x0), barHeight, bars[i]);
    }

    // Bottom third: black to white gradient
    const int16_t gradientTop = static_cast<int16_t>(1 + barHeight);
    const int16_t gradientHeight = static_cast<int16_t>(h - 1 - gradientTop);
    for (int16_t x = 1; x < w - 1; ++x) {
        const auto t = static_cast<uint8_t>((x - 1) * 255 / (innerWidth - 1));
        display.fillRect(x, gradientTop, 1, gradientHeight, Color::lerp(Colors::Black, Colors::White, t));
    }

    // Border last so it is never overdrawn
    display.fillRect(0, 0, w, 1, Colors::White);
    display.fillRect(0, static_cast<int16_t>(h - 1), w, 1, Colors::White);
    display.fillRect(0, 0, 1, h, Colors::White);
    display.fillRect(static_cast<int16_t>(w - 1), 0, 1, h, Colors::White);
}
