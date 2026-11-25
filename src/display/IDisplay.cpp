#include "display/IDisplay.h"

#include <algorithm>

IDisplay::Span IDisplay::clip(const int16_t x, const int16_t y, const int16_t w, const int16_t h) const {
    return {std::max<int32_t>(x, 0), std::max<int32_t>(y, 0),
            std::min<int32_t>(static_cast<int32_t>(x) + w, width()),
            std::min<int32_t>(static_cast<int32_t>(y) + h, height())};
}

void IDisplay::fillRect(const int16_t x, const int16_t y, const int16_t w, const int16_t h, const Color color) {
    const Span span = clip(x, y, w, h);
    for (int32_t row = span.y0; row < span.y1; ++row) {
        for (int32_t col = span.x0; col < span.x1; ++col) {
            setPixel(static_cast<int16_t>(col), static_cast<int16_t>(row), color);
        }
    }
}
