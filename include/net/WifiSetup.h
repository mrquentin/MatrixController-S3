#pragma once

#ifndef _WIFISETUP_h
#define _WIFISETUP_h

#include <cstdint>
#include <string>
#include <ESPAsyncWebServer.h>
#include "SystemManager.h"
#include "display/DisplayConfig.h"
#include "display/IDisplay.h"
#include "display/QrCode.h"
#include "net/CaptivePortal.h"
#include "net/WifiStore.h"

/// Getting the board onto a WiFi network: joining the saved one, or setup mode, where the board runs
/// its own network and a captive portal to pick one, shown on the panel with a QR code to join it.
///
/// The panel can only be set up once per boot (see Hub75Display::begin), and setup mode needs it set
/// up differently. So when the saved network turns out to be unreachable, the board restarts straight
/// into setup mode.
class WifiSetup {
public:
    WifiSetup(AsyncWebServer& server, SystemManager& system);

    /// True when this boot runs setup mode: nothing is saved, or the last boot couldn't join it.
    /// Read once, before the panel is set up.
    [[nodiscard]] bool setupModeDue(const WifiCredentials& saved);
    /// Setup mode's panel settings: a refresh fast enough for phone cameras to read the QR code,
    /// bright enough for them. The configured ones are back after the next restart.
    static void adjustForSetupMode(DisplayConfig& config);

    /// Joins the network, showing progress on the panel. Returns false on timeout or if the network
    /// rejects the password.
    bool join(IDisplay& display, const WifiCredentials& credentials);
    /// Says so on the panel and restarts into setup mode.
    void restartIntoSetupMode(IDisplay& display, const std::string& ssid);

    /// Starts the setup network and portal. Call server.begin() afterwards.
    void beginSetupMode(IDisplay& display);
    /// Answers the portal's DNS and keeps the panel up to date. Call from loop() in setup mode.
    void loopSetupMode(IDisplay& display, uint32_t now);

private:
    void drawSetupScreen(IDisplay& display);

    CaptivePortal _portal;
    std::string _ssid;
    std::string _password;
    QrCode _qr;
    uint32_t _lastDrawMs = 0;
};

#endif
