#include "display/QrCode.h"

#include <qrcode.h>

namespace {
    // esp_qrcode_generate reports the result through a plain function pointer with no context,
    // so the code being generated is reached through this; generate() runs on one task at a time
    int* generatingSize = nullptr;
    std::vector<bool>* generatingModules = nullptr;

    void copyModules(const esp_qrcode_handle_t qrcode) {
        const int size = esp_qrcode_get_size(qrcode);
        *generatingSize = size;
        generatingModules->assign(static_cast<size_t>(size * size), false);
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                (*generatingModules)[static_cast<size_t>(y * size + x)] = esp_qrcode_get_module(qrcode, x, y);
            }
        }
    }
}

bool QrCode::generate(const char* text, const int maxVersion) {
    _size = 0;
    _modules.clear();

    esp_qrcode_config_t config = ESP_QRCODE_CONFIG_DEFAULT();
    config.display_func = copyModules;
    config.max_qrcode_version = maxVersion;
    config.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;

    generatingSize = &_size;
    generatingModules = &_modules;
    const esp_err_t result = esp_qrcode_generate(&config, text);
    generatingSize = nullptr;
    generatingModules = nullptr;

    if (result != ESP_OK) {
        _size = 0;
        _modules.clear();
        return false;
    }
    return _size > 0;
}

int32_t QrCode::drawnSize(const uint8_t scale, const uint8_t quietZone) const {
    return (_size + 2 * quietZone) * scale;
}

void QrCode::draw(IDisplay& display, const int32_t x, const int32_t y, const uint8_t scale, const uint8_t quietZone) const {
    if (_size == 0) return;

    const int32_t side = drawnSize(scale, quietZone);
    display.fillRect(static_cast<int16_t>(x), static_cast<int16_t>(y), static_cast<int16_t>(side), static_cast<int16_t>(side),
                     Colors::White);

    const int32_t originX = x + quietZone * scale;
    const int32_t originY = y + quietZone * scale;
    for (int row = 0; row < _size; ++row) {
        for (int col = 0; col < _size; ++col) {
            if (!_modules[static_cast<size_t>(row * _size + col)]) continue;
            display.fillRect(static_cast<int16_t>(originX + col * scale), static_cast<int16_t>(originY + row * scale),
                             scale, scale, Colors::Black);
        }
    }
}
