#include "apps/ClockApp.h"

#include <cstdio>
#include <ctime>
#include "display/Text.h"

namespace {
    // Before the first NTP sync the clock starts at 1970
    constexpr int FirstValidYear = 2024;
    constexpr int32_t DateGap = 6;
    constexpr int32_t SuffixGap = 2;

    int32_t centeredX(const IDisplay& display, const int32_t width) {
        return (static_cast<int32_t>(display.width()) - width) / 2;
    }
}

void ClockApp::describeConfig(ConfigSchema& schema) const {
    schema.toggle("use24Hour", "24-hour format");
    schema.toggle("showSeconds", "Show seconds");
    schema.toggle("showDate", "Show date");
    schema.color("timeColor", "Time color");
    schema.color("dateColor", "Date color");
}

void ClockApp::getConfig(JsonObject config) const {
    config["use24Hour"] = _use24Hour;
    config["showSeconds"] = _showSeconds;
    config["showDate"] = _showDate;
    writeColor(config, "timeColor", _timeColor);
    writeColor(config, "dateColor", _dateColor);
}

void ClockApp::setConfig(const JsonObjectConst config) {
    _use24Hour = config["use24Hour"] | _use24Hour;
    _showSeconds = config["showSeconds"] | _showSeconds;
    _showDate = config["showDate"] | _showDate;
    _timeColor = readColor(config["timeColor"], _timeColor);
    _dateColor = readColor(config["dateColor"], _dateColor);
}

void ClockApp::render(IDisplay& display, uint32_t) {
    const time_t now = time(nullptr);
    tm local{};
    localtime_r(&now, &local);

    if (local.tm_year + 1900 < FirstValidYear) {
        const char* waiting = "Syncing time...";
        drawText(display, centeredX(display, textWidth(waiting)),
                 (static_cast<int32_t>(display.height()) - textHeight()) / 2, waiting, _dateColor);
        return;
    }

    int hour = local.tm_hour;
    if (!_use24Hour) {
        hour %= 12;
        if (hour == 0) hour = 12;
    }

    char timeText[12];
    if (_showSeconds) {
        snprintf(timeText, sizeof(timeText), _use24Hour ? "%02d:%02d:%02d" : "%d:%02d:%02d",
                 hour, local.tm_min, local.tm_sec);
    } else {
        snprintf(timeText, sizeof(timeText), _use24Hour ? "%02d:%02d" : "%d:%02d", hour, local.tm_min);
    }

    // As big as fits on 128 px: "00:00" at 3x, "00:00:00" at 2x
    const uint8_t scale = _showSeconds ? 2 : 3;
    const char* suffix = _use24Hour ? "" : (local.tm_hour < 12 ? "AM" : "PM");
    const int32_t timeWidth = textWidth(timeText, Fonts::Font5x7, scale);
    const int32_t suffixWidth = _use24Hour ? 0 : SuffixGap + textWidth(suffix);

    char dateText[16];
    strftime(dateText, sizeof(dateText), "%a %d %b", &local);

    // Center the time (and date) block vertically
    const int32_t timeHeight = textHeight(Fonts::Font5x7, scale);
    const int32_t blockHeight = _showDate ? timeHeight + DateGap + textHeight() : timeHeight;
    const int32_t top = (static_cast<int32_t>(display.height()) - blockHeight) / 2;

    const int32_t x = centeredX(display, timeWidth + suffixWidth);
    drawText(display, x, top, timeText, _timeColor, Fonts::Font5x7, scale);
    if (!_use24Hour) drawText(display, x + timeWidth + SuffixGap, top, suffix, _timeColor);

    if (_showDate) {
        drawText(display, centeredX(display, textWidth(dateText)), top + timeHeight + DateGap, dateText, _dateColor);
    }
}
