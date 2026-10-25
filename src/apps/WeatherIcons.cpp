#include "apps/WeatherIcons.h"

#include <initializer_list>
#include "display/Shapes.h"

namespace {
    constexpr Color Sun{255, 190, 0};
    constexpr Color Moon{230, 230, 170};
    constexpr Color LightCloud{200, 200, 205};
    constexpr Color DarkCloud{110, 110, 125};
    constexpr Color RainDrop{60, 130, 255};
    constexpr Color Snow{255, 255, 255};
    constexpr Color Bolt{255, 230, 0};
    constexpr Color Fog{150, 150, 150};

    // WMO weather interpretation codes, grouped by how they are drawn
    enum class Sky { Clear, PartlyCloudy, Overcast, Fog, Drizzle, Rain, Snow, Storm, Unknown };

    Sky skyFor(const int code) {
        if (code == 0) return Sky::Clear;
        if (code == 1 || code == 2) return Sky::PartlyCloudy;
        if (code == 3) return Sky::Overcast;
        if (code == 45 || code == 48) return Sky::Fog;
        if (code >= 51 && code <= 57) return Sky::Drizzle;
        if ((code >= 61 && code <= 67) || (code >= 80 && code <= 82)) return Sky::Rain;
        if ((code >= 71 && code <= 77) || code == 85 || code == 86) return Sky::Snow;
        if (code >= 95 && code <= 99) return Sky::Storm;
        return Sky::Unknown;
    }

    void sun(IDisplay& d, const int32_t cx, const int32_t cy, const int32_t radius) {
        fillCircle(d, cx, cy, radius, Sun);
        const int32_t inner = radius + 2;
        const int32_t outer = radius + 4;
        const int32_t innerDiagonal = (inner * 7) / 10;
        const int32_t outerDiagonal = (outer * 7) / 10;
        drawLine(d, cx, cy - inner, cx, cy - outer, Sun);
        drawLine(d, cx, cy + inner, cx, cy + outer, Sun);
        drawLine(d, cx - inner, cy, cx - outer, cy, Sun);
        drawLine(d, cx + inner, cy, cx + outer, cy, Sun);
        drawLine(d, cx - innerDiagonal, cy - innerDiagonal, cx - outerDiagonal, cy - outerDiagonal, Sun);
        drawLine(d, cx + innerDiagonal, cy - innerDiagonal, cx + outerDiagonal, cy - outerDiagonal, Sun);
        drawLine(d, cx - innerDiagonal, cy + innerDiagonal, cx - outerDiagonal, cy + outerDiagonal, Sun);
        drawLine(d, cx + innerDiagonal, cy + innerDiagonal, cx + outerDiagonal, cy + outerDiagonal, Sun);
    }

    void moon(IDisplay& d, const int32_t cx, const int32_t cy, const int32_t radius) {
        fillCircle(d, cx, cy, radius, Moon);
        // Cut out a crescent; the background is black because the frame starts cleared
        fillCircle(d, cx + radius / 2 + 1, cy - radius / 2, radius - 1, Colors::Black);
    }

    // Cloud about 20 x 13 px with its top-left near (x, y)
    void cloud(IDisplay& d, const int32_t x, const int32_t y, const Color color) {
        fillCircle(d, x + 6, y + 8, 4, color);
        fillCircle(d, x + 11, y + 5, 5, color);
        fillCircle(d, x + 16, y + 8, 4, color);
        d.fillRect(static_cast<int16_t>(x + 2), static_cast<int16_t>(y + 8), 19, 5, color);
    }

    void celestial(IDisplay& d, const int32_t cx, const int32_t cy, const int32_t radius, const bool isDay) {
        if (isDay) {
            sun(d, cx, cy, radius);
        } else {
            moon(d, cx, cy, radius + 2);
        }
    }
}

void drawWeatherIcon(IDisplay& display, const int32_t x, const int32_t y, const int weatherCode, const bool isDay) {
    switch (skyFor(weatherCode)) {
        case Sky::Clear:
            celestial(display, x + 12, y + 12, 5, isDay);
            break;
        case Sky::PartlyCloudy:
            celestial(display, x + 15, y + 8, 3, isDay);
            cloud(display, x, y + 8, LightCloud);
            break;
        case Sky::Overcast:
            cloud(display, x + 3, y + 2, DarkCloud);
            cloud(display, x, y + 9, LightCloud);
            break;
        case Sky::Fog:
            for (int32_t row = 0; row < 4; ++row) {
                const int32_t indent = (row % 2) * 3;
                display.fillRect(static_cast<int16_t>(x + 2 + indent), static_cast<int16_t>(y + 6 + row * 4),
                                 static_cast<int16_t>(17), 2, Fog);
            }
            break;
        case Sky::Drizzle:
            cloud(display, x, y + 2, LightCloud);
            for (const int32_t dropX : {6, 11, 16}) {
                display.setPixel(static_cast<int16_t>(x + dropX), static_cast<int16_t>(y + 18), RainDrop);
                display.setPixel(static_cast<int16_t>(x + dropX - 2), static_cast<int16_t>(y + 21), RainDrop);
            }
            break;
        case Sky::Rain:
            cloud(display, x, y + 2, LightCloud);
            for (const int32_t dropX : {6, 11, 16}) {
                drawLine(display, x + dropX, y + 17, x + dropX - 2, y + 22, RainDrop);
            }
            break;
        case Sky::Snow:
            cloud(display, x, y + 2, LightCloud);
            for (const int32_t flakeX : {5, 11, 17}) {
                display.setPixel(static_cast<int16_t>(x + flakeX), static_cast<int16_t>(y + 18), Snow);
                display.setPixel(static_cast<int16_t>(x + flakeX + 2), static_cast<int16_t>(y + 21), Snow);
            }
            break;
        case Sky::Storm:
            cloud(display, x, y + 2, DarkCloud);
            drawLine(display, x + 13, y + 15, x + 10, y + 19, Bolt);
            drawLine(display, x + 10, y + 19, x + 14, y + 19, Bolt);
            drawLine(display, x + 14, y + 19, x + 11, y + 23, Bolt);
            break;
        case Sky::Unknown:
            cloud(display, x, y + 6, DarkCloud);
            break;
    }
}

const char* describeWeather(const int code) {
    switch (code) {
        case 0: return "Clear";
        case 1: return "Mostly clear";
        case 2: return "Partly cloudy";
        case 3: return "Overcast";
        case 45: return "Fog";
        case 48: return "Freezing fog";
        case 51: return "Light drizzle";
        case 53: return "Drizzle";
        case 55: return "Heavy drizzle";
        case 56: case 57: return "Freezing drizzle";
        case 61: return "Light rain";
        case 63: return "Rain";
        case 65: return "Heavy rain";
        case 66: case 67: return "Freezing rain";
        case 71: return "Light snow";
        case 73: return "Snow";
        case 75: return "Heavy snow";
        case 77: return "Snow grains";
        case 80: return "Light showers";
        case 81: return "Showers";
        case 82: return "Heavy showers";
        case 85: case 86: return "Snow showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Storm with hail";
        default: return "Unknown";
    }
}
