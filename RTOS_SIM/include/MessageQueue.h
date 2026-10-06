// MessageQueue.h — Bounded FIFO circular buffer.
// Capacity is fixed at construction. When full, senders block;
// when empty, receivers block.
#pragma once

#include "Common.h"
#include "Task.h"
#include <vector>
#include <queue>
#include <string>

namespace rtos {

class MessageQueue {
public:
    explicit MessageQueue(std::size_t capacity, const std::string& name = "");

    // Returns true if the message was enqueued immediately.
    // Returns false if the queue is full and the sender must block.
    bool send(const Message& msg, Task* sender);

    // Returns true if a message was delivered to `out`.
    // Returns false if the queue is empty and the receiver must block.
    bool receive(Message& out, Task* receiver);

    // Unblock the next waiting sender (after a receive frees a slot).
    Task* popWaitingSender();
    // Unblock the next waiting receiver (after a send adds a message).
    Task* popWaitingReceiver();

    bool        isEmpty() const { return size_ == 0; }
    bool        isFull()  const { return size_ == capacity_; }
    std::size_t size()    const { return size_; }
    std::size_t capacity() const { return capacity_; }
    const std::string& getName() const { return name_; }

private:
    std::vector<Message> buffer_;
    std::size_t capacity_;
    std::size_t head_{0};
    std::size_t tail_{0};
    std::size_t size_{0};
    std::string name_;
    std::queue<Task*> sendWaiters_;
    std::queue<Task*> recvWaiters_;
};

} // namespace rtos