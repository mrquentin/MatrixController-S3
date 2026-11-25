#include "SystemManager.h"

#include <WiFi.h>
#include "Log.h"
#include "net/Hostname.h"
#include "net/WifiScan.h"

namespace {
    // Why the chip last restarted. BROWNOUT means the supply voltage sagged (e.g. the panel drew too much)
    const char* resetReasonName(const esp_reset_reason_t reason) {
        switch (reason) {
            case ESP_RST_POWERON: return "POWER_ON";
            case ESP_RST_EXT: return "EXTERNAL";
            case ESP_RST_SW: return "SOFTWARE";
            case ESP_RST_PANIC: return "PANIC";
            case ESP_RST_INT_WDT: return "INTERRUPT_WATCHDOG";
            case ESP_RST_TASK_WDT: return "TASK_WATCHDOG";
            case ESP_RST_WDT: return "WATCHDOG";
            case ESP_RST_DEEPSLEEP: return "DEEP_SLEEP";
            case ESP_RST_BROWNOUT: return "BROWNOUT";
            case ESP_RST_SDIO: return "SDIO";
            // This SDK version has no code for USB resets, so a reset after flashing lands here
            default: return "UNKNOWN";
        }
    }

    /// Restarts nobody asked for: crashes, hangs, or the supply sagging
    bool isFault(const esp_reset_reason_t reason) {
        switch (reason) {
            case ESP_RST_PANIC:
            case ESP_RST_INT_WDT:
            case ESP_RST_TASK_WDT:
            case ESP_RST_WDT:
            case ESP_RST_BROWNOUT:
                return true;
            default:
                return false;
        }
    }
}

void SystemManager::describe(const JsonObject out) const {
    const bool connected = WiFi.status() == WL_CONNECTED;
    out["wifiConnected"] = connected;
    if (connected) {
        const int32_t rssi = WiFi.RSSI();
        out["wifiRssi"] = rssi;
        out["wifiQuality"] = WifiScan::qualityFromRssi(rssi);
        out["wifiIp"] = WiFi.localIP().toString();
        out["wifiChannel"] = WiFi.channel();
        out["wifiGatewayIp"] = WiFi.gatewayIP().toString();
        out["wifiMACAddress"] = WiFi.macAddress();
        out["wifiMDNSName"] = Hostname;
    }

    out["memoryFreeHeap"] = ESP.getFreeHeap();
    out["memoryMaxBlock"] = ESP.getMaxAllocHeap();
    out["memoryMinFreeHeap"] = ESP.getMinFreeHeap();
    out["psramSize"] = ESP.getPsramSize();
    out["psramFree"] = ESP.getFreePsram();

    out["cpuFreqMHz"] = ESP.getCpuFreqMHz();
    out["chipModel"] = ESP.getChipModel();
    out["uptimeSeconds"] = static_cast<uint32_t>(esp_timer_get_time() / 1000000ULL);
    const esp_reset_reason_t reset = esp_reset_reason();
    out["resetReason"] = resetReasonName(reset);
    out["resetFault"] = isFault(reset);
}

void SystemManager::scheduleRestart(const std::string& note, const uint32_t delayMs) {
    // Only the first request counts: the note must not change while loop() is showing it
    if (restartPending()) return;
    _restartNote = note;
    // 0 means "none scheduled", so never store it as a deadline
    _restartAtMs = (millis() + delayMs) | 1;
    LOGF("Restart scheduled in %u ms\n", static_cast<unsigned>(delayMs));
}

void SystemManager::restartIfDue(const uint32_t now) const {
    const uint32_t restartAt = _restartAtMs;
    if (restartAt != 0 && static_cast<int32_t>(now - restartAt) >= 0) ESP.restart();
}
