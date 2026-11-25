#include "apps/ClockApp.h"

#include <cstdio>
#include <ctime>
#include "SystemClock.h"
#include "display/Text.h"

namespace {
    constexpr int32_t DateGap = 6;
    constexpr int32_t SuffixGap = 2;
}

void ClockApp::settings(AppSettings& settings) {
    settings.toggle("use24Hour", "24-hour format", _use24Hour);
    settings.toggle("showSeconds", "Show seconds", _showSeconds);
    settings.toggle("showDate", "Show date", _showDate);
    settings.color("timeColor", "Time color", _timeColor);
    settings.color("dateColor", "Date color", _dateColor);
}

void ClockApp::render(IDisplay& display, uint32_t) {
    if (!SystemClock::synced()) {
        drawLines(display, {{"Syncing time...", _dateColor}});
        return;
    }

    const time_t now = time(nullptr);
    tm local{};
    localtime_r(&now, &local);

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

    const int32_t timeHeight = textHeight(Fonts::Font5x7, scale);
    const int32_t blockHeight = _showDate ? timeHeight + DateGap + textHeight() : timeHeight;
    const int32_t top = (static_cast<int32_t>(display.height()) - blockHeight) / 2;

    const int32_t x = (static_cast<int32_t>(display.width()) - timeWidth - suffixWidth) / 2;
    drawText(display, x, top, timeText, _timeColor, Fonts::Font5x7, scale);
    if (!_use24Hour) drawText(display, x + timeWidth + SuffixGap, top, suffix, _timeColor);

    if (_showDate) drawTextCentered(display, top + timeHeight + DateGap, dateText, _dateColor);
}
