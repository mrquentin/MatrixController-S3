#include "ota/FirmwareUpdate.h"

#include <algorithm>
#include <cstring>
#include <Arduino.h>
#include <Update.h>
#include <esp_app_format.h>
#include <esp_ota_ops.h>

#include "FirmwareVersion.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

// The core accepts new firmware as soon as it boots unless this says otherwise; returning true
// leaves that to FirmwareUpdate::confirmWhenHealthy()
extern "C" bool verifyRollbackLater() {
    return true;
}

namespace {
    // Long enough for a crash shortly after start-up to still roll back
    constexpr uint32_t HealthyAfterMs = 30000;

    static_assert(sizeof(esp_image_header_t) == 24 && sizeof(esp_image_segment_header_t) == 8,
                  "FirmwareUpdate::CheckedLength assumes these sizes");

    /// Returns why the start of an image can't be installed, or nullptr if it looks right.
    const char* checkImage(const uint8_t* head) {
        esp_image_header_t header;
        memcpy(&header, head, sizeof(header));
        if (header.magic != ESP_IMAGE_HEADER_MAGIC) return "Not a firmware image";
        if (static_cast<uint32_t>(header.chip_id) != CONFIG_IDF_FIRMWARE_CHIP_ID) return "Built for a different chip";

        // Apps start with their description, which bootloaders and merged images don't have
        uint32_t descriptionMagic;
        memcpy(&descriptionMagic, head + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t),
               sizeof(descriptionMagic));
        if (descriptionMagic != ESP_APP_DESC_MAGIC_WORD) return "Not an app image (use firmware.bin)";
        return nullptr;
    }

    void accept() {
        if (FirmwareUpdate::onProbation() && esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
            DEBUG_PRINT.println("Update: running firmware accepted");
        }
    }

    const char* sourceName(const UpdateSource source) {
        switch (source) {
            case UpdateSource::Upload: return "upload";
            case UpdateSource::Network: return "network";
            case UpdateSource::Release: return "release";
            default: return nullptr;
        }
    }

    const char* stateName(const FirmwareUpdate::State state) {
        switch (state) {
            case FirmwareUpdate::State::Running: return "running";
            case FirmwareUpdate::State::Installed: return "installed";
            case FirmwareUpdate::State::Failed: return "failed";
            default: return "idle";
        }
    }
}

uint32_t FirmwareUpdate::claim(const UpdateSource source, const size_t size, const bool files) {
    // Not while another update runs, nor after one installed: the board is about to restart
    State current = _state;
    do {
        if (current == State::Running || current == State::Installed) return 0;
    } while (!_state.compare_exchange_weak(current, State::Running));

    _source = source;
    _files = files;
    _written = 0;
    _size = size;
    _error[0] = '\0';
    // Never 0, which means "didn't start"
    uint32_t attempt = _attempt + 1;
    if (attempt == 0) attempt = 1;
    _attempt = attempt;
    DEBUG_PRINTF("Update: %s started, %u bytes\n", sourceName(source), static_cast<unsigned>(size));

    // Receiving an update shows this firmware works. Accepting it now also means a rollback
    // after the new one can't land on firmware that was never confirmed
    accept();
    return attempt;
}

uint32_t FirmwareUpdate::begin(const UpdateSource source, const size_t size) {
    const uint32_t attempt = claim(source, size, false);
    if (attempt == 0) return 0;
    _headLength = 0;

    const esp_partition_t* slot = esp_ota_get_next_update_partition(nullptr);
    if (!slot) {
        fail("No app slot to install into");
    } else if (size > slot->size) {
        fail("Too large for the app slot");
    }
    return attempt;
}

bool FirmwareUpdate::startWriting() {
    if (const char* problem = checkImage(_head)) {
        fail(problem);
        return false;
    }
    const size_t size = _size;
    if (!Update.begin(size > 0 ? size : UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        fail(Update.errorString());
        return false;
    }
    if (Update.write(_head, CheckedLength) != CheckedLength) {
        fail(Update.errorString());
        return false;
    }
    return true;
}

bool FirmwareUpdate::write(const uint32_t attempt, const uint8_t* data, size_t length) {
    if (!isRunning(attempt)) return false;
    const size_t received = length;

    if (_headLength < CheckedLength) {
        const size_t take = std::min(length, CheckedLength - _headLength);
        memcpy(_head + _headLength, data, take);
        _headLength += take;
        data += take;
        length -= take;
        if (_headLength == CheckedLength && !startWriting()) return false;
    }

    // Update only reads the data, it's just not declared const
    if (length > 0 && Update.write(const_cast<uint8_t*>(data), length) != length) {
        fail(Update.errorString());
        return false;
    }
    _written += received;
    return true;
}

bool FirmwareUpdate::finish(const uint32_t attempt) {
    if (!isRunning(attempt)) return false;
    if (_headLength < CheckedLength) {
        fail("Image too short");
        return false;
    }
    const size_t size = _size;
    if (size > 0 && _written != size) {
        fail("Image incomplete");
        return false;
    }
    // With an unknown size, whatever arrived is the image
    if (!Update.end(size == 0)) {
        fail(Update.errorString());
        return false;
    }
    DEBUG_PRINT.println("Update: installed, boots after the restart");
    _state = State::Installed;
    return true;
}

void FirmwareUpdate::cancel(const uint32_t attempt, const char* reason) {
    if (isRunning(attempt)) fail(reason);
}

void FirmwareUpdate::progress(const uint32_t attempt, const size_t written) {
    if (isRunning(attempt)) _written = written;
}

void FirmwareUpdate::complete(const uint32_t attempt, const bool succeeded, const char* error) {
    if (!isRunning(attempt)) return;
    if (!succeeded) {
        fail(error ? error : "Update failed");
        return;
    }
    DEBUG_PRINT.println("Update: installed, boots after the restart");
    _state = State::Installed;
}

void FirmwareUpdate::fail(const char* reason) {
    // ArduinoOTA has stopped writing by the time it reports an error, so this only ever ends
    // our own writes
    if (Update.isRunning()) Update.abort();
    strlcpy(_error, reason, sizeof(_error));
    DEBUG_PRINTF("Update: failed, %s\n", reason);
    _state = State::Failed;
}

void FirmwareUpdate::confirmWhenHealthy(const uint32_t upForMs) {
    // Only ever needed once per boot, and reading the flash state each loop would be wasted
    static bool checked = false;
    if (checked || upForMs < HealthyAfterMs) return;
    checked = true;
    accept();
}

bool FirmwareUpdate::onProbation() {
    esp_ota_img_states_t state;
    return esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK
        && state == ESP_OTA_IMG_PENDING_VERIFY;
}

bool FirmwareUpdate::rolledBack() {
    // The slot the failed update went into stays marked invalid until the next update replaces it
    return esp_ota_get_last_invalid_partition() != nullptr;
}

const char* FirmwareUpdate::version() {
    return FIRMWARE_VERSION;
}

const char* FirmwareUpdate::runningSlot() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    return running ? running->label : "";
}

void FirmwareUpdate::describe(const JsonObject out) const {
    out["version"] = version();
    out["slot"] = runningSlot();
    out["onProbation"] = onProbation();
    out["rolledBack"] = rolledBack();

    const State state = _state;
    out["state"] = stateName(state);
    out["source"] = sourceName(_source);
    out["files"] = _files.load();
    out["written"] = _written.load();
    out["size"] = _size.load();
    if (state == State::Failed) out["error"] = _error;
}
