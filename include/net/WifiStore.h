#pragma once

#ifndef _WIFISTORE_h
#define _WIFISTORE_h

#include <string>

struct WifiCredentials {
    std::string ssid;
    std::string password;  // empty for open networks
};

/// WiFi settings kept in NVS (flash key-value storage). Unlike config.json on LittleFS, NVS
/// survives "pio run -t uploadfs" and firmware updates.
namespace WifiStore {
    /// The network to join; ssid is empty until one has been set up.
    WifiCredentials load();
    void save(const WifiCredentials& credentials);
    /// Forgets the network, so the next boot starts in setup mode. The setup password is kept.
    void clear();

    /// Name of the setup network the board creates when it can't join one: "Matrix-" plus three
    /// hex digits from the MAC address, short enough to show on the panel.
    std::string setupSsid();
    /// Password of the setup network. Generated once per board and kept, so it doesn't change
    /// between attempts.
    std::string setupPassword();

    /// SSIDs are 1-32 bytes; WPA2 passwords are empty (open network) or 8-63 characters.
    bool isValid(const WifiCredentials& credentials);
}

#endif
