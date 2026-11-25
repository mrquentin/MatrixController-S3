#pragma once

#ifndef _NVS_h
#define _NVS_h

#include <string>

/// Strings in NVS (flash key-value storage). Unlike files on LittleFS, NVS survives
/// "pio run -t uploadfs" and firmware updates.
namespace Nvs {
    /// Empty when the key was never written.
    std::string read(const char* space, const char* key);
    void write(const char* space, const char* key, const std::string& value);
    void remove(const char* space, const char* key);
}

#endif
