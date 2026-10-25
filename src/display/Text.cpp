#include "display/Text.h"

namespace {
    const uint8_t* glyphFor(const Font& font, const char c) {
        // Compare as unsigned so bytes above 0x7F (e.g. UTF-8) don't wrap to negative values
        const auto code = static_cast<unsigned char>(c);
        const bool inFont = code >= static_cast<unsigned char>(font.first) &&
                            code <= static_cast<unsigned char>(font.last);
        const auto index = static_cast<unsigned char>(inFont ? c : '?') - static_cast<unsigned char>(font.first);
        return font.columns + index * font.glyphWidth;
    }

    void drawGlyph(IDisplay& display, const int32_t x, const int32_t y, const uint8_t* glyph,
                   const Color color, const Font& font, const uint8_t scale) {
        for (uint8_t col = 0; col < font.glyphWidth; ++col) {
            const uint8_t bits = glyph[col];
            for (uint8_t row = 0; row < font.glyphHeight; ++row) {
                if (!(bits & (1 << row))) continue;

                const int32_t px = x + col * scale;
                const int32_t py = y + row * scale;
                if (scale == 1) {
                    if (px >= 0 && py >= 0 && px < display.width() && py < display.height()) {
                        display.setPixel(static_cast<int16_t>(px), static_cast<int16_t>(py), color);
                    }
                } else {
                    // fillRect clips, so only coordinates that fit in int16 need care here
                    if (px > -scale && py > -scale && px < display.width() && py < display.height()) {
                        display.fillRect(static_cast<int16_t>(px), static_cast<int16_t>(py), scale, scale, color);
                    }
                }
            }
        }
    }
}

uint16_t textWidth(const std::string_view text, const Font& font, const uint8_t scale) {
    if (text.empty()) return 0;
    return static_cast<uint16_t>((text.size() * font.advance() - font.spacing) * scale);
}

uint16_t textHeight(const Font& font, const uint8_t scale) {
    return static_cast<uint16_t>(font.glyphHeight * scale);
}

int32_t drawText(IDisplay& display, int32_t x, const int32_t y, const std::string_view text, const Color color,
                 const Font& font, uint8_t scale) {
    if (scale == 0) scale = 1;
    const int32_t advance = font.advance() * scale;

    for (const char c : text) {
        // Long scrolling texts are mostly off screen, skip those glyphs without touching their pixels
        if (x + advance > 0 && x < display.width()) {
            drawGlyph(display, x, y, glyphFor(font, c), color, font, scale);
        }
        x += advance;
    }
    return x;
}
