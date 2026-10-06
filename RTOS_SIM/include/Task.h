// Task.h — The Task Control Block (TCB).
// A Task is the software abstraction of an RTOS task. It holds every piece of
// state the kernel needs to schedule, suspend, resume and account for a task.
// All attributes are private — access goes through well-defined methods so
// state transitions are always valid.
#pragma once

#include "Common.h"
#include <string>

namespace rtos {

class Task {
public:
    // Construct a periodic task.
    Task(int id, std::string name, int priority,
         int executionTime, int period, int relativeDeadline);

    // Construct an aperiodic (one-shot) task.
    Task(int id, std::string name, int priority, int executionTime);

    // --- Job lifecycle ---
    void release(int currentTick);          // Make the current job READY
    void executeOneTick();                  // Consume 1 tick of CPU time
    bool isCompleted() const;               // Job finished its execution?
    void resetForNextPeriod();              // Prepare the next job
    void terminate();                       // Move to TERMINATED state

    // --- State transitions ---
    void setState(TaskState s);
    TaskState getState() const;

    // --- Blocking / unblocking ---
    void block(BlockReason reason);
    void unblock();
    BlockReason getBlockReason() const;

    // --- Suspend / resume ---
    void suspend();
    void resume();
    bool isSuspended() const;

    // --- Priority inheritance helpers ---
    void setEffectivePriority(int p);
    int  getEffectivePriority() const;
    int  getBasePriority() const;

    // --- Accessors ---
    int  getId() const;
    const std::string& getName() const;
    int  getExecutionTime() const;
    int  getRemainingExecutionTime() const;
    int  getPeriod() const;
    int  getRelativeDeadline() const;
    int  getAbsoluteDeadline() const;
    int  getReleaseTime() const;
    int  getNextReleaseTime() const;
    bool isPeriodic() const;

    // --- Round-Robin time-slice management ---
    void setTimeSlice(int q);
    int  getRemainingTimeSlice() const;
    void decrementTimeSlice();
    void resetTimeSlice();

    // --- Statistics ---
    void recordJobCompletion(int currentTick);
    void recordDeadlineMiss(int currentTick);
    void addWaitingTime(int t);
    int  getCompletedJobs() const;
    int  getMissedDeadlines() const;
    int  getTotalExecutionTime() const;
    int  getTotalWaitingTime() const;
    int  getResponseTime() const;
    void resetStatistics();

private:
    int         id_;
    std::string name_;
    int         basePriority_;
    int         effectivePriority_;

    int         executionTime_;
    int         remainingExecutionTime_;

    int         period_;              // 0 for aperiodic
    int         relativeDeadline_;
    int         absoluteDeadline_;

    int         releaseTime_;
    int         nextReleaseTime_;

    TaskState   state_;
    BlockReason blockReason_;
    bool        periodic_;
    bool        suspended_;

    int         completedJobs_;
    int         missedDeadlines_;
    int         totalExecutionTime_;
    int         totalWaitingTime_;
    int         responseTime_;

    int         timeSlice_;
    int         remainingTimeSlice_;
};

} // namespace rtos