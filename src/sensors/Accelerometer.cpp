#include <Arduino.h>
#include <Wire.h>
#include "sensors/Accelerometer.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

namespace {
    // The MatrixPortal S3 tested here answers at 0x29 (WHO_AM_I 0x33 and the LIS3DH's registers), not at
    // the datasheet's 0x19 or 0x18. Those are still tried after it, for other board revisions
    constexpr uint8_t Addresses[] = {0x29, 0x19, 0x18};

    constexpr uint8_t RegWhoAmI = 0x0F;
    constexpr uint8_t WhoAmIValue = 0x33;
    constexpr uint8_t RegCtrl1 = 0x20;
    constexpr uint8_t RegCtrl4 = 0x23;
    constexpr uint8_t RegStatus = 0x27;
    // In RegStatus: a new X, Y and Z reading is ready
    constexpr uint8_t XyzReady = 0x08;
    constexpr uint8_t RegOutXLow = 0x28;
    // Just started, the chip reads zero until its first measurement: a few readings at 25 Hz
    constexpr uint32_t FirstReadingTimeoutMs = 200;
    // Set on a register address to read several registers in one go
    constexpr uint8_t AutoIncrement = 0x80;

    // 25 Hz, X, Y and Z on
    constexpr uint8_t Ctrl1Value = 0x37;
    // Block data update (low and high bytes from the same reading), ±2 g, high resolution
    constexpr uint8_t Ctrl4Value = 0x88;
    // Readings are left-aligned 16-bit values, so at ±2 g one g is 2^14
    constexpr float CountsPerG = 16384.0f;

    bool writeRegister(const uint8_t address, const uint8_t reg, const uint8_t value) {
        Wire.beginTransmission(address);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    }

    bool readRegisters(const uint8_t address, const uint8_t reg, uint8_t* out, const size_t count) {
        Wire.beginTransmission(address);
        Wire.write(count > 1 ? reg | AutoIncrement : reg);
        if (Wire.endTransmission(false) != 0) return false;
        if (Wire.requestFrom(address, count) != count) return false;
        for (size_t i = 0; i < count; ++i) out[i] = Wire.read();
        return true;
    }
}

bool Accelerometer::begin() {
    // The board's default pins are its I2C bus (SDA 16, SCL 17)
    Wire.begin();
    Wire.setClock(400000);

    for (const uint8_t address : Addresses) {
        uint8_t id = 0;
        if (!readRegisters(address, RegWhoAmI, &id, 1)) continue;
        if (id != WhoAmIValue) {
            DEBUG_PRINTF("I2C device at 0x%02X is not a LIS3DH (id 0x%02X)\n", address, id);
            continue;
        }
        if (!writeRegister(address, RegCtrl1, Ctrl1Value) || !writeRegister(address, RegCtrl4, Ctrl4Value)) {
            DEBUG_PRINTF("LIS3DH at 0x%02X did not accept its settings\n", address);
            continue;
        }
        _address = address;

        // So the first read() already tells which way up the board is
        const uint32_t start = millis();
        uint8_t status = 0;
        while (millis() - start < FirstReadingTimeoutMs) {
            if (readRegisters(address, RegStatus, &status, 1) && (status & XyzReady)) break;
            delay(5);
        }
        DEBUG_PRINTF("LIS3DH accelerometer found at 0x%02X\n", address);
        return true;
    }

    // Lists what is on the bus instead, to tell a missing chip from a different one or a dead bus
    DEBUG_PRINT.print("No LIS3DH accelerometer found. I2C devices:");
    bool any = false;
    for (uint8_t address = 1; address < 0x7F; ++address) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() != 0) continue;
        DEBUG_PRINTF(" 0x%02X", address);
        any = true;
    }
    DEBUG_PRINT.println(any ? "" : " none");
    return false;
}

bool Accelerometer::read(Acceleration& out) const {
    if (!isPresent()) return false;
    uint8_t raw[6];
    if (!readRegisters(_address, RegOutXLow, raw, sizeof(raw))) return false;

    const auto axis = [&](const int i) {
        return static_cast<float>(static_cast<int16_t>(raw[i] | raw[i + 1] << 8)) / CountsPerG;
    };
    out = {axis(0), axis(2), axis(4)};
    return true;
}
