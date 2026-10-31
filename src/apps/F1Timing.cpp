#include "apps/F1Timing.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace F1 {
    namespace {
        /// Integers arrive as numbers or as strings depending on the field (TrackStatus.Status
        /// is "1", GraphQL BigInts may be strings), so accept both.
        int64_t readInt(const JsonVariantConst value, const int64_t fallback = 0) {
            if (value.is<int64_t>()) return value.as<int64_t>();
            if (const char* text = value.as<const char*>()) {
                char* end = nullptr;
                const long long parsed = strtoll(text, &end, 10);
                if (end != text) return parsed;
            }
            return fallback;
        }

        bool contains(const char* text, const char* part) {
            return text && strstr(text, part) != nullptr;
        }

        // Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's days_from_civil)
        int64_t daysFromCivil(int64_t year, const unsigned month, const unsigned day) {
            year -= month <= 2;
            const int64_t era = (year >= 0 ? year : year - 399) / 400;
            const auto yearOfEra = static_cast<unsigned>(year - era * 400);
            const unsigned dayOfYear = (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
            const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
            return era * 146097 + static_cast<int64_t>(dayOfEra) - 719468;
        }

        /// Reads a fraction of a second after the '.' into milliseconds, ignoring extra digits.
        int64_t readMillis(const char*& p) {
            int64_t millis = 0;
            int digits = 0;
            while (isdigit(static_cast<unsigned char>(*p))) {
                if (digits < 3) millis = millis * 10 + (*p - '0');
                ++digits;
                ++p;
            }
            for (; digits < 3; ++digits) millis *= 10;
            return millis;
        }
    }

    SessionKind sessionKind(const char* type, const char* name) {
        // Sprint qualifying was called "Sprint Shootout" in 2023
        const bool sprint = contains(name, "Sprint");
        if (contains(type, "Practice")) return SessionKind::Practice;
        if (contains(type, "Qualifying") || contains(name, "Qualifying") || contains(name, "Shootout")) {
            return sprint ? SessionKind::SprintQualifying : SessionKind::Qualifying;
        }
        if (contains(type, "Race")) return sprint ? SessionKind::Sprint : SessionKind::Race;
        return SessionKind::Unknown;
    }

    bool isQualifying(const SessionKind kind) {
        return kind == SessionKind::Qualifying || kind == SessionKind::SprintQualifying;
    }

    bool showsTopDrivers(const SessionKind kind) {
        return kind == SessionKind::Practice || isQualifying(kind);
    }

    bool parseIsoUtc(const char* text, int64_t& ms) {
        if (!text) return false;
        int year, month, day, hour, minute, second, consumed = 0;
        if (sscanf(text, "%4d-%2d-%2dT%2d:%2d:%2d%n", &year, &month, &day, &hour, &minute, &second, &consumed) != 6) {
            return false;
        }
        const char* p = text + consumed;
        int64_t millis = 0;
        if (*p == '.') {
            ++p;
            millis = readMillis(p);
        }

        const int64_t days = daysFromCivil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
        ms = ((days * 24 + hour) * 60 + minute) * 60 * 1000 + static_cast<int64_t>(second) * 1000 + millis;
        return true;
    }

    bool parseDuration(const char* text, int64_t& ms) {
        if (!text) return false;
        int parts[3] = {0, 0, 0};
        int count = 0;
        const char* p = text;
        while (count < 3) {
            if (!isdigit(static_cast<unsigned char>(*p))) return false;
            int value = 0;
            while (isdigit(static_cast<unsigned char>(*p))) value = value * 10 + (*p++ - '0');
            parts[count++] = value;
            if (*p != ':') break;
            ++p;
        }
        if (count < 2) return false;

        int64_t millis = 0;
        if (*p == '.') {
            ++p;
            millis = readMillis(p);
        }

        // "MM:SS" or "HH:MM:SS"
        const int hours = count == 3 ? parts[0] : 0;
        const int minutes = count == 3 ? parts[1] : parts[0];
        const int seconds = count == 3 ? parts[2] : parts[1];
        ms = ((static_cast<int64_t>(hours) * 60 + minutes) * 60 + seconds) * 1000 + millis;
        return true;
    }

    std::string buildQuery(const LiveState* last) {
        std::string topics = "SessionInfo SessionStatus TrackStatus LapCount ExtrapolatedClock";
        if (last) {
            // Qualifying needs TimingData for SessionPart, practice its Lines plus DriverList for the codes
            if (isQualifying(last->kind) || showsTopDrivers(last->kind)) topics += " TimingData";
            if (showsTopDrivers(last->kind)) topics += " DriverList";
        }
        // Grows to hundreds of messages over a race, but it is the only source for double yellows
        // and pit lane closures; the response filter keeps only a few short fields of each
        topics += " RaceControlMessages";
        return "{ f1LiveTimingState { " + topics + " } f1LiveTimingClock { paused trackTime systemTime } }";
    }

    void buildResponseFilter(JsonDocument& filter) {
        JsonObject timing = filter["data"]["f1LiveTimingState"].to<JsonObject>();
        timing["SessionInfo"]["Meeting"]["Name"] = true;
        timing["SessionInfo"]["Type"] = true;
        timing["SessionInfo"]["Name"] = true;
        timing["SessionInfo"]["Number"] = true;
        timing["SessionStatus"]["Status"] = true;
        timing["TrackStatus"]["Status"] = true;
        timing["LapCount"] = true;
        timing["ExtrapolatedClock"] = true;
        timing["TimingData"]["SessionPart"] = true;
        // "*" matches every car number
        timing["TimingData"]["Lines"]["*"]["Position"] = true;
        timing["TimingData"]["Lines"]["*"]["BestLapTime"]["Value"] = true;
        // Qualifying: one best time per part, [0] applies to every element
        timing["TimingData"]["Lines"]["*"]["BestLapTimes"][0]["Value"] = true;
        timing["DriverList"]["*"]["Tla"] = true;
        timing["DriverList"]["*"]["TeamColour"] = true;
        // Messages is an array; [0] applies to every element
        timing["RaceControlMessages"]["Messages"][0]["Flag"] = true;
        timing["RaceControlMessages"]["Messages"][0]["Scope"] = true;
        timing["RaceControlMessages"]["Messages"][0]["Sector"] = true;
        timing["RaceControlMessages"]["Messages"][0]["Message"] = true;
        filter["data"]["f1LiveTimingClock"] = true;
    }

    bool parseLiveState(const JsonObjectConst data, const uint32_t fetchedAtMs, const int64_t hostNowMs,
                        LiveState& state) {
        const JsonObjectConst timing = data["f1LiveTimingState"];
        const JsonObjectConst info = timing["SessionInfo"];
        if (info.isNull()) return false;

        state = LiveState{};
        state.fetchedAtMs = fetchedAtMs;
        state.meeting = info["Meeting"]["Name"] | "";
        const char* type = info["Type"] | "";
        const char* name = info["Name"] | "";
        state.kind = sessionKind(type, name);
        if (state.kind == SessionKind::Practice) {
            state.practiceNumber = static_cast<int>(readInt(info["Number"]));
            // Fall back to the digit in "Practice 2"
            if (state.practiceNumber == 0) {
                for (const char* p = name; *p; ++p) {
                    if (isdigit(static_cast<unsigned char>(*p))) {
                        state.practiceNumber = *p - '0';
                        break;
                    }
                }
            }
        }

        state.sessionStatus = timing["SessionStatus"]["Status"] | "";
        state.trackStatus = static_cast<int>(readInt(timing["TrackStatus"]["Status"]));
        state.currentLap = static_cast<int>(readInt(timing["LapCount"]["CurrentLap"]));
        state.totalLaps = static_cast<int>(readInt(timing["LapCount"]["TotalLaps"]));
        state.qualifyingPart = static_cast<int>(readInt(timing["TimingData"]["SessionPart"]));

        // Replay the flag messages per marshal sector: a yellow or double yellow holds until that
        // sector (or the whole track) is cleared
        constexpr int MaxSectors = 64;
        enum : uint8_t { NoFlag, YellowFlag, DoubleYellowFlag };
        uint8_t sectors[MaxSectors] = {};
        for (const JsonObjectConst message : timing["RaceControlMessages"]["Messages"].as<JsonArrayConst>()) {
            applyPitMessage(message["Message"] | "", state);

            const char* flag = message["Flag"] | "";
            const char* scope = message["Scope"] | "";
            const bool clear = strcmp(flag, "CLEAR") == 0 || strcmp(flag, "GREEN") == 0;
            if (strcmp(scope, "Track") == 0) {
                if (clear) memset(sectors, NoFlag, sizeof(sectors));
            } else if (strcmp(scope, "Sector") == 0) {
                const auto sector = readInt(message["Sector"], -1);
                if (sector < 0 || sector >= MaxSectors) continue;
                if (clear) sectors[sector] = NoFlag;
                else if (strcmp(flag, "DOUBLE YELLOW") == 0) sectors[sector] = DoubleYellowFlag;
                else if (strcmp(flag, "YELLOW") == 0) sectors[sector] = YellowFlag;
            }
        }
        for (const uint8_t sector : sectors) state.doubleYellow |= sector == DoubleYellowFlag;

        // TimingData.Lines and DriverList are both keyed by car number
        const JsonObjectConst drivers = timing["DriverList"];
        bool filled[TopCount] = {};
        for (const JsonPairConst line : timing["TimingData"]["Lines"].as<JsonObjectConst>()) {
            const auto position = readInt(line.value()["Position"]);
            if (position < 1 || position > TopCount) continue;

            Driver& driver = state.top[position - 1];
            const JsonObjectConst info = drivers[line.key().c_str()];
            driver.code = info["Tla"] | line.key().c_str();
            // In qualifying, the time that counts is the best one in the current part ("--" until set);
            // BestLapTime is the best of the whole session
            const JsonArrayConst partTimes = line.value()["BestLapTimes"];
            if (state.qualifyingPart > 0 && !partTimes.isNull()) {
                driver.bestLap = partTimes[state.qualifyingPart - 1]["Value"] | "";
            } else {
                driver.bestLap = line.value()["BestLapTime"]["Value"] | "";
            }
            if (const char* colour = info["TeamColour"]) driver.teamColor = strtoul(colour, nullptr, 16);
            filled[position - 1] = true;
        }
        while (state.topCount < TopCount && filled[state.topCount]) ++state.topCount;

        const JsonObjectConst clock = timing["ExtrapolatedClock"];
        state.hasClock = parseDuration(clock["Remaining"] | "", state.remainingMs) &&
                         parseIsoUtc(clock["Utc"] | "", state.clockUtcMs);
        state.extrapolating = clock["Extrapolating"] | false;

        const JsonObjectConst trackClock = data["f1LiveTimingClock"];
        state.trackTimeMs = readInt(trackClock["trackTime"]);
        state.clockPaused = trackClock["paused"] | false;
        // trackTime was taken at systemTime (last play/seek), so move it on to now while playing.
        // The board's and the PC's clocks are both NTP-synced, which is close enough for a 1 s display
        const int64_t systemTimeMs = readInt(trackClock["systemTime"]);
        if (!state.clockPaused && hostNowMs > 0 && systemTimeMs > 0 && state.trackTimeMs > 0) {
            state.trackTimeMs += hostNowMs - systemTimeMs;
        }
        return true;
    }

    int64_t remainingMs(const LiveState& state, const uint32_t nowMs) {
        if (!state.hasClock) return -1;
        if (!state.extrapolating || state.trackTimeMs == 0) return state.remainingMs;

        // Track time now: the fetched track time, moved on by the time since the fetch unless paused
        const int64_t trackNow = state.trackTimeMs + (state.clockPaused ? 0 : static_cast<int64_t>(nowMs - state.fetchedAtMs));
        const int64_t remaining = state.remainingMs - (trackNow - state.clockUtcMs);
        if (remaining < 0) return 0;
        return remaining > state.remainingMs ? state.remainingMs : remaining;
    }

    Flag currentFlag(const LiveState& state) {
        if (state.sessionStatus == "Finished" || state.sessionStatus == "Finalised") return Flag::Chequered;
        switch (state.trackStatus) {
            case 1: return Flag::Green;
            case 2: return state.doubleYellow ? Flag::DoubleYellow : Flag::Yellow;
            case 4: return Flag::SafetyCar;
            case 5: return Flag::Red;
            case 6: return Flag::VirtualSafetyCar;
            case 7: return Flag::VscEnding;
            default: return Flag::None;
        }
    }

    void applyPitMessage(const char* message, LiveState& state) {
        if (!contains(message, "PIT") || contains(message, "WILL")) return;
        const bool closed = contains(message, "CLOSED");
        if (!closed && !contains(message, "OPEN")) return;

        bool entry = contains(message, "ENTRY");
        bool exit = contains(message, "EXIT");
        // "PIT LANE CLOSED": both ends
        if (!entry && !exit && contains(message, "PIT LANE")) entry = exit = true;
        if (entry) state.pitEntryClosed = closed;
        if (exit) state.pitExitClosed = closed;
    }

    PitLane pitLane(const LiveState& state) {
        if (currentFlag(state) == Flag::Chequered) return PitLane::Open;
        // The pit exit is closed while the session is red flagged, message or not
        const bool exitClosed = state.pitExitClosed || state.trackStatus == 5 || state.sessionStatus == "Aborted";
        if (state.pitEntryClosed) return exitClosed ? PitLane::Closed : PitLane::EntryClosed;
        return exitClosed ? PitLane::ExitClosed : PitLane::Open;
    }

    void formatClock(const int64_t ms, char* out, const size_t size) {
        // Round up, like the official timing: "0:01" until the clock actually reaches zero
        const int64_t totalSeconds = ms <= 0 ? 0 : (ms + 999) / 1000;
        const auto hours = static_cast<int>(totalSeconds / 3600);
        const auto minutes = static_cast<int>(totalSeconds / 60 % 60);
        const auto seconds = static_cast<int>(totalSeconds % 60);
        if (hours > 0) {
            snprintf(out, size, "%d:%02d:%02d", hours, minutes, seconds);
        } else {
            snprintf(out, size, "%02d:%02d", minutes, seconds);
        }
    }

    std::string shortMeetingName(const std::string& meeting) {
        std::string name = meeting;
        const auto position = name.find("Grand Prix");
        if (position != std::string::npos) name.replace(position, strlen("Grand Prix"), "GP");
        return name;
    }
}
