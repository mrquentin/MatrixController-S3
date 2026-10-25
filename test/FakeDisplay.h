#pragma once

#include <algorithm>
#include <vector>
#include "display/IDisplay.h"

/// In-memory IDisplay for tests: keeps every pixel so drawing can be checked.
class FakeDisplay final : public IDisplay {
public:
    FakeDisplay(const uint16_t width, const uint16_t height)
        : _width(width), _height(height), _pixels(static_cast<size_t>(width) * height, Colors::Black) {}

    [[nodiscard]] uint16_t width() const override { return _width; }
    [[nodiscard]] uint16_t height() const override { return _height; }

    void setPixel(const int16_t x, const int16_t y, const Color color) override {
        // IDisplay's users clip before drawing, so this counts as a bug in the code under test
        if (x < 0 || y < 0 || x >= _width || y >= _height) {
            ++outOfBounds;
            return;
        }
        _pixels[static_cast<size_t>(y) * _width + x] = color;
    }
    void fill(const Color color) override { std::fill(_pixels.begin(), _pixels.end(), color); }

    void setBrightness(const uint8_t level) override { _brightness = level; }
    [[nodiscard]] uint8_t brightness() const override { return _brightness; }
    void present() override { ++presents; }

    [[nodiscard]] Color at(const int16_t x, const int16_t y) const { return _pixels[static_cast<size_t>(y) * _width + x]; }
    [[nodiscard]] size_t litPixels() const {
        return std::count_if(_pixels.begin(), _pixels.end(), [](const Color& pixel) { return !(pixel == Colors::Black); });
    }

    int outOfBounds = 0;
    int presents = 0;

private:
    uint16_t _width;
    uint16_t _height;
    std::vector<Color> _pixels;
    uint8_t _brightness = 255;
};
