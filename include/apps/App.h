#pragma once

#ifndef _APP_h
#define _APP_h

#include <cstdint>
#include "apps/AppSettings.h"
#include "display/IDisplay.h"

/// A small program that owns the matrix while it is the active app (clock, weather, F1...).
/// Register instances with AppManager, which persists their settings and switches between them.
///
/// Threading: render() runs on the loop task. The other methods may run on the web server
/// task, but AppManager never calls two of them at the same time, so apps need no locking
/// of their own. render() and activate() must not block: fetch network data in a background
/// task started in activate() and stopped in deactivate(), and let render() show the latest result.
class App {
public:
    virtual ~App() = default;

    /// Stable identifier used in the API and in the saved config ("clock", "f1"). Never change it,
    /// or saved settings are lost.
    [[nodiscard]] virtual const char* id() const = 0;
    /// Name shown in the web UI.
    [[nodiscard]] virtual const char* name() const = 0;

    /// Declares the app's settings, see AppSettings. Called to describe, save and change them, so
    /// react to a change by comparing with the values before. Leave empty for an app without settings.
    virtual void settings(AppSettings& settings) {}

    /// Called when the app becomes the active one, and when it stops being active.
    virtual void activate() {}
    virtual void deactivate() {}

    /// Draws one frame. The frame is already cleared and is presented afterwards.
    virtual void render(IDisplay& display, uint32_t nowMs) = 0;
};

#endif
