// Mutex.h — Mutual-exclusion lock with priority inheritance.
// Only the owner may unlock. If a higher-priority task blocks on the mutex,
// the owner temporarily inherits the higher priority.
#pragma once

#include "Task.h"
#include <queue>
#include <string>

namespace rtos {

class Mutex {
public:
    explicit Mutex(const std::string& name = "");

    // Returns true if the task acquired the mutex immediately.
    // Returns false if the task must block.
    bool lock(Task* task);

    // Unlock the mutex. Returns the highest-priority waiter that should
    // become READY (nullptr if no waiters).
    Task* unlock(Task* task);

    Task* getOwner() const { return owner_; }
    bool  isLocked() const { return owner_ != nullptr; }
    const std::string& getName() const { return name_; }
    bool  hasWaiters() const { return !waitQueue_.empty(); }

private:
    // Wait queue ordered by priority (smaller number = higher priority).
    struct WaiterCompare {
        bool operator()(const Task* a, const Task* b) const;
    };
    std::priority_queue<Task*, std::vector<Task*>, WaiterCompare> waitQueue_;
    Task* owner_{nullptr};
    std::string name_;
};

} // namespace rtos