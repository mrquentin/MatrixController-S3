#pragma once

#ifndef _COLOR_h
#define _COLOR_h

#include <cstdint>

/// 24-bit RGB color. The core display API keeps full 8 bits per channel;
/// convert to RGB565 only at the edge (e.g. for an Adafruit_GFX adapter).
struct Color {
    uint8_t r{};
    uint8_t g{};
    uint8_t b{};

    /// Builds a color from a 0xRRGGBB literal.
    static constexpr Color fromHex(const uint32_t rgb) {
        return {static_cast<uint8_t>(rgb >> 16), static_cast<uint8_t>(rgb >> 8), static_cast<uint8_t>(rgb)};
    }

    /// Linear blend: t = 0 returns a, t = 255 returns b.
    static constexpr Color lerp(const Color a, const Color b, const uint8_t t) {
        return {lerpChannel(a.r, b.r, t), lerpChannel(a.g, b.g, t), lerpChannel(a.b, b.b, t)};
    }

    [[nodiscard]] constexpr uint32_t toHex() const {
        return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
    }

    [[nodiscard]] constexpr uint16_t toRgb565() const {
        return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

    /// Scales every channel by factor / 255 (255 keeps the color, 0 gives black).
    [[nodiscard]] constexpr Color scaled(const uint8_t factor) const {
        return {scaleChannel(r, factor), scaleChannel(g, factor), scaleChannel(b, factor)};
    }

    constexpr bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }

    constexpr bool operator!=(const Color& other) const {
        return !(*this == other);
    }

private:
    static constexpr uint8_t scaleChannel(const uint8_t value, const uint8_t factor) {
        return static_cast<uint8_t>((value * factor + 127) / 255);
    }

    static constexpr uint8_t lerpChannel(const uint8_t from, const uint8_t to, const uint8_t t) {
        return static_cast<uint8_t>(from + ((to - from) * t + (to >= from ? 127 : -127)) / 255);
    }
};

namespace Colors {
    constexpr Color Black{0, 0, 0};
    constexpr Color White{255, 255, 255};
    constexpr Color Red{255, 0, 0};
    constexpr Color Green{0, 255, 0};
    constexpr Color Blue{0, 0, 255};
    constexpr Color Yellow{255, 255, 0};
    constexpr Color Cyan{0, 255, 255};
    constexpr Color Magenta{255, 0, 255};
    constexpr Color Orange{255, 128, 0};
}

#endif
