#pragma once

#ifndef _IDISPLAY_h
#define _IDISPLAY_h

#include <cstdint>
#include "display/Color.h"

/// Hardware-agnostic drawing surface. Features draw through this interface only,
/// so they never depend on the HUB75 driver.
///
/// Contract:
///  - Origin (0, 0) is top-left. Out-of-bounds drawing is clipped, never undefined behaviour.
///  - Call present() to show the frame. When double-buffered, the back buffer holds an
///    older frame after a swap, so redraw the whole frame before every present().
///  - Not thread-safe: draw from a single task (the Arduino loop). Other tasks, such as web
///    handlers, should hand work over to that task instead of drawing directly.
class IDisplay {
public:
    virtual ~IDisplay() = default;

    [[nodiscard]] virtual uint16_t width() const = 0;
    [[nodiscard]] virtual uint16_t height() const = 0;

    virtual void setPixel(int16_t x, int16_t y, Color color) = 0;
    virtual void fill(Color color) = 0;
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color);
    void clear() { fill(Colors::Black); }

    virtual void setBrightness(uint8_t level) = 0;
    [[nodiscard]] virtual uint8_t brightness() const = 0;

    virtual void present() = 0;
};

#endif
