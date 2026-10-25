#include <ESPAsyncWebServer.h>
#include "HttpHandlers.h"
#include <ArduinoJson.h>
#include <AsyncJson.h>

#include <ui/index_gz.h>
#include <ui/favicon_gz.h>
#include <ui/icons_gz.h>
#include <ui/sw_gz.h>
#include <ui/manifest_webmanifest.h>
#include <ui/apple_touch_icon_png.h>
#include <ui/icon_192_png.h>
#include <ui/icon_512_png.h>
#include <ui/icon_maskable_512_png.h>

#include "Config.h"
#include "SystemManager.h"
#include "net/WifiScan.h"
#include "net/WifiStore.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

HttpHandlers::HttpHandlers(Config& config, AsyncWebServer &server, SystemManager &systemManager, AppManager &appManager) :
    _config(config), _server(server), _systemManager(systemManager), _appManager(appManager) {}

void HttpHandlers::setup() {
    addHandlerRoot();
    addHandlerApiStatus();
    addHandlerApiConfig();
    addHandlerApiApps();
    addHandlerApiWifi();
    addHandlerApiRestart();

    _server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        }
        else {
            Serial.println("Not found");
            request->send(404);
        }
    });
}

void HttpHandlers::addHandlerRoot() {
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        Serial.println("Root URL requested");
        AsyncWebServerResponse * response = request->beginResponse(200, "text/html", index_gz, index_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Files index.html links to: the favicon, plus the manifest, icons and service worker that let
    // phones install the UI as an app. The build gzips text files only; PNGs are compressed already
    const auto serve = [this](const char* uri, const char* contentType, const uint8_t* data, const size_t length, const bool gzipped) {
        _server.on(uri, HTTP_GET, [contentType, data, length, gzipped](AsyncWebServerRequest * request) {
            AsyncWebServerResponse * response = request->beginResponse(200, contentType, data, length);
            if (gzipped) response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        });
    };
    serve("/favicon.svg", "image/svg+xml", favicon_gz, favicon_gz_len, true);
    serve("/sw.js", "text/javascript", sw_gz, sw_gz_len, true);
    serve("/manifest.webmanifest", "application/manifest+json", manifest_webmanifest, manifest_webmanifest_len, false);
    serve("/apple_touch_icon.png", "image/png", apple_touch_icon_png, apple_touch_icon_png_len, false);
    serve("/icon_192.png", "image/png", icon_192_png, icon_192_png_len, false);
    serve("/icon_512.png", "image/png", icon_512_png, icon_512_png_len, false);
    serve("/icon_maskable_512.png", "image/png", icon_maskable_512_png, icon_maskable_512_png_len, false);
}

void HttpHandlers::addHandlerApiStatus() {
    _server.on("/api/system/status", HTTP_GET, [&](AsyncWebServerRequest * request) {
        SystemManager::Status status = _systemManager.getStatus();

        JsonDocument doc;
        doc["cpuFreqMHz"] = status.cpuFreqMHz;
        doc["chipModel"] = status.chipModel;
        doc["memoryFreeHeap"] = status.memoryFreeHeap;
        doc["memoryMaxBlock"] = status.memoryMaxBlock;
        doc["memoryMinFreeHeap"] = status.memoryMinFreeHeap;
        doc["psramFree"] = status.psramFree;
        doc["psramSize"] = status.psramSize;
        doc["wifiChannel"] = status.wifiChannel;
        doc["wifiConnected"] = status.wifiConnected;
        doc["wifiGatewayIp"] = status.wifiGatewayIp;
        doc["wifiIp"] = status.wifiIp;
        doc["wifiMACAddress"] = status.wifiMACAddress;
        doc["wifiMDNSName"] = status.wifiMDNSName;
        doc["wifiQuality"] = status.wifiQuality;
        doc["wifiRssi"] = status.wifiRssi;
        doc["uptimeSeconds"] = status.uptimeSeconds;
        doc["resetReason"] = status.resetReason;

        String response;
        serializeJson(doc, response);

        request->send(200, "application/json", response);
    });
}

void HttpHandlers::addHandlerApiConfig(){
    // Endpoint to receive and save JSON settings
    _server.on("/api/system/config", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "application/json", "{\"result\":\"success\"}");
    }, NULL, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        _config.save(data, len, index, total);
    });

    _server.on("/api/system/config", HTTP_GET, [&](AsyncWebServerRequest *request) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        if (LittleFS.exists("/config.json")) {
            // Send the file directly from the filesystem
            request->send(LittleFS, _config.getFilePath().data(), "application/json");
        } else {
            // Return an empty object or error if file doesn't exist
            request->send(404, "application/json", "{\"result\":\"error\",\"details\":\"No settings found\"}");
        }
    });

}

void HttpHandlers::addHandlerApiApps() {
    _server.on("/api/apps", HTTP_GET, [&](AsyncWebServerRequest *request) {
        JsonDocument doc;
        _appManager.describe(doc.to<JsonObject>());

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Body: {"id": "clock"}
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/apps/active", [&](AsyncWebServerRequest *request, JsonVariant &json) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        const char* id = json["id"];
        if (!id || !_appManager.activate(id)) {
            request->send(404, "application/json", "{\"result\":\"error\",\"details\":\"unknown app\"}");
            return;
        }
        request->send(200, "application/json", "{\"result\":\"success\"}");
    }));

    // Body: {"id": "clock", "config": {...}}. Responds with the settings the app actually applied
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/apps/config", [&](AsyncWebServerRequest *request, JsonVariant &json) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        const char* id = json["id"];
        const JsonObjectConst config = json["config"];
        if (!id || config.isNull()) {
            request->send(400, "application/json", "{\"result\":\"error\",\"details\":\"id and config are required\"}");
            return;
        }

        JsonDocument doc;
        if (!_appManager.setConfig(id, config, doc["config"].to<JsonObject>())) {
            request->send(404, "application/json", "{\"result\":\"error\",\"details\":\"unknown app\"}");
            return;
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    }));
}

void HttpHandlers::addHandlerApiWifi() {
    // A plain "/api/system/wifi" route also matches the paths below it, so these come first

    _server.on("/api/system/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        WifiScan::listNetworks(doc.to<JsonArray>());

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Forgets the network; the board restarts into setup mode
    _server.on("/api/system/wifi/forget", HTTP_POST, [&](AsyncWebServerRequest *request) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        WifiStore::clear();
        request->send(200, "application/json", "{\"result\":\"success\"}");
        _systemManager.scheduleRestart("WiFi forgotten");
    });

    // The saved network's name, never its password
    _server.on("/api/system/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["ssid"] = WifiStore::load().ssid;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Body: {"ssid": "Home", "password": "..."}. The board restarts and joins it, falling back to
    // setup mode if it can't
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/system/wifi", [&](AsyncWebServerRequest *request, JsonVariant &json) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        WifiCredentials credentials;
        credentials.ssid = json["ssid"] | "";
        credentials.password = json["password"] | "";
        if (!WifiStore::isValid(credentials)) {
            request->send(400, "application/json", "{\"result\":\"error\",\"details\":\"invalid network name or password\"}");
            return;
        }

        WifiStore::save(credentials);
        DEBUG_PRINTF("WiFi credentials saved for \"%s\"\n", credentials.ssid.c_str());
        request->send(200, "application/json", "{\"result\":\"success\"}");
        _systemManager.scheduleRestart(credentials.ssid.substr(0, 21));
    }));
}

void HttpHandlers::addHandlerApiRestart() {
    _server.on("/api/system/restart", HTTP_POST, [&](AsyncWebServerRequest *request) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        request->send(200, "application/json", "{\"result\":\"success\"}");
        _systemManager.scheduleRestart("");
    });
}
