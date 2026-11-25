#include "SystemRoutes.h"

#include <AsyncJson.h>
#include "Log.h"
#include "ota/FirmwareUpdate.h"
#include "web/Api.h"

void addSystemRoutes(AsyncWebServer& server, SystemManager& system, Config& config, const AutoOrientation& orientation) {
    server.on("/api/system/status", HTTP_GET, [&system, &orientation](AsyncWebServerRequest* request) {
        JsonDocument doc;
        system.describe(doc.to<JsonObject>());
        doc["firmwareVersion"] = FirmwareUpdate::version();
        orientation.describe(doc["orientation"].to<JsonObject>());
        sendJson(request, doc);
    });

    server.on("/api/system/config", HTTP_GET, [&config](AsyncWebServerRequest* request) {
        JsonDocument doc;
        config.describe(doc.to<JsonObject>());
        sendJson(request, doc);
    });

    // Body: the whole config, as GET gives it. Applies without a restart, except for what the
    // panel only reads at start-up (see DisplayConfig)
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/system/config", [&config](AsyncWebServerRequest* request, JsonVariant& json) {
        LOGF("Request to %s\n", request->url().c_str());
        if (config.update(json.as<JsonObjectConst>())) {
            sendOk(request);
        } else {
            sendError(request, 500, "Applied, but couldn't be saved: they're lost at the next restart");
        }
    }));

    server.on("/api/system/restart", HTTP_POST, [&system](AsyncWebServerRequest* request) {
        LOGF("Request to %s\n", request->url().c_str());
        sendOk(request);
        system.scheduleRestart("");
    });
}
