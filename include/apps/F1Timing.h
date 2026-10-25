#pragma once

#ifndef _F1TIMING_h
#define _F1TIMING_h

#include <cstddef>
#include <cstdint>
#include <string>
#include <ArduinoJson.h>

/// Reads F1 live timing as served by MultiViewer's GraphQL API (f1LiveTimingState and
/// f1LiveTimingClock). Pure parsing and time math, no networking or drawing, so it can be tested
/// off the board.
namespace F1 {
    enum class SessionKind : uint8_t { Practice, Qualifying, SprintQualifying, Sprint, Race, Unknown };

    /// What the bottom band shows, from most to least important.
    enum class Flag : uint8_t { None, Green, Yellow, DoubleYellow, SafetyCar, VirtualSafetyCar, VscEnding, Red, Chequered };

    /// A driver in the session's top positions.
    struct Driver {
        std::string code;             // three-letter code, e.g. "VER"
        std::string bestLap;          // e.g. "1:17.432", empty before a timed lap (in qualifying: in this part)
        uint32_t teamColor = 0xFFFFFF;
    };

    constexpr int TopCount = 3;

    struct LiveState {
        std::string meeting;          // e.g. "Italian Grand Prix"
        SessionKind kind = SessionKind::Unknown;
        int practiceNumber = 0;       // 1-3 for practice sessions
        std::string sessionStatus;    // "Inactive", "Started", "Aborted", "Finished", "Finalised", "Ends"
        int trackStatus = 0;          // 1 clear, 2 yellow, 4 SC, 5 red, 6 VSC, 7 VSC ending
        // TrackStatus only says "yellow"; whether a sector has a double yellow comes from the race
        // control messages, which are only fetched while TrackStatus is yellow
        bool doubleYellow = false;
        int currentLap = 0;           // 0 when the session has no lap count
        int totalLaps = 0;
        int qualifyingPart = 0;       // 1-3 during (sprint) qualifying, 0 otherwise

        // Top positions, only filled when TimingData.Lines and DriverList were fetched
        Driver top[TopCount];
        int topCount = 0;             // leading entries of top that are known

        // Session clock: remainingMs was the time left at clockUtcMs (track time)
        bool hasClock = false;
        int64_t remainingMs = 0;
        bool extrapolating = false;   // true while the clock is running
        int64_t clockUtcMs = 0;

        // Track time when the data was fetched (follows MultiViewer's video, replays included)
        int64_t trackTimeMs = 0;
        bool clockPaused = false;
        uint32_t fetchedAtMs = 0;     // board millis() when the data was fetched
    };

    /// GraphQL query for the next poll. last is the previous answer (nullptr if none): its session
    /// type and flag decide which of the large topics are worth fetching.
    std::string buildQuery(const LiveState* last);

    /// Filter for the GraphQL response that keeps only what parseLiveState reads, so large topics
    /// like TimingData never take more memory than the few fields used.
    void buildResponseFilter(JsonDocument& filter);

    /// Parses the "data" object of the GraphQL response. Returns false if there is no session.
    /// hostNowMs is the current time in ms since the epoch (from NTP), or 0 if unknown: MultiViewer's
    /// trackTime is only refreshed on play/seek, paired with the host time it was taken at, so the
    /// track time now is trackTime + (now - systemTime).
    bool parseLiveState(JsonObjectConst data, uint32_t fetchedAtMs, int64_t hostNowMs, LiveState& state);

    /// Time left in the session at board time nowMs, or -1 if the session has no clock.
    int64_t remainingMs(const LiveState& state, uint32_t nowMs);

    Flag currentFlag(const LiveState& state);

    /// True for sessions that report a qualifying part (Q1-Q3 / SQ1-SQ3).
    bool isQualifying(SessionKind kind);

    /// True for sessions that show the top drivers, which needs TimingData and DriverList.
    bool showsTopDrivers(SessionKind kind);

    SessionKind sessionKind(const char* type, const char* name);

    /// "2024-09-01T13:05:12.345Z" to milliseconds since the epoch. Fractions and the Z are optional.
    bool parseIsoUtc(const char* text, int64_t& ms);

    /// "01:23:45", "23:45" or "01:23:45.678" to milliseconds.
    bool parseDuration(const char* text, int64_t& ms);

    /// Session clock text: "1:02:03" from one hour up, "12:34" below.
    void formatClock(int64_t ms, char* out, size_t size);

    /// Shortens "Grand Prix" to "GP" so meeting names fit on one line.
    std::string shortMeetingName(const std::string& meeting);
}

#endif
