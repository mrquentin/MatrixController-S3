#include "display/Shapes.h"

#include <cstdlib>

namespace {
    void plot(IDisplay& display, const int32_t x, const int32_t y, const Color color) {
        if (x < 0 || y < 0 || x >= display.width() || y >= display.height()) return;
        display.setPixel(static_cast<int16_t>(x), static_cast<int16_t>(y), color);
    }

    // Horizontal run from x0 to x1 included, clipped before narrowing to fillRect's int16 arguments
    void hline(IDisplay& display, int32_t x0, int32_t x1, const int32_t y, const Color color) {
        if (y < 0 || y >= display.height()) return;
        if (x0 < 0) x0 = 0;
        if (x1 >= display.width()) x1 = display.width() - 1;
        if (x0 > x1) return;
        display.fillRect(static_cast<int16_t>(x0), static_cast<int16_t>(y), static_cast<int16_t>(x1 - x0 + 1), 1, color);
    }
}

void drawLine(IDisplay& display, int32_t x0, int32_t y0, const int32_t x1, const int32_t y1, const Color color) {
    // Bresenham
    const int32_t dx = std::abs(x1 - x0);
    const int32_t dy = -std::abs(y1 - y0);
    const int32_t sx = x0 < x1 ? 1 : -1;
    const int32_t sy = y0 < y1 ? 1 : -1;
    int32_t error = dx + dy;

    while (true) {
        plot(display, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        const int32_t doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void drawCircle(IDisplay& display, const int32_t cx, const int32_t cy, const int32_t radius, const Color color) {
    if (radius < 0) return;

    // Midpoint circle: walk one octant and mirror it
    int32_t x = radius;
    int32_t y = 0;
    int32_t error = 1 - radius;
    while (x >= y) {
        plot(display, cx + x, cy + y, color);
        plot(display, cx - x, cy + y, color);
        plot(display, cx + x, cy - y, color);
        plot(display, cx - x, cy - y, color);
        plot(display, cx + y, cy + x, color);
        plot(display, cx - y, cy + x, color);
        plot(display, cx + y, cy - x, color);
        plot(display, cx - y, cy - x, color);

        ++y;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            --x;
            error += 2 * (y - x) + 1;
        }
    }
}

void fillCircle(IDisplay& display, const int32_t cx, const int32_t cy, const int32_t radius, const Color color) {
    if (radius < 0) return;

    // One run per row; the + radius keeps the edge rounder than a strict x² + y² <= r² test
    const int32_t limit = radius * radius + radius;
    for (int32_t dy = -radius; dy <= radius; ++dy) {
        int32_t half = 0;
        while ((half + 1) * (half + 1) + dy * dy <= limit) ++half;
        hline(display, cx - half, cx + half, cy + dy, color);
    }
}
