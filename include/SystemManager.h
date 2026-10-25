#pragma once

#ifndef _SYSTEMMANAGER_h
#define _SYSTEMMANAGER_h
#include <atomic>
#include <cstdint>
#include <string>
#include <Arduino.h>

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
        const char * resetReason;
    };
    SystemManager();
    Status getStatus();

    /// Asks for a restart once delayMs have passed, so the web request that asked for it can still
    /// be answered. note is shown on the panel meanwhile (e.g. the network about to be joined).
    /// Safe to call from the web server task; the restart itself happens in restartIfDue().
    void scheduleRestart(const std::string& note, uint32_t delayMs = 2000);
    [[nodiscard]] bool restartPending() const { return _restartAtMs != 0; }
    /// Only read once restartPending() is true
    [[nodiscard]] const std::string& restartNote() const { return _restartNote; }
    /// Restarts if a scheduled restart is due. Call from loop().
    void restartIfDue(uint32_t now) const;

private:
    std::string _restartNote;
    // millis() at which to restart, 0 when none is scheduled
    std::atomic<uint32_t> _restartAtMs{0};
};

#endif
