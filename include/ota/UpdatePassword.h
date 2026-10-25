#pragma once

#ifndef _UPDATEPASSWORD_h
#define _UPDATEPASSWORD_h

#include <cstdint>
#include <string>

/// Password that firmware updates need, kept in NVS like the WiFi settings, so it survives
/// updates. Only its MD5 is stored: the form ArduinoOTA (PlatformIO's espota uploads) checks.
/// Until one is set, updates are refused.
namespace UpdatePassword {
    enum class Result : uint8_t {
        Ok,
        NotSet,
        Wrong,
        /// Too many wrong guesses in a row; refused for a while, even if right
        LockedOut,
    };

    [[nodiscard]] bool isSet();
    /// MD5 of the password as lowercase hex, empty when none is set.
    [[nodiscard]] std::string hash();
    Result check(const std::string& password);
    /// Stores a new password. Returns false if it isn't valid.
    bool set(const std::string& password);
    /// 8 to 64 characters.
    [[nodiscard]] bool isValid(const std::string& password);
}

#endif
