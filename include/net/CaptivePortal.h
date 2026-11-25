#pragma once

#ifndef _CAPTIVEPORTAL_h
#define _CAPTIVEPORTAL_h

#include <string>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "SystemManager.h"

/// Setup mode: the board runs its own WiFi network and answers every DNS lookup with its own
/// address, so phones that join it open a "sign in" page where the WiFi to use is chosen.
/// Saving stores the credentials (see WifiStore) and restarts the board to join the network.
class CaptivePortal {
public:
    CaptivePortal(AsyncWebServer& server, SystemManager& system);

    /// Starts the setup network and registers the portal pages. Call server.begin() afterwards.
    void begin(const std::string& ssid, const std::string& password);

    /// Answers DNS queries. Call from loop().
    void loop();

    /// Number of devices currently joined to the setup network.
    [[nodiscard]] int connectedDevices() const;

private:
    void addRoutes();

    AsyncWebServer& _server;
    SystemManager& _system;
    DNSServer _dns;
};

#endif
