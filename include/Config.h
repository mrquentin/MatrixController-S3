#pragma once

#include <functional>
#include <string>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "display/DisplayConfig.h"

#ifndef _CONFIG_h
#define _CONFIG_h

#define MDNSName "matrixcontroller"

class Config {
    public:
        using SavedCallback = std::function<void(const Config&)>;

        explicit Config(std::string_view filePath);
        void load();
        void save(const uint8_t *data, size_t len, size_t index, size_t total);
        [[nodiscard]] std::string_view getFilePath() const;
        [[nodiscard]] bool isDebug() const;
        [[nodiscard]] const DisplayConfig& getDisplayConfig() const;
        /// POSIX TZ string, e.g. "CET-1CEST,M3.5.0,M10.5.0/3". Used for local time on the board.
        [[nodiscard]] const std::string& getTimezone() const;
        /// Called after a config uploaded by save() has been reloaded. Runs on the web server task.
        void onSaved(SavedCallback callback);
    private:
        static DisplayConfig parseDisplayConfig(JsonObjectConst json);

        bool Debug{};
        DisplayConfig Display;
        std::string Timezone = "UTC0";
        std::string_view FilePath;
        SavedCallback OnSaved;
};

#endif
