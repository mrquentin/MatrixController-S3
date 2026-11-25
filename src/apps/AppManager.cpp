#include "apps/AppManager.h"

#include <LittleFS.h>
#include "display/Text.h"
#include "Log.h"

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
        if (error) LOGF("Failed to read %s: %s, using app defaults\n", _filePath, error.c_str());
    }

    const JsonObjectConst saved = doc["apps"];
    for (App* app : _apps) {
        AppSettings settings = AppSettings::write(saved[app->id()]);
        app->settings(settings);
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

AppManager::Result AppManager::activate(const std::string_view id) {
    std::lock_guard lock(_mutex);

    App* app = find(id);
    if (!app) return Result::UnknownApp;
    switchTo(app);
    return save() ? Result::Ok : Result::NotSaved;
}

AppManager::Result AppManager::setConfig(const std::string_view id, const JsonObjectConst config, JsonObject result) {
    std::lock_guard lock(_mutex);

    App* app = find(id);
    if (!app) return Result::UnknownApp;
    AppSettings changes = AppSettings::write(config);
    app->settings(changes);
    AppSettings applied = AppSettings::read(result);
    app->settings(applied);
    return save() ? Result::Ok : Result::NotSaved;
}

void AppManager::describe(JsonObject out) {
    std::lock_guard lock(_mutex);

    out["active"] = _active ? _active->id() : nullptr;
    JsonArray list = out["apps"].to<JsonArray>();
    for (App* app : _apps) {
        JsonObject entry = list.add<JsonObject>();
        entry["id"] = app->id();
        entry["name"] = app->name();
        AppSettings schema = AppSettings::describe(entry["schema"].to<JsonArray>());
        app->settings(schema);
        AppSettings values = AppSettings::read(entry["config"].to<JsonObject>());
        app->settings(values);
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
        LOGF("Active app: %s\n", _active->id());
    }
}

bool AppManager::save() {
    JsonDocument doc;
    doc["active"] = _active ? _active->id() : nullptr;
    JsonObject apps = doc["apps"].to<JsonObject>();
    for (App* app : _apps) {
        AppSettings values = AppSettings::read(apps[app->id()].to<JsonObject>());
        app->settings(values);
    }

    File file = LittleFS.open(_filePath, FILE_WRITE);
    if (!file) {
        LOGF("Failed to open %s for writing\n", _filePath);
        return false;
    }
    serializeJson(doc, file);
    file.close();
    return true;
}
