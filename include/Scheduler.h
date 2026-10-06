// Scheduler.h — Strategy pattern for scheduling algorithms.
// The kernel only knows about the abstract Scheduler interface. Concrete
// policies (Priority, RoundRobin, EDF, RMS) are swappable at runtime.
#pragma once

#include "Common.h"
#include <vector>
#include <string>
#include <memory>

namespace rtos {

class Task; // forward declaration

class Scheduler {
public:
    virtual ~Scheduler() = default;

    // Select the next task to run from the set of READY tasks.
    // Returns nullptr when no task is eligible (CPU will be IDLE).
    virtual Task* selectNextTask(std::vector<Task*>& readyTasks,
                                 int currentTime) = 0;

    // Called when a task becomes ready — lets RR / priority queues update.
    virtual void onTaskReady(Task* task) = 0;

    // Called when a task stops running (completes / blocks / is preempted).
    virtual void onTaskStopped(Task* task) = 0;

    virtual std::string getName() const = 0;
    virtual SchedulingPolicy getPolicy() const = 0;
};

// ---------- Concrete schedulers ----------
class PriorityScheduler : public Scheduler {
public:
    Task* selectNextTask(std::vector<Task*>& readyTasks, int currentTime) override;
    void  onTaskReady(Task* task) override;
    void  onTaskStopped(Task* task) override;
    std::string getName() const override { return "FIXED PRIORITY"; }
    SchedulingPolicy getPolicy() const override { return SchedulingPolicy::PRIORITY; }
};

class RoundRobinScheduler : public Scheduler {
public:
    explicit RoundRobinScheduler(int quantum = 2);
    Task* selectNextTask(std::vector<Task*>& readyTasks, int currentTime) override;
    void  onTaskReady(Task* task) override;
    void  onTaskStopped(Task* task) override;
    std::string getName() const override { return "ROUND ROBIN (q=" + std::to_string(quantum_) + ")"; }
    SchedulingPolicy getPolicy() const override { return SchedulingPolicy::ROUND_ROBIN; }
    int  getQuantum() const { return quantum_; }
private:
    int quantum_;
    std::vector<Task*> queue_; // FIFO ready queue
};

class EDFScheduler : public Scheduler {
public:
    Task* selectNextTask(std::vector<Task*>& readyTasks, int currentTime) override;
    void  onTaskReady(Task* task) override;
    void  onTaskStopped(Task* task) override;
    std::string getName() const override { return "EARLIEST DEADLINE FIRST"; }
    SchedulingPolicy getPolicy() const override { return SchedulingPolicy::EDF; }
};

class RMSScheduler : public Scheduler {
public:
    Task* selectNextTask(std::vector<Task*>& readyTasks, int currentTime) override;
    void  onTaskReady(Task* task) override;
    void  onTaskStopped(Task* task) override;
    std::string getName() const override { return "RATE MONOTONIC"; }
    SchedulingPolicy getPolicy() const override { return SchedulingPolicy::RMS; }
};

} // namespace rtos