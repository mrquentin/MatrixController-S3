#include "apps/PeriodicTask.h"

#include <utility>

namespace {
    // Core 0 runs WiFi and the web server, core 1 the Arduino loop that draws
    constexpr BaseType_t TaskCore = 0;
    constexpr UBaseType_t TaskPriority = 1;
}

PeriodicTask::PeriodicTask(const char* name, const uint32_t stackSize, Job job)
    : _name(name), _stackSize(stackSize), _job(std::move(job)) {}

void PeriodicTask::setEnabled(const bool enabled) {
    _enabled = enabled;

    // Created on first use, so apps that are never shown don't cost a task stack
    if (enabled && !_handle) {
        xTaskCreatePinnedToCore(entry, _name, _stackSize, this, TaskPriority, &_handle, TaskCore);
        return;  // a new task starts with a run
    }
    wake();
}

void PeriodicTask::setInterval(const uint32_t intervalMs) {
    _intervalMs = intervalMs;
}

void PeriodicTask::runNow() {
    wake();
}

void PeriodicTask::wake() {
    if (_handle) xTaskNotifyGive(_handle);
}

void PeriodicTask::entry(void* self) {
    static_cast<PeriodicTask*>(self)->run();
}

void PeriodicTask::run() {
    for (;;) {
        if (_enabled) _job();

        // Sleep until the interval is up, or until woken by runNow()/setEnabled(); paused while disabled
        const TickType_t wait = _enabled ? pdMS_TO_TICKS(_intervalMs.load()) : portMAX_DELAY;
        ulTaskNotifyTake(pdTRUE, wait);
    }
}
