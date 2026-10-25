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
#include "display/Hub75Display.h"
#include "display/ScrollingText.h"
#include "display/TestPattern.h"
#include "display/Text.h"

#define DEBUG true
#define DEBUG_PRINT if(DEBUG) Serial
#define DEBUG_PRINTF if(DEBUG) printf

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

Config config = Config("/config.json");
SystemManager systemManager = SystemManager();
HttpHandlers httpHandlers = HttpHandlers(config, server, systemManager);
WebSocketHandler wsHandler = WebSocketHandler(server, ws, systemManager);
Hub75Display display;
ScrollingText banner;

// The boot test pattern stays up this long after setup() before the text demo starts
constexpr uint32_t TestPatternDurationMs = 3000;
uint32_t setupDoneMs = 0;
constexpr uint32_t FrameIntervalMs = 1000 / 60;

// Set from the web server task when a config is saved, applied in loop(): only the loop task may draw
std::atomic<int16_t> pendingBrightness{-1};

namespace {
    void drawCentered(const int32_t y, const char* text, const Color color) {
        const int32_t x = (static_cast<int32_t>(display.width()) - textWidth(text)) / 2;
        drawText(display, x, y, text, color);
    }

    // Placeholder screen showing off text rendering until there's a real content system
    void drawDemo(const uint32_t nowMs) {
        char ip[16] = "No WiFi";
        if (WiFi.status() == WL_CONNECTED) {
            const IPAddress addr = WiFi.localIP();
            snprintf(ip, sizeof(ip), "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);
        }

        display.clear();
        drawCentered(2, "Matrix Controller", Colors::Cyan);
        drawCentered(12, ip, Colors::White);
        banner.draw(display, nowMs);
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

    banner.setText("Hello from the Matrix Controller!");
    banner.setColor(Colors::Orange);
    banner.setFont(Fonts::Font5x7, 2);
    banner.setY(static_cast<int32_t>(display.height()) - textHeight(Fonts::Font5x7, 2) - 6);
    banner.setSpeed(40);

    config.onSaved([](const Config& saved) {
        pendingBrightness = saved.getDisplayConfig().brightness;
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

    static uint32_t lastFrameMs = 0;
    const uint32_t now = millis();
    if (now - setupDoneMs < TestPatternDurationMs || now - lastFrameMs < FrameIntervalMs) {
        delay(1);
        return;
    }
    lastFrameMs = now;
    drawDemo(now);
}