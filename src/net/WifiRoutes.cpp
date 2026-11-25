#include "net/WifiRoutes.h"

#include <AsyncJson.h>
#include "Log.h"
#include "net/Hostname.h"
#include "net/WifiScan.h"
#include "net/WifiStore.h"
#include "web/Api.h"

void addWifiScanRoute(AsyncWebServer& server) {
    server.on("/api/system/wifi/scan", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        WifiScan::listNetworks(doc.to<JsonArray>());
        sendJson(request, doc);
    });
}

void addWifiRoutes(AsyncWebServer& server, SystemManager& system) {
    // A plain "/api/system/wifi" route also matches the paths below it, so these come first
    addWifiScanRoute(server);

    // The board restarts into setup mode
    server.on("/api/system/wifi/forget", HTTP_POST, [&system](AsyncWebServerRequest* request) {
        LOGF("Request to %s\n", request->url().c_str());
        WifiStore::clear();
        sendOk(request);
        system.scheduleRestart("WiFi forgotten");
    });

    // The saved network's name, never its password, and the name the board answers to on it
    server.on("/api/system/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["ssid"] = WifiStore::load().ssid;
        doc["hostname"] = String(Hostname) + ".local";
        sendJson(request, doc);
    });

    // Body: {"ssid": "Home", "password": "..."}. The board restarts and joins it, falling back to
    // setup mode if it can't
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/system/wifi", [&system](AsyncWebServerRequest* request, JsonVariant& json) {
        LOGF("Request to %s\n", request->url().c_str());
        const WifiCredentials credentials{json["ssid"] | "", json["password"] | ""};
        if (!WifiStore::isValid(credentials)) {
            sendError(request, 400, WifiStore::ValidityRule);
            return;
        }
        WifiStore::save(credentials);
        LOGF("WiFi credentials saved for \"%s\"\n", credentials.ssid.c_str());
        sendOk(request);
        system.scheduleRestart(credentials.ssid);
    }));
}
