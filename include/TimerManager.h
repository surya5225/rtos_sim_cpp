// TimerManager.h — Manages delayed-task wake-ups and one-shot timers.
// Uses a min-heap ordered by wake-up time for O(log n) insert / extract-min.
#pragma once

#include "Task.h"
#include <vector>
#include <functional>

namespace rtos {

struct TimerEvent {
    int  wakeTime{0};
    int  taskId{-1};
    int  timerId{-1};
    std::function<void()> callback;
};

class TimerManager {
public:
    // Schedule a task to wake at wakeTime.
    void scheduleDelay(int taskId, int wakeTime);

    // Schedule a one-shot timer with a callback.
    void scheduleTimer(int timerId, int wakeTime, std::function<void()> cb);

    // Process all timers whose wakeTime <= currentTime.
    void tick(int currentTime,
              std::function<void(int)> onTaskWake,
              std::function<void(int)> onTimerFire);

    bool empty() const { return heap_.empty(); }
    void clear();

private:
    struct Compare {
        bool operator()(const TimerEvent& a, const TimerEvent& b) const {
            // std::priority_queue is a max-heap; we want a min-heap.
            if (a.wakeTime != b.wakeTime) return a.wakeTime > b.wakeTime;
            return a.taskId > b.taskId; // deterministic tie-break
        }
    };
    std::vector<TimerEvent> heap_;
};

} // namespace rtos