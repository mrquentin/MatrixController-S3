#pragma once

#ifndef _HUB75DISPLAY_h
#define _HUB75DISPLAY_h

#include <memory>
#include "display/IDisplay.h"
#include "display/DisplayConfig.h"

class MatrixPanel_I2S_DMA;

/// IDisplay backed by a HUB75 panel on the Adafruit MatrixPortal S3,
/// driven by the ESP32-HUB75-MatrixPanel-DMA library.
/// Every drawing call is a safe no-op until begin() succeeds.
class Hub75Display final : public IDisplay {
public:
    Hub75Display();
    ~Hub75Display() override;

    Hub75Display(const Hub75Display&) = delete;
    Hub75Display& operator=(const Hub75Display&) = delete;

    /// Allocates the DMA buffers and starts refreshing the panel. Returns false if allocation fails.
    /// Call only once: the driver doesn't release its DMA channel when a panel is destroyed, so a
    /// second panel can't connect while the first one's channel keeps reading freed memory.
    bool begin(const DisplayConfig& config);
    [[nodiscard]] bool isReady() const;

    [[nodiscard]] uint16_t width() const override;
    [[nodiscard]] uint16_t height() const override;

    void setPixel(int16_t x, int16_t y, Color color) override;
    void fill(Color color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) override;

    void setBrightness(uint8_t level) override;
    [[nodiscard]] uint8_t brightness() const override;

    void present() override;

private:
    std::unique_ptr<MatrixPanel_I2S_DMA> _panel;
    DisplayConfig _config;
};

#endif
