#pragma once

#ifndef _UPDATESCREEN_h
#define _UPDATESCREEN_h

#include <cstdint>
#include "display/IDisplay.h"
#include "ota/FirmwareUpdate.h"

/// Shows updates on the panel: the progress of a running one, and a failed one for a few seconds.
class UpdateScreen {
public:
    explicit UpdateScreen(const FirmwareUpdate& update) : _update(update) {}

    /// Draws the update's progress or failure if there is one to show. Returns false when the panel
    /// is free for the apps. Call from loop().
    bool draw(IDisplay& display, uint32_t now);
    /// Draws the progress of the running update, a few times a second at most: it's also called for
    /// every block PlatformIO sends, and drawing slows the flash writes down.
    void drawProgress(IDisplay& display);

private:
    const FirmwareUpdate& _update;
    uint32_t _lastProgressMs = 0;
    uint32_t _shownFailure = 0;
    // millis() when the latest failure was first shown, 0 once it's been shown long enough
    uint32_t _failureShownAtMs = 0;
};

#endif
