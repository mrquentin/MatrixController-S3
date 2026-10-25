#pragma once

#ifndef _SHAPES_h
#define _SHAPES_h

#include <cstdint>
#include "display/IDisplay.h"

/// Basic shapes on top of IDisplay. Like everything drawn through IDisplay, they are clipped
/// to the display, so coordinates may be partly or fully off screen.

/// Line from (x0, y0) to (x1, y1), both ends included.
void drawLine(IDisplay& display, int32_t x0, int32_t y0, int32_t x1, int32_t y1, Color color);

/// Circle outline centered on (cx, cy).
void drawCircle(IDisplay& display, int32_t cx, int32_t cy, int32_t radius, Color color);

/// Filled circle centered on (cx, cy).
void fillCircle(IDisplay& display, int32_t cx, int32_t cy, int32_t radius, Color color);

#endif
