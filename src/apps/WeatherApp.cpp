#include "apps/WeatherApp.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "apps/WeatherIcons.h"
#include "display/Shapes.h"
#include "display/Text.h"
#include "net/HttpJson.h"

namespace {
    constexpr uint16_t MaxCityLength = 64;
    // After a network error, try again sooner than the normal refresh interval
    constexpr uint32_t RetryIntervalMs = 60 * 1000;
    // TLS handshakes need a deep stack
    constexpr uint32_t TaskStackSize = 12 * 1024;

    constexpr Color PlaceColor{170, 170, 180};
    constexpr Color HighColor{255, 140, 40};
    constexpr Color LowColor{80, 160, 255};
    constexpr Color DetailColor{140, 140, 150};

    uint32_t minutesToMs(const int32_t minutes) {
        return static_cast<uint32_t>(minutes) * 60 * 1000;
    }

    std::string trimmed(const char* text) {
        std::string value(text);
        const auto first = value.find_first_not_of(" \t");
        if (first == std::string::npos) return {};
        const auto last = value.find_last_not_of(" \t");
        return value.substr(first, last - first + 1);
    }

    /// Keeps a valid two-letter country code (uppercased), anything else becomes "no country".
    std::string countryCode(const char* text) {
        std::string code = trimmed(text);
        if (code.size() != 2 || !isalpha(static_cast<unsigned char>(code[0])) ||
            !isalpha(static_cast<unsigned char>(code[1]))) {
            return {};
        }
        for (char& c : code) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        return code;
    }

    /// Rounded to whole degrees; lroundf returns an integer, so -0.3 shows as "0", not "-0".
    void formatTemperature(char* out, const size_t size, const float value) {
        snprintf(out, size, "%ld", lroundf(value));
    }

    /// Draws a temperature followed by a degree ring. Returns its width.
    int32_t drawTemperature(IDisplay& display, const int32_t x, const int32_t y, const float value,
                            const Color color, const uint8_t scale, const bool draw = true) {
        char text[8];
        formatTemperature(text, sizeof(text), value);
        const int32_t textEnd = x + textWidth(text, Fonts::Font5x7, scale);
        const int32_t radius = scale > 1 ? 2 : 1;
        if (draw) {
            drawText(display, x, y, text, color, Fonts::Font5x7, scale);
            drawCircle(display, textEnd + 1 + radius, y + radius, radius, color);
        }
        return textEnd + 1 + 2 * radius + 1 - x;
    }

    void drawCenteredLine(IDisplay& display, const int32_t y, const char* text, const Color color) {
        drawText(display, (static_cast<int32_t>(display.width()) - textWidth(text)) / 2, y, text, color);
    }

    void drawMessage(IDisplay& display, const char* first, const char* second) {
        const int32_t lineHeight = textHeight() + 3;
        const int32_t lines = second ? 2 : 1;
        const int32_t top = (static_cast<int32_t>(display.height()) - lines * lineHeight + 3) / 2;
        drawCenteredLine(display, top, first, Colors::White);
        if (second) drawCenteredLine(display, top + lineHeight, second, DetailColor);
    }
}

WeatherApp::WeatherApp() : _task("weather", TaskStackSize, [this] { refresh(); }) {
    _task.setInterval(minutesToMs(_settings.refreshMinutes));
}

void WeatherApp::describeConfig(ConfigSchema& schema) const {
    schema.text("city", "City", MaxCityLength);
    schema.text("country", "Country code (optional, e.g. FR)", 2);
    schema.select("units", "Units", {{"celsius", "Celsius (°C, km/h)"}, {"fahrenheit", "Fahrenheit (°F, mph)"}});
    schema.number("refreshMinutes", "Refresh every (minutes)", 5, 180);
}

void WeatherApp::getConfig(JsonObject config) const {
    std::lock_guard lock(_mutex);
    config["city"] = _settings.city;
    config["country"] = _settings.country;
    config["units"] = _settings.fahrenheit ? "fahrenheit" : "celsius";
    config["refreshMinutes"] = _settings.refreshMinutes;
}

void WeatherApp::setConfig(const JsonObjectConst config) {
    std::lock_guard lock(_mutex);

    Settings next = _settings;
    if (const char* city = config["city"]) next.city = trimmed(city).substr(0, MaxCityLength);
    if (const char* country = config["country"]) next.country = countryCode(country);
    if (const char* units = config["units"]) next.fahrenheit = strcmp(units, "fahrenheit") == 0;
    next.refreshMinutes = readNumber(config["refreshMinutes"], next.refreshMinutes, 5, 180);

    const bool dataOutdated = next.city != _settings.city || next.country != _settings.country ||
                              next.fahrenheit != _settings.fahrenheit;
    _settings = next;
    _task.setInterval(minutesToMs(_settings.refreshMinutes));

    if (dataOutdated) {
        ++_generation;
        _status = _settings.city.empty() ? Status::NoCity : Status::Loading;
        _task.runNow();
    }
}

void WeatherApp::activate() {
    _task.setEnabled(true);
}

void WeatherApp::deactivate() {
    _task.setEnabled(false);
}

void WeatherApp::refresh() {
    Settings settings;
    uint32_t generation;
    {
        std::lock_guard lock(_mutex);
        settings = _settings;
        generation = _generation;
    }

    // Results are dropped if the settings changed while the request was running
    const auto fail = [&](const Status status) {
        std::lock_guard lock(_mutex);
        if (generation != _generation) return;
        // A failed refresh keeps showing the last weather rather than an error
        if (status == Status::CityNotFound || _status != Status::Ready) _status = status;
    };

    if (settings.city.empty()) return;

    if (!_hasLocation || settings.city != _locationCity || settings.country != _locationCountry) {
        Location location;
        switch (findCity(settings, location)) {
            case Lookup::Found:
                break;
            case Lookup::NotFound:
                fail(Status::CityNotFound);
                return;
            case Lookup::Failed:
                fail(Status::Unavailable);
                _task.setInterval(RetryIntervalMs);
                return;
        }
        _location = location;
        _locationCity = settings.city;
        _locationCountry = settings.country;
        _hasLocation = true;
    }

    Weather weather;
    if (!fetchWeather(settings, _location, weather)) {
        fail(Status::Unavailable);
        _task.setInterval(RetryIntervalMs);
        return;
    }

    {
        std::lock_guard lock(_mutex);
        if (generation == _generation) {
            _weather = weather;
            _place = _location.name;
            _status = Status::Ready;
        }
    }
    _task.setInterval(minutesToMs(settings.refreshMinutes));
}

WeatherApp::Lookup WeatherApp::findCity(const Settings& settings, Location& location) const {
    String url = "https://geocoding-api.open-meteo.com/v1/search?count=1&language=en&format=json&name=";
    url += urlEncode(settings.city.c_str());
    if (!settings.country.empty()) {
        url += "&countryCode=";
        url += settings.country.c_str();
    }

    JsonDocument filter;
    filter["results"][0]["name"] = true;
    filter["results"][0]["latitude"] = true;
    filter["results"][0]["longitude"] = true;

    JsonDocument doc;
    if (!fetchJson(url, doc, &filter)) return Lookup::Failed;

    // No "results" at all means nothing matched
    const JsonObjectConst result = doc["results"][0];
    if (result.isNull()) return Lookup::NotFound;

    location.name = result["name"] | settings.city.c_str();
    location.latitude = result["latitude"] | 0.0f;
    location.longitude = result["longitude"] | 0.0f;
    return Lookup::Found;
}

bool WeatherApp::fetchWeather(const Settings& settings, const Location& location, Weather& weather) const {
    char url[384];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
             "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,is_day"
             "&daily=temperature_2m_max,temperature_2m_min&forecast_days=1&timezone=auto%s",
             location.latitude, location.longitude,
             settings.fahrenheit ? "&temperature_unit=fahrenheit&wind_speed_unit=mph" : "");

    JsonDocument doc;
    if (!fetchJson(url, doc)) return false;

    const JsonObjectConst current = doc["current"];
    if (current.isNull()) return false;

    weather.temperature = current["temperature_2m"] | 0.0f;
    weather.humidity = current["relative_humidity_2m"] | 0;
    weather.code = current["weather_code"] | -1;
    weather.wind = current["wind_speed_10m"] | 0.0f;
    weather.isDay = (current["is_day"] | 1) == 1;
    weather.high = doc["daily"]["temperature_2m_max"][0] | weather.temperature;
    weather.low = doc["daily"]["temperature_2m_min"][0] | weather.temperature;
    return true;
}

void WeatherApp::render(IDisplay& display, uint32_t) {
    std::lock_guard lock(_mutex);

    switch (_status) {
        case Status::NoCity:
            drawMessage(display, "Set a city", "in the web UI");
            break;
        case Status::Loading:
            drawMessage(display, "Loading weather...", nullptr);
            break;
        case Status::CityNotFound:
            drawMessage(display, "City not found", _settings.city.c_str());
            break;
        case Status::Unavailable:
            drawMessage(display, "Weather", "unavailable");
            break;
        case Status::Ready:
            renderReady(display);
            break;
    }
}

void WeatherApp::renderReady(IDisplay& display) const {
    const auto width = static_cast<int32_t>(display.width());

    // Top: place name
    drawCenteredLine(display, 1, _place.c_str(), PlaceColor);

    // Middle: icon, current temperature in large digits, today's high and low on the right
    constexpr int32_t RowTop = 12;
    drawWeatherIcon(display, 4, RowTop, _weather.code, _weather.isDay);
    drawTemperature(display, 34, RowTop + 2, _weather.temperature, Colors::White, 3);

    const int32_t highWidth = drawTemperature(display, 0, 0, _weather.high, HighColor, 1, false);
    const int32_t lowWidth = drawTemperature(display, 0, 0, _weather.low, LowColor, 1, false);
    drawTemperature(display, width - 2 - highWidth, RowTop + 3, _weather.high, HighColor, 1);
    drawTemperature(display, width - 2 - lowWidth, RowTop + 14, _weather.low, LowColor, 1);

    // Bottom: conditions, then humidity and wind
    drawCenteredLine(display, 41, describeWeather(_weather.code), Colors::White);

    char details[32];
    snprintf(details, sizeof(details), "%d%%  %ld %s", _weather.humidity, lroundf(_weather.wind),
             _settings.fahrenheit ? "mph" : "km/h");
    drawCenteredLine(display, 53, details, DetailColor);
}
