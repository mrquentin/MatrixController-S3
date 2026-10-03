#pragma once

#include <LittleFS.h>
#include <ArduinoJson.h>

#ifndef _CONFIG_h
#define _CONFIG_h

#define MDNSName "matrixcontroller"

class Config {
    public:
        explicit Config(std::string_view filePath);
        void load();
        void save(const uint8_t *data, size_t len, size_t index, size_t total);
        [[nodiscard]] std::string_view getFilePath() const;
    private:
        bool Debug{};
        std::string_view FilePath;
};

#endif
