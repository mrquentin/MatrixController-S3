#pragma once

#ifndef _F1APP_h
#define _F1APP_h

#include <mutex>
#include <string>
#include "apps/App.h"
#include "apps/F1Timing.h"
#include "apps/PeriodicTask.h"

/// F1 session status from a MultiViewer instance on the local network, read through its
/// GraphQL API: lap count in races and sprints, Q1-Q3 (SQ1-SQ3) and time left in qualifying,
/// time left in practice, and the current flag and any pit lane closure in every session.
class F1App final : public App {
public:
    F1App();

    [[nodiscard]] const char* id() const override { return "f1"; }
    [[nodiscard]] const char* name() const override { return "F1"; }

    void describeConfig(ConfigSchema& schema) const override;
    void getConfig(JsonObject config) const override;
    void setConfig(JsonObjectConst config) override;

    void activate() override;
    void deactivate() override;
    void render(IDisplay& display, uint32_t nowMs) override;

private:
    enum class Status : uint8_t { NotConfigured, Connecting, Unreachable, NoSession, Live };

    // Background task
    void poll();

    void renderLive(IDisplay& display, uint32_t nowMs);
    /// Practice and qualifying: session label and time left on top, then the top 3.
    void renderTopDrivers(IDisplay& display, uint32_t nowMs, const char* label) const;

    PeriodicTask _task;

    // Shared between the task and the other methods, guarded by _mutex
    mutable std::mutex _mutex;
    std::string _host;
    int32_t _port = 10101;
    uint32_t _generation = 0;     // bumped when the address changes, to drop answers from the old one
    Status _status = Status::NotConfigured;
    F1::LiveState _state;
    uint32_t _lastSuccessMs = 0;

    // Flag changes, tracked on the render side to show new flags full screen for a moment
    F1::Flag _flag = F1::Flag::None;
    bool _flagKnown = false;      // false until the first live frame, whose flag is not "new"
    uint32_t _flagChangedMs = 0;
};

#endif
