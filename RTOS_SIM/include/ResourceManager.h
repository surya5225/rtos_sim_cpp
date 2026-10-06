// ResourceManager.h — Aggregates IPC resources (mutexes, semaphores, queues,
// event flags) so the kernel can access them through one facade.
#pragma once

#include "Semaphore.h"
#include "Mutex.h"
#include "MessageQueue.h"
#include "EventFlags.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace rtos {

class ResourceManager {
public:
    Semaphore*    createSemaphore(const std::string& name, int initial);
    Mutex*        createMutex(const std::string& name);
    MessageQueue* createMessageQueue(const std::string& name, std::size_t capacity);
    EventFlags*   createEventFlags(const std::string& name);

    Semaphore*    getSemaphore(const std::string& name) const;
    Mutex*        getMutex(const std::string& name) const;
    MessageQueue* getMessageQueue(const std::string& name) const;
    EventFlags*   getEventFlags(const std::string& name) const;

    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<Semaphore>>    semaphores_;
    std::unordered_map<std::string, std::unique_ptr<Mutex>>        mutexes_;
    std::unordered_map<std::string, std::unique_ptr<MessageQueue>> queues_;
    std::unordered_map<std::string, std::unique_ptr<EventFlags>>   events_;
};

} // namespace rtos