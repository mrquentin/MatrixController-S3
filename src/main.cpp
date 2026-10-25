#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <wifi_creds.h>
#include "Config.h"
#include "HttpHandlers.h"
#include "WebSocketHandler.h"
#include "display/Hub75Display.h"
#include "display/TestPattern.h"

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

    MDNS.begin(MDNSName);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,PUT,POST");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

    wsHandler.setup();
    httpHandlers.setup();

    server.begin();
}

void loop() {
// write your code here
}