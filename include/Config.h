#pragma once

#include <LittleFS.h>
#include <ArduinoJson.h>

#ifndef _CONFIG_h
#define _CONFIG_h

#define MDNSName "matrixcontroller"

class Config {
    public:
        Config(std::string_view filePath);
        void load();
        void save(uint8_t *data, size_t len, size_t index, size_t total);
        void save(JsonVariantConst& doc);
        std::string_view getFilePath();
    private:
        bool Debug;
        std::string_view FilePath;
};

#endif
