#pragma once

#ifndef _APPMANAGER_h
#define _APPMANAGER_h

#include <mutex>
#include <string_view>
#include <vector>
#include <ArduinoJson.h>
#include "apps/App.h"
#include "display/IDisplay.h"

/// Owns the list of apps, renders the active one and switches between them.
/// Settings and the active app are saved to a JSON file and restored by begin().
///
/// render() is for the loop task; the other public methods are safe to call from any task.
class AppManager {
public:
    explicit AppManager(const char* filePath);

    /// Registers an app. Call before begin(). The app must outlive the manager.
    void add(App& app);

    /// Restores saved settings and activates the saved app (or the first one).
    void begin();

    /// Draws a full frame of the active app and presents it.
    void render(IDisplay& display, uint32_t nowMs);

    /// Switches to the app with this id and saves the choice. Returns false if there is no such app.
    bool activate(std::string_view id);

    /// Applies and saves settings for the app with this id, then writes the resulting
    /// settings into result. Returns false if there is no such app.
    bool setConfig(std::string_view id, JsonObjectConst config, JsonObject result);

    /// Writes {active, apps: [{id, name, schema, config}]} for the web UI.
    void describe(JsonObject out);

private:
    [[nodiscard]] App* find(std::string_view id) const;
    void switchTo(App* app);
    void save() const;

    const char* _filePath;
    std::vector<App*> _apps;
    App* _active = nullptr;
    std::mutex _mutex;
};

#endif
