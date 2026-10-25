#include <Arduino.h>
#include <atomic>
#include <cstdio>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "Config.h"
#include "HttpHandlers.h"
#include "WebSocketHandler.h"
#include "apps/AppManager.h"
#include "apps/ClockApp.h"
#include "apps/F1App.h"
#include "apps/TextApp.h"
#include "apps/WeatherApp.h"
#include "display/Hub75Display.h"
#include "display/QrCode.h"
#include "display/TestPattern.h"
#include "display/Text.h"
#include "net/CaptivePortal.h"
#include "net/WifiStore.h"

#define DEBUG true
#define DEBUG_PRINT if(DEBUG) Serial
#define DEBUG_PRINTF if(DEBUG) printf

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Config config = Config("/config.json");
SystemManager systemManager = SystemManager();
AppManager appManager = AppManager("/apps.json");
HttpHandlers httpHandlers = HttpHandlers(config, server, systemManager, appManager);
WebSocketHandler wsHandler = WebSocketHandler(server, ws, systemManager);
Hub75Display display;
CaptivePortal portal(server);

TextApp textApp;
ClockApp clockApp;
WeatherApp weatherApp;
F1App f1App;

// Boot: test pattern, connecting screen, then a screen with the IP address before the apps start
constexpr uint32_t TestPatternDurationMs = 3000;
constexpr uint32_t ConnectTimeoutMs = 15000;
constexpr uint32_t AddressScreenDurationMs = 4000;
uint32_t setupDoneMs = 0;
constexpr uint32_t FrameIntervalMs = 1000 / 60;

// Setup mode: no WiFi to join, the board runs its own network with the captive portal instead
bool setupMode = false;
std::string setupSsid;
std::string setupPassword;
QrCode setupQr;
// Phone cameras read the image line by line and see stripes instead of the QR code unless the
// panel refreshes fast and bright enough (see DisplayConfig::fastRefresh)
constexpr uint8_t SetupMinBrightness = 96;
// The panel can only be set up once per boot (see Hub75Display::begin), so when the saved network
// turns out to be unreachable the board restarts straight into setup mode, with the panel in fast
// refresh from the start. This note survives the software restart (RTC memory isn't cleared by it)
// but not a power cycle, which then simply tries the saved network again.
constexpr uint32_t SetupRequestMagic = 0x53455455;  // "SETU"
RTC_NOINIT_ATTR uint32_t setupRequest;
// Restart this long after the credentials were saved, so the portal's answer reaches the phone
constexpr uint32_t RestartDelayMs = 2500;
uint32_t savedAtMs = 0;

// Set from the web server task when a config is saved, applied in loop(): only the loop task may draw
std::atomic<int16_t> pendingBrightness{-1};
std::atomic<bool> timezoneChanged{false};

namespace {
    constexpr Color DetailColor{140, 140, 150};

    void drawCentered(const int32_t y, const char* text, const Color color) {
        const int32_t x = (static_cast<int32_t>(display.width()) - textWidth(text)) / 2;
        drawText(display, x, y, text, color);
    }

    void drawLines(const char* first, const char* second, const char* third = nullptr) {
        const char* lines[] = {first, second, third};
        int32_t count = 0;
        for (const char* line : lines) count += line != nullptr;

        const int32_t lineHeight = textHeight() + 4;
        int32_t y = (static_cast<int32_t>(display.height()) - count * lineHeight + 4) / 2;
        display.clear();
        for (int32_t i = 0; i < 3; ++i) {
            if (!lines[i]) continue;
            drawCentered(y, lines[i], i == 0 ? Colors::Cyan : (i == 1 ? Colors::White : DetailColor));
            y += lineHeight;
        }
        display.present();
    }

    // Shown at boot so the address can be read off the panel without a serial cable
    void drawAddressScreen() {
        char ip[16];
        const IPAddress addr = WiFi.localIP();
        snprintf(ip, sizeof(ip), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        drawLines("Matrix Controller", ip);
    }

    void drawSetupScreen() {
        display.clear();

        // QR code on the left (joins the setup network when scanned), details on the right
        int32_t textX = 2;
        if (setupQr.size() > 0) {
            setupQr.draw(display, 1, 1, 2, 1);
            textX = setupQr.drawnSize(2, 1) + 3;
        }

        const bool joined = portal.connectedDevices() > 0;
        drawText(display, textX, 2, "WIFI SETUP", Colors::Cyan);
        drawText(display, textX, 14, "Network", DetailColor);
        drawText(display, textX, 23, setupSsid.c_str(), Colors::White);
        drawText(display, textX, 35, "Password", DetailColor);
        drawText(display, textX, 44, setupPassword.c_str(), Colors::White);
        drawText(display, textX, 56, joined ? "Joined" : "Scan QR", joined ? Colors::Green : DetailColor);
        display.present();
    }

    /// Joins the saved network, showing progress on the panel. Returns false on timeout or if the
    /// network rejects the password.
    bool connect(const WifiCredentials& credentials) {
        DEBUG_PRINTF("Connecting to \"%s\"...\n", credentials.ssid.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(credentials.ssid.c_str(), credentials.password.c_str());

        // Long names are cut to one line
        const std::string name = credentials.ssid.substr(0, 21);
        const uint32_t start = millis();
        while (millis() - start < ConnectTimeoutMs) {
            const wl_status_t status = WiFi.status();
            if (status == WL_CONNECTED) {
                DEBUG_PRINTF("Connected, IP address %s\n", WiFi.localIP().toString().c_str());
                return true;
            }
            if (status == WL_CONNECT_FAILED) {
                DEBUG_PRINT.println("Connection rejected, wrong password?");
                break;
            }

            static constexpr const char* Dots[] = {"", ".", "..", "..."};
            drawLines("Connecting to", name.c_str(), Dots[(millis() - start) / 400 % 4]);
            delay(50);
        }

        DEBUG_PRINTF("Could not join \"%s\"\n", credentials.ssid.c_str());
        WiFi.disconnect(true);
        return false;
    }

    void startSetupMode() {
        setupSsid = WifiStore::setupSsid();
        setupPassword = WifiStore::setupPassword();

        // Standard WiFi QR format, understood by the iOS and Android cameras
        const std::string qrText = "WIFI:T:WPA;S:" + setupSsid + ";P:" + setupPassword + ";;";
        // Version 3 (29 modules) is the largest that fits the panel's height at 2x
        if (!setupQr.generate(qrText.c_str(), 3)) DEBUG_PRINT.println("Setup QR code too large, showing text only");

        portal.begin(setupSsid, setupPassword);
        server.begin();
        drawSetupScreen();
    }

    void startNormalMode() {
        // Local time for the apps; the clock syncs in the background
        configTzTime(config.getTimezone().c_str(), "pool.ntp.org", "time.google.com");

        appManager.add(clockApp);
        appManager.add(weatherApp);
        appManager.add(f1App);
        appManager.add(textApp);
        appManager.begin();

        config.onSaved([](const Config& saved) {
            pendingBrightness = saved.getDisplayConfig().brightness;
            timezoneChanged = true;
        });

        MDNS.begin(MDNSName);

        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,PUT,POST");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

        wsHandler.setup();
        httpHandlers.setup();

        server.begin();
    }

    void loopSetupMode(const uint32_t now) {
        portal.loop();

        if (portal.saved()) {
            if (savedAtMs == 0) {
                savedAtMs = now;
                drawLines("Saved!", WifiStore::load().ssid.substr(0, 21).c_str(), "Restarting...");
            }
            if (now - savedAtMs > RestartDelayMs) ESP.restart();
            return;
        }

        // The screen only changes when a phone joins or leaves, a few redraws per second are plenty
        static uint32_t lastDrawMs = 0;
        if (now - lastDrawMs > 250) {
            lastDrawMs = now;
            drawSetupScreen();
        }
        delay(5);
    }
}

void setup() {
    DEBUG_PRINT.begin(115200);
    DEBUG_PRINT.println("Booted");

    // The Matrix Portal's partition table (partitions-8MB-tinyuf2.csv) has no "spiffs" partition,
    // which is LittleFS's default label. Its data partition is "ffat", where uploadfs writes the image.
    if (!LittleFS.begin(false, "/littlefs", 10, "ffat")) {
        DEBUG_PRINT.println("Failed to mount LittleFS");
    }

    config.load();

    // Setup mode when nothing is saved, or when the last boot couldn't join the saved network
    const WifiCredentials credentials = WifiStore::load();
    setupMode = credentials.ssid.empty() || setupRequest == SetupRequestMagic;
    setupRequest = 0;

    // The panel comes first so every WiFi step after this can be shown on it. In setup mode it trades
    // colour depth for a refresh fast enough for phone cameras to read the QR code, and is kept
    // bright enough for them; the configured settings are back after the next restart
    DisplayConfig displayConfig = config.getDisplayConfig();
    if (setupMode) {
        displayConfig.fastRefresh = true;
        if (displayConfig.brightness < SetupMinBrightness) displayConfig.brightness = SetupMinBrightness;
    }
    if (display.begin(displayConfig)) {
        drawTestPattern(display);
        display.present();
        delay(TestPatternDurationMs);
    }

    if (setupMode) {
        startSetupMode();
    } else if (connect(credentials)) {
        startNormalMode();
    } else {
        setupRequest = SetupRequestMagic;
        drawLines("No WiFi", credentials.ssid.substr(0, 21).c_str(), "Starting setup...");
        delay(2000);
        ESP.restart();
    }
    setupDoneMs = millis();
}

void loop() {
    const uint32_t now = millis();
    if (setupMode) {
        loopSetupMode(now);
        return;
    }

    // Asked for from the web UI (restart, WiFi changed or forgotten): say so on the panel until it happens
    if (systemManager.restartPending()) {
        static bool shown = false;
        if (!shown) {
            shown = true;
            const std::string& note = systemManager.restartNote();
            drawLines("Restarting...", note.empty() ? nullptr : note.c_str());
        }
        systemManager.restartIfDue(now);
        delay(5);
        return;
    }

    const int16_t brightness = pendingBrightness.exchange(-1);
    if (brightness >= 0) display.setBrightness(static_cast<uint8_t>(brightness));

    if (timezoneChanged.exchange(false)) {
        setenv("TZ", config.getTimezone().c_str(), 1);
        tzset();
    }

    static uint32_t lastFrameMs = 0;
    if (now - lastFrameMs < FrameIntervalMs) {
        delay(1);
        return;
    }
    lastFrameMs = now;

    if (now - setupDoneMs < AddressScreenDurationMs) {
        drawAddressScreen();
    } else {
        appManager.render(display, now);
    }
}
