#pragma once

#ifndef _SYSTEMMANAGER_h
#define _SYSTEMMANAGER_h

#include <atomic>
#include <cstdint>
#include <string>
#include <ArduinoJson.h>

class SystemManager {
public:
    /// Writes the board's state for the web UI's status page: WiFi, memory, chip, uptime, last reset.
    void describe(JsonObject out) const;

    /// Asks for a restart once delayMs have passed, so the web request that asked for it can still
    /// be answered. note is shown on the panel meanwhile (e.g. the network about to be joined).
    /// Safe to call from any task; the restart itself happens in restartIfDue().
    void scheduleRestart(const std::string& note, uint32_t delayMs = 2000);
    [[nodiscard]] bool restartPending() const { return _restartAtMs != 0; }
    /// Only read once restartPending() is true
    [[nodiscard]] const std::string& restartNote() const { return _restartNote; }
    /// Restarts if a scheduled restart is due. Call from loop().
    void restartIfDue(uint32_t now) const;

private:
    std::string _restartNote;
    // millis() at which to restart, 0 when none is scheduled
    std::atomic<uint32_t> _restartAtMs{0};
};

#endif
