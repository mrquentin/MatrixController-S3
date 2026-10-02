#include <WiFi.h>
#include "SystemManager.h"
#include "Config.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

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

    return status;
}
