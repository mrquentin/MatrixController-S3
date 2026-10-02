#include "Config.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

Config::Config(std::string_view filePath) : FilePath(filePath) {}

void Config::load() {
    if (!LittleFS.exists(FilePath.data())) {
        DEBUG_PRINTF("Config file %s not found in filesystem!\n", FilePath.data());
        return;
    }

    File file = LittleFS.open(FilePath.data(), "r");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        DEBUG_PRINT.print("Failed to load JSON. Error: ");
        DEBUG_PRINT.println(error.f_str());
        return;
    }

    Debug = doc["debug"] | false;
}

std::string_view Config::getFilePath() {
    return FilePath;
}

void Config::save(uint8_t *data, size_t len, size_t index, size_t total) {
    File file = LittleFS.open(FilePath.data(), (index == 0) ? FILE_WRITE : FILE_APPEND);

    if (file) {
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
    }
}
