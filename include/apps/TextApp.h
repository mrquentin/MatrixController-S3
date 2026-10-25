#pragma once

#ifndef _TEXTAPP_h
#define _TEXTAPP_h

#include <string>
#include "apps/App.h"
#include "display/ScrollingText.h"

/// Scrolls a configurable message across the middle of the matrix.
class TextApp final : public App {
public:
    TextApp();

    [[nodiscard]] const char* id() const override { return "text"; }
    [[nodiscard]] const char* name() const override { return "Text"; }

    void describeConfig(ConfigSchema& schema) const override;
    void getConfig(JsonObject config) const override;
    void setConfig(JsonObjectConst config) override;

    void activate() override;
    void render(IDisplay& display, uint32_t nowMs) override;

private:
    void applySettings();

    std::string _message = "Hello from the Matrix Controller!";
    Color _color = Colors::Orange;
    int32_t _speed = 40;
    int32_t _size = 2;

    ScrollingText _scroller;
};

#endif
