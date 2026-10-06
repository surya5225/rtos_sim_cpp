#include "StatisticsAnalyzer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
namespace rtos {

namespace {
TaskStats& ensure(std::unordered_map<int, TaskStats>& m, int id, const std::string& name) {
    auto it = m.find(id);
    if (it == m.end()) {
        TaskStats s; s.taskId = id; s.taskName = name;
        m[id] = s;
        return m[id];
    }
    return it->second;
}
} // namespace

void StatisticsAnalyzer::onContextSwitch(int fromId, int toId) {
    if (fromId == toId) return;
    ++global_.contextSwitches;
}

void StatisticsAnalyzer::onPreemption(int /*runningId*/, int /*selectedId*/) {
    ++global_.preemptions;
}

void StatisticsAnalyzer::onDeadlineMiss(Task* t) {
    ++global_.deadlineMisses;
    if (t) ensure(perTask_, t->getId(), t->getName()).missedDeadlines++;
}

void StatisticsAnalyzer::onWatchdogFault() { ++global_.watchdogFaults; }
void StatisticsAnalyzer::onIpcOperation()  { ++global_.ipcOperations; }
void StatisticsAnalyzer::onMutexContention() { ++global_.mutexContention; }

void StatisticsAnalyzer::onBlock(Task* t) {
    if (t) ensure(perTask_, t->getId(), t->getName()).blocks++;
}

void StatisticsAnalyzer::onJobComplete(Task* t) {
    if (!t) return;
    auto& s = ensure(perTask_, t->getId(), t->getName());
    s.completedJobs      = t->getCompletedJobs();
    s.missedDeadlines    = t->getMissedDeadlines();
    s.totalExecutionTime = t->getTotalExecutionTime();
    s.totalWaitingTime   = t->getTotalWaitingTime();
    s.lastResponseTime   = t->getResponseTime();
}

void StatisticsAnalyzer::onTick(bool cpuBusy) {
    ++global_.totalTicks;
    if (cpuBusy) ++global_.busyTicks;
    else         ++global_.idleTicks;
}

const TaskStats& StatisticsAnalyzer::getTaskStats(int taskId) const {
    static TaskStats empty{};
    auto it = perTask_.find(taskId);
    return it == perTask_.end() ? empty : it->second;
}

GlobalStats StatisticsAnalyzer::getGlobalStats() const {
    GlobalStats g = global_;
    if (g.totalTicks > 0) {
        g.cpuUtilization  = 100.0 * g.busyTicks / g.totalTicks;
        g.idlePercentage  = 100.0 * g.idleTicks / g.totalTicks;
    }
    return g;
}

std::vector<TaskStats> StatisticsAnalyzer::getAllTaskStats() const {
    std::vector<TaskStats> out;
    out.reserve(perTask_.size());
    for (auto& kv : perTask_) out.push_back(kv.second);
        std::sort(out.begin(), out.end(),
              [](const TaskStats& a, const TaskStats& b){ return a.taskId < b.taskId; });
        return out;
}

void StatisticsAnalyzer::reset() {
    perTask_.clear();
    global_ = GlobalStats{};
}

std::string StatisticsAnalyzer::formatGlobalReport() const {
    GlobalStats g = getGlobalStats();
    std::ostringstream os;
    os << std::fixed << std::setprecision(2);
    os << "Total Ticks        : " << g.totalTicks      << "\n";
    os << "Busy Ticks         : " << g.busyTicks       << "\n";
    os << "Idle Ticks         : " << g.idleTicks       << "\n";
    os << "CPU Utilization    : " << g.cpuUtilization  << " %\n";
    os << "CPU Idle           : " << g.idlePercentage  << " %\n";
    os << "Context Switches   : " << g.contextSwitches << "\n";
    os << "Preemptions        : " << g.preemptions     << "\n";
    os << "Deadline Misses    : " << g.deadlineMisses  << "\n";
    os << "Watchdog Faults    : " << g.watchdogFaults  << "\n";
    os << "IPC Operations     : " << g.ipcOperations   << "\n";
    os << "Mutex Contention   : " << g.mutexContention << "\n";
    return os.str();
}

std::string StatisticsAnalyzer::formatTaskReport(int taskId) const {
    auto it = perTask_.find(taskId);
    if (it == perTask_.end()) return "(no stats for task " + std::to_string(taskId) + ")";
    const TaskStats& s = it->second;
    std::ostringstream os;
    os << "Task " << s.taskId << " [" << s.taskName << "]\n";
    os << "  Completed jobs   : " << s.completedJobs      << "\n";
    os << "  Missed deadlines : " << s.missedDeadlines    << "\n";
    os << "  Execution time   : " << s.totalExecutionTime << "\n";
    os << "  Waiting time     : " << s.totalWaitingTime   << "\n";
    os << "  Last response    : " << s.lastResponseTime   << "\n";
    os << "  Preemptions      : " << s.preemptions        << "\n";
    os << "  Blocks           : " << s.blocks             << "\n";
    return os.str();
}

} // namespace rtos