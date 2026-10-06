// SimulatorController.h — Presentation-layer bridge.
// The future GUI (Qt / HTML / Python) talks to this class instead of the
// kernel directly, so the kernel remains UI-agnostic.
#pragma once

#include "Kernel.h"
#include <memory>
#include <vector>
#include <string>

namespace rtos {

struct TaskInfo {
    int id;
    std::string name;
    TaskState state;
    int priority;
    int period;
    int deadline;
    int remainingExecution;
    int completedJobs;
    int missedDeadlines;
};

class SimulatorController {
public:
    SimulatorController();

    void createPeriodicTask(int id, const std::string& name, int priority,
                            int execTime, int period, int deadline);
    void createAperiodicTask(int id, const std::string& name, int priority,
                             int execTime);
    void deleteTask(int id);
    void selectScheduler(SchedulingPolicy policy, int rrQuantum = 2);

    void run(int ticks);
    void step();
    void reset();

    std::vector<TaskInfo> getTasks() const;
    std::vector<GanttSegment> getGanttData() const;
    GlobalStats getStatistics() const;
    std::vector<ExecutionRecord> getExecutionHistory() const;
    std::vector<std::pair<int,std::string>> getEvents() const;
    int getCurrentTick() const;

    Kernel& kernel() { return kernel_; }

private:
    Kernel kernel_;
};

} // namespace rtos