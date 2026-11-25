#pragma once

#ifndef _TEXT_h
#define _TEXT_h

#include <cstdint>
#include <initializer_list>
#include <string_view>
#include "display/Font.h"
#include "display/IDisplay.h"

/// Width in pixels of text drawn on a single line (no trailing spacing).
uint16_t textWidth(std::string_view text, const Font& font = Fonts::Font5x7, uint8_t scale = 1);

/// Height in pixels of one line of text.
uint16_t textHeight(const Font& font = Fonts::Font5x7, uint8_t scale = 1);

/// Draws a single line with its top-left corner at (x, y). Only lit pixels are drawn, so the
/// background shows through. Clipped to the display: x can be negative, e.g. when scrolling.
/// Characters missing from the font are drawn as '?'. scale enlarges each pixel to a square.
/// Returns the x where the next character would go.
int32_t drawText(IDisplay& display, int32_t x, int32_t y, std::string_view text, Color color,
                 const Font& font = Fonts::Font5x7, uint8_t scale = 1);

/// Draws a single line centered horizontally on the display.
void drawTextCentered(IDisplay& display, int32_t y, std::string_view text, Color color,
                      const Font& font = Fonts::Font5x7, uint8_t scale = 1);

/// Draws a single line that ends at x = right.
void drawTextRight(IDisplay& display, int32_t right, int32_t y, std::string_view text, Color color,
                   const Font& font = Fonts::Font5x7, uint8_t scale = 1);

struct TextLine {
    std::string_view text;
    Color color;
};

/// Draws a message: lines centered on the display, gap pixels apart. Empty lines are left out and
/// lines too long for the display are cut.
void drawLines(IDisplay& display, std::initializer_list<TextLine> lines, int32_t gap = 3);

/// A full-screen status message from the system (connecting, restarting, update failed): a title,
/// then a line and a detail, each optional. Clears, draws and presents the frame.
void showStatus(IDisplay& display, std::string_view title, std::string_view line = {}, std::string_view detail = {});

#endif
