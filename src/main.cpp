#include <Arduino.h>
#include <atomic>
#include <cstdio>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <wifi_creds.h>
#include "Config.h"
#include "HttpHandlers.h"
#include "WebSocketHandler.h"
#include "apps/AppManager.h"
#include "apps/ClockApp.h"
#include "apps/TextApp.h"
#include "apps/WeatherApp.h"
#include "display/Hub75Display.h"
#include "display/TestPattern.h"
#include "display/Text.h"

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

TextApp textApp;
ClockApp clockApp;
WeatherApp weatherApp;

// After setup() the test pattern stays up, then a screen with the IP address, then the apps start
constexpr uint32_t TestPatternDurationMs = 3000;
constexpr uint32_t AddressScreenDurationMs = 4000;
uint32_t setupDoneMs = 0;
constexpr uint32_t FrameIntervalMs = 1000 / 60;

// Set from the web server task when a config is saved, applied in loop(): only the loop task may draw
std::atomic<int16_t> pendingBrightness{-1};
std::atomic<bool> timezoneChanged{false};

namespace {
    void drawCentered(const int32_t y, const char* text, const Color color) {
        const int32_t x = (static_cast<int32_t>(display.width()) - textWidth(text)) / 2;
        drawText(display, x, y, text, color);
    }

    // Shown at boot so the address can be read off the panel without a serial cable
    void drawAddressScreen() {
        char ip[16] = "No WiFi";
        if (WiFi.status() == WL_CONNECTED) {
            const IPAddress addr = WiFi.localIP();
            snprintf(ip, sizeof(ip), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        }

        display.clear();
        const int32_t top = (static_cast<int32_t>(display.height()) - 2 * textHeight() - 4) / 2;
        drawCentered(top, "Matrix Controller", Colors::Cyan);
        drawCentered(top + textHeight() + 4, ip, Colors::White);
        display.present();
    }
}

void setup() {
    DEBUG_PRINT.begin(115200);
    DEBUG_PRINT.println("Booted");

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASSWORD);

    DEBUG_PRINT.println("Connecting to WiFi...");
    unsigned long startAttemptTime = millis();

    while (WiFi.status()  != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        delay(10);
        DEBUG_PRINT.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINT.println("\nConnected!");
    } else {
        DEBUG_PRINT.println("\nFailed to connect.");
    }

    DEBUG_PRINT.print("IP Address: ");
    DEBUG_PRINT.println(WiFi.localIP());

    // The Matrix Portal's partition table (partitions-8MB-tinyuf2.csv) has no "spiffs" partition,
    // which is LittleFS's default label. Its data partition is "ffat", where uploadfs writes the image.
    if (!LittleFS.begin(false, "/littlefs", 10, "ffat")) {
        DEBUG_PRINT.println("Failed to mount LittleFS");
    }
    
    config.load();

    if (display.begin(config.getDisplayConfig())) {
        drawTestPattern(display);
        display.present();
    }

    // Local time for the apps; the clock syncs in the background once WiFi is up
    configTzTime(config.getTimezone().c_str(), "pool.ntp.org", "time.google.com");

    appManager.add(clockApp);
    appManager.add(weatherApp);
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
    setupDoneMs = millis();
}

void loop() {
    const int16_t brightness = pendingBrightness.exchange(-1);
    if (brightness >= 0) display.setBrightness(static_cast<uint8_t>(brightness));

    if (timezoneChanged.exchange(false)) {
        setenv("TZ", config.getTimezone().c_str(), 1);
        tzset();
    }

    static uint32_t lastFrameMs = 0;
    const uint32_t now = millis();
    const uint32_t sinceSetup = now - setupDoneMs;
    if (sinceSetup < TestPatternDurationMs || now - lastFrameMs < FrameIntervalMs) {
        delay(1);
        return;
    }
    lastFrameMs = now;

    if (sinceSetup < TestPatternDurationMs + AddressScreenDurationMs) {
        drawAddressScreen();
    } else {
        appManager.render(display, now);
    }
}