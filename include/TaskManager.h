// TaskManager.h — Owns all Task objects.
// Uses std::unordered_map<int, std::unique_ptr<Task>> for O(1) lookup by id.
#pragma once

#include "Task.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace rtos {

class TaskManager {
public:
    // Create a periodic task. Returns the raw pointer for convenience.
    Task* createPeriodicTask(int id, const std::string& name, int priority,
                             int executionTime, int period, int relativeDeadline);

    // Create an aperiodic task.
    Task* createAperiodicTask(int id, const std::string& name, int priority,
                              int executionTime);

    void  removeTask(int id);
    Task* getTask(int id) const;
    Task* getTaskByName(const std::string& name) const;

    std::vector<Task*> allTasks() const;
    std::vector<Task*> readyTasks() const;
    std::vector<Task*> blockedTasks() const;
    std::vector<Task*> suspendedTasks() const;

    void resetAll();
    std::size_t size() const { return tasks_.size(); }

private:
    std::unordered_map<int, std::unique_ptr<Task>> tasks_;
};

} // namespace rtos