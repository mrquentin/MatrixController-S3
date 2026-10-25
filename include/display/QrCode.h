#pragma once

#ifndef _QRCODE_DISPLAY_h
#define _QRCODE_DISPLAY_h

#include <cstdint>
#include <vector>
#include "display/IDisplay.h"

/// A QR code encoded with the ESP-IDF's encoder, drawable on the matrix.
class QrCode {
public:
    /// Encodes text using at most maxVersion (version 3 is 29 x 29 modules, about 50 bytes of
    /// text). Returns false if the text doesn't fit.
    bool generate(const char* text, int maxVersion);

    /// Modules per side, 0 if nothing was generated.
    [[nodiscard]] int size() const { return _size; }

    /// Side length in pixels of draw() with this scale and quiet zone.
    [[nodiscard]] int32_t drawnSize(uint8_t scale, uint8_t quietZone) const;

    /// Draws dark modules on a white square, with quietZone white modules around it so phone
    /// cameras can find the code. (x, y) is the top-left of the white square.
    void draw(IDisplay& display, int32_t x, int32_t y, uint8_t scale, uint8_t quietZone) const;

private:
    int _size = 0;
    std::vector<bool> _modules;
};

#endif
