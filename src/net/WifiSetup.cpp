#include "net/WifiSetup.h"

#include <Arduino.h>
#include <WiFi.h>
#include "Log.h"
#include "display/Text.h"

namespace {
    constexpr uint32_t ConnectTimeoutMs = 15000;
    // Phone cameras read the image line by line and see stripes instead of the QR code unless the
    // panel refreshes fast and bright enough (see DisplayConfig::fastRefresh)
    constexpr uint8_t SetupMinBrightness = 96;
    // The screen only changes when a phone joins or leaves, a few redraws per second are plenty
    constexpr uint32_t SetupRedrawMs = 250;

    // Survives the software restart into setup mode (RTC memory isn't cleared by it) but not a power
    // cycle, which then simply tries the saved network again
    constexpr uint32_t SetupRequestMagic = 0x53455455;  // "SETU"
    RTC_NOINIT_ATTR uint32_t setupRequest;
}

WifiSetup::WifiSetup(AsyncWebServer& server, SystemManager& system) : _portal(server, system) {}

bool WifiSetup::setupModeDue(const WifiCredentials& saved) {
    const bool due = saved.ssid.empty() || setupRequest == SetupRequestMagic;
    setupRequest = 0;
    return due;
}

void WifiSetup::adjustForSetupMode(DisplayConfig& config) {
    config.fastRefresh = true;
    if (config.brightness < SetupMinBrightness) config.brightness = SetupMinBrightness;
}

bool WifiSetup::join(IDisplay& display, const WifiCredentials& credentials) {
    LOGF("Connecting to \"%s\"...\n", credentials.ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());

    const uint32_t start = millis();
    while (millis() - start < ConnectTimeoutMs) {
        const wl_status_t status = WiFi.status();
        if (status == WL_CONNECTED) {
            LOGF("Connected, IP address %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        if (status == WL_CONNECT_FAILED) {
            LOGF("Connection rejected, wrong password?\n");
            break;
        }

        static constexpr const char* Dots[] = {".", "..", "..."};
        showStatus(display, "Connecting to", credentials.ssid, Dots[(millis() - start) / 400 % 3]);
        delay(50);
    }

    LOGF("Could not join \"%s\"\n", credentials.ssid.c_str());
    WiFi.disconnect(true);
    return false;
}

void WifiSetup::restartIntoSetupMode(IDisplay& display, const std::string& ssid) {
    setupRequest = SetupRequestMagic;
    showStatus(display, "No WiFi", ssid, "Starting setup...");
    delay(2000);
    ESP.restart();
}

void WifiSetup::beginSetupMode(IDisplay& display) {
    _ssid = WifiStore::setupSsid();
    _password = WifiStore::setupPassword();

    // Standard WiFi QR format, understood by the iOS and Android cameras
    const std::string qrText = "WIFI:T:WPA;S:" + _ssid + ";P:" + _password + ";;";
    // Version 3 (29 modules) is the largest that fits the panel's height at 2x
    if (!_qr.generate(qrText.c_str(), 3)) LOGF("Setup QR code too large, showing text only\n");

    _portal.begin(_ssid, _password);
    drawSetupScreen(display);
}

void WifiSetup::loopSetupMode(IDisplay& display, const uint32_t now) {
    _portal.loop();
    if (now - _lastDrawMs > SetupRedrawMs) {
        _lastDrawMs = now;
        drawSetupScreen(display);
    }
}

void WifiSetup::drawSetupScreen(IDisplay& display) {
    display.clear();

    // QR code on the left (joins the setup network when scanned), details on the right
    int32_t textX = 2;
    if (_qr.size() > 0) {
        _qr.draw(display, 1, 1, 2, 1);
        textX = _qr.drawnSize(2, 1) + 3;
    }

    const bool joined = _portal.connectedDevices() > 0;
    drawText(display, textX, 2, "WIFI SETUP", Colors::Cyan);
    drawText(display, textX, 14, "Network", Colors::Gray);
    drawText(display, textX, 23, _ssid, Colors::White);
    drawText(display, textX, 35, "Password", Colors::Gray);
    drawText(display, textX, 44, _password, Colors::White);
    drawText(display, textX, 56, joined ? "Joined" : "Scan QR", joined ? Colors::Green : Colors::Gray);
    display.present();
}
