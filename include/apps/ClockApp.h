#pragma once

#ifndef _CLOCKAPP_h
#define _CLOCKAPP_h

#include "apps/App.h"

/// Shows the local time, optionally with seconds and the date. Uses the system clock, which is
/// synced over NTP and follows the timezone from the system configuration.
class ClockApp final : public App {
public:
    [[nodiscard]] const char* id() const override { return "clock"; }
    [[nodiscard]] const char* name() const override { return "Clock"; }

    void describeConfig(ConfigSchema& schema) const override;
    void getConfig(JsonObject config) const override;
    void setConfig(JsonObjectConst config) override;

    void render(IDisplay& display, uint32_t nowMs) override;

private:
    bool _use24Hour = true;
    bool _showSeconds = false;
    bool _showDate = true;
    Color _timeColor = Colors::Cyan;
    Color _dateColor = Colors::White;
};

#endif
