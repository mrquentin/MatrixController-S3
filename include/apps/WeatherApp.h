#pragma once

#ifndef _WEATHERAPP_h
#define _WEATHERAPP_h

#include <mutex>
#include <string>
#include "apps/App.h"
#include "apps/PeriodicTask.h"

/// Current weather and today's high/low for a city, from Open-Meteo (free, no API key).
/// The city name is turned into coordinates with Open-Meteo's geocoding service, and the
/// weather is refreshed in the background while the app is shown.
class WeatherApp final : public App {
public:
    WeatherApp();

    [[nodiscard]] const char* id() const override { return "weather"; }
    [[nodiscard]] const char* name() const override { return "Weather"; }

    void describeConfig(ConfigSchema& schema) const override;
    void getConfig(JsonObject config) const override;
    void setConfig(JsonObjectConst config) override;

    void activate() override;
    void deactivate() override;
    void render(IDisplay& display, uint32_t nowMs) override;

private:
    enum class Status : uint8_t { NoCity, Loading, CityNotFound, Unavailable, Ready };
    enum class Lookup : uint8_t { Found, NotFound, Failed };

    struct Settings {
        std::string city;
        std::string country;  // optional ISO 3166-1 alpha-2 code to pick the right city
        bool fahrenheit = false;
        int32_t refreshMinutes = 15;
    };

    struct Location {
        std::string name;
        float latitude = 0;
        float longitude = 0;
    };

    struct Weather {
        float temperature = 0;
        float high = 0;
        float low = 0;
        int code = 0;
        bool isDay = true;
        int humidity = 0;
        float wind = 0;
    };

    // Background task
    void refresh();
    Lookup findCity(const Settings& settings, Location& location) const;
    bool fetchWeather(const Settings& settings, const Location& location, Weather& weather) const;

    void renderReady(IDisplay& display) const;

    PeriodicTask _task;

    // Shared between the task and the other methods, guarded by _mutex
    mutable std::mutex _mutex;
    Settings _settings;
    uint32_t _generation = 0;  // bumped when a setting change makes the shown data outdated
    Status _status = Status::NoCity;
    Weather _weather;
    std::string _place;

    // Only touched by the task: the city lookup is kept until the city setting changes
    Location _location;
    std::string _locationCity;
    std::string _locationCountry;
    bool _hasLocation = false;
};

#endif
