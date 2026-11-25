#include "ota/UpdateRoutes.h"

#include <AsyncJson.h>
#include "Log.h"
#include "ota/UpdatePassword.h"
#include "web/Api.h"

namespace {
    // Also in the web UI (UPDATE_PASSWORD_HEADER in ui/src/lib/config.ts)
    constexpr const char* PasswordHeader = "X-Update-Password";

    /// A request turned down: the HTTP status and why. status 0 when it wasn't.
    struct Refusal {
        int status;
        const char* message;
    };

    Refusal passwordRefusal(const UpdatePassword::Result result) {
        switch (result) {
            case UpdatePassword::Result::Ok: return {0, nullptr};
            case UpdatePassword::Result::NotSet: return {409, "Set an update password first"};
            case UpdatePassword::Result::LockedOut: return {429, "Too many wrong passwords, try again in a minute"};
            case UpdatePassword::Result::Wrong: break;
        }
        return {401, "Wrong update password"};
    }

    Refusal checkPassword(AsyncWebServerRequest* request) {
        return passwordRefusal(UpdatePassword::check(request->header(PasswordHeader).c_str()));
    }

    /// Answers the request and returns false unless it carries the right update password.
    bool authorize(AsyncWebServerRequest* request) {
        const Refusal refusal = checkPassword(request);
        if (refusal.status) sendError(request, refusal.status, refusal.message);
        return !refusal.status;
    }

    /// What the body handler of an upload decided, for the request handler that answers it
    struct UploadState {
        Refusal refusal;
        // The update this upload feeds, 0 if refused
        uint32_t attempt;
    };

    void addReleaseRoutes(AsyncWebServer& server, ReleaseUpdate& releases) {
        // Looks up the latest GitHub release in the background; GET /api/system/update tells the result
        server.on("/api/system/update/release/check", HTTP_POST, [&releases](AsyncWebServerRequest* request) {
            if (!releases.check()) {
                sendError(request, 409, "A check or install is already underway");
                return;
            }
            sendOk(request, 202);
        });

        // Downloads and installs the release the last check found, in the background
        server.on("/api/system/update/release/install", HTTP_POST, [&releases](AsyncWebServerRequest* request) {
            LOGF("Request to %s\n", request->url().c_str());
            if (!authorize(request)) return;
            if (!releases.install()) {
                sendError(request, 409, "Check for a release first, or wait for the one underway");
                return;
            }
            sendOk(request, 202);
        });
    }

    // Body: firmware.bin as application/octet-stream, written to flash as it arrives. The board
    // restarts into it once it's in (see loop())
    void addUploadRoute(AsyncWebServer& server, FirmwareUpdate& update) {
        server.on("/api/system/update", HTTP_POST, [&update](AsyncWebServerRequest* request) {
            // Runs once the whole body is in
            const auto* upload = static_cast<const UploadState*>(request->_tempObject);
            const bool ours = upload && update.attempt() == upload->attempt;
            if (!upload) {
                sendError(request, 400, "No firmware received");
            } else if (upload->refusal.status) {
                sendError(request, upload->refusal.status, upload->refusal.message);
            } else if (ours && update.state() == FirmwareUpdate::State::Installed) {
                sendOk(request);
            } else {
                sendError(request, 422, ours && update.state() == FirmwareUpdate::State::Failed ? update.error() : "Update failed");
            }
        }, nullptr, [&update](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
            if (index == 0) {
                LOGF("Request to %s, %u bytes\n", request->url().c_str(), static_cast<unsigned>(total));
                UploadState upload{checkPassword(request), 0};
                if (!upload.refusal.status) {
                    const FirmwareUpdate::Start start = update.begin(UpdateSource::Upload, total);
                    upload.attempt = start.attempt;
                    if (start.refusal) upload.refusal = {409, start.refusal};
                }
                // The request frees it with free()
                request->_tempObject = malloc(sizeof(UploadState));
                if (request->_tempObject) *static_cast<UploadState*>(request->_tempObject) = upload;
                if (upload.attempt != 0) {
                    // A dropped connection leaves the image incomplete
                    request->onDisconnect([&update, attempt = upload.attempt] {
                        update.cancel(attempt, "Upload interrupted");
                    });
                }
            }

            // Refused uploads are still received, just not written
            const auto* upload = static_cast<const UploadState*>(request->_tempObject);
            if (!upload || upload->attempt == 0) return;
            if (update.write(upload->attempt, data, len) && index + len == total) {
                update.finish(upload->attempt);
            }
        });
    }
}

void addUpdateRoutes(AsyncWebServer& server, FirmwareUpdate& update, NetworkUpload& networkUpload,
                     ReleaseUpdate& releases) {
    // A plain "/api/system/update" route also matches the paths below it, so these come first

    // Checks a password without changing anything, so the UI can find out before a long upload
    server.on("/api/system/update/verify", HTTP_POST, [](AsyncWebServerRequest* request) {
        if (authorize(request)) sendOk(request);
    });

    // Body: {"current": "...", "password": "..."}. current is only needed when one is set already
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/system/update/password", [&networkUpload](AsyncWebServerRequest* request, JsonVariant& json) {
        LOGF("Request to %s\n", request->url().c_str());
        const std::string password = json["password"] | "";
        if (!UpdatePassword::isValid(password)) {
            char details[64];
            snprintf(details, sizeof(details), "The password must be %u to %u characters",
                     static_cast<unsigned>(UpdatePassword::MinLength), static_cast<unsigned>(UpdatePassword::MaxLength));
            sendError(request, 400, details);
            return;
        }
        if (UpdatePassword::isSet()) {
            const UpdatePassword::Result result = UpdatePassword::check(json["current"] | "");
            const Refusal refusal = passwordRefusal(result);
            if (refusal.status) {
                sendError(request, refusal.status,
                          result == UpdatePassword::Result::Wrong ? "Wrong current password" : refusal.message);
                return;
            }
        }

        UpdatePassword::set(password);
        networkUpload.passwordChanged();
        sendOk(request);
    }));

    if (ReleaseUpdate::available()) addReleaseRoutes(server, releases);

    server.on("/api/system/update", HTTP_GET, [&update, &networkUpload, &releases](AsyncWebServerRequest* request) {
        JsonDocument doc;
        update.describe(doc.to<JsonObject>());
        doc["passwordSet"] = UpdatePassword::isSet();
        doc["passwordMinLength"] = UpdatePassword::MinLength;
        doc["passwordMaxLength"] = UpdatePassword::MaxLength;

        const JsonObject network = doc["networkUpload"].to<JsonObject>();
        network["listening"] = networkUpload.listening();
        network["port"] = NetworkUpload::Port;
        network["restartNeeded"] = networkUpload.restartNeeded();

        if (ReleaseUpdate::available()) {
            releases.describe(doc["release"].to<JsonObject>());
        } else {
            doc["release"] = nullptr;
        }
        sendJson(request, doc);
    });

    addUploadRoute(server, update);
}
