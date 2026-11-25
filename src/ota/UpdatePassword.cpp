#include "ota/UpdatePassword.h"

#include <atomic>
#include <Arduino.h>
#include <MD5Builder.h>
#include "Nvs.h"

namespace {
    constexpr const char* Space = "update";
    constexpr const char* HashKey = "passwordMd5";
    // After this many wrong passwords in a row, every check fails for LockoutMs. Slows guessing
    // down to a handful of tries a minute
    constexpr uint8_t MaxFailures = 5;
    constexpr uint32_t LockoutMs = 60000;

    std::atomic<uint8_t> failures{0};
    std::atomic<uint32_t> lockedAtMs{0};

    std::string md5Hex(const std::string& text) {
        MD5Builder md5;
        md5.begin();
        md5.add(text.c_str());
        md5.calculate();
        return md5.toString().c_str();
    }

    // Takes as long whatever the first differing character, so timing doesn't give the hash away
    bool equalConstantTime(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        uint8_t difference = 0;
        for (size_t i = 0; i < a.size(); ++i) difference |= a[i] ^ b[i];
        return difference == 0;
    }
}

namespace UpdatePassword {
    bool isSet() {
        return !hash().empty();
    }

    std::string hash() {
        return Nvs::read(Space, HashKey);
    }

    Result check(const std::string& password) {
        const uint32_t lockedAt = lockedAtMs;
        if (lockedAt != 0) {
            if (millis() - lockedAt < LockoutMs) return Result::LockedOut;
            lockedAtMs = 0;
            failures = 0;
        }

        const std::string stored = hash();
        if (stored.empty()) return Result::NotSet;
        if (equalConstantTime(md5Hex(password), stored)) {
            failures = 0;
            return Result::Ok;
        }

        if (++failures >= MaxFailures) lockedAtMs = millis() | 1;
        return Result::Wrong;
    }

    bool set(const std::string& password) {
        if (!isValid(password)) return false;
        Nvs::write(Space, HashKey, md5Hex(password));
        return true;
    }

    bool isValid(const std::string& password) {
        return password.size() >= MinLength && password.size() <= MaxLength;
    }
}
