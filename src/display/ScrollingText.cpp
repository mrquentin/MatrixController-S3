#include "display/ScrollingText.h"

#include <cmath>
#include "display/Text.h"

void ScrollingText::setText(const std::string_view text) {
    _text = text;
    _started = false;
}

void ScrollingText::setColor(const Color color) {
    _color = color;
}

void ScrollingText::setY(const int32_t y) {
    _y = y;
}

void ScrollingText::setSpeed(const float pixelsPerSecond) {
    _speed = pixelsPerSecond;
}

void ScrollingText::setFont(const Font& font, const uint8_t scale) {
    _font = &font;
    _scale = scale == 0 ? 1 : scale;
}

void ScrollingText::draw(IDisplay& display, const uint32_t nowMs) {
    if (!_started) {
        _x = display.width();
        _lastMs = nowMs;
        _started = true;
    }

    _x -= _speed * static_cast<float>(nowMs - _lastMs) / 1000.0f;
    _lastMs = nowMs;

    // Once fully off the left edge, come back in from the right
    const auto width = static_cast<float>(textWidth(_text, *_font, _scale));
    if (_x < -width) _x = display.width();

    drawText(display, static_cast<int32_t>(std::floor(_x)), _y, _text, _color, *_font, _scale);
}
