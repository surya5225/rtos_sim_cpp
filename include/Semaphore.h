// Semaphore.h — Counting semaphore with a FIFO wait queue.
#pragma once

#include "Task.h"
#include <queue>
#include <string>

namespace rtos {

class Semaphore {
public:
    explicit Semaphore(int initialCount, const std::string& name = "");

    // Returns true if the calling task acquired the semaphore immediately.
    // Returns false if the task must block (count was zero).
    bool wait(Task* task);

    // Release the semaphore. If tasks are waiting, the first one becomes ready.
    // Returns the task that was unblocked (nullptr if none).
    Task* signal();

    int  getCount() const { return count_; }
    const std::string& getName() const { return name_; }
    bool hasWaiters() const { return !waitQueue_.empty(); }
    std::size_t waitersCount() const { return waitQueue_.size(); }

private:
    int count_;
    std::string name_;
    std::queue<Task*> waitQueue_;
};

} // namespace rtos