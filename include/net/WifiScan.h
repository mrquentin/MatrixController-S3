#pragma once

#ifndef _WIFISCAN_h
#define _WIFISCAN_h

#include <ArduinoJson.h>

namespace WifiScan {
    /// Fills list with the networks found by the last scan, strongest first and each name once:
    /// [{"ssid": "Home", "quality": 80, "secure": true}, ...]. Then starts a fresh scan in the
    /// background, so asking again picks up changes. The list stays empty while the first scan is
    /// still running; callers ask again a moment later.
    void listNetworks(JsonArray list);
}

#endif
