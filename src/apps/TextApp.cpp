#include "apps/TextApp.h"

#include "display/Text.h"

namespace {
    constexpr uint16_t MaxMessageLength = 200;
}

TextApp::TextApp() {
    applySettings();
}

void TextApp::describeConfig(ConfigSchema& schema) const {
    schema.text("message", "Message", MaxMessageLength);
    schema.color("color", "Color");
    schema.number("speed", "Speed (pixels per second)", 5, 200);
    schema.number("size", "Size", 1, 4);
}

void TextApp::getConfig(JsonObject config) const {
    config["message"] = _message;
    writeColor(config, "color", _color);
    config["speed"] = _speed;
    config["size"] = _size;
}

void TextApp::setConfig(const JsonObjectConst config) {
    if (const char* message = config["message"]) {
        _message = std::string(message).substr(0, MaxMessageLength);
    }
    _color = readColor(config["color"], _color);
    _speed = readNumber(config["speed"], _speed, 5, 200);
    _size = readNumber(config["size"], _size, 1, 4);
    applySettings();
}

void TextApp::activate() {
    // Start from the right edge every time the app is shown
    _scroller.setText(_message);
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
