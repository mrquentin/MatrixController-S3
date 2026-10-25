#include <WiFi.h>
#include "SystemManager.h"
#include "Config.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

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
            default: return "UNKNOWN (USB/flash reset?)";
        }
    }
}

SystemManager::SystemManager() {}

SystemManager::Status SystemManager::getStatus() {

    Status status;
    if (WiFi.status() == WL_CONNECTED) {
        status.wifiConnected = true;
        status.wifiRssi = WiFi.RSSI();
        status.wifiIp = WiFi.localIP().toString();
        status.wifiChannel = WiFi.channel();
        status.wifiGatewayIp = WiFi.gatewayIP().toString();
        status.wifiMACAddress = WiFi.macAddress();
        status.wifiMDNSName = MDNSName;

        status.wifiQuality = 0;
        if (status.wifiRssi <= -100) status.wifiQuality = 0;
        else if (status.wifiRssi >= -50) status.wifiQuality = 100;
        else status.wifiQuality = 2 * (status.wifiRssi + 100 );
    }

    status.memoryFreeHeap = ESP.getFreeHeap();
    status.memoryMaxBlock = ESP.getMaxAllocHeap();
    status.memoryMinFreeHeap = ESP.getMinFreeHeap();
    status.psramSize = ESP.getPsramSize();
    status.psramFree = ESP.getFreePsram();

    status.cpuFreqMHz = ESP.getCpuFreqMHz();
    status.chipModel = ESP.getChipModel();
    status.uptimeSeconds = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    status.resetReason = resetReasonName(esp_reset_reason());

    return status;
}
