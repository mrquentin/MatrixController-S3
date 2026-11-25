#pragma once

#ifndef _WIFISTORE_h
#define _WIFISTORE_h

#include <string>

struct WifiCredentials {
    std::string ssid;
    std::string password;  // empty for open networks
};

/// WiFi settings, kept in NVS so they survive firmware and file system updates.
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
    /// What isValid() asks for, to tell whoever typed the credentials
    constexpr const char* ValidityRule =
        "The network name must be 1 to 32 characters, and the password empty or 8 to 63 characters";
}

#endif
