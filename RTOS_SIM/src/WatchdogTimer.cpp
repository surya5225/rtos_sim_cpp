#include "WatchdogTimer.h"
#include "Common.h"
namespace rtos {

WatchdogTimer::WatchdogTimer(int timeoutTicks, const std::string& name)
    : timeout_(timeoutTicks), name_(name) {
    if (timeoutTicks <= 0) throw InvalidParameter("watchdog timeout must be > 0");
}

void WatchdogTimer::start(int currentTick) {
    lastPetTime_ = currentTick;
    enabled_ = true;
    expired_ = false;
}

void WatchdogTimer::pet(int currentTick) {
    if (!enabled_) return;
    lastPetTime_ = currentTick;
}

void WatchdogTimer::tick(int currentTick) {
    if (!enabled_ || expired_) return;
    if (lastPetTime_ < 0) return; // never started
    if (currentTick - lastPetTime_ >= timeout_) expired_ = true;
}

void WatchdogTimer::reset(int currentTick) {
    lastPetTime_ = currentTick;
    expired_ = false;
}

void WatchdogTimer::disable() { enabled_ = false; }

void WatchdogTimer::enable(int currentTick) {
    enabled_ = true;
    lastPetTime_ = currentTick;
    expired_ = false;
}

} // namespace rtos