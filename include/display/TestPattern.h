#pragma once

#ifndef _TESTPATTERN_h
#define _TESTPATTERN_h

#include "display/IDisplay.h"

/// Draws a frame to check wiring: white border (full area), R/G/B/white bars
/// (color order) and a gray gradient (24-bit depth). Call present() afterwards.
void drawTestPattern(IDisplay& display);

#endif
