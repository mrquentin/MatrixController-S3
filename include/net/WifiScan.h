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

    /// 0-100 from a signal strength in dBm: -100 or weaker is 0, -50 or stronger is 100.
    int qualityFromRssi(int32_t rssi);
}

#endif
