#include <Arduino.h>
#include <cmath>
#include "sensors/AutoOrientation.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

namespace {
    // The accelerometer axis that runs along the panel's height, and its sign with the panel upright.
    // Set by how the board sits on the panel's HUB75 connector: if the screen turns the wrong way,
    // check the readings on the status page with the panel upright and change these
    // Checked on the MatrixPortal S3: Y reads -1 g with the panel upright
    constexpr int VerticalAxis = 1;  // 0 = X, 1 = Y, 2 = Z
    constexpr float VerticalSign = -1.0f;

    constexpr uint32_t ReadIntervalMs = 100;

    int16_t toMilliG(const float g) {
        return static_cast<int16_t>(std::lround(g * 1000.0f));
    }
}

void AutoOrientation::begin(const OrientationMode mode) {
    _sensorPresent = _accelerometer.begin();

    // Straight to the orientation the board is in now, without waiting for it to settle
    float vertical = 0;
    if (readVertical(vertical)) {
        _tracker = OrientationTracker(vertical < 0 ? Orientation::UpsideDown : Orientation::Upright);
        const Acceleration reading = lastReading();
        DEBUG_PRINTF("Accelerometer: x %.2f g, y %.2f g, z %.2f g, panel %s\n", reading.x, reading.y, reading.z,
                     vertical < 0 ? "upside down" : "upright");
    }
    setMode(mode);
}

void AutoOrientation::setMode(const OrientationMode mode) {
    _mode = mode;
    switch (mode) {
        case OrientationMode::Upright: _current = Orientation::Upright; break;
        case OrientationMode::UpsideDown: _current = Orientation::UpsideDown; break;
        default: _current = _tracker.current(); break;
    }
}

Orientation AutoOrientation::update(const uint32_t now) {
    if (now - _lastReadMs < ReadIntervalMs) return _current;
    _lastReadMs = now;

    // Read in every mode, so the status page can show the sensor working before Auto is picked
    float vertical = 0;
    if (!readVertical(vertical)) return _current;
    const Orientation tracked = _tracker.update(vertical, now);
    if (_mode == OrientationMode::Auto && tracked != _current) {
        _current = tracked;
        DEBUG_PRINTF("Panel turned %s\n", tracked == Orientation::UpsideDown ? "upside down" : "upright");
    }
    return _current;
}

Acceleration AutoOrientation::lastReading() const {
    return {_xMilliG / 1000.0f, _yMilliG / 1000.0f, _zMilliG / 1000.0f};
}

bool AutoOrientation::readVertical(float& verticalG) {
    Acceleration reading;
    if (!_accelerometer.read(reading)) return false;
    _xMilliG = toMilliG(reading.x);
    _yMilliG = toMilliG(reading.y);
    _zMilliG = toMilliG(reading.z);

    const float axes[] = {reading.x, reading.y, reading.z};
    verticalG = VerticalSign * axes[VerticalAxis];
    return true;
}
