#include <Arduino.h>
#include <algorithm>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "display/Hub75Display.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

namespace {
    // Adafruit MatrixPortal S3 HUB75 connector wiring. The board labels its color pins
    // R1/G1/B1 = 42/41/40 and R2/G2/B2 = 38/39/37, but our panel takes its color inputs in a
    // different order (sending red lit blue, green lit red, blue lit green), so the color pins
    // are reassigned here to keep Color values true RGB.
    constexpr HUB75_I2S_CFG::i2s_pins MatrixPortalS3Pins = {
        41, 40, 42,          // R1, G1, B1
        39, 37, 38,          // R2, G2, B2
        45, 36, 48, 35, 21,  // A, B, C, D, E
        47, 14, 2            // LAT, OE, CLK
    };

    // The driver sets every HUB75 pin to maximum drive strength. The resulting sharp edges
    // radiate enough noise to cripple WiFi on the MatrixPortal, and 8 MHz doesn't need them.
    constexpr gpio_drive_cap_t Hub75DriveStrength = GPIO_DRIVE_CAP_0;

    void reduceDriveStrength(const HUB75_I2S_CFG::i2s_pins& pins) {
        const int8_t all[] = {pins.r1, pins.g1, pins.b1, pins.r2, pins.g2, pins.b2,
                              pins.a, pins.b, pins.c, pins.d, pins.e, pins.lat, pins.oe, pins.clk};
        for (const int8_t pin : all) {
            if (pin >= 0) gpio_set_drive_capability(static_cast<gpio_num_t>(pin), Hub75DriveStrength);
        }
    }
}

Hub75Display::Hub75Display() = default;

// Defined here because MatrixPanel_I2S_DMA is only a complete type in this file
Hub75Display::~Hub75Display() = default;

bool Hub75Display::begin(const DisplayConfig& config) {
    _config = config;
    _config.brightness = std::min(config.brightness, MaxBrightness);

    HUB75_I2S_CFG cfg(config.width, config.height, 1, MatrixPortalS3Pins);
    cfg.double_buff = config.doubleBuffer;

    _panel = std::make_unique<MatrixPanel_I2S_DMA>(cfg);
    if (!_panel->begin()) {
        DEBUG_PRINTF("HUB75 init failed for %ux%u (double buffer: %d), not enough DMA memory?\n",
                     config.width, config.height, config.doubleBuffer);
        _panel.reset();
        return false;
    }

    reduceDriveStrength(MatrixPortalS3Pins);
    _panel->setBrightness8(_config.brightness);
    clear();
    present();

    DEBUG_PRINTF("HUB75 ready: %ux%u, brightness %u, double buffer: %d\n",
                 config.width, config.height, _config.brightness, config.doubleBuffer);
    return true;
}

bool Hub75Display::isReady() const {
    return _panel != nullptr;
}

uint16_t Hub75Display::width() const {
    return _config.width;
}

uint16_t Hub75Display::height() const {
    return _config.height;
}

void Hub75Display::setPixel(const int16_t x, const int16_t y, const Color color) {
    if (!_panel || x < 0 || y < 0 || x >= _config.width || y >= _config.height) return;
    _panel->drawPixelRGB888(x, y, color.r, color.g, color.b);
}

void Hub75Display::fill(const Color color) {
    if (!_panel) return;
    _panel->fillScreenRGB888(color.r, color.g, color.b);
}

void Hub75Display::fillRect(const int16_t x, const int16_t y, const int16_t w, const int16_t h, const Color color) {
    if (!_panel) return;

    // Clip here: the driver's fast rect fill doesn't handle empty or off-screen rects
    const int32_t x0 = std::max<int32_t>(x, 0);
    const int32_t y0 = std::max<int32_t>(y, 0);
    const int32_t x1 = std::min<int32_t>(static_cast<int32_t>(x) + w, _config.width);
    const int32_t y1 = std::min<int32_t>(static_cast<int32_t>(y) + h, _config.height);
    if (x0 >= x1 || y0 >= y1) return;

    _panel->fillRect(static_cast<int16_t>(x0), static_cast<int16_t>(y0),
                     static_cast<int16_t>(x1 - x0), static_cast<int16_t>(y1 - y0),
                     color.r, color.g, color.b);
}

void Hub75Display::setBrightness(const uint8_t level) {
    _config.brightness = std::min(level, MaxBrightness);
    if (_panel) _panel->setBrightness8(_config.brightness);
}

uint8_t Hub75Display::brightness() const {
    return _config.brightness;
}

void Hub75Display::present() {
    // flipDMABuffer is ignored by the driver when double buffering is off
    if (_panel) _panel->flipDMABuffer();
}
