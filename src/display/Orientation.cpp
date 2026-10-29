#include "display/Orientation.h"

OrientationMode parseOrientationMode(const std::string_view name, const OrientationMode fallback) {
    if (name == "auto") return OrientationMode::Auto;
    if (name == "upright") return OrientationMode::Upright;
    if (name == "upsideDown") return OrientationMode::UpsideDown;
    return fallback;
}

const char* orientationModeName(const OrientationMode mode) {
    switch (mode) {
        case OrientationMode::Upright: return "upright";
        case OrientationMode::UpsideDown: return "upsideDown";
        default: return "auto";
    }
}

Orientation OrientationTracker::update(const float verticalG, const uint32_t now) {
    const bool turned = _current == Orientation::Upright ? verticalG < -FlipThresholdG : verticalG > FlipThresholdG;
    if (!turned) {
        _turned = false;
        return _current;
    }

    if (!_turned) {
        _turned = true;
        _turnedSinceMs = now;
    }
    if (now - _turnedSinceMs >= SettleMs) {
        _current = _current == Orientation::Upright ? Orientation::UpsideDown : Orientation::Upright;
        _turned = false;
    }
    return _current;
}
