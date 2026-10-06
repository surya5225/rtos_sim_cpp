#include "EventFlags.h"

namespace rtos {

EventFlags::EventFlags(const std::string& name) : name_(name) {}

void EventFlags::set(std::uint32_t bits) { flags_ |= bits; }
void EventFlags::clear(std::uint32_t bits) { flags_ &= ~bits; }

bool EventFlags::waitAny(std::uint32_t bits, Task* task) {
    if (flags_ & bits) return true;
    waiters_.push_back({task, bits, false});
    return false;
}

bool EventFlags::waitAll(std::uint32_t bits, Task* task) {
    if ((flags_ & bits) == bits) return true;
    waiters_.push_back({task, bits, true});
    return false;
}

std::vector<Task*> EventFlags::evaluateWaiters() {
    std::vector<Task*> toWake;
    auto it = waiters_.begin();
    while (it != waiters_.end()) {
        bool satisfied = it->waitAll
            ? ((flags_ & it->mask) == it->mask)
            : ((flags_ & it->mask) != 0);
        if (satisfied) {
            toWake.push_back(it->task);
            it = waiters_.erase(it);
        } else {
            ++it;
        }
    }
    return toWake;
}

} // namespace rtos