#pragma once

#ifndef _FIRMWAREUPDATE_h
#define _FIRMWAREUPDATE_h

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <ArduinoJson.h>

/// Where an update comes from.
enum class UpdateSource : uint8_t {
    None,
    /// firmware.bin uploaded from the web UI
    Upload,
    /// PlatformIO over WiFi (espota, see NetworkUpload)
    Network,
    /// Downloaded from the project's GitHub releases (see ReleaseUpdate)
    Release,
};

/// Installs new firmware into the spare app slot while the current one keeps running, then
/// the board restarts into it. One update at a time, whatever its source.
///
/// Uploads and downloads stream through begin(), write() and finish(), which check that the
/// image is an app for this chip before anything is written. ArduinoOTA drives the flash
/// writer itself and only reports here (claim(), progress(), complete()).
///
/// The new firmware starts out on probation: if it crashes or restarts before
/// confirmWhenHealthy() accepts it, the bootloader goes back to the previous one.
///
/// State can be read from any task; an update is driven from one task at a time.
class FirmwareUpdate {
public:
    enum class State : uint8_t { Idle, Running, Installed, Failed };

    /// Starts an update of size bytes (0 if unknown). Returns its attempt number, which the
    /// calls below take so that a caller only ever affects its own update; 0 if another update
    /// runs. When the image can't fit, the update has already failed (see isRunning()).
    uint32_t begin(UpdateSource source, size_t size);
    /// Writes the next part of the image. Returns false, and fails the update, on errors.
    bool write(uint32_t attempt, const uint8_t* data, size_t length);
    /// Checks the complete image and makes it the one to boot. Returns false if it's incomplete
    /// or invalid.
    bool finish(uint32_t attempt);
    /// Gives up on an update, e.g. when the upload connection drops.
    void cancel(uint32_t attempt, const char* reason);

    /// For updaters that write the flash themselves: marks one as running, like begin(). files
    /// is true when it replaces the file system (LittleFS) instead of the firmware.
    uint32_t claim(UpdateSource source, size_t size, bool files);
    void progress(uint32_t attempt, size_t written);
    void complete(uint32_t attempt, bool succeeded, const char* error = nullptr);

    [[nodiscard]] bool isRunning(uint32_t attempt) const { return _state == State::Running && _attempt == attempt; }
    [[nodiscard]] State state() const { return _state; }
    [[nodiscard]] UpdateSource source() const { return _source; }
    [[nodiscard]] bool replacesFiles() const { return _files; }
    [[nodiscard]] size_t written() const { return _written; }
    /// 0 when unknown
    [[nodiscard]] size_t size() const { return _size; }
    /// Why the last update failed. Only meaningful once state() is Failed
    [[nodiscard]] const char* error() const { return _error; }
    /// Number of the latest update started, so a new failure can be told from one already shown.
    [[nodiscard]] uint32_t attempt() const { return _attempt; }

    /// Accepts the running firmware once it has served the web UI (or the setup portal) for a
    /// while, ending its probation after an update. upForMs is how long it has. Call from loop().
    static void confirmWhenHealthy(uint32_t upForMs);
    /// True while the running firmware is on probation after an update.
    [[nodiscard]] static bool onProbation();
    /// True if the last update didn't come up properly and the board went back to this firmware.
    [[nodiscard]] static bool rolledBack();
    [[nodiscard]] static const char* version();
    /// Label of the app slot running now ("ota_0" or "ota_1").
    [[nodiscard]] static const char* runningSlot();

    /// Fills in the update state for the web UI.
    void describe(JsonObject out) const;

private:
    void fail(const char* reason);
    bool startWriting();

    // The start of an image is held back until it can be checked: the image header, the first
    // segment's header and the app description's magic word
    static constexpr size_t CheckedLength = 24 + 8 + 4;
    uint8_t _head[CheckedLength] = {};
    size_t _headLength = 0;

    std::atomic<State> _state{State::Idle};
    std::atomic<UpdateSource> _source{UpdateSource::None};
    std::atomic<bool> _files{false};
    std::atomic<size_t> _written{0};
    std::atomic<size_t> _size{0};
    std::atomic<uint32_t> _attempt{0};
    char _error[64] = {};
};

#endif
