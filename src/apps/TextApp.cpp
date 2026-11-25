#include "apps/TextApp.h"

#include "display/Text.h"

namespace {
    constexpr uint16_t MaxMessageLength = 200;
}

TextApp::TextApp() {
    applySettings();
}

void TextApp::settings(AppSettings& settings) {
    settings.text("message", "Message", _message, MaxMessageLength);
    settings.color("color", "Color", _color);
    settings.number("speed", "Speed (pixels per second)", _speed, 5, 200);
    settings.number("size", "Size", _size, 1, 4);
    applySettings();
}

void TextApp::activate() {
    _scroller.restart();
}

void TextApp::render(IDisplay& display, const uint32_t nowMs) {
    const auto scale = static_cast<uint8_t>(_size);
    _scroller.setY((static_cast<int32_t>(display.height()) - textHeight(Fonts::Font5x7, scale)) / 2);
    _scroller.draw(display, nowMs);
}

void TextApp::applySettings() {
    _scroller.setText(_message);
    _scroller.setColor(_color);
    _scroller.setSpeed(static_cast<float>(_speed));
    _scroller.setFont(Fonts::Font5x7, static_cast<uint8_t>(_size));
}
