#include "apps/ConfigSchema.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

ConfigSchema::ConfigSchema(const JsonArray fields) : _fields(fields) {}

JsonObject ConfigSchema::add(const char* key, const char* label, const char* type) {
    JsonObject field = _fields.add<JsonObject>();
    field["key"] = key;
    field["label"] = label;
    field["type"] = type;
    return field;
}

void ConfigSchema::text(const char* key, const char* label, const uint16_t maxLength) {
    add(key, label, "text")["maxLength"] = maxLength;
}

void ConfigSchema::number(const char* key, const char* label, const int32_t min, const int32_t max) {
    JsonObject field = add(key, label, "number");
    field["min"] = min;
    field["max"] = max;
}

void ConfigSchema::toggle(const char* key, const char* label) {
    add(key, label, "bool");
}

void ConfigSchema::color(const char* key, const char* label) {
    add(key, label, "color");
}

void ConfigSchema::select(const char* key, const char* label,
                          const std::initializer_list<std::pair<const char*, const char*>> options) {
    JsonArray list = add(key, label, "select")["options"].to<JsonArray>();
    for (const auto& [value, optionLabel] : options) {
        JsonObject option = list.add<JsonObject>();
        option["value"] = value;
        option["label"] = optionLabel;
    }
}

Color readColor(const JsonVariantConst value, const Color fallback) {
    const char* text = value.as<const char*>();
    if (!text || text[0] != '#' || strlen(text) != 7) return fallback;

    char* end = nullptr;
    const unsigned long rgb = strtoul(text + 1, &end, 16);
    if (*end != '\0') return fallback;
    return Color::fromHex(static_cast<uint32_t>(rgb));
}

void writeColor(JsonObject config, const char* key, const Color color) {
    // toHex() fits in 24 bits, but the compiler can't tell and warns about "#%06lX" overflowing 8
    char text[12];
    snprintf(text, sizeof(text), "#%06lX", static_cast<unsigned long>(color.toHex()));
    config[key] = text;
}

int32_t readNumber(const JsonVariantConst value, const int32_t fallback, const int32_t min, const int32_t max) {
    if (!value.is<int32_t>()) return fallback;
    return std::clamp(value.as<int32_t>(), min, max);
}
