#include "Config.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

Config::Config(const std::string_view filePath) : FilePath(filePath) {}

void Config::load() {
    if (!LittleFS.exists(FilePath.data())) {
        DEBUG_PRINTF("Config file %s not found in filesystem!\n", FilePath.data());
        return;
    }

    File file = LittleFS.open(FilePath.data(), "r");

    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        DEBUG_PRINT.print("Failed to load JSON. Error: ");
        DEBUG_PRINT.println(error.f_str());
        return;
    }

    Debug = doc["debug"] | false;
    Display = parseDisplayConfig(doc["display"]);
    Timezone = doc["timezone"] | "UTC0";
}

DisplayConfig Config::parseDisplayConfig(const JsonObjectConst json) {
    const DisplayConfig defaults;
    DisplayConfig display;

    display.width = json["width"] | defaults.width;
    if (display.width < 16 || display.width > 256) {
        DEBUG_PRINTF("Invalid display width %u, using %u\n", display.width, defaults.width);
        display.width = defaults.width;
    }

    display.height = json["height"] | defaults.height;
    if (display.height != 16 && display.height != 32 && display.height != 64) {
        DEBUG_PRINTF("Invalid display height %u (expected 16, 32 or 64), using %u\n", display.height, defaults.height);
        display.height = defaults.height;
    }

    display.brightness = json["brightness"] | defaults.brightness;
    if (display.brightness > MaxBrightness) {
        DEBUG_PRINTF("Brightness %u above the %u limit, using %u\n", display.brightness, MaxBrightness, MaxBrightness);
        display.brightness = MaxBrightness;
    }
    display.doubleBuffer = json["doubleBuffer"] | defaults.doubleBuffer;
    display.orientation = parseOrientationMode(json["orientation"] | "", defaults.orientation);

    return display;
}

std::string_view Config::getFilePath() const {
    return FilePath;
}

bool Config::isDebug() const {
    return Debug;
}

const DisplayConfig& Config::getDisplayConfig() const {
    return Display;
}

const std::string& Config::getTimezone() const {
    return Timezone;
}

void Config::onSaved(SavedCallback callback) {
    OnSaved = std::move(callback);
}

void Config::save(const uint8_t *data, const size_t len, const size_t index, const size_t total) {
    if (File file = LittleFS.open(FilePath.data(), (index == 0) ? FILE_WRITE : FILE_APPEND)) {
        file.write(data, len);
        file.close();
    }

    if (index + len == total) {
        DEBUG_PRINT.println("JSON Settings saved to LittleFS.");

        File checkFile = LittleFS.open(FilePath.data(), FILE_READ);
        JsonDocument doc;
        deserializeJson(doc, checkFile);
        checkFile.close();

        DEBUG_PRINT.println("Reloading config... ");
        load();
        if (OnSaved) OnSaved(*this);
    }
}
