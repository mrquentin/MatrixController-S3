#pragma once

#ifndef _ACCELEROMETER_h
#define _ACCELEROMETER_h

#include <cstdint>

/// Acceleration in g along the sensor's axes. At rest, the axis pointing up reads +1.
struct Acceleration {
    float x{};
    float y{};
    float z{};
};

/// The LIS3DH accelerometer on the MatrixPortal S3, on its I2C bus (SDA 16, SCL 17, shared with the
/// STEMMA QT connector). Just enough of it to tell which way up the board is: ±2 g, 25 readings a
/// second, no interrupts.
class Accelerometer {
public:
    /// Looks for the chip and starts it. Returns false when there is none, every read then fails.
    bool begin();
    [[nodiscard]] bool isPresent() const { return _address != 0; }
    /// I2C address the chip answered on, 0 when not found
    [[nodiscard]] uint8_t address() const { return _address; }

    /// Latest reading. Returns false if the chip doesn't answer.
    bool read(Acceleration& out) const;

private:
    uint8_t _address = 0;
};

#endif
