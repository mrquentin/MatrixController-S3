#include "ota/NetworkUpload.h"

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "Config.h"
#include "ota/UpdatePassword.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

namespace {
    const char* errorName(const ota_error_t error) {
        switch (error) {
            case OTA_AUTH_ERROR: return "Wrong update password";
            case OTA_BEGIN_ERROR: return "Couldn't start writing";
            case OTA_CONNECT_ERROR: return "Couldn't connect back to the computer";
            case OTA_RECEIVE_ERROR: return "Upload interrupted";
            case OTA_END_ERROR: return "Image invalid";
            default: return "Update failed";
        }
    }
}

NetworkUpload::NetworkUpload(FirmwareUpdate& update) : _update(update) {}

void NetworkUpload::begin() {
    ArduinoOTA.setPort(Port);
    // mDNS is already running (MDNSName), the service is announced on it in start()
    ArduinoOTA.setMdnsEnabled(false);
    // The restart is left to loop(), which says so on the panel first
    ArduinoOTA.setRebootOnSuccess(false);

    ArduinoOTA.onStart([this] {
        const bool files = ArduinoOTA.getCommand() == U_SPIFFS;
        _attempt = _update.claim(UpdateSource::Network, Update.size(), files);
        // A web upload or release download got there first: stop this one, whose writes then fail
        if (_attempt == 0) Update.abort();
        if (_onProgress) _onProgress();
    });
    ArduinoOTA.onProgress([this](const unsigned int written, unsigned int) {
        if (_attempt == 0) return;
        _update.progress(_attempt, written);
        if (_onProgress) _onProgress();
    });
    ArduinoOTA.onEnd([this] {
        _update.complete(_attempt, true);
        _attempt = 0;
    });
    ArduinoOTA.onError([this](const ota_error_t error) {
        DEBUG_PRINTF("Network upload: %s\n", errorName(error));
        // A wrong password is refused before anything starts
        _update.complete(_attempt, false, errorName(error));
        _attempt = 0;
    });

    if (UpdatePassword::isSet()) start();
}

void NetworkUpload::start() {
    ArduinoOTA.setPasswordHash(UpdatePassword::hash().c_str());
    ArduinoOTA.begin();
    // Lets PlatformIO and the Arduino IDE find the board, marked as needing a password
    MDNS.enableArduino(Port, true);
    _listening = true;
    DEBUG_PRINTF("Network upload: listening on port %u\n", Port);
}

void NetworkUpload::passwordChanged() {
    if (_listening) {
        _restartNeeded = true;
    } else {
        _startRequested = true;
    }
}

void NetworkUpload::handle() {
    if (_startRequested.exchange(false)) start();
    if (_listening) ArduinoOTA.handle();
}

void NetworkUpload::onProgress(std::function<void()> callback) {
    _onProgress = std::move(callback);
}
