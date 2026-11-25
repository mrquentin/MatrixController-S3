#include "net/WifiStore.h"

#include <esp_mac.h>
#include <esp_random.h>
#include <cstdio>
#include "Nvs.h"

namespace {
    constexpr const char* Space = "wifi";
    constexpr size_t SetupPasswordLength = 8;
    // No 0/o, 1/l/i: the password is read off the panel and typed on a phone
    constexpr char PasswordAlphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";
}

namespace WifiStore {
    WifiCredentials load() {
        return {Nvs::read(Space, "ssid"), Nvs::read(Space, "password")};
    }

    void save(const WifiCredentials& credentials) {
        Nvs::write(Space, "ssid", credentials.ssid);
        Nvs::write(Space, "password", credentials.password);
    }

    void clear() {
        Nvs::remove(Space, "ssid");
        Nvs::remove(Space, "password");
    }

    std::string setupSsid() {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        char ssid[16];
        snprintf(ssid, sizeof(ssid), "Matrix-%03X", ((mac[4] << 8) | mac[5]) & 0xFFF);
        return ssid;
    }

    std::string setupPassword() {
        std::string password = Nvs::read(Space, "setupPassword");
        if (password.size() != SetupPasswordLength) {
            password.clear();
            for (size_t i = 0; i < SetupPasswordLength; ++i) {
                password += PasswordAlphabet[esp_random() % (sizeof(PasswordAlphabet) - 1)];
            }
            Nvs::write(Space, "setupPassword", password);
        }
        return password;
    }

    bool isValid(const WifiCredentials& credentials) {
        const size_t ssidLength = credentials.ssid.size();
        const size_t passwordLength = credentials.password.size();
        return ssidLength >= 1 && ssidLength <= 32 && (passwordLength == 0 || (passwordLength >= 8 && passwordLength <= 63));
    }
}
