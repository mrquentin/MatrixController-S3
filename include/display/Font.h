#pragma once

#ifndef _FONT_h
#define _FONT_h

#include <cstdint>

/// Fixed-width bitmap font. Each glyph is glyphWidth column bytes; in each byte bit 0 is
/// the top row, so a glyph can be up to 8 pixels tall.
struct Font {
    uint8_t glyphWidth;
    uint8_t glyphHeight;
    uint8_t spacing;         // empty columns between two glyphs
    char first;              // first character in the table
    char last;               // last character in the table
    const uint8_t* columns;  // (last - first + 1) * glyphWidth bytes

    /// Horizontal distance from one glyph to the next, in pixels at scale 1.
    [[nodiscard]] constexpr uint8_t advance() const {
        return glyphWidth + spacing;
    }
};

namespace Fonts {
    /// Classic 5x7 font, printable ASCII (space to ~). 6 px per character, 7 px tall.
    extern const Font Font5x7;
}

#endif
