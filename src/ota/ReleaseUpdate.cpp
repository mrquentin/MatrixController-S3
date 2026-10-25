#include "ota/ReleaseUpdate.h"

#include <algorithm>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include "ota/Version.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_PRINT if(DEBUG)Serial
#define DEBUG_PRINTF if(DEBUG)printf

#ifndef UPDATE_GITHUB_REPO
#define UPDATE_GITHUB_REPO ""
#endif

// Mozilla's root certificates, built into the SDK's mbedTLS. Unlike the apps' data requests,
// firmware must come from GitHub and nobody in between, so the server certificate is checked
extern const uint8_t rootCertificates[] asm("_binary_x509_crt_bundle_start");

namespace {
    constexpr const char* AssetName = "firmware.bin";
    constexpr uint16_t TimeoutMs = 15000;
    // TLS needs about 8 KB of stack on top of the download buffer
    constexpr uint32_t StackSize = 12288;

    // Automatic checks. The first waits for start-up to settle (the apps fetch their data then too)
    constexpr uint32_t FirstCheckAfterMs = 60UL * 1000;
    constexpr uint32_t CheckIntervalMs = 12UL * 60 * 60 * 1000;
    constexpr uint32_t RetryIntervalMs = 60UL * 60 * 1000;

    void prepare(HTTPClient& http) {
        http.setTimeout(TimeoutMs);
        http.setConnectTimeout(TimeoutMs);
        // No chunked responses: the body can be read straight from the stream
        http.useHTTP10(true);
        // GitHub's API refuses requests without one
        http.setUserAgent(String("MatrixController/") + FirmwareUpdate::version());
        // Asset downloads redirect to a storage host
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    }
}

ReleaseUpdate::ReleaseUpdate(FirmwareUpdate& update) : _update(update) {}

bool ReleaseUpdate::available() {
    return UPDATE_GITHUB_REPO[0] != '\0';
}

bool ReleaseUpdate::check() {
    return start(Job::Check);
}

bool ReleaseUpdate::install() {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_checked) return false;
    }
    return start(Job::Install);
}

bool ReleaseUpdate::start(const Job job) {
    if (!available() || _busy.exchange(true)) return false;
    _job = job;
    if (job == Job::Check) _lastCheckMs = millis() | 1;
    setError("");
    if (xTaskCreate(entry, "release", StackSize, this, 1, nullptr) != pdPASS) {
        setError("Not enough memory");
        _busy = false;
        return false;
    }
    return true;
}

void ReleaseUpdate::entry(void* self) {
    auto* releases = static_cast<ReleaseUpdate*>(self);
    if (releases->_job == Job::Check) {
        releases->_lastCheckFailed = !releases->runCheck();
    } else {
        Release release;
        {
            std::lock_guard<std::mutex> lock(releases->_mutex);
            release = releases->_latest;
        }
        releases->runInstall(release);
    }
    releases->_busy = false;
    vTaskDelete(nullptr);
}

bool ReleaseUpdate::runCheck() {
    WiFiClientSecure client;
    client.setCACertBundle(rootCertificates);
    HTTPClient http;
    prepare(http);

    const String url = String("https://api.github.com/repos/") + UPDATE_GITHUB_REPO + "/releases/latest";
    if (!http.begin(client, url)) {
        setError("Invalid repository name");
        return false;
    }
    http.addHeader("Accept", "application/vnd.github+json");

    const int status = http.GET();
    if (status != HTTP_CODE_OK) {
        DEBUG_PRINTF("Releases: %s answered %d\n", url.c_str(), status);
        // Also what private repositories answer without a token
        if (status == HTTP_CODE_NOT_FOUND) setError("No release published yet");
        else if (status < 0) setError(http.errorToString(status).c_str());
        else setError((String("GitHub answered ") + status).c_str());
        http.end();
        return false;
    }

    // Release notes and the like can be long; only these fields are kept
    JsonDocument filter;
    filter["tag_name"] = true;
    filter["published_at"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["size"] = true;
    filter["assets"][0]["browser_download_url"] = true;

    JsonDocument doc;
    const DeserializationError error = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
    http.end();
    if (error) {
        setError("Unexpected answer from GitHub");
        return false;
    }

    Release release;
    release.tag = doc["tag_name"] | "";
    release.publishedAt = doc["published_at"] | "";
    for (const JsonObjectConst asset : doc["assets"].as<JsonArrayConst>()) {
        if (strcmp(asset["name"] | "", AssetName) != 0) continue;
        release.url = asset["browser_download_url"] | "";
        release.size = asset["size"] | 0;
    }
    if (release.url.empty()) {
        setError("The latest release has no firmware.bin");
        return false;
    }

    DEBUG_PRINTF("Releases: latest is %s, %u bytes\n", release.tag.c_str(), static_cast<unsigned>(release.size));
    std::lock_guard<std::mutex> lock(_mutex);
    _latest = release;
    _checkedAtMs = millis();
    _checked = true;
    return true;
}

void ReleaseUpdate::checkWhenDue(const uint32_t now) {
    if (!available() || _busy || _update.state() == FirmwareUpdate::State::Running) return;
    const uint32_t last = _lastCheckMs;
    const bool due = last == 0
        ? now >= FirstCheckAfterMs
        : now - last >= (_lastCheckFailed ? RetryIntervalMs : CheckIntervalMs);
    if (due) check();
}

void ReleaseUpdate::runInstall(const Release& release) {
    WiFiClientSecure client;
    client.setCACertBundle(rootCertificates);
    HTTPClient http;
    prepare(http);

    if (!http.begin(client, release.url.c_str())) {
        setError("Invalid download address");
        return;
    }
    const int status = http.GET();
    if (status != HTTP_CODE_OK) {
        DEBUG_PRINTF("Releases: download answered %d\n", status);
        setError(status < 0 ? http.errorToString(status).c_str() : (String("Download answered ") + status).c_str());
        http.end();
        return;
    }

    const int length = http.getSize();
    const size_t size = length > 0 ? static_cast<size_t>(length) : release.size;
    const uint32_t attempt = _update.begin(UpdateSource::Release, size);
    if (attempt == 0) setError("Another update is running");
    // Also when the image can't fit, which the update's own state tells
    if (!_update.isRunning(attempt)) {
        http.end();
        return;
    }

    WiFiClient* stream = http.getStreamPtr();
    uint8_t buffer[1024];
    size_t remaining = size;
    uint32_t lastDataMs = millis();
    while (remaining > 0) {
        const size_t waiting = stream->available();
        if (waiting == 0) {
            if (!stream->connected() || millis() - lastDataMs > TimeoutMs) {
                _update.cancel(attempt, "Download interrupted");
                http.end();
                return;
            }
            delay(5);
            continue;
        }
        const int read = stream->read(buffer, std::min({waiting, sizeof(buffer), remaining}));
        if (read <= 0) continue;
        lastDataMs = millis();
        // On errors the update records why
        if (!_update.write(attempt, buffer, read)) {
            http.end();
            return;
        }
        remaining -= read;
    }
    http.end();
    _update.finish(attempt);
}

void ReleaseUpdate::setError(const char* error) {
    std::lock_guard<std::mutex> lock(_mutex);
    _error = error;
}

void ReleaseUpdate::describe(const JsonObject out) {
    out["repo"] = UPDATE_GITHUB_REPO;
    out["busy"] = _busy.load();
    out["job"] = _job == Job::Check ? "check" : "install";

    std::lock_guard<std::mutex> lock(_mutex);
    if (_checked) {
        out["latest"] = _latest.tag;
        out["publishedAt"] = _latest.publishedAt;
        out["size"] = _latest.size;
        out["updateAvailable"] = Version::isNewer(_latest.tag.c_str(), FirmwareUpdate::version());
        out["checkedSecondsAgo"] = (millis() - _checkedAtMs) / 1000;
    }
    if (!_error.empty()) out["error"] = _error;
}
