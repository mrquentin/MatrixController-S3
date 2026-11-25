#pragma once

#include <cstdarg>

/// Serial debug output, switched by the "debug" setting in config.json. On until the config is read,
/// so the boot shows up either way.
namespace Log {
    void setEnabled(bool enabled);
    void printf(const char* format, ...) __attribute__((format(printf, 1, 2)));
}

#define LOGF(...) Log::printf(__VA_ARGS__)
