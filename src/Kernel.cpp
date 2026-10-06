#include "Kernel.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

namespace rtos {

Kernel::Kernel() {
    // Default scheduler: fixed priority.
    scheduler_ = std::make_unique<PriorityScheduler>();
}

void Kernel::setScheduler(std::unique_ptr<Scheduler> s) {
    if (!s) throw InvalidParameter("null scheduler");
    scheduler_ = std::move(s);
}

Task* Kernel::addPeriodicTask(int id, const std::string& name, int priority,
                              int execTime, int period, int deadline) {
    return taskManager_.createPeriodicTask(id, name, priority, execTime, period, deadline);
}

Task* Kernel::addAperiodicTask(int id, const std::string& name, int priority, int execTime) {
    return taskManager_.createAperiodicTask(id, name, priority, execTime);
}

void Kernel::removeTask(int id) {
    Task* t = taskManager_.getTask(id);
    if (t == currentRunning_) currentRunning_ = nullptr;
    taskManager_.removeTask(id);
}

Task* Kernel::getTask(int id) const { return taskManager_.getTask(id); }

std::vector<Task*> Kernel::getTasks() const { return taskManager_.allTasks(); }

Semaphore*    Kernel::createSemaphore(const std::string& n, int i)    { return resourceManager_.createSemaphore(n, i); }
Mutex*        Kernel::createMutex(const std::string& n)               { return resourceManager_.createMutex(n); }
MessageQueue* Kernel::createMessageQueue(const std::string& n, std::size_t c) { return resourceManager_.createMessageQueue(n, c); }
EventFlags*   Kernel::createEventFlags(const std::string& n)          { return resourceManager_.createEventFlags(n); }

WatchdogTimer* Kernel::createWatchdog(const std::string& name, int timeout) {
    auto w = std::make_unique<WatchdogTimer>(timeout, name);
    WatchdogTimer* raw = w.get();
    watchdogs_[name] = std::move(w);
    return raw;
}

WatchdogTimer* Kernel::getWatchdog(const std::string& name) const {
    auto it = watchdogs_.find(name);
    return it == watchdogs_.end() ? nullptr : it->second.get();
}

void Kernel::reset() {
    currentTick_ = 0;
    currentRunning_ = nullptr;
    taskManager_.resetAll();
    timerManager_.clear();
    resourceManager_.clear();
    watchdogs_.clear();
    delayWakeTime_.clear();
    stats_.reset();
    logger_.clear();
}

// ---------------------------------------------------------------------
// tick() — one discrete simulation step.
// Order of phases is critical for determinism and correctness.
// ---------------------------------------------------------------------
void Kernel::tick() {
    // 1. Release periodic jobs whose nextReleaseTime == currentTick
    releasePeriodicJobs();

    // 2. Process timers and delayed-task wake-ups
    processTimers();

    // 3. Re-evaluate event-flag waiters (bits may have been set earlier)
    evaluateEventFlags();

    // 4. Deadline check for the currently running task (before execution)
    //    and for any READY task whose deadline has already passed.
    checkDeadlines();

    // 5. Update watchdogs
    updateWatchdogs();

    // 6. Schedule and execute one tick of CPU work
    scheduleAndExecute();

    // 7. Record the tick for Gantt / history
    recordTick();

    // 8. Advance simulated time
    ++currentTick_;
}

void Kernel::run(int totalTicks) {
    for (int i = 0; i < totalTicks; ++i) tick();
}

// ---------------------------------------------------------------------
// Phase 1 — Release periodic jobs
// ---------------------------------------------------------------------
void Kernel::releasePeriodicJobs() {
    for (Task* t : taskManager_.allTasks()) {
        if (!t->isPeriodic()) continue;
        if (t->isSuspended()) continue;
        // A new job is released when:
        //   - the task has never been released (releaseTime_ == -1), or
        //   - the previous job has completed AND currentTick >= nextReleaseTime.
        bool neverReleased = (t->getReleaseTime() < 0);
        bool periodElapsed = (t->getState() == TaskState::DORMANT ||
                              t->getState() == TaskState::TERMINATED) &&
                             currentTick_ >= t->getNextReleaseTime();
        if (neverReleased || periodElapsed) {
            t->release(currentTick_);
            scheduler_->onTaskReady(t);
            logEvent("RELEASE " + t->getName());
        }
    }
}

// ---------------------------------------------------------------------
// Phase 2 — Process timers
// ---------------------------------------------------------------------
void Kernel::processTimers() {
    timerManager_.tick(currentTick_,
        [this](int taskId) {
            Task* t = taskManager_.getTask(taskId);
            if (!t) return;
            if (t->getState() == TaskState::BLOCKED &&
                t->getBlockReason() == BlockReason::DELAY) {
                makeReady(t);
                logEvent("DELAY_EXPIRED " + t->getName());
            }
            delayWakeTime_.erase(taskId);
        },
        [](int /*timerId*/) { /* user callbacks */ });
}

// ---------------------------------------------------------------------
// Phase 3 — Event flags
// ---------------------------------------------------------------------
void Kernel::evaluateEventFlags() {
    // Iterate over every event-flag group and wake satisfied waiters.
    // The ResourceManager doesn't expose a generic iterator, so we rely on
    // scenarios/tests to call eventSet() which internally evaluates waiters.
    // (Kept simple for educational clarity.)
}

// ---------------------------------------------------------------------
// Phase 4 — Deadline check
// ---------------------------------------------------------------------
void Kernel::checkDeadlines() {
    for (Task* t : taskManager_.allTasks()) {
        if (t->getState() == TaskState::DORMANT ||
            t->getState() == TaskState::TERMINATED) continue;
        if (t->getReleaseTime() < 0) continue;
        if (currentTick_ >= t->getAbsoluteDeadline() && !t->isCompleted()) {
            // Deadline miss — only record once per job.
            if (t->getRemainingExecutionTime() > 0) {
                stats_.onDeadlineMiss(t);
                t->recordDeadlineMiss(currentTick_);
                std::ostringstream os;
                os << "DEADLINE_MISS task=" << t->getName()
                   << " deadline=" << t->getAbsoluteDeadline()
                   << " remaining=" << t->getRemainingExecutionTime();
                logEvent(os.str());
                // Abort the current job so the next period starts clean.
                t->terminate();
                if (currentRunning_ == t) currentRunning_ = nullptr;
            }
        }
    }
}

// ---------------------------------------------------------------------
// Phase 5 — Watchdogs
// ---------------------------------------------------------------------
void Kernel::updateWatchdogs() {
    for (auto& kv : watchdogs_) {
        WatchdogTimer* w = kv.second.get();
        w->tick(currentTick_);
        if (w->isExpired()) {
            stats_.onWatchdogFault();
            logEvent("WATCHDOG_EXPIRED " + w->getName());
            // Prevent repeated logging of the same expiration.
            w->disable();
        }
    }
}

// ---------------------------------------------------------------------
// Phase 6 — Schedule and execute
// ---------------------------------------------------------------------
void Kernel::scheduleAndExecute() {
    auto ready = taskManager_.readyTasks();
    Task* next = scheduler_->selectNextTask(ready, currentTick_);

    // Preemption check: if a higher-priority task became ready while we were
    // running something else, the current task is preempted.
    if (currentRunning_ && currentRunning_->getState() == TaskState::RUNNING) {
        if (next && next != currentRunning_) {
            preemptIfNeeded(next);
        }
    }

    if (next && next != currentRunning_) {
        contextSwitchTo(next);
    }

    if (currentRunning_ && currentRunning_->getState() == TaskState::RUNNING) {
        currentRunning_->executeOneTick();
        stats_.onTick(true);

        // Round-Robin: decrement time slice.
        if (scheduler_->getPolicy() == SchedulingPolicy::ROUND_ROBIN) {
            currentRunning_->decrementTimeSlice();
        }

        if (currentRunning_->isCompleted()) {
            stats_.onJobComplete(currentRunning_);
            currentRunning_->recordJobCompletion(currentTick_);
            logEvent("TASK_COMPLETE " + currentRunning_->getName());
            scheduler_->onTaskStopped(currentRunning_);
            if (currentRunning_->isPeriodic()) {
                currentRunning_->resetForNextPeriod();
            } else {
                currentRunning_->terminate();
            }
            currentRunning_ = nullptr;
        } else {
            // Round-Robin: rotate when the quantum expires.
            if (scheduler_->getPolicy() == SchedulingPolicy::ROUND_ROBIN &&
                currentRunning_->getRemainingTimeSlice() <= 0) {
                logEvent("QUANTUM_EXPIRED " + currentRunning_->getName());
                currentRunning_->setState(TaskState::READY);
                scheduler_->onTaskStopped(currentRunning_);
                currentRunning_ = nullptr;
            }
        }
    } else {
        // CPU is idle this tick.
        stats_.onTick(false);
        logger_.record(currentTick_, -1, "IDLE", TaskState::DORMANT, "IDLE");
    }
}

void Kernel::recordTick() {
    // Already recorded inside scheduleAndExecute (busy or idle).
}

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------
void Kernel::makeReady(Task* task) {
    task->unblock();
    scheduler_->onTaskReady(task);
}

void Kernel::makeBlocked(Task* task, BlockReason reason) {
    task->block(reason);
    stats_.onBlock(task);
    if (currentRunning_ == task) currentRunning_ = nullptr;
}

void Kernel::contextSwitchTo(Task* next) {
    int fromId = currentRunning_ ? currentRunning_->getId() : -1;
    if (currentRunning_ && currentRunning_->getState() == TaskState::RUNNING) {
        currentRunning_->setState(TaskState::READY);
    }
    next->setState(TaskState::RUNNING);
    currentRunning_ = next;
    stats_.onContextSwitch(fromId, next->getId());

    std::ostringstream os;
    os << "CONTEXT_SWITCH ";
    if (fromId >= 0) {
        Task* from = taskManager_.getTask(fromId);
        os << (from ? from->getName() : "?");
    } else os << "IDLE";
    os << " -> " << next->getName();
    logEvent(os.str());
}

void Kernel::preemptIfNeeded(Task* /*next*/) {
    if (!currentRunning_) return;
    // Preemption happens when the scheduler would choose a different task.
    auto ready = taskManager_.readyTasks();
    // Include the currently running task in the candidate set.
    bool hasRunning = false;
    for (Task* t : ready) if (t == currentRunning_) { hasRunning = true; break; }
    if (!hasRunning) ready.push_back(currentRunning_);

    Task* best = scheduler_->selectNextTask(ready, currentTick_);
    if (best && best != currentRunning_) {
        stats_.onPreemption(currentRunning_->getId(), best->getId());
        std::ostringstream os;
        os << "PREEMPTION running=" << currentRunning_->getName()
           << " selected=" << best->getName();
        logEvent(os.str());
    }
}

void Kernel::logEvent(const std::string& event) {
    logger_.recordEvent(currentTick_, event);
}

// ---------------------------------------------------------------------
// IPC operations
// ---------------------------------------------------------------------
bool Kernel::semaphoreWait(const std::string& name, Task* task) {
    Semaphore* s = resourceManager_.getSemaphore(name);
    if (!s) throw ResourceError("unknown semaphore: " + name);
    stats_.onIpcOperation();
    if (s->wait(task)) return true;
    makeBlocked(task, BlockReason::SEMAPHORE);
    logEvent("BLOCKED_SEM " + task->getName() + " on " + name);
    return false;
}

Task* Kernel::semaphoreSignal(const std::string& name) {
    Semaphore* s = resourceManager_.getSemaphore(name);
    if (!s) throw ResourceError("unknown semaphore: " + name);
    stats_.onIpcOperation();
    Task* w = s->signal();
    if (w) {
        makeReady(w);
        logEvent("UNBLOCKED " + w->getName() + " from sem " + name);
    }
    return w;
}

bool Kernel::mutexLock(const std::string& name, Task* task) {
    Mutex* m = resourceManager_.getMutex(name);
    if (!m) throw ResourceError("unknown mutex: " + name);
    stats_.onIpcOperation();
    if (m->lock(task)) return true;

    // Priority inheritance: the owner temporarily takes the caller's priority.
    Task* owner = m->getOwner();
    if (owner && task->getEffectivePriority() < owner->getEffectivePriority()) {
        int oldP = owner->getEffectivePriority();
        owner->setEffectivePriority(task->getEffectivePriority());
        std::ostringstream os;
        os << "PRIORITY_INHERITANCE task=" << owner->getName()
           << " " << oldP << " -> " << task->getEffectivePriority()
           << " (blocking " << task->getName() << ")";
        logEvent(os.str());
        stats_.onMutexContention();
    } else {
        stats_.onMutexContention();
    }
    makeBlocked(task, BlockReason::MUTEX);
    logEvent("BLOCKED_MUTEX " + task->getName() + " on " + name);
    return false;
}

Task* Kernel::mutexUnlock(const std::string& name, Task* task) {
    Mutex* m = resourceManager_.getMutex(name);
    if (!m) throw ResourceError("unknown mutex: " + name);
    stats_.onIpcOperation();

    // Restore original priority if the owner had inherited one.
    Task* owner = m->getOwner();
    if (owner && owner->getEffectivePriority() != owner->getBasePriority()) {
        int oldP = owner->getEffectivePriority();
        owner->setEffectivePriority(owner->getBasePriority());
        std::ostringstream os;
        os << "PRIORITY_RESTORED task=" << owner->getName()
           << " " << oldP << " -> " << owner->getBasePriority();
        logEvent(os.str());
    }

    Task* w = m->unlock(task);
    if (w) {
        makeReady(w);
        logEvent("UNBLOCKED " + w->getName() + " from mutex " + name);
    }
    return w;
}

bool Kernel::messageSend(const std::string& name, const Message& msg, Task* sender) {
    MessageQueue* q = resourceManager_.getMessageQueue(name);
    if (!q) throw ResourceError("unknown queue: " + name);
    stats_.onIpcOperation();
    if (q->send(msg, sender)) {
        Task* r = q->popWaitingReceiver();
        if (r) { makeReady(r); logEvent("UNBLOCKED " + r->getName() + " recv " + name); }
        return true;
    }
    makeBlocked(sender, BlockReason::MESSAGE_QUEUE_SEND);
    logEvent("BLOCKED_MQ_SEND " + sender->getName() + " on " + name);
    return false;
}

bool Kernel::messageReceive(const std::string& name, Message& out, Task* receiver) {
    MessageQueue* q = resourceManager_.getMessageQueue(name);
    if (!q) throw ResourceError("unknown queue: " + name);
    stats_.onIpcOperation();
    if (q->receive(out, receiver)) {
        Task* s = q->popWaitingSender();
        if (s) { makeReady(s); logEvent("UNBLOCKED " + s->getName() + " send " + name); }
        return true;
    }
    makeBlocked(receiver, BlockReason::MESSAGE_QUEUE_RECEIVE);
    logEvent("BLOCKED_MQ_RECV " + receiver->getName() + " on " + name);
    return false;
}

void Kernel::eventSet(const std::string& name, std::uint32_t bits) {
    EventFlags* e = resourceManager_.getEventFlags(name);
    if (!e) throw ResourceError("unknown event flag: " + name);
    stats_.onIpcOperation();
    e->set(bits);
    for (Task* t : e->evaluateWaiters()) {
        makeReady(t);
        logEvent("UNBLOCKED " + t->getName() + " from event " + name);
    }
}

bool Kernel::eventWaitAny(const std::string& name, std::uint32_t bits, Task* task) {
    EventFlags* e = resourceManager_.getEventFlags(name);
    if (!e) throw ResourceError("unknown event flag: " + name);
    stats_.onIpcOperation();
    if (e->waitAny(bits, task)) return true;
    makeBlocked(task, BlockReason::EVENT_FLAG);
    logEvent("BLOCKED_EVENT " + task->getName() + " on " + name);
    return false;
}

bool Kernel::eventWaitAll(const std::string& name, std::uint32_t bits, Task* task) {
    EventFlags* e = resourceManager_.getEventFlags(name);
    if (!e) throw ResourceError("unknown event flag: " + name);
    stats_.onIpcOperation();
    if (e->waitAll(bits, task)) return true;
    makeBlocked(task, BlockReason::EVENT_FLAG);
    logEvent("BLOCKED_EVENT " + task->getName() + " on " + name);
    return false;
}

void Kernel::taskDelay(Task* task, int ticks) {
    if (ticks <= 0) return;
    int wake = currentTick_ + ticks;
    delayWakeTime_[task->getId()] = wake;
    timerManager_.scheduleDelay(task->getId(), wake);
    makeBlocked(task, BlockReason::DELAY);
    logEvent("DELAY " + task->getName() + " until " + std::to_string(wake));
}

// ---------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------
std::string Kernel::generateGanttText() const {
    auto segs = logger_.buildGanttSegments();
    std::ostringstream os;
    os << "Tick  Task\n";
    os << "-----------\n";
    for (const auto& s : segs) {
        os << s.startTick << "-" << (s.endTick - 1) << "  " << s.taskName << "\n";
    }
    return os.str();
}

void Kernel::exportFiles(const std::string& dir) const {
    logger_.exportCsv(dir + "/execution_history.csv");
    logger_.exportGanttCsv(dir + "/gantt_data.csv");
    logger_.exportEvents(dir + "/simulation_log.txt");

    std::ofstream f(dir + "/statistics.txt");
    if (f) {
        f << "========== GLOBAL ==========\n";
        f << stats_.formatGlobalReport();
        f << "\n========== PER TASK ==========\n";
        for (const auto& s : stats_.getAllTaskStats()) {
            f << stats_.formatTaskReport(s.taskId) << "\n";
        }
    }
}

} // namespace rtos