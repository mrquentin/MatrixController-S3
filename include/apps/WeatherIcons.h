#pragma once

#ifndef _WEATHERICONS_h
#define _WEATHERICONS_h

#include <cstdint>
#include "display/IDisplay.h"

/// Size of the square drawn by drawWeatherIcon.
constexpr int32_t WeatherIconSize = 24;

/// Draws the icon for a WMO weather code (as returned by Open-Meteo) with its top-left at (x, y).
/// isDay picks the sun or the moon for clear and partly cloudy skies.
void drawWeatherIcon(IDisplay& display, int32_t x, int32_t y, int weatherCode, bool isDay);

/// Short description of a WMO weather code, at most 21 characters (one 128 px line).
const char* describeWeather(int weatherCode);

#endif
