// ExecutionLogger.h — Stores the execution history used for Gantt charts,
// CSV export and the event log.
#pragma once

#include "Common.h"
#include <vector>
#include <string>

namespace rtos {

class ExecutionLogger {
public:
    void record(int tick, int taskId, const std::string& taskName,
                TaskState state, const std::string& event);

    void recordEvent(int tick, const std::string& event);

    const std::vector<ExecutionRecord>& getHistory() const { return history_; }
    const std::vector<std::pair<int,std::string>>& getEvents() const { return events_; }

    std::vector<GanttSegment> buildGanttSegments() const;

    void exportCsv(const std::string& path) const;
    void exportGanttCsv(const std::string& path) const;
    void exportEvents(const std::string& path) const;

    void clear();

private:
    std::vector<ExecutionRecord> history_;
    std::vector<std::pair<int,std::string>> events_;
};

} // namespace rtos