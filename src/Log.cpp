#include "Log.h"

#include <atomic>
#include <Arduino.h>

namespace {
    std::atomic<bool> enabled{true};
}

namespace Log {
    void setEnabled(const bool on) {
        enabled = on;
    }

    void printf(const char* format, ...) {
        if (!enabled) return;
        va_list args;
        va_start(args, format);
        char line[256];
        vsnprintf(line, sizeof(line), format, args);
        va_end(args);
        Serial.print(line);
    }
}
