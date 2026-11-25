#pragma once

#ifndef _DISPLAYCONFIG_h
#define _DISPLAYCONFIG_h

#include <cstdint>
#include "display/Orientation.h"

/// Hard brightness limit. Above this the panel draws more current than the supply
/// can deliver (it cuts out at 192 on full white) and heats up noticeably.
constexpr uint8_t MaxBrightness = 128;

/// The panel's size in pixels. Every screen (apps, setup, updates) is laid out for it.
constexpr uint16_t PanelWidth = 128;
constexpr uint16_t PanelHeight = 64;

/// Panel settings. Buffering is applied once at boot (DMA buffers are allocated in
/// Hub75Display::begin), so changing it needs a reboot.
struct DisplayConfig {
    uint8_t brightness = MaxBrightness;
    bool doubleBuffer = true;
    /// Applies when saved, no reboot needed
    OrientationMode orientation = OrientationMode::Auto;
    /// Trades colour for speed: 2 bits per channel and a doubled clock refresh the panel fast enough
    /// for phone cameras, which otherwise see stripes. Used for the WiFi setup QR code, never saved.
    bool fastRefresh = false;
};

#endif
