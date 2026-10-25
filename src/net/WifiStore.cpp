#include "net/WifiStore.h"

#include <Preferences.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <cstdio>

namespace {
    constexpr const char* Namespace = "wifi";
    constexpr size_t SetupPasswordLength = 8;
    // No 0/o, 1/l/i: the password is read off the panel and typed on a phone
    constexpr char PasswordAlphabet[] = "abcdefghjkmnpqrstuvwxyz23456789";
}

namespace WifiStore {
    WifiCredentials load() {
        Preferences prefs;
        WifiCredentials credentials;
        // Opened read-write so a missing namespace is created instead of logged as an error;
        // isKey() avoids the library's error log for keys that were never saved
        prefs.begin(Namespace, false);
        if (prefs.isKey("ssid")) credentials.ssid = prefs.getString("ssid").c_str();
        if (prefs.isKey("password")) credentials.password = prefs.getString("password").c_str();
        prefs.end();
        return credentials;
    }

    void save(const WifiCredentials& credentials) {
        Preferences prefs;
        prefs.begin(Namespace, false);
        prefs.putString("ssid", credentials.ssid.c_str());
        prefs.putString("password", credentials.password.c_str());
        prefs.end();
    }

    void clear() {
        Preferences prefs;
        prefs.begin(Namespace, false);
        prefs.remove("ssid");
        prefs.remove("password");
        prefs.end();
    }

    std::string setupSsid() {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
        char ssid[16];
        snprintf(ssid, sizeof(ssid), "Matrix-%03X", ((mac[4] << 8) | mac[5]) & 0xFFF);
        return ssid;
    }

    std::string setupPassword() {
        Preferences prefs;
        prefs.begin(Namespace, false);
        std::string password = prefs.isKey("setupPassword") ? prefs.getString("setupPassword").c_str() : "";
        if (password.size() != SetupPasswordLength) {
            password.clear();
            for (size_t i = 0; i < SetupPasswordLength; ++i) {
                password += PasswordAlphabet[esp_random() % (sizeof(PasswordAlphabet) - 1)];
            }
            prefs.putString("setupPassword", password.c_str());
        }
        prefs.end();
        return password;
    }

    bool isValid(const WifiCredentials& credentials) {
        const size_t ssidLength = credentials.ssid.size();
        const size_t passwordLength = credentials.password.size();
        return ssidLength >= 1 && ssidLength <= 32 && (passwordLength == 0 || (passwordLength >= 8 && passwordLength <= 63));
    }
}
