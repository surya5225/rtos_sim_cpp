#include "Mutex.h"

namespace rtos {

Mutex::Mutex(const std::string& name) : name_(name) {}

bool Mutex::Mutex::WaiterCompare::operator()(const Task* a, const Task* b) const {
    // std::priority_queue is a max-heap; we want the HIGHEST priority
    // (smallest number) at the top, so we invert the comparison.
    if (a->getEffectivePriority() != b->getEffectivePriority())
        return a->getEffectivePriority() > b->getEffectivePriority();
    return a->getId() > b->getId();
}

bool Mutex::lock(Task* task) {
    if (!task) throw InvalidParameter("null task");
    if (owner_ == nullptr) {
        owner_ = task;
        return true;
    }
    if (owner_ == task) {
        // Recursive lock attempt — treat as error for simplicity.
        throw ResourceError("task already owns mutex '" + name_ + "'");
    }
    waitQueue_.push(task);
    return false;
}

Task* Mutex::unlock(Task* task) {
    if (!task) throw InvalidParameter("null task");
    if (owner_ != task) {
        throw ResourceError("task " + std::to_string(task->getId()) +
                            " tried to unlock mutex '" + name_ +
                            "' owned by task " +
                            (owner_ ? std::to_string(owner_->getId()) : std::string("none")));
    }
    owner_ = nullptr;
    if (!waitQueue_.empty()) {
        Task* next = waitQueue_.top();
        waitQueue_.pop();
        return next;
    }
    return nullptr;
}

} // namespace rtos