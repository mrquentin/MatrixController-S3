#pragma once

#ifndef _DISPLAYCONFIG_h
#define _DISPLAYCONFIG_h

#include <cstdint>

/// Hard brightness limit. Above this the panel draws more current than the supply
/// can deliver (it cuts out at 192 on full white) and heats up noticeably.
constexpr uint8_t MaxBrightness = 128;

/// Physical matrix settings. Width/height and buffering are applied once at boot
/// (DMA buffers are allocated in Hub75Display::begin), so changing them needs a reboot.
struct DisplayConfig {
    uint16_t width = 128;
    uint16_t height = 64;
    uint8_t brightness = 128;
    bool doubleBuffer = true;
};

#endif
