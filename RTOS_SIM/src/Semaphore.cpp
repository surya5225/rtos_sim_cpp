#include "Semaphore.h"

namespace rtos {

Semaphore::Semaphore(int initialCount, const std::string& name)
    : count_(initialCount), name_(name) {
    if (initialCount < 0) throw InvalidParameter("semaphore count must be >= 0");
}

bool Semaphore::wait(Task* task) {
    if (count_ > 0) {
        --count_;
        return true;
    }
    waitQueue_.push(task);
    return false;
}

Task* Semaphore::signal() {
    if (!waitQueue_.empty()) {
        Task* t = waitQueue_.front();
        waitQueue_.pop();
        return t; // caller should make this task READY
    }
    ++count_;
    return nullptr;
}

} // namespace rtos