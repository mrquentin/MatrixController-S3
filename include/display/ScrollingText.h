#pragma once

#ifndef _SCROLLINGTEXT_h
#define _SCROLLINGTEXT_h

#include <cstdint>
#include <string>
#include <string_view>
#include "display/Font.h"
#include "display/IDisplay.h"

/// A line of text that enters from the right edge, scrolls left and starts over once it
/// has fully left the screen. Speed is time based, so it doesn't depend on the frame rate.
class ScrollingText {
public:
    /// Replaces the text and restarts from the right edge.
    void setText(std::string_view text);
    void setColor(Color color);
    void setY(int32_t y);
    void setSpeed(float pixelsPerSecond);
    void setFont(const Font& font, uint8_t scale = 1);

    /// Advances the scroll to nowMs and draws the text. Call once per frame, between
    /// clearing the frame and present().
    void draw(IDisplay& display, uint32_t nowMs);

private:
    std::string _text;
    Color _color = Colors::White;
    int32_t _y = 0;
    float _speed = 30.0f;
    const Font* _font = &Fonts::Font5x7;
    uint8_t _scale = 1;

    float _x = 0.0f;
    uint32_t _lastMs = 0;
    bool _started = false;
};

#endif
