#pragma once

#ifndef _ORIENTATION_h
#define _ORIENTATION_h

#include <cstdint>
#include <string_view>

/// Which way up the panel is shown. Only landscape: the image is either upright or turned by 180°,
/// so the apps keep drawing on the same width × height.
enum class Orientation : uint8_t { Upright, UpsideDown };

/// Saved choice: follow the accelerometer, or keep one orientation whatever the sensor says
enum class OrientationMode : uint8_t { Auto, Upright, UpsideDown };

/// "auto", "upright" or "upsideDown", as in the config file. Anything else gives fallback.
[[nodiscard]] OrientationMode parseOrientationMode(std::string_view name, OrientationMode fallback = OrientationMode::Auto);
[[nodiscard]] const char* orientationModeName(OrientationMode mode);

/// Turns accelerometer readings into an orientation, without flickering between the two.
///
/// Fed the gravity along the panel's vertical axis, in g: +1 when the panel stands upright, -1 when
/// it's upside down. The orientation only changes once the panel has clearly been turned over
/// (FlipThresholdG) for a while (SettleMs). Lying flat or standing in portrait gives readings near
/// 0, which keep the current orientation.
class OrientationTracker {
public:
    static constexpr float FlipThresholdG = 0.5f;
    static constexpr uint32_t SettleMs = 400;

    explicit OrientationTracker(Orientation initial = Orientation::Upright) : _current(initial) {}

    /// Returns the orientation to show after this reading
    Orientation update(float verticalG, uint32_t now);
    [[nodiscard]] Orientation current() const { return _current; }

private:
    Orientation _current;
    // Whether readings point the other way, and since when (millis())
    bool _turned = false;
    uint32_t _turnedSinceMs = 0;
};

#endif
