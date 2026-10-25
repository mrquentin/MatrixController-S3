#include "display/IDisplay.h"

#include <algorithm>

void IDisplay::fillRect(const int16_t x, const int16_t y, const int16_t w, const int16_t h, const Color color) {
    const int32_t x0 = std::max<int32_t>(x, 0);
    const int32_t y0 = std::max<int32_t>(y, 0);
    const int32_t x1 = std::min<int32_t>(static_cast<int32_t>(x) + w, width());
    const int32_t y1 = std::min<int32_t>(static_cast<int32_t>(y) + h, height());

    for (int32_t row = y0; row < y1; ++row) {
        for (int32_t col = x0; col < x1; ++col) {
            setPixel(static_cast<int16_t>(col), static_cast<int16_t>(row), color);
        }
    }
}
