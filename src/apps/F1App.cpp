#include "apps/F1App.h"

#include <Arduino.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>
#include "display/Text.h"
#include "net/HttpJson.h"

namespace {
    constexpr uint16_t MaxHostLength = 64;
    // Flags must show up quickly; MultiViewer is on the local network, so polling is cheap
    constexpr uint32_t PollIntervalMs = 1000;
    constexpr uint32_t RetryIntervalMs = 5000;
    constexpr uint16_t RequestTimeoutMs = 3000;
    // A few failed polls in a row are tolerated before the data is treated as gone
    constexpr uint32_t StaleAfterMs = 15000;
    constexpr uint32_t TaskStackSize = 8 * 1024;

    // A new flag takes the whole screen this long, then moves to the bottom band
    constexpr uint32_t FlagFullScreenMs = 5000;
    // The green flag's band then goes away after this long; other flags keep their band
    constexpr uint32_t GreenBandMs = 10000;

    constexpr Color MeetingColor{170, 170, 180};
    constexpr Color LabelColor{140, 140, 150};
    constexpr Color MessageDetailColor{140, 140, 150};

    constexpr int32_t BandHeight = 14;

    struct FlagStyle {
        const char* band;       // one line, for the bottom band
        const char* lines[2];   // full screen: two lines at 2x, or one line at 3x if the second is empty
        Color background;
        Color foreground;
    };

    FlagStyle styleFor(const F1::Flag flag) {
        switch (flag) {
            case F1::Flag::Green: return {"GREEN FLAG", {"GREEN", "FLAG"}, Color{0, 120, 0}, Colors::White};
            case F1::Flag::Yellow: return {"YELLOW FLAG", {"YELLOW", "FLAG"}, Color{255, 210, 0}, Colors::Black};
            case F1::Flag::DoubleYellow:
                return {"DOUBLE YELLOW", {"DOUBLE", "YELLOW"}, Color{255, 210, 0}, Colors::Black};
            case F1::Flag::SafetyCar: return {"SAFETY CAR", {"SAFETY", "CAR"}, Color{255, 170, 0}, Colors::Black};
            case F1::Flag::VirtualSafetyCar: return {"VSC", {"VSC", ""}, Color{255, 170, 0}, Colors::Black};
            case F1::Flag::VscEnding: return {"VSC ENDING", {"VSC", "ENDING"}, Color{255, 210, 0}, Colors::Black};
            case F1::Flag::Red: return {"RED FLAG", {"RED", "FLAG"}, Color{220, 0, 0}, Colors::White};
            case F1::Flag::Chequered: return {"CHEQUERED FLAG", {"CHEQUERED", "FLAG"}, Colors::White, Colors::Black};
            case F1::Flag::None: break;
        }
        return {"", {"", ""}, Colors::Black, Colors::Black};
    }

    int32_t centeredX(const IDisplay& display, const int32_t width) {
        return (static_cast<int32_t>(display.width()) - width) / 2;
    }

    void drawCenteredLine(IDisplay& display, const int32_t y, const char* text, const Color color,
                          const uint8_t scale = 1) {
        drawText(display, centeredX(display, textWidth(text, Fonts::Font5x7, scale)), y, text, color,
                 Fonts::Font5x7, scale);
    }

    void drawRightAligned(IDisplay& display, const int32_t right, const int32_t y, const char* text,
                          const Color color, const uint8_t scale = 1) {
        drawText(display, right - textWidth(text, Fonts::Font5x7, scale), y, text, color, Fonts::Font5x7, scale);
    }

    void drawMessage(IDisplay& display, const char* first, const char* second, const char* third = nullptr) {
        const char* lines[] = {first, second, third};
        int32_t count = 0;
        for (const char* line : lines) count += line != nullptr;

        const int32_t lineHeight = textHeight() + 3;
        int32_t y = (static_cast<int32_t>(display.height()) - count * lineHeight + 3) / 2;
        for (int32_t i = 0; i < 3; ++i) {
            if (!lines[i]) continue;
            drawCenteredLine(display, y, lines[i], i == 0 ? Colors::White : MessageDetailColor);
            y += lineHeight;
        }
    }

    void drawCheckerboard(IDisplay& display, const int16_t top, const int16_t height, const int16_t square) {
        const auto width = static_cast<int16_t>(display.width());
        for (int16_t y = 0; y < height; y += square) {
            for (int16_t x = 0; x < width; x += square) {
                const bool white = ((x / square) + (y / square)) % 2 == 0;
                display.fillRect(x, static_cast<int16_t>(top + y), square, square, white ? Colors::White : Colors::Black);
            }
        }
    }

    void drawFlagBand(IDisplay& display, const F1::Flag flag) {
        const FlagStyle style = styleFor(flag);
        const auto top = static_cast<int16_t>(display.height() - BandHeight);
        const int32_t textY = top + (BandHeight - textHeight()) / 2;

        if (flag == F1::Flag::Chequered) {
            // Checkerboard with the text on a black plate in the middle
            drawCheckerboard(display, top, BandHeight, 4);
            const int32_t plateWidth = textWidth(style.band) + 6;
            display.fillRect(static_cast<int16_t>(centeredX(display, plateWidth)), static_cast<int16_t>(top + 2),
                             static_cast<int16_t>(plateWidth), BandHeight - 4, Colors::Black);
            drawCenteredLine(display, textY, style.band, Colors::White);
            return;
        }

        display.fillRect(0, top, static_cast<int16_t>(display.width()), BandHeight, style.background);
        drawCenteredLine(display, textY, style.band, style.foreground);
    }

    /// True for flags announced full screen before moving to the band.
    bool hasFullScreen(const F1::Flag flag) {
        // The chequered flag ends the session, the band is enough
        return flag != F1::Flag::None && flag != F1::Flag::Chequered;
    }

    void drawFlagFullScreen(IDisplay& display, const F1::Flag flag) {
        const FlagStyle style = styleFor(flag);
        const auto height = static_cast<int32_t>(display.height());
        display.fill(style.background);

        if (style.lines[1][0] == '\0') {
            drawCenteredLine(display, (height - textHeight(Fonts::Font5x7, 3)) / 2, style.lines[0], style.foreground, 3);
            return;
        }

        const int32_t lineHeight = textHeight(Fonts::Font5x7, 2);
        constexpr int32_t Gap = 5;
        const int32_t top = (height - 2 * lineHeight - Gap) / 2;
        drawCenteredLine(display, top, style.lines[0], style.foreground, 2);
        drawCenteredLine(display, top + lineHeight + Gap, style.lines[1], style.foreground, 2);
    }
}

namespace {
    /// Current time in ms since the epoch, or 0 before the first NTP sync (the clock starts at 1970).
    int64_t currentEpochMs() {
        timeval tv{};
        gettimeofday(&tv, nullptr);
        constexpr time_t FirstValidSecond = 1704067200;  // 2024-01-01
        if (tv.tv_sec < FirstValidSecond) return 0;
        return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    }
}

F1App::F1App() : _task("f1", TaskStackSize, [this] { poll(); }) {
    _task.setInterval(PollIntervalMs);
}

void F1App::describeConfig(ConfigSchema& schema) const {
    schema.text("host", "MultiViewer address (IP of the PC)", MaxHostLength);
    schema.number("port", "MultiViewer port", 1, 65535);
}

void F1App::getConfig(JsonObject config) const {
    std::lock_guard lock(_mutex);
    config["host"] = _host;
    config["port"] = _port;
}

void F1App::setConfig(const JsonObjectConst config) {
    std::lock_guard lock(_mutex);

    std::string host = _host;
    if (const char* text = config["host"]) {
        // Accept a pasted URL like "http://192.168.1.20:10101/" and keep only the host
        host = text;
        const auto scheme = host.find("://");
        if (scheme != std::string::npos) host.erase(0, scheme + 3);
        const auto end = host.find_first_of(":/ ");
        if (end != std::string::npos) host.erase(end);
        host = host.substr(0, MaxHostLength);
    }
    const int32_t port = readNumber(config["port"], _port, 1, 65535);

    if (host == _host && port == _port) return;
    _host = host;
    _port = port;
    ++_generation;
    _status = _host.empty() ? Status::NotConfigured : Status::Connecting;
    _task.runNow();
}

void F1App::activate() {
    {
        std::lock_guard lock(_mutex);
        // A flag that changed while another app was shown isn't new anymore
        _flagKnown = false;
    }
    _task.setEnabled(true);
}

void F1App::deactivate() {
    _task.setEnabled(false);
}

void F1App::poll() {
    String url;
    std::string query;
    uint32_t generation;
    {
        std::lock_guard lock(_mutex);
        if (_host.empty()) return;
        url = "http://" + String(_host.c_str()) + ":" + String(_port) + "/api/graphql";
        // The last answer's session type and flag decide how much to fetch
        query = F1::buildQuery(_status == Status::Live ? &_state : nullptr);
        generation = _generation;
    }

    JsonDocument body;
    body["query"] = query.c_str();
    String payload;
    serializeJson(body, payload);

    // Keep only what is displayed, so TimingData's per-driver details are never stored
    JsonDocument filter;
    F1::buildResponseFilter(filter);

    JsonDocument doc;
    const bool reached = postJson(url, payload, doc, &filter, RequestTimeoutMs);
    const uint32_t now = millis();

    F1::LiveState state;
    const bool live = reached && F1::parseLiveState(doc["data"], now, currentEpochMs(), state);

    std::lock_guard lock(_mutex);
    if (generation != _generation) return;

    if (live) {
        _state = state;
        _status = Status::Live;
        _lastSuccessMs = now;
    } else if (reached) {
        // MultiViewer answered, but no live timing session is open
        _status = Status::NoSession;
        _lastSuccessMs = now;
    } else if (_status != Status::Live || now - _lastSuccessMs > StaleAfterMs) {
        _status = Status::Unreachable;
    }
    _task.setInterval(reached ? PollIntervalMs : RetryIntervalMs);
}

void F1App::render(IDisplay& display, const uint32_t nowMs) {
    std::lock_guard lock(_mutex);

    if (_status != Status::Live) _flagKnown = false;

    char address[80];
    snprintf(address, sizeof(address), "%s:%ld", _host.c_str(), static_cast<long>(_port));

    switch (_status) {
        case Status::NotConfigured:
            drawMessage(display, "Set the MultiViewer", "address in the", "web UI");
            break;
        case Status::Connecting:
            drawMessage(display, "Connecting to", "MultiViewer...", address);
            break;
        case Status::Unreachable:
            drawMessage(display, "MultiViewer", "not reachable", address);
            break;
        case Status::NoSession:
            drawMessage(display, "No live session", "Open live timing", "in MultiViewer");
            break;
        case Status::Live:
            renderLive(display, nowMs);
            break;
    }
}

void F1App::renderLive(IDisplay& display, const uint32_t nowMs) {
    using F1::Flag;
    using F1::SessionKind;

    const Flag flag = F1::currentFlag(_state);
    if (!_flagKnown) {
        // Whatever is out when the app starts showing isn't a change: no full screen, and a
        // green flag's band is already over
        _flagKnown = true;
        _flag = flag;
        _flagChangedMs = nowMs - FlagFullScreenMs - GreenBandMs;
    } else if (flag != _flag) {
        _flag = flag;
        _flagChangedMs = nowMs;
    }

    const uint32_t flagAge = nowMs - _flagChangedMs;
    if (hasFullScreen(flag) && flagAge < FlagFullScreenMs) {
        drawFlagFullScreen(display, flag);
        return;
    }

    constexpr int32_t LabelY = 12;
    constexpr int32_t ValueY = 22;
    char value[16];
    char label[24];

    switch (_state.kind) {
        case SessionKind::Race:
        case SessionKind::Sprint: {
            drawCenteredLine(display, 1, F1::shortMeetingName(_state.meeting).c_str(), MeetingColor);
            drawCenteredLine(display, LabelY, _state.kind == SessionKind::Race ? "LAP" : "SPRINT LAP", LabelColor);
            if (_state.totalLaps > 0) {
                snprintf(value, sizeof(value), "%d/%d", _state.currentLap, _state.totalLaps);
            } else {
                snprintf(value, sizeof(value), "%d", _state.currentLap);
            }
            drawCenteredLine(display, ValueY, value, Colors::White, 3);
            break;
        }

        case SessionKind::Qualifying:
        case SessionKind::SprintQualifying: {
            // "Q2" / "SQ2", or just "Q" before the first part is known
            const char* prefix = _state.kind == SessionKind::SprintQualifying ? "SQ" : "Q";
            if (_state.qualifyingPart > 0) {
                snprintf(label, sizeof(label), "%s%d", prefix, _state.qualifyingPart);
            } else {
                snprintf(label, sizeof(label), "%s", prefix);
            }
            renderTopDrivers(display, nowMs, label);
            break;
        }

        case SessionKind::Practice:
            if (_state.practiceNumber > 0) {
                snprintf(label, sizeof(label), "FP%d", _state.practiceNumber);
            } else {
                snprintf(label, sizeof(label), "FP");
            }
            renderTopDrivers(display, nowMs, label);
            break;

        case SessionKind::Unknown:
            drawCenteredLine(display, 1, F1::shortMeetingName(_state.meeting).c_str(), MeetingColor);
            snprintf(label, sizeof(label), "SESSION");
            drawCenteredLine(display, LabelY, label, LabelColor);
            if (_state.hasClock) {
                F1::formatClock(F1::remainingMs(_state, nowMs), value, sizeof(value));
                drawCenteredLine(display, ValueY, value, Colors::White, strlen(value) > 5 ? 2 : 3);
            }
            break;
    }

    const bool bandVisible = flag != Flag::None && (flag != Flag::Green || flagAge < FlagFullScreenMs + GreenBandMs);
    if (bandVisible) drawFlagBand(display, flag);
}

void F1App::renderTopDrivers(IDisplay& display, const uint32_t nowMs, const char* label) const {
    const auto width = static_cast<int32_t>(display.width());

    // Header: "FP2" / "Q2" on the left, time left on the right
    drawText(display, 2, 1, label, Colors::White, Fonts::Font5x7, 2);
    if (_state.hasClock) {
        char clock[16];
        F1::formatClock(F1::remainingMs(_state, nowMs), clock, sizeof(clock));
        drawRightAligned(display, width - 2, 1, clock, Colors::White, 2);
    }

    // Top 3: team color mark, position, driver code, best lap (in qualifying: in this part)
    if (_state.topCount == 0) {
        drawCenteredLine(display, 28, "No laps yet", LabelColor);
        return;
    }
    constexpr int32_t FirstRowY = 20;
    constexpr int32_t RowHeight = 10;
    for (int i = 0; i < _state.topCount; ++i) {
        const F1::Driver& driver = _state.top[i];
        const int32_t y = FirstRowY + i * RowHeight;

        display.fillRect(2, static_cast<int16_t>(y - 1), 2, 9, Color::fromHex(driver.teamColor));
        char position[4];
        snprintf(position, sizeof(position), "%d", i + 1);
        drawText(display, 7, y, position, LabelColor);
        drawText(display, 16, y, driver.code.c_str(), Colors::White);
        drawRightAligned(display, width - 2, y, driver.bestLap.empty() ? "--" : driver.bestLap.c_str(), Colors::White);
    }
}
