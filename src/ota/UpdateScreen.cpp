#include "ota/UpdateScreen.h"

#include <Arduino.h>
#include <algorithm>
#include <cstdio>
#include "display/Text.h"

namespace {
    constexpr uint32_t ProgressIntervalMs = 150;
    // A failed update is mentioned on the panel this long before the apps come back
    constexpr uint32_t FailureScreenMs = 5000;
}

bool UpdateScreen::draw(IDisplay& display, const uint32_t now) {
    // Uploads from the web UI and release downloads run on other tasks
    if (_update.state() == FirmwareUpdate::State::Running) {
        drawProgress(display);
        return true;
    }

    // A failed firmware update changed nothing: it went to the spare slot, the running firmware
    // simply carries on. A file system update writes over the files in place, so those may be gone
    if (_update.state() == FirmwareUpdate::State::Failed && _update.attempt() != _shownFailure) {
        _shownFailure = _update.attempt();
        _failureShownAtMs = now | 1;
        showStatus(display, "Update failed", _update.replacesFiles() ? "Files incomplete" : "Nothing changed",
                   "See the web page");
    }
    if (_failureShownAtMs == 0) return false;
    if (now - _failureShownAtMs < FailureScreenMs) return true;
    _failureShownAtMs = 0;
    return false;
}

void UpdateScreen::drawProgress(IDisplay& display) {
    const uint32_t now = millis();
    if (now - _lastProgressMs < ProgressIntervalMs) return;
    _lastProgressMs = now;

    const size_t size = _update.size();
    const size_t written = _update.written();
    char amount[16];
    if (size > 0) {
        snprintf(amount, sizeof(amount), "%u%%", static_cast<unsigned>(static_cast<uint64_t>(written) * 100 / size));
    } else {
        snprintf(amount, sizeof(amount), "%u KB", static_cast<unsigned>(written / 1024));
    }

    // Title, amount and a progress bar, then a reminder not to unplug it
    constexpr int32_t BarHeight = 3;
    const int32_t lineHeight = textHeight() + 4;
    int32_t y = (static_cast<int32_t>(display.height()) - 3 * lineHeight - BarHeight) / 2;
    display.clear();
    drawTextCentered(display, y, _update.replacesFiles() ? "Updating files" : "Updating", Colors::Cyan);
    y += lineHeight;
    drawTextCentered(display, y, amount, Colors::White);
    y += lineHeight;

    const int32_t barWidth = display.width() - 16;
    display.fillRect(8, y, barWidth, BarHeight, Colors::Gray.scaled(60));
    if (size > 0) {
        const auto done = static_cast<int32_t>(static_cast<uint64_t>(barWidth) * std::min(written, size) / size);
        display.fillRect(8, y, done, BarHeight, Colors::Cyan);
    }
    y += BarHeight + 4;
    drawTextCentered(display, y, "Keep it powered", Colors::Gray);
    display.present();
}
