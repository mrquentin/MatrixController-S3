#pragma once

#ifndef _TEXT_h
#define _TEXT_h

#include <cstdint>
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

#endif
