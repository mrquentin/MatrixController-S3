#pragma once

#ifndef _CONFIGSCHEMA_h
#define _CONFIGSCHEMA_h

#include <cstdint>
#include <initializer_list>
#include <utility>
#include <ArduinoJson.h>
#include "display/Color.h"

/// Describes an app's settings so the web UI can build a form for them.
/// Each call adds one field; key is the name used in the app's config JSON.
class ConfigSchema {
public:
    explicit ConfigSchema(JsonArray fields);

    void text(const char* key, const char* label, uint16_t maxLength = 64);
    void number(const char* key, const char* label, int32_t min, int32_t max);
    void toggle(const char* key, const char* label);
    /// Stored in the config as a "#RRGGBB" string, see readColor/writeColor.
    void color(const char* key, const char* label);
    /// options are {value, label} pairs; the value is what ends up in the config.
    void select(const char* key, const char* label,
                std::initializer_list<std::pair<const char*, const char*>> options);

private:
    JsonObject add(const char* key, const char* label, const char* type);

    JsonArray _fields;
};

/// Helpers for reading and writing app config values. Readers return fallback when the
/// value is missing or has the wrong type, so apps keep their current setting.
Color readColor(JsonVariantConst value, Color fallback);
void writeColor(JsonObject config, const char* key, Color color);
int32_t readNumber(JsonVariantConst value, int32_t fallback, int32_t min, int32_t max);

#endif
