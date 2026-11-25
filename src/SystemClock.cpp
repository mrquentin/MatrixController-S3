#include "SystemClock.h"

#include <Arduino.h>
#include <sys/time.h>
#include <ctime>

namespace {
    constexpr time_t FirstValidSecond = 1704067200;  // 2024-01-01

    timeval now() {
        timeval tv{};
        gettimeofday(&tv, nullptr);
        return tv;
    }
}

namespace SystemClock {
    void begin(const std::string& timezone) {
        configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com");
    }

    void setTimezone(const std::string& timezone) {
        setenv("TZ", timezone.c_str(), 1);
        tzset();
    }

    bool synced() {
        return now().tv_sec >= FirstValidSecond;
    }

    int64_t epochMs() {
        const timeval tv = now();
        if (tv.tv_sec < FirstValidSecond) return 0;
        return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
    }
}
