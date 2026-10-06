#include "MessageQueue.h"

namespace rtos {

MessageQueue::MessageQueue(std::size_t capacity, const std::string& name)
    : capacity_(capacity), name_(name) {
    if (capacity == 0) throw InvalidParameter("queue capacity must be > 0");
    buffer_.resize(capacity);
}

bool MessageQueue::send(const Message& msg, Task* sender) {
    if (isFull()) {
        sendWaiters_.push(sender);
        return false;
    }
    buffer_[tail_] = msg;
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
    return true;
}

bool MessageQueue::receive(Message& out, Task* receiver) {
    if (isEmpty()) {
        recvWaiters_.push(receiver);
        return false;
    }
    out = buffer_[head_];
    head_ = (head_ + 1) % capacity_;
    --size_;
    return true;
}

Task* MessageQueue::popWaitingSender() {
    if (sendWaiters_.empty()) return nullptr;
    Task* t = sendWaiters_.front();
    sendWaiters_.pop();
    return t;
}

Task* MessageQueue::popWaitingReceiver() {
    if (recvWaiters_.empty()) return nullptr;
    Task* t = recvWaiters_.front();
    recvWaiters_.pop();
    return t;
}

} // namespace rtos