#include "ExecutionLogger.h"
#include <fstream>
#include <sstream>

namespace rtos {

void ExecutionLogger::record(int tick, int taskId, const std::string& taskName,
                             TaskState state, const std::string& event) {
    history_.push_back({tick, taskId, taskName, state, event});
}

void ExecutionLogger::recordEvent(int tick, const std::string& event) {
    events_.emplace_back(tick, event);
}

std::vector<GanttSegment> ExecutionLogger::buildGanttSegments() const {
    std::vector<GanttSegment> out;
    if (history_.empty()) return out;

    GanttSegment cur;
    cur.startTick = history_.front().tick;
    cur.endTick   = cur.startTick + 1;
    cur.taskId    = history_.front().taskId;
    cur.taskName  = history_.front().taskName;

    for (std::size_t i = 1; i < history_.size(); ++i) {
        const auto& r = history_[i];
        if (r.taskId == cur.taskId && r.tick == cur.endTick) {
            cur.endTick = r.tick + 1;
        } else {
            out.push_back(cur);
            cur.startTick = r.tick;
            cur.endTick   = r.tick + 1;
            cur.taskId    = r.taskId;
            cur.taskName  = r.taskName;
        }
    }
    out.push_back(cur);
    return out;
}

void ExecutionLogger::exportCsv(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;
    f << "tick,task_id,task_name,state,event\n";
    for (const auto& r : history_) {
        f << r.tick << "," << r.taskId << "," << r.taskName << ","
          << toString(r.state) << "," << r.event << "\n";
    }
}

void ExecutionLogger::exportGanttCsv(const std::string& path) const {
    auto segs = buildGanttSegments();
    std::ofstream f(path);
    if (!f) return;
    f << "start_tick,end_tick,task_id,task_name\n";
    for (const auto& s : segs) {
        f << s.startTick << "," << s.endTick << "," << s.taskId << ","
          << s.taskName << "\n";
    }
}

void ExecutionLogger::exportEvents(const std::string& path) const {
    std::ofstream f(path);
    if (!f) return;
    for (const auto& e : events_) f << "[" << e.first << "] " << e.second << "\n";
}

void ExecutionLogger::clear() { history_.clear(); events_.clear(); }

} // namespace rtos