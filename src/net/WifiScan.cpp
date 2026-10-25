#include "net/WifiScan.h"

#include <WiFi.h>
#include <set>

namespace {
    int qualityFromRssi(const int32_t rssi) {
        if (rssi <= -100) return 0;
        if (rssi >= -50) return 100;
        return 2 * (rssi + 100);
    }
}

namespace WifiScan {
    void listNetworks(JsonArray list) {
        const int16_t count = WiFi.scanComplete();
        if (count == WIFI_SCAN_FAILED) {
            // No scan yet (or the last one failed)
            WiFi.scanNetworks(true);
            return;
        }
        // WIFI_SCAN_RUNNING: nothing to report yet
        if (count < 0) return;

        // Strongest first, each name once (mesh systems repeat the same SSID)
        std::set<String> seen;
        for (int16_t pass = 0; pass < count; ++pass) {
            int16_t best = -1;
            for (int16_t i = 0; i < count; ++i) {
                const String name = WiFi.SSID(i);
                if (name.isEmpty() || seen.count(name)) continue;
                if (best < 0 || WiFi.RSSI(i) > WiFi.RSSI(best)) best = i;
            }
            if (best < 0) break;
            seen.insert(WiFi.SSID(best));
            JsonObject network = list.add<JsonObject>();
            network["ssid"] = WiFi.SSID(best);
            network["quality"] = qualityFromRssi(WiFi.RSSI(best));
            network["secure"] = WiFi.encryptionType(best) != WIFI_AUTH_OPEN;
        }

        WiFi.scanDelete();
        WiFi.scanNetworks(true);
    }
}
