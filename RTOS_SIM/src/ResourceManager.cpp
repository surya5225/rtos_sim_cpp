#include "ResourceManager.h"

namespace rtos {

Semaphore* ResourceManager::createSemaphore(const std::string& name, int initial) {
    auto s = std::make_unique<Semaphore>(initial, name);
    Semaphore* raw = s.get();
    semaphores_[name] = std::move(s);
    return raw;
}

Mutex* ResourceManager::createMutex(const std::string& name) {
    auto m = std::make_unique<Mutex>(name);
    Mutex* raw = m.get();
    mutexes_[name] = std::move(m);
    return raw;
}

MessageQueue* ResourceManager::createMessageQueue(const std::string& name, std::size_t cap) {
    auto q = std::make_unique<MessageQueue>(cap, name);
    MessageQueue* raw = q.get();
    queues_[name] = std::move(q);
    return raw;
}

EventFlags* ResourceManager::createEventFlags(const std::string& name) {
    auto e = std::make_unique<EventFlags>(name);
    EventFlags* raw = e.get();
    events_[name] = std::move(e);
    return raw;
}

Semaphore*    ResourceManager::getSemaphore(const std::string& n) const {
    auto it = semaphores_.find(n); return it == semaphores_.end() ? nullptr : it->second.get();
}
Mutex*        ResourceManager::getMutex(const std::string& n) const {
    auto it = mutexes_.find(n); return it == mutexes_.end() ? nullptr : it->second.get();
}
MessageQueue* ResourceManager::getMessageQueue(const std::string& n) const {
    auto it = queues_.find(n); return it == queues_.end() ? nullptr : it->second.get();
}
EventFlags*   ResourceManager::getEventFlags(const std::string& n) const {
    auto it = events_.find(n); return it == events_.end() ? nullptr : it->second.get();
}

void ResourceManager::clear() {
    semaphores_.clear(); mutexes_.clear(); queues_.clear(); events_.clear();
}

} // namespace rtos