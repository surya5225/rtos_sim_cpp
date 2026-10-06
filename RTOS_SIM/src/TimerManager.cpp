#include "TimerManager.h"
#include <algorithm>

namespace rtos {

void TimerManager::scheduleDelay(int taskId, int wakeTime) {
    TimerEvent e;
    e.wakeTime = wakeTime;
    e.taskId   = taskId;
    e.timerId  = -1;
    heap_.push_back(e);
    std::push_heap(heap_.begin(), heap_.end(), Compare{});
}

void TimerManager::scheduleTimer(int timerId, int wakeTime, std::function<void()> cb) {
    TimerEvent e;
    e.wakeTime = wakeTime;
    e.taskId   = -1;
    e.timerId  = timerId;
    e.callback = std::move(cb);
    heap_.push_back(e);
    std::push_heap(heap_.begin(), heap_.end(), Compare{});
}

void TimerManager::tick(int currentTime,
                        std::function<void(int)> onTaskWake,
                        std::function<void(int)> onTimerFire) {
    while (!heap_.empty() && heap_.front().wakeTime <= currentTime) {
        std::pop_heap(heap_.begin(), heap_.end(), Compare{});
        TimerEvent e = std::move(heap_.back());
        heap_.pop_back();
        if (e.taskId >= 0 && onTaskWake) onTaskWake(e.taskId);
        else if (e.timerId >= 0 && e.callback && onTimerFire) onTimerFire(e.timerId);
    }
}

void TimerManager::clear() { heap_.clear(); }

} // namespace rtos