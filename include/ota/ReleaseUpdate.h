#pragma once

#ifndef _RELEASEUPDATE_h
#define _RELEASEUPDATE_h

#include <atomic>
#include <mutex>
#include <string>
#include <ArduinoJson.h>
#include "ota/FirmwareUpdate.h"

/// Installs firmware published as a GitHub release: the latest release of UPDATE_GITHUB_REPO
/// ("owner/repo", a build flag in platformio.ini), whose firmware.bin asset is the image. The
/// release workflow in .github/workflows builds and attaches it for every v* tag.
///
/// Checking and installing run on a task of their own (TLS requests take seconds) and report
/// through describe(). Besides checks asked for from the web UI, checkWhenDue() looks for new
/// releases by itself now and then. Without UPDATE_GITHUB_REPO, available() is false and
/// nothing runs.
class ReleaseUpdate {
public:
    explicit ReleaseUpdate(FirmwareUpdate& update);

    [[nodiscard]] static bool available();

    /// Starts looking up the latest release. Returns false if a check or install is underway.
    bool check();
    /// Starts installing the release the last check found. Returns false if there is none, or a
    /// check or install is underway.
    bool install();
    /// Checks for a new release when one is due: a minute after start-up, then twice a day (hourly
    /// after a failed check, e.g. while the internet is down). Call from loop().
    void checkWhenDue(uint32_t now);

    void describe(JsonObject out);

private:
    enum class Job : uint8_t { Check, Install };
    struct Release {
        std::string tag;
        std::string url;
        size_t size = 0;
        std::string publishedAt;
    };

    bool start(Job job);
    static void entry(void* self);
    /// Returns false if the check failed
    bool runCheck();
    void runInstall(const Release& release);
    void setError(const char* error);

    FirmwareUpdate& _update;
    std::atomic<bool> _busy{false};
    Job _job = Job::Check;
    // millis() when the last check started, 0 before the first
    std::atomic<uint32_t> _lastCheckMs{0};
    std::atomic<bool> _lastCheckFailed{false};

    // Shared between the worker task and the web server task
    std::mutex _mutex;
    bool _checked = false;
    // millis() when _latest was found
    uint32_t _checkedAtMs = 0;
    Release _latest;
    std::string _error;
};

#endif
