#pragma once

#ifndef _CAPTIVEPORTAL_h
#define _CAPTIVEPORTAL_h

#include <atomic>
#include <string>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

/// Setup mode: the board runs its own WiFi network and answers every DNS lookup with its own
/// address, so phones that join it open a "sign in" page where the WiFi to use is chosen.
/// Saving stores the credentials (see WifiStore); the caller then restarts the board.
class CaptivePortal {
public:
    explicit CaptivePortal(AsyncWebServer& server);

    /// Starts the setup network and registers the portal pages. Call server.begin() afterwards.
    void begin(const std::string& ssid, const std::string& password);

    /// Answers DNS queries. Call from loop().
    void loop();

    /// True once new credentials have been saved from the portal.
    [[nodiscard]] bool saved() const { return _saved; }
    /// Number of devices currently joined to the setup network.
    [[nodiscard]] int connectedDevices() const;

private:
    void addRoutes();

    AsyncWebServer& _server;
    DNSServer _dns;
    std::atomic<bool> _saved{false};
};

#endif
