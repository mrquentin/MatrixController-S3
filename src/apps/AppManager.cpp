#include "apps/AppManager.h"

#include <LittleFS.h>
#include "display/Text.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

// Saved file layout: {"active": "clock", "apps": {"clock": {...settings}, "text": {...}}}

AppManager::AppManager(const char* filePath) : _filePath(filePath) {}

void AppManager::add(App& app) {
    _apps.push_back(&app);
}

void AppManager::begin() {
    std::lock_guard lock(_mutex);

    JsonDocument doc;
    if (File file = LittleFS.open(_filePath, FILE_READ)) {
        const DeserializationError error = deserializeJson(doc, file);
        file.close();
        if (error) DEBUG_PRINTF("Failed to read %s: %s, using app defaults\n", _filePath, error.c_str());
    }

    const JsonObjectConst saved = doc["apps"];
    for (App* app : _apps) {
        const JsonObjectConst config = saved[app->id()];
        if (!config.isNull()) app->setConfig(config);
    }

    App* active = find(doc["active"] | "");
    if (!active && !_apps.empty()) active = _apps.front();
    switchTo(active);
}

void AppManager::render(IDisplay& display, const uint32_t nowMs) {
    std::lock_guard lock(_mutex);

    display.clear();
    if (_active) {
        _active->render(display, nowMs);
    } else {
        drawText(display, 1, 1, "No app", Colors::White);
    }
    display.present();
}

bool AppManager::activate(const std::string_view id) {
    std::lock_guard lock(_mutex);

    App* app = find(id);
    if (!app) return false;
    switchTo(app);
    save();
    return true;
}

bool AppManager::setConfig(const std::string_view id, const JsonObjectConst config, JsonObject result) {
    std::lock_guard lock(_mutex);

    App* app = find(id);
    if (!app) return false;
    app->setConfig(config);
    app->getConfig(result);
    save();
    return true;
}

void AppManager::describe(JsonObject out) {
    std::lock_guard lock(_mutex);

    out["active"] = _active ? _active->id() : nullptr;
    JsonArray list = out["apps"].to<JsonArray>();
    for (const App* app : _apps) {
        JsonObject entry = list.add<JsonObject>();
        entry["id"] = app->id();
        entry["name"] = app->name();
        ConfigSchema schema(entry["schema"].to<JsonArray>());
        app->describeConfig(schema);
        app->getConfig(entry["config"].to<JsonObject>());
    }
}

App* AppManager::find(const std::string_view id) const {
    for (App* app : _apps) {
        if (id == app->id()) return app;
    }
    return nullptr;
}

void AppManager::switchTo(App* app) {
    if (app == _active) return;
    if (_active) _active->deactivate();
    _active = app;
    if (_active) {
        _active->activate();
        DEBUG_PRINTF("Active app: %s\n", _active->id());
    }
}

void AppManager::save() const {
    JsonDocument doc;
    doc["active"] = _active ? _active->id() : nullptr;
    JsonObject apps = doc["apps"].to<JsonObject>();
    for (const App* app : _apps) {
        app->getConfig(apps[app->id()].to<JsonObject>());
    }

    File file = LittleFS.open(_filePath, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTF("Failed to open %s for writing\n", _filePath);
        return;
    }
    serializeJson(doc, file);
    file.close();
}
