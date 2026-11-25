#include "apps/AppRoutes.h"

#include <AsyncJson.h>
#include "Log.h"
#include "web/Api.h"

namespace {
    /// Answers a change that failed. Returns false if there is nothing to answer yet.
    bool sendFailure(AsyncWebServerRequest* request, const AppManager::Result result) {
        switch (result) {
            case AppManager::Result::Ok: return false;
            case AppManager::Result::UnknownApp: sendError(request, 404, "unknown app"); return true;
            case AppManager::Result::NotSaved:
                sendError(request, 500, "Applied, but couldn't be saved: it's lost at the next restart");
                return true;
        }
        return false;
    }
}

void addAppRoutes(AsyncWebServer& server, AppManager& apps) {
    server.on("/api/apps", HTTP_GET, [&apps](AsyncWebServerRequest* request) {
        JsonDocument doc;
        apps.describe(doc.to<JsonObject>());
        sendJson(request, doc);
    });

    // Body: {"id": "clock"}
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/apps/active", [&apps](AsyncWebServerRequest* request, JsonVariant& json) {
        LOGF("Request to %s\n", request->url().c_str());
        const char* id = json["id"];
        if (!id) {
            sendError(request, 404, "unknown app");
            return;
        }
        if (!sendFailure(request, apps.activate(id))) sendOk(request);
    }));

    // Body: {"id": "clock", "config": {...}}. Responds with the settings the app actually applied
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/apps/config", [&apps](AsyncWebServerRequest* request, JsonVariant& json) {
        LOGF("Request to %s\n", request->url().c_str());
        const char* id = json["id"];
        const JsonObjectConst config = json["config"];
        if (!id || config.isNull()) {
            sendError(request, 400, "id and config are required");
            return;
        }

        JsonDocument doc;
        if (!sendFailure(request, apps.setConfig(id, config, doc["config"].to<JsonObject>()))) sendJson(request, doc);
    }));
}
