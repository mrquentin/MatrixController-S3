#pragma once

#ifndef _AUTOORIENTATION_h
#define _AUTOORIENTATION_h

#include <atomic>
#include <cstdint>
#include "display/Orientation.h"
#include "sensors/Accelerometer.h"

/// Decides which way up the panel is shown: from the accelerometer in OrientationMode::Auto,
/// or the fixed choice otherwise. Without an accelerometer, Auto stays upright.
/// Call begin(), setMode() and update() from the loop task only; the getters for the web UI
/// are safe from any task.
class AutoOrientation {
public:
    /// Starts the accelerometer and takes a first reading, so the boot screens already show the
    /// right way up
    void begin(OrientationMode mode);
    void setMode(OrientationMode mode);
    /// Reads the accelerometer now and then. Returns the orientation to draw in.
    Orientation update(uint32_t now);
    [[nodiscard]] Orientation current() const { return _current; }

    [[nodiscard]] bool sensorPresent() const { return _sensorPresent; }
    [[nodiscard]] OrientationMode mode() const { return _mode; }
    /// Last reading in g, all zero before the first one
    [[nodiscard]] Acceleration lastReading() const;

private:
    [[nodiscard]] bool readVertical(float& verticalG);

    Accelerometer _accelerometer;
    OrientationTracker _tracker;
    uint32_t _lastReadMs = 0;

    std::atomic<OrientationMode> _mode{OrientationMode::Auto};
    std::atomic<Orientation> _current{Orientation::Upright};
    std::atomic<bool> _sensorPresent{false};
    // In thousandths of a g, so each axis is read and written whole
    std::atomic<int16_t> _xMilliG{0};
    std::atomic<int16_t> _yMilliG{0};
    std::atomic<int16_t> _zMilliG{0};
};

#endif
