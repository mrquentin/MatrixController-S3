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
#include "ota/UpdatePassword.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

HttpHandlers::HttpHandlers(Config& config, AsyncWebServer &server, SystemManager &systemManager, AppManager &appManager,
                           FirmwareUpdate& firmwareUpdate, NetworkUpload& networkUpload, ReleaseUpdate& releaseUpdate) :
    _config(config), _server(server), _systemManager(systemManager), _appManager(appManager),
    _firmwareUpdate(firmwareUpdate), _networkUpload(networkUpload), _releaseUpdate(releaseUpdate) {}

void HttpHandlers::setup() {
    addHandlerRoot();
    addHandlerApiStatus();
    addHandlerApiConfig();
    addHandlerApiApps();
    addHandlerApiWifi();
    addHandlerApiRestart();
    addHandlerApiUpdate();

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
        doc["firmwareVersion"] = FirmwareUpdate::version();

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

namespace {
    // Requests that update send the update password in this header
    constexpr const char* PasswordHeader = "X-Update-Password";

    /// What the body handler of an upload decided, for the request handler that answers it
    struct UploadState {
        int status;
        // The update this upload feeds, 0 if refused
        uint32_t attempt;
    };

    void sendError(AsyncWebServerRequest *request, const int status, const char* details) {
        JsonDocument doc;
        doc["result"] = "error";
        doc["details"] = details;

        String response;
        serializeJson(doc, response);
        request->send(status, "application/json", response);
    }

    /// HTTP status for a password check: 200 when it's right
    int passwordStatus(const UpdatePassword::Result result) {
        switch (result) {
            case UpdatePassword::Result::Ok: return 200;
            case UpdatePassword::Result::NotSet: return 409;
            case UpdatePassword::Result::LockedOut: return 429;
            default: return 401;
        }
    }

    const char* passwordError(const int status) {
        switch (status) {
            case 409: return "Set an update password first";
            case 429: return "Too many wrong passwords, try again in a minute";
            default: return "Wrong update password";
        }
    }

    int authorizeUpdate(AsyncWebServerRequest *request) {
        return passwordStatus(UpdatePassword::check(request->header(PasswordHeader).c_str()));
    }
}

void HttpHandlers::addHandlerApiUpdate() {
    // As with WiFi, a plain "/api/system/update" route also matches the paths below it, so these come first

    // Checks a password without changing anything, so the UI can find out before a long upload
    _server.on("/api/system/update/verify", HTTP_POST, [](AsyncWebServerRequest *request) {
        const int status = authorizeUpdate(request);
        if (status != 200) {
            sendError(request, status, passwordError(status));
            return;
        }
        request->send(200, "application/json", "{\"result\":\"success\"}");
    });

    // Body: {"current": "...", "password": "..."}. current is only needed when one is set already
    _server.addHandler(new AsyncCallbackJsonWebHandler("/api/system/update/password", [&](AsyncWebServerRequest *request, JsonVariant &json) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        const std::string password = json["password"] | "";
        if (!UpdatePassword::isValid(password)) {
            sendError(request, 400, "The password must be 8 to 64 characters");
            return;
        }
        if (UpdatePassword::isSet()) {
            const int status = passwordStatus(UpdatePassword::check(json["current"] | ""));
            if (status != 200) {
                sendError(request, status, status == 401 ? "Wrong current password" : passwordError(status));
                return;
            }
        }

        UpdatePassword::set(password);
        _networkUpload.passwordChanged();
        request->send(200, "application/json", "{\"result\":\"success\"}");
    }));

    // Looks up the latest GitHub release in the background; GET /api/system/update tells the result
    _server.on("/api/system/update/release/check", HTTP_POST, [&](AsyncWebServerRequest *request) {
        if (!ReleaseUpdate::available()) {
            sendError(request, 404, "No release repository is set up in this firmware");
            return;
        }
        if (!_releaseUpdate.check()) {
            sendError(request, 409, "A check or install is already underway");
            return;
        }
        request->send(202, "application/json", "{\"result\":\"success\"}");
    });

    // Downloads and installs the release the last check found, in the background
    _server.on("/api/system/update/release/install", HTTP_POST, [&](AsyncWebServerRequest *request) {
        DEBUG_PRINTF("Request to %s\n", request->url().c_str());
        const int status = authorizeUpdate(request);
        if (status != 200) {
            sendError(request, status, passwordError(status));
            return;
        }
        if (!ReleaseUpdate::available()) {
            sendError(request, 404, "No release repository is set up in this firmware");
            return;
        }
        if (!_releaseUpdate.install()) {
            sendError(request, 409, "Check for a release first, or wait for the one underway");
            return;
        }
        request->send(202, "application/json", "{\"result\":\"success\"}");
    });

    _server.on("/api/system/update", HTTP_GET, [&](AsyncWebServerRequest *request) {
        JsonDocument doc;
        _firmwareUpdate.describe(doc.to<JsonObject>());
        doc["passwordSet"] = UpdatePassword::isSet();

        const JsonObject network = doc["networkUpload"].to<JsonObject>();
        network["listening"] = _networkUpload.listening();
        network["port"] = NetworkUpload::Port;
        network["restartNeeded"] = _networkUpload.restartNeeded();

        if (ReleaseUpdate::available()) {
            _releaseUpdate.describe(doc["release"].to<JsonObject>());
        } else {
            doc["release"] = nullptr;
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Body: firmware.bin as application/octet-stream, written to flash as it arrives. The board
    // restarts into it once it's in (see loop())
    _server.on("/api/system/update", HTTP_POST, [&](AsyncWebServerRequest *request) {
        // Runs once the whole body is in
        const auto* upload = static_cast<const UploadState*>(request->_tempObject);
        const int status = upload ? upload->status : 400;
        if (status == 400) {
            sendError(request, 400, "No firmware received");
        } else if (status != 200) {
            sendError(request, status, status == 503 ? "Another update is running" : passwordError(status));
        } else if (_firmwareUpdate.attempt() == upload->attempt
                   && _firmwareUpdate.state() == FirmwareUpdate::State::Installed) {
            request->send(200, "application/json", "{\"result\":\"success\"}");
        } else {
            sendError(request, 422, _firmwareUpdate.attempt() == upload->attempt
                && _firmwareUpdate.state() == FirmwareUpdate::State::Failed ? _firmwareUpdate.error() : "Update failed");
        }
    }, nullptr, [&](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (index == 0) {
            DEBUG_PRINTF("Request to %s, %u bytes\n", request->url().c_str(), static_cast<unsigned>(total));
            UploadState upload{authorizeUpdate(request), 0};
            if (upload.status == 200) {
                // Reasons it fails straight away (e.g. too large) are in the update's own state
                upload.attempt = _firmwareUpdate.begin(UpdateSource::Upload, total);
                if (upload.attempt == 0) upload.status = 503;
            }
            // The request frees it with free()
            request->_tempObject = malloc(sizeof(UploadState));
            if (request->_tempObject) *static_cast<UploadState*>(request->_tempObject) = upload;
            if (upload.attempt != 0) {
                // A dropped connection leaves the image incomplete
                request->onDisconnect([this, attempt = upload.attempt] {
                    _firmwareUpdate.cancel(attempt, "Upload interrupted");
                });
            }
        }

        // Refused uploads are still received, just not written
        const auto* upload = static_cast<const UploadState*>(request->_tempObject);
        if (!upload || upload->attempt == 0) return;
        if (_firmwareUpdate.write(upload->attempt, data, len) && index + len == total) {
            _firmwareUpdate.finish(upload->attempt);
        }
    });
}
