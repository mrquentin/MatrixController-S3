#pragma once

#ifndef _NETWORKUPLOAD_h
#define _NETWORKUPLOAD_h

#include <atomic>
#include <functional>
#include "ota/FirmwareUpdate.h"

/// Lets PlatformIO flash the board over WiFi ("pio run -e ota -t upload", see platformio.ini),
/// through ArduinoOTA. Uses the update password, and only runs once one is set.
///
/// An upload runs inside handle(), which then doesn't return until it's done, so loop() can't
/// draw meanwhile: the onProgress callback is the place to update the panel.
class NetworkUpload {
public:
    static constexpr uint16_t Port = 3232;

    explicit NetworkUpload(FirmwareUpdate& update);

    /// Starts listening if an update password is set. Call once WiFi and mDNS are up.
    void begin();
    /// Call when the update password was set. A first password starts listening (in handle());
    /// ArduinoOTA can't change one it has, so a new password applies after the next restart.
    void passwordChanged();
    /// Call from loop().
    void handle();
    void onProgress(std::function<void()> callback);

    [[nodiscard]] bool listening() const { return _listening; }
    /// True when PlatformIO needs the password that was in place at start-up, not the current one.
    [[nodiscard]] bool restartNeeded() const { return _restartNeeded; }

private:
    void start();

    FirmwareUpdate& _update;
    std::function<void()> _onProgress;
    std::atomic<bool> _listening{false};
    std::atomic<bool> _startRequested{false};
    std::atomic<bool> _restartNeeded{false};
    // The update this upload runs as, 0 when none
    uint32_t _attempt = 0;
};

#endif
