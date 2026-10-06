// StatisticsAnalyzer.h — Collects and reports per-task and global statistics.
#pragma once

#include "Task.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace rtos {

struct TaskStats {
    int taskId{0};
    std::string taskName;
    int completedJobs{0};
    int missedDeadlines{0};
    int totalExecutionTime{0};
    int totalWaitingTime{0};
    int lastResponseTime{0};
    int preemptions{0};
    int blocks{0};
};

struct GlobalStats {
    int totalTicks{0};
    int busyTicks{0};
    int idleTicks{0};
    int contextSwitches{0};
    int preemptions{0};
    int deadlineMisses{0};
    int watchdogFaults{0};
    int ipcOperations{0};
    int mutexContention{0};
    double cpuUtilization{0.0};
    double idlePercentage{0.0};
};

class StatisticsAnalyzer {
public:
    void onContextSwitch(int fromTaskId, int toTaskId);
    void onPreemption(int runningTaskId, int selectedTaskId);
    void onDeadlineMiss(Task* task);
    void onWatchdogFault();
    void onIpcOperation();
    void onMutexContention();
    void onBlock(Task* task);
    void onJobComplete(Task* task);
    void onTick(bool cpuBusy);

    const TaskStats&  getTaskStats(int taskId) const;
    GlobalStats       getGlobalStats() const;
    std::vector<TaskStats> getAllTaskStats() const;

    void reset();
    std::string formatGlobalReport() const;
    std::string formatTaskReport(int taskId) const;

private:
    std::unordered_map<int, TaskStats> perTask_;
    GlobalStats global_{};
};

} // namespace rtos