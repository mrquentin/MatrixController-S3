#pragma once

#ifndef _SYSTEMMANAGER_h
#define _SYSTEMMANAGER_h
#include <cstdint>

class SystemManager {
public:
    struct Status {
        bool wifiConnected;
        String wifiIp;
        int32_t wifiChannel;
        int32_t wifiQuality;
        int32_t wifiRssi;
        String wifiGatewayIp;
        String wifiMACAddress;
        const char * wifiMDNSName;
        size_t memoryFreeHeap;
        size_t memoryMaxBlock;
        uint32_t memoryMinFreeHeap;
        uint32_t psramSize;
        uint32_t psramFree;
        uint32_t cpuFreqMHz;
        const char * chipModel;
        uint32_t uptimeSeconds;
    };
    SystemManager();
    Status getStatus();
};

#endif
