#include "Scheduler.h"
#include "Task.h"
#include <algorithm>
#include <deque>

namespace rtos {

// ---------- Deterministic tie-breaker used by all priority-based schedulers ----------
// Order: primary criterion, then base priority, then release time, then task id.
// This guarantees reproducible results regardless of container ordering.
namespace {
bool taskLessByPriority(const Task* a, const Task* b) {
    // "less" means a is scheduled BEFORE b.
    if (a->getEffectivePriority() != b->getEffectivePriority())
        return a->getEffectivePriority() < b->getEffectivePriority();
    if (a->getReleaseTime() != b->getReleaseTime())
        return a->getReleaseTime() < b->getReleaseTime();
    return a->getId() < b->getId();
}

bool taskLessByDeadline(const Task* a, const Task* b) {
    if (a->getAbsoluteDeadline() != b->getAbsoluteDeadline())
        return a->getAbsoluteDeadline() < b->getAbsoluteDeadline();
    if (a->getEffectivePriority() != b->getEffectivePriority())
        return a->getEffectivePriority() < b->getEffectivePriority();
    return a->getId() < b->getId();
}

bool taskLessByPeriod(const Task* a, const Task* b) {
    // Rate Monotonic: shorter period = higher priority.
    if (a->getPeriod() != b->getPeriod())
        return a->getPeriod() < b->getPeriod();
    if (a->getEffectivePriority() != b->getEffectivePriority())
        return a->getEffectivePriority() < b->getEffectivePriority();
    return a->getId() < b->getId();
}
} // namespace

// =====================================================================
// PriorityScheduler — Fixed-priority preemptive scheduler.
// Complexity: O(n) linear scan. For teaching clarity we use a vector;
// a std::priority_queue would give O(log n) selection.
// =====================================================================
Task* PriorityScheduler::selectNextTask(std::vector<Task*>& readyTasks, int /*t*/) {
    if (readyTasks.empty()) return nullptr;
    Task* best = readyTasks.front();
    for (std::size_t i = 1; i < readyTasks.size(); ++i) {
        if (taskLessByPriority(readyTasks[i], best)) best = readyTasks[i];
    }
    return best;
}

void PriorityScheduler::onTaskReady(Task*)   {}
void PriorityScheduler::onTaskStopped(Task*) {}

// =====================================================================
// RoundRobinScheduler — FIFO queue with a configurable time quantum.
// Uses std::vector as a deque-style FIFO (O(1) push_back / erase begin).
// =====================================================================
RoundRobinScheduler::RoundRobinScheduler(int quantum) : quantum_(quantum) {
    if (quantum_ <= 0) quantum_ = 1;
}

Task* RoundRobinScheduler::selectNextTask(std::vector<Task*>& readyTasks, int /*t*/) {
    // Merge any newly-ready tasks into the queue (preserving FIFO order).
    for (Task* t : readyTasks) {
        bool already = false;
        for (Task* q : queue_) if (q == t) { already = true; break; }
        if (!already) {
            t->setTimeSlice(quantum_);
            queue_.push_back(t);
        }
    }
    if (queue_.empty()) return nullptr;

    Task* head = queue_.front();
    // If the head has exhausted its quantum, rotate it to the back.
    if (head->getRemainingTimeSlice() <= 0) {
        queue_.erase(queue_.begin());
        head->resetTimeSlice();
        queue_.push_back(head);
        head = queue_.front();
    }
    return head;
}

void RoundRobinScheduler::onTaskReady(Task* t) {
    // Already handled in selectNextTask by merging readyTasks.
    (void)t;
}

void RoundRobinScheduler::onTaskStopped(Task* t) {
    // Remove the task from the queue (it completed or blocked).
    for (auto it = queue_.begin(); it != queue_.end(); ++it) {
        if (*it == t) { queue_.erase(it); return; }
    }
}

// =====================================================================
// EDFScheduler — Earliest Deadline First.
// Selects the ready task with the smallest absolute deadline.
// Complexity: O(n) linear scan.
// =====================================================================
Task* EDFScheduler::selectNextTask(std::vector<Task*>& readyTasks, int /*t*/) {
    if (readyTasks.empty()) return nullptr;
    Task* best = readyTasks.front();
    for (std::size_t i = 1; i < readyTasks.size(); ++i) {
        if (taskLessByDeadline(readyTasks[i], best)) best = readyTasks[i];
    }
    return best;
}

void EDFScheduler::onTaskReady(Task*)   {}
void EDFScheduler::onTaskStopped(Task*) {}

// =====================================================================
// RMSScheduler — Rate Monotonic.
// Shorter period = higher priority. Ties broken by base priority then id.
// =====================================================================
Task* RMSScheduler::selectNextTask(std::vector<Task*>& readyTasks, int /*t*/) {
    if (readyTasks.empty()) return nullptr;
    Task* best = readyTasks.front();
    for (std::size_t i = 1; i < readyTasks.size(); ++i) {
        if (taskLessByPeriod(readyTasks[i], best)) best = readyTasks[i];
    }
    return best;
}

void RMSScheduler::onTaskReady(Task*)   {}
void RMSScheduler::onTaskStopped(Task*) {}

} // namespace rtos