#pragma once

#ifndef _SYSTEMCLOCK_h
#define _SYSTEMCLOCK_h

#include <cstdint>
#include <string>

/// The board's wall clock: synced over NTP, local time from a POSIX TZ string
/// (e.g. "CET-1CEST,M3.5.0,M10.5.0/3").
namespace SystemClock {
    /// Starts syncing in the background. Call once WiFi is up.
    void begin(const std::string& timezone);
    void setTimezone(const std::string& timezone);
    /// False until the first sync: the clock starts at 1970.
    [[nodiscard]] bool synced();
    /// Milliseconds since the epoch, or 0 until the first sync.
    [[nodiscard]] int64_t epochMs();
}

#endif
