// Kernel.h — The central simulation engine.
// Runs a deterministic discrete-time tick loop. Does NOT use OS threads.
#pragma once

#include "Common.h"
#include "TaskManager.h"
#include "TimerManager.h"
#include "ResourceManager.h"
#include "StatisticsAnalyzer.h"
#include "ExecutionLogger.h"
#include "Scheduler.h"
#include "WatchdogTimer.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace rtos {

class Kernel {
public:
    Kernel();

    // --- Configuration ---
    void setScheduler(std::unique_ptr<Scheduler> scheduler);
    Scheduler* getScheduler() const { return scheduler_.get(); }

    // --- Task management ---
    Task* addPeriodicTask(int id, const std::string& name, int priority,
                          int execTime, int period, int deadline);
    Task* addAperiodicTask(int id, const std::string& name, int priority,
                           int execTime);
    void  removeTask(int id);
    Task* getTask(int id) const;
    std::vector<Task*> getTasks() const;

    // --- Resource management ---
    Semaphore*    createSemaphore(const std::string& name, int initial);
    Mutex*        createMutex(const std::string& name);
    MessageQueue* createMessageQueue(const std::string& name, std::size_t capacity);
    EventFlags*   createEventFlags(const std::string& name);

    // --- Watchdog ---
    WatchdogTimer* createWatchdog(const std::string& name, int timeout);
    WatchdogTimer* getWatchdog(const std::string& name) const;

    // --- Simulation control ---
    void run(int totalTicks);
    void tick();
    void reset();
    int  getCurrentTick() const { return currentTick_; }

    // --- Direct IPC operations (used by scenarios / tests) ---
    bool semaphoreWait(const std::string& name, Task* task);
    Task* semaphoreSignal(const std::string& name);
    bool mutexLock(const std::string& name, Task* task);
    Task* mutexUnlock(const std::string& name, Task* task);
    bool messageSend(const std::string& name, const Message& msg, Task* sender);
    bool messageReceive(const std::string& name, Message& out, Task* receiver);
    void eventSet(const std::string& name, std::uint32_t bits);
    bool eventWaitAny(const std::string& name, std::uint32_t bits, Task* task);
    bool eventWaitAll(const std::string& name, std::uint32_t bits, Task* task);
    void taskDelay(Task* task, int ticks);

    // --- Reporting ---
    const ExecutionLogger& getExecutionLogger() const { return logger_; }
    const StatisticsAnalyzer& getStats() const { return stats_; }
    std::string generateGanttText() const;
    void exportFiles(const std::string& dir) const;

private:
    // --- Per-tick phases ---
    void releasePeriodicJobs();
    void processTimers();
    void evaluateEventFlags();
    void checkDeadlines();
    void updateWatchdogs();
    void scheduleAndExecute();
    void recordTick();

    // --- Helpers ---
    void makeReady(Task* task);
    void makeBlocked(Task* task, BlockReason reason);
    void contextSwitchTo(Task* next);
    void preemptIfNeeded(Task* next);
    void logEvent(const std::string& event);

    int currentTick_{0};
    TaskManager        taskManager_;
    TimerManager       timerManager_;
    ResourceManager    resourceManager_;
    StatisticsAnalyzer stats_;
    ExecutionLogger    logger_;

    std::unique_ptr<Scheduler> scheduler_;

    Task* currentRunning_{nullptr};
    int   idleTaskId_{-1};

    std::unordered_map<std::string, std::unique_ptr<WatchdogTimer>> watchdogs_;
    std::unordered_map<int, int> delayWakeTime_; // taskId -> wake tick
};

} // namespace rtos