#include "TaskManager.h"
#include <algorithm>
namespace rtos {

Task* TaskManager::createPeriodicTask(int id, const std::string& name, int priority,
                                      int executionTime, int period, int relativeDeadline) {
    if (tasks_.count(id)) throw DuplicateTaskId(id);
    auto t = std::make_unique<Task>(id, name, priority, executionTime, period, relativeDeadline);
    Task* raw = t.get();
    tasks_[id] = std::move(t);
    return raw;
}

Task* TaskManager::createAperiodicTask(int id, const std::string& name, int priority,
                                       int executionTime) {
    if (tasks_.count(id)) throw DuplicateTaskId(id);
    auto t = std::make_unique<Task>(id, name, priority, executionTime);
    Task* raw = t.get();
    tasks_[id] = std::move(t);
    return raw;
}

void TaskManager::removeTask(int id) {
    auto it = tasks_.find(id);
    if (it == tasks_.end()) throw InvalidTaskId(id);
    tasks_.erase(it);
}

Task* TaskManager::getTask(int id) const {
    auto it = tasks_.find(id);
    return it == tasks_.end() ? nullptr : it->second.get();
}

Task* TaskManager::getTaskByName(const std::string& name) const {
    for (auto& kv : tasks_)
        if (kv.second->getName() == name) return kv.second.get();
    return nullptr;
}

std::vector<Task*> TaskManager::allTasks() const {
    std::vector<Task*> out;
    out.reserve(tasks_.size());
    for (auto& kv : tasks_) out.push_back(kv.second.get());
    // Sort by id for deterministic iteration order.
    std::sort(out.begin(), out.end(),
              [](const Task* a, const Task* b){ return a->getId() < b->getId(); });
    return out;
}

std::vector<Task*> TaskManager::readyTasks() const {
    std::vector<Task*> out;
    for (auto& kv : tasks_)
        if (kv.second->getState() == TaskState::READY) out.push_back(kv.second.get());
    std::sort(out.begin(), out.end(),
              [](const Task* a, const Task* b){ return a->getId() < b->getId(); });
    return out;
}

std::vector<Task*> TaskManager::blockedTasks() const {
    std::vector<Task*> out;
    for (auto& kv : tasks_)
        if (kv.second->getState() == TaskState::BLOCKED) out.push_back(kv.second.get());
    return out;
}

std::vector<Task*> TaskManager::suspendedTasks() const {
    std::vector<Task*> out;
    for (auto& kv : tasks_)
        if (kv.second->getState() == TaskState::SUSPENDED) out.push_back(kv.second.get());
    return out;
}

void TaskManager::resetAll() { tasks_.clear(); }

} // namespace rtos