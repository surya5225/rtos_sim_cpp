// WatchdogTimer.h — Monitors that a task pets the watchdog within a timeout.
#pragma once

#include <string>

namespace rtos {

class WatchdogTimer {
public:
    WatchdogTimer(int timeoutTicks, const std::string& name = "");
    void start(int currentTick);
    void pet(int currentTick);
    void tick(int currentTick);     // Check whether the timeout has elapsed.
    void reset(int currentTick);
    void disable();
    void enable(int currentTick);
    bool isExpired() const { return expired_; }
    bool isEnabled() const { return enabled_; }
    int  getTimeout() const { return timeout_; }
    int  getLastPetTime() const { return lastPetTime_; }
    const std::string& getName() const { return name_; }

private:
    int  timeout_;
    int  lastPetTime_{-1};
    bool enabled_{false};
    bool expired_{false};
    std::string name_;
};

} // namespace rtos