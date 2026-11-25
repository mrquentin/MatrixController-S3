#include "ota/ReleaseUpdate.h"

#include <algorithm>

#include "net/HttpJson.h"
#include "ota/Version.h"
#include "Log.h"

#ifndef UPDATE_GITHUB_REPO
#define UPDATE_GITHUB_REPO ""
#endif

namespace {
    constexpr const char* AssetName = "firmware.bin";
    constexpr uint16_t TimeoutMs = 15000;
    // TLS needs about 8 KB of stack on top of the download buffer
    constexpr uint32_t StackSize = 12288;

    // Automatic checks. The first waits for start-up to settle (the apps fetch their data then too)
    constexpr uint32_t FirstCheckAfterMs = 60UL * 1000;
    constexpr uint32_t CheckIntervalMs = 12UL * 60 * 60 * 1000;
    constexpr uint32_t RetryIntervalMs = 60UL * 60 * 1000;

    HttpOptions gitHubOptions() {
        // GitHub's API refuses requests without one
        static const String userAgent = String("MatrixController/") + FirmwareUpdate::version();
        HttpOptions options;
        options.timeoutMs = TimeoutMs;
        // Unlike the apps' data, firmware must come from GitHub and nobody in between
        options.verifyCertificate = true;
        // Asset downloads redirect to a storage host
        options.followRedirects = true;
        options.userAgent = userAgent.c_str();
        return options;
    }

    /// Why a request to GitHub failed, for the web UI
    String failure(const int status, const char* answered) {
        if (status == HttpInvalidUrl) return "Invalid address";
        if (status == HttpInvalidJson) return "Unexpected answer from GitHub";
        if (status < 0) return HTTPClient::errorToString(status);
        return String(answered) + " " + status;
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
    const String url = String("https://api.github.com/repos/") + UPDATE_GITHUB_REPO + "/releases/latest";
    HttpOptions options = gitHubOptions();
    options.accept = "application/vnd.github+json";

    // Release notes and the like can be long; only these fields are kept
    JsonDocument filter;
    filter["tag_name"] = true;
    filter["published_at"] = true;
    filter["assets"][0]["name"] = true;
    filter["assets"][0]["size"] = true;
    filter["assets"][0]["browser_download_url"] = true;

    JsonDocument doc;
    const int status = fetchJson(url, doc, &filter, options);
    if (status != HTTP_CODE_OK) {
        // Also what private repositories answer without a token
        setError(status == HTTP_CODE_NOT_FOUND ? "No release published yet" : failure(status, "GitHub answered").c_str());
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

    LOGF("Releases: latest is %s, %u bytes\n", release.tag.c_str(), static_cast<unsigned>(release.size));
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
    HttpRequest request;
    const int status = request.send(release.url.c_str(), gitHubOptions());
    if (status != HTTP_CODE_OK) {
        setError(failure(status, "Download answered").c_str());
        return;
    }

    const int length = request.size();
    const size_t size = length > 0 ? static_cast<size_t>(length) : release.size;
    const FirmwareUpdate::Start start = _update.begin(UpdateSource::Release, size);
    if (start.refusal) {
        setError(start.refusal);
        return;
    }
    const uint32_t attempt = start.attempt;

    WiFiClient& stream = request.body();
    uint8_t buffer[1024];
    size_t remaining = size;
    uint32_t lastDataMs = millis();
    while (remaining > 0) {
        const size_t waiting = stream.available();
        if (waiting == 0) {
            if (!stream.connected() || millis() - lastDataMs > TimeoutMs) {
                _update.cancel(attempt, "Download interrupted");
                return;
            }
            delay(5);
            continue;
        }
        const int read = stream.read(buffer, std::min({waiting, sizeof(buffer), remaining}));
        if (read <= 0) continue;
        lastDataMs = millis();
        // On errors the update records why
        if (!_update.write(attempt, buffer, read)) return;
        remaining -= read;
    }
    _update.finish(attempt);
}

void ReleaseUpdate::setError(const char* error) {
    std::lock_guard<std::mutex> lock(_mutex);
    _error = error;
}

void ReleaseUpdate::describe(const JsonObject out) {
    out["repo"] = UPDATE_GITHUB_REPO;
    out["checkIntervalHours"] = CheckIntervalMs / (60UL * 60 * 1000);
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
