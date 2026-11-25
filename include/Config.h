#pragma once

#ifndef _CONFIG_h
#define _CONFIG_h

#include <atomic>
#include <mutex>
#include <string>
#include <ArduinoJson.h>
#include "display/DisplayConfig.h"

struct SystemSettings {
    bool debug = true;
    DisplayConfig display;
    /// POSIX TZ string, e.g. "CET-1CEST,M3.5.0,M10.5.0/3"
    std::string timezone = "UTC0";
};

/// The system settings, kept in a JSON file on LittleFS. Values that are missing or out of range
/// get their default, so what get() returns is always usable. Safe to use from any task.
class Config {
public:
    explicit Config(const char* filePath);

    void load();
    /// Replaces the settings with the ones in json and saves them. Returns false if they couldn't
    /// be saved; they apply until the next restart anyway.
    bool update(JsonObjectConst json);
    [[nodiscard]] SystemSettings get() const;
    /// Changes with every update(), so the loop can tell when to apply the settings.
    [[nodiscard]] uint32_t revision() const { return _revision; }

    /// Writes {config, limits} for the web UI: the settings, and the ranges it may pick from.
    void describe(JsonObject out) const;

private:
    void set(JsonObjectConst json);

    const char* _filePath;
    mutable std::mutex _mutex;
    SystemSettings _settings;
    std::atomic<uint32_t> _revision{0};
};

#endif
