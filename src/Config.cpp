#include "Config.h"

#include <LittleFS.h>
#include "Log.h"

namespace {
    SystemSettings parse(const JsonObjectConst json) {
        const SystemSettings defaults;
        SystemSettings settings;
        settings.debug = json["debug"] | defaults.debug;
        settings.timezone = json["timezone"] | defaults.timezone;

        const JsonObjectConst display = json["display"];
        DisplayConfig& out = settings.display;
        out.brightness = display["brightness"] | defaults.display.brightness;
        if (out.brightness > MaxBrightness) {
            LOGF("Brightness %u above the %u limit, using %u\n", out.brightness, MaxBrightness, MaxBrightness);
            out.brightness = MaxBrightness;
        }
        out.doubleBuffer = display["doubleBuffer"] | defaults.display.doubleBuffer;
        out.orientation = parseOrientationMode(display["orientation"] | "", defaults.display.orientation);
        return settings;
    }

    void write(const SystemSettings& settings, const JsonObject json) {
        json["debug"] = settings.debug;
        json["timezone"] = settings.timezone;
        const JsonObject display = json["display"].to<JsonObject>();
        display["brightness"] = settings.display.brightness;
        display["doubleBuffer"] = settings.display.doubleBuffer;
        display["orientation"] = orientationModeName(settings.display.orientation);
    }
}

Config::Config(const char* filePath) : _filePath(filePath) {}

void Config::load() {
    JsonDocument doc;
    if (File file = LittleFS.open(_filePath, FILE_READ)) {
        const DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (error) LOGF("Failed to read %s: %s, using the defaults\n", _filePath, error.c_str());
    } else {
        LOGF("No %s, using the defaults\n", _filePath);
    }
    set(doc.as<JsonObjectConst>());
}

bool Config::update(const JsonObjectConst json) {
    set(json);
    ++_revision;

    JsonDocument doc;
    write(get(), doc.to<JsonObject>());
    File file = LittleFS.open(_filePath, FILE_WRITE);
    if (!file) {
        LOGF("Failed to open %s for writing\n", _filePath);
        return false;
    }
    serializeJson(doc, file);
    file.close();
    return true;
}

void Config::set(const JsonObjectConst json) {
    const SystemSettings settings = parse(json);
    std::lock_guard lock(_mutex);
    _settings = settings;
}

SystemSettings Config::get() const {
    std::lock_guard lock(_mutex);
    return _settings;
}

void Config::describe(const JsonObject out) const {
    write(get(), out["config"].to<JsonObject>());

    const JsonObject limits = out["limits"].to<JsonObject>();
    limits["panelWidth"] = PanelWidth;
    limits["panelHeight"] = PanelHeight;
    limits["maxBrightness"] = MaxBrightness;
}
