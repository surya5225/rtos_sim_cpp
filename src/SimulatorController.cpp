#include "SimulatorController.h"

namespace rtos {

SimulatorController::SimulatorController() = default;

void SimulatorController::createPeriodicTask(int id, const std::string& name, int priority,
                                             int execTime, int period, int deadline) {
    kernel_.addPeriodicTask(id, name, priority, execTime, period, deadline);
}

void SimulatorController::createAperiodicTask(int id, const std::string& name, int priority,
                                              int execTime) {
    kernel_.addAperiodicTask(id, name, priority, execTime);
}

void SimulatorController::deleteTask(int id) { kernel_.removeTask(id); }

void SimulatorController::selectScheduler(SchedulingPolicy policy, int rrQuantum) {
    std::unique_ptr<Scheduler> s;
    switch (policy) {
        case SchedulingPolicy::PRIORITY:    s = std::make_unique<PriorityScheduler>(); break;
        case SchedulingPolicy::ROUND_ROBIN: s = std::make_unique<RoundRobinScheduler>(rrQuantum); break;
        case SchedulingPolicy::EDF:         s = std::make_unique<EDFScheduler>(); break;
        case SchedulingPolicy::RMS:         s = std::make_unique<RMSScheduler>(); break;
    }
    kernel_.setScheduler(std::move(s));
}

void SimulatorController::run(int ticks) { kernel_.run(ticks); }
void SimulatorController::step()         { kernel_.tick(); }
void SimulatorController::reset()        { kernel_.reset(); }

std::vector<TaskInfo> SimulatorController::getTasks() const {
    std::vector<TaskInfo> out;
    for (Task* t : kernel_.getTasks()) {
        TaskInfo i;
        i.id = t->getId();
        i.name = t->getName();
        i.state = t->getState();
        i.priority = t->getEffectivePriority();
        i.period = t->getPeriod();
        i.deadline = t->getAbsoluteDeadline();
        i.remainingExecution = t->getRemainingExecutionTime();
        i.completedJobs = t->getCompletedJobs();
        i.missedDeadlines = t->getMissedDeadlines();
        out.push_back(i);
    }
    return out;
}

std::vector<GanttSegment> SimulatorController::getGanttData() const {
    return kernel_.getExecutionLogger().buildGanttSegments();
}

GlobalStats SimulatorController::getStatistics() const {
    return kernel_.getStats().getGlobalStats();
}

std::vector<ExecutionRecord> SimulatorController::getExecutionHistory() const {
    return kernel_.getExecutionLogger().getHistory();
}

std::vector<std::pair<int,std::string>> SimulatorController::getEvents() const {
    return kernel_.getExecutionLogger().getEvents();
}

int SimulatorController::getCurrentTick() const { return kernel_.getCurrentTick(); }

} // namespace rtos