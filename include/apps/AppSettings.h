#pragma once

#ifndef _APPSETTINGS_h
#define _APPSETTINGS_h

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <ArduinoJson.h>
#include "display/Color.h"

/// An app's settings, declared once in App::settings(): each call names a setting, labels it for the
/// web UI and binds it to the app's variable. Depending on how it was made, the same calls describe
/// the settings for the web UI's form, write their current values, or apply new ones. New values are
/// corrected (clamped, cut, normalized) rather than refused; missing or mistyped ones change nothing.
class AppSettings {
public:
    /// Lists the settings as form fields: [{key, label, type, ...}]
    static AppSettings describe(JsonArray fields);
    /// Writes every setting's current value into values
    static AppSettings read(JsonObject values);
    /// Applies the settings present in values
    static AppSettings write(JsonObjectConst values);

    /// Turns what was typed into the value to keep, e.g. trimmed. Applied before the length limit.
    using Normalize = std::string (*)(std::string_view text);

    /// maxLength is in bytes of UTF-8
    void text(const char* key, const char* label, std::string& value, uint16_t maxLength = 64,
              Normalize normalize = nullptr);
    void number(const char* key, const char* label, int32_t& value, int32_t min, int32_t max);
    void toggle(const char* key, const char* label, bool& value);
    /// Kept as a "#RRGGBB" string in the JSON
    void color(const char* key, const char* label, Color& value);
    /// options are {value, label} pairs; values not among them are ignored
    void select(const char* key, const char* label, std::string& value,
                std::initializer_list<std::pair<const char*, const char*>> options);

private:
    enum class Mode : uint8_t { Describe, Read, Write };

    explicit AppSettings(Mode mode) : _mode(mode) {}
    JsonObject field(const char* key, const char* label, const char* type);

    Mode _mode;
    JsonArray _fields;
    JsonObject _out;
    JsonObjectConst _in;
};

#endif
