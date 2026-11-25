#include "apps/AppSettings.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    bool parseColor(const char* text, Color& color) {
        if (!text || text[0] != '#' || strlen(text) != 7) return false;
        char* end = nullptr;
        const unsigned long rgb = strtoul(text + 1, &end, 16);
        if (*end != '\0') return false;
        color = Color::fromHex(static_cast<uint32_t>(rgb));
        return true;
    }

    /// The first maxBytes bytes of text, less any UTF-8 character that doesn't fit whole.
    std::string cut(std::string text, const size_t maxBytes) {
        if (text.size() <= maxBytes) return text;
        size_t end = maxBytes;
        // Continuation bytes are 10xxxxxx: back up to the start of the character they belong to
        while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
        text.resize(end);
        return text;
    }

    std::string formatColor(const Color color) {
        // toHex() fits in 24 bits, but the compiler can't tell and warns about "#%06lX" overflowing 8
        char text[12];
        snprintf(text, sizeof(text), "#%06lX", static_cast<unsigned long>(color.toHex()));
        return text;
    }
}

AppSettings AppSettings::describe(const JsonArray fields) {
    AppSettings settings(Mode::Describe);
    settings._fields = fields;
    return settings;
}

AppSettings AppSettings::read(const JsonObject values) {
    AppSettings settings(Mode::Read);
    settings._out = values;
    return settings;
}

AppSettings AppSettings::write(const JsonObjectConst values) {
    AppSettings settings(Mode::Write);
    settings._in = values;
    return settings;
}

JsonObject AppSettings::field(const char* key, const char* label, const char* type) {
    JsonObject field = _fields.add<JsonObject>();
    field["key"] = key;
    field["label"] = label;
    field["type"] = type;
    return field;
}

void AppSettings::text(const char* key, const char* label, std::string& value, const uint16_t maxLength,
                       const Normalize normalize) {
    switch (_mode) {
        case Mode::Describe:
            field(key, label, "text")["maxLength"] = maxLength;
            break;
        case Mode::Read:
            _out[key] = value;
            break;
        case Mode::Write:
            if (const char* text = _in[key]) {
                value = cut(normalize ? normalize(text) : std::string(text), maxLength);
            }
            break;
    }
}

void AppSettings::number(const char* key, const char* label, int32_t& value, const int32_t min, const int32_t max) {
    switch (_mode) {
        case Mode::Describe: {
            JsonObject f = field(key, label, "number");
            f["min"] = min;
            f["max"] = max;
            break;
        }
        case Mode::Read:
            _out[key] = value;
            break;
        case Mode::Write:
            if (_in[key].is<int32_t>()) value = std::clamp(_in[key].as<int32_t>(), min, max);
            break;
    }
}

void AppSettings::toggle(const char* key, const char* label, bool& value) {
    switch (_mode) {
        case Mode::Describe:
            field(key, label, "bool");
            break;
        case Mode::Read:
            _out[key] = value;
            break;
        case Mode::Write:
            if (_in[key].is<bool>()) value = _in[key].as<bool>();
            break;
    }
}

void AppSettings::color(const char* key, const char* label, Color& value) {
    switch (_mode) {
        case Mode::Describe:
            field(key, label, "color");
            break;
        case Mode::Read:
            _out[key] = formatColor(value);
            break;
        case Mode::Write:
            parseColor(_in[key].as<const char*>(), value);
            break;
    }
}

void AppSettings::select(const char* key, const char* label, std::string& value,
                         const std::initializer_list<std::pair<const char*, const char*>> options) {
    switch (_mode) {
        case Mode::Describe: {
            JsonArray list = field(key, label, "select")["options"].to<JsonArray>();
            for (const auto& [optionValue, optionLabel] : options) {
                JsonObject option = list.add<JsonObject>();
                option["value"] = optionValue;
                option["label"] = optionLabel;
            }
            break;
        }
        case Mode::Read:
            _out[key] = value;
            break;
        case Mode::Write: {
            const char* text = _in[key];
            if (!text) break;
            for (const auto& option : options) {
                if (strcmp(option.first, text) == 0) value = text;
            }
            break;
        }
    }
}
