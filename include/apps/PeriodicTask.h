#pragma once

#ifndef _PERIODICTASK_h
#define _PERIODICTASK_h

#include <atomic>
#include <cstdint>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/// Runs a job on its own FreeRTOS task, again and again at an interval, while enabled.
/// Meant for apps that fetch network data: the job may block as long as it needs
/// (DNS, TLS, slow servers) without stalling the display or the web server.
///
/// Enabling runs the job right away, then every interval. Disabling lets a running job
/// finish and then pauses. All methods are safe to call from any task.
class PeriodicTask {
public:
    using Job = std::function<void()>;

    /// stackSize is in bytes; HTTPS requests need about 8 KB or more.
    PeriodicTask(const char* name, uint32_t stackSize, Job job);

    void setEnabled(bool enabled);
    /// Time between the end of one run and the start of the next. The job may call this
    /// itself, e.g. to retry sooner after an error.
    void setInterval(uint32_t intervalMs);
    /// Runs the job as soon as possible if enabled, without waiting for the interval.
    void runNow();

private:
    static void entry(void* self);
    [[noreturn]] void run();
    void wake();

    const char* _name;
    uint32_t _stackSize;
    Job _job;
    TaskHandle_t _handle = nullptr;
    std::atomic<bool> _enabled{false};
    std::atomic<uint32_t> _intervalMs{60000};
};

#endif
