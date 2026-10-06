// EventFlags.h — Bit-mask event group.
// Tasks can wait for ANY or ALL of a set of bits.
#pragma once

#include "Task.h"
#include <vector>
#include <cstdint>

namespace rtos {

class EventFlags {
public:
    explicit EventFlags(const std::string& name = "");

    void set(std::uint32_t bits);
    void clear(std::uint32_t bits);
    std::uint32_t get() const { return flags_; }

    // Returns true if the task's condition is satisfied immediately.
    // Otherwise the task is added to the wait list.
    bool waitAny(std::uint32_t bits, Task* task);
    bool waitAll(std::uint32_t bits, Task* task);

    // After set()/clear(), check whether any waiting tasks should wake.
    // Returns the list of tasks that become READY.
    std::vector<Task*> evaluateWaiters();

    const std::string& getName() const { return name_; }

private:
    struct Waiter {
        Task*        task;
        std::uint32_t mask;
        bool         waitAll;
    };
    std::uint32_t flags_{0};
    std::string   name_;
    std::vector<Waiter> waiters_;
};

} // namespace rtos