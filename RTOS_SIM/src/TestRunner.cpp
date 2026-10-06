#include "TestRunner.h"
#include "Kernel.h"
#include <iostream>
#include <cmath>

namespace rtos {

void TestRunner::addResult(const std::string& name, bool passed, const std::string& msg) {
    results_.push_back({name, passed, msg});
}

int TestRunner::passedCount() const {
    int n = 0; for (auto& r : results_) if (r.passed) ++n; return n;
}
int TestRunner::failedCount() const {
    int n = 0; for (auto& r : results_) if (!r.passed) ++n; return n;
}

void TestRunner::printReport() const {
    for (const auto& r : results_) {
        std::cout << (r.passed ? "[PASS] " : "[FAIL] ") << r.name;
        if (!r.passed && !r.message.empty()) std::cout << " — " << r.message;
        std::cout << "\n";
    }
    std::cout << "\nTests Passed: " << passedCount()
              << "\nTests Failed: " << failedCount() << "\n";
}

void TestRunner::runAll() {
    // 1. Task creation
    try {
        Kernel k;
        Task* t = k.addPeriodicTask(1, "T1", 2, 3, 10, 10);
        addResult("Task Creation", t && t->getId() == 1 && t->getName() == "T1");
    } catch (...) { addResult("Task Creation", false, "exception"); }

    // 2. Duplicate task id
    try {
        Kernel k;
        k.addPeriodicTask(1, "A", 1, 2, 10, 10);
        k.addPeriodicTask(1, "B", 2, 2, 10, 10);
        addResult("Duplicate Task Id Rejected", false);
    } catch (const DuplicateTaskId&) {
        addResult("Duplicate Task Id Rejected", true);
    } catch (...) { addResult("Duplicate Task Id Rejected", false, "wrong exception"); }

    // 3. Task state transitions
    try {
        Kernel k;
        Task* t = k.addPeriodicTask(1, "T", 1, 2, 10, 10);
        bool ok = (t->getState() == TaskState::DORMANT);
        t->release(0);
        ok = ok && (t->getState() == TaskState::READY);
        t->setState(TaskState::RUNNING);
        ok = ok && (t->getState() == TaskState::RUNNING);
        t->block(BlockReason::MUTEX);
        ok = ok && (t->getState() == TaskState::BLOCKED);
        t->unblock();
        ok = ok && (t->getState() == TaskState::READY);
        addResult("Task State Transitions", ok);
    } catch (...) { addResult("Task State Transitions", false, "exception"); }

    // 4. Priority scheduling
    try {
        Kernel k;
        k.setScheduler(std::make_unique<PriorityScheduler>());
        k.addPeriodicTask(1, "Low",    5, 2, 20, 20);
        k.addPeriodicTask(2, "Medium", 3, 2, 20, 20);
        k.addPeriodicTask(3, "High",   1, 2, 20, 20);
        k.tick(); // tick 0: all three released, High should run
        Task* high = k.getTask(3);
        addResult("Priority Scheduler", high && high->getState() == TaskState::RUNNING);
    } catch (...) { addResult("Priority Scheduler", false, "exception"); }

    // 5. EDF scheduling
    try {
        Kernel k;
        k.setScheduler(std::make_unique<EDFScheduler>());
        k.addPeriodicTask(1, "A", 3, 2, 20, 20);
        k.addPeriodicTask(2, "B", 2, 2, 15, 15);
        k.addPeriodicTask(3, "C", 1, 2, 25, 25);
        k.tick();
        Task* b = k.getTask(2);
        addResult("EDF Scheduler", b && b->getState() == TaskState::RUNNING);
    } catch (...) { addResult("EDF Scheduler", false, "exception"); }

    // 6. RMS scheduling
    try {
        Kernel k;
        k.setScheduler(std::make_unique<RMSScheduler>());
        k.addPeriodicTask(1, "LongPeriod",  5, 2, 30, 30);
        k.addPeriodicTask(2, "MediumPeriod",4, 2, 20, 20);
        k.addPeriodicTask(3, "ShortPeriod", 3, 2, 10, 10);
        k.tick();
        Task* s = k.getTask(3);
        addResult("RMS Scheduler", s && s->getState() == TaskState::RUNNING);
    } catch (...) { addResult("RMS Scheduler", false, "exception"); }

    // 7. Round Robin
    try {
        Kernel k;
        k.setScheduler(std::make_unique<RoundRobinScheduler>(2));
        k.addPeriodicTask(1, "A", 1, 4, 20, 20);
        k.addPeriodicTask(2, "B", 1, 4, 20, 20);
        k.tick(); // tick 0: A runs
        k.tick(); // tick 1: A runs (quantum = 2)
        Task* a = k.getTask(1);
        Task* b = k.getTask(2);
        // After tick 1, A's quantum is exhausted; next tick B should run.
        k.tick(); // tick 2: B runs
        bool ok = (b->getState() == TaskState::RUNNING) &&
                  (a->getState() == TaskState::READY);
        addResult("Round Robin Scheduler", ok);
    } catch (...) { addResult("Round Robin Scheduler", false, "exception"); }

    // 8. Semaphore
    try {
        Kernel k;
        k.addPeriodicTask(1, "P", 1, 2, 20, 20);
        k.addPeriodicTask(2, "C", 2, 2, 20, 20);
        Semaphore* s = k.createSemaphore("S", 1);
        Task* p = k.getTask(1); p->release(0);
        Task* c = k.getTask(2); c->release(0);
        bool a = k.semaphoreWait("S", p);
        bool b = k.semaphoreWait("S", c);
        addResult("Semaphore", a && !b && c->getState() == TaskState::BLOCKED && s->getCount() == 0);
    } catch (...) { addResult("Semaphore", false, "exception"); }

    // 9. Mutex
    try {
        Kernel k;
        k.addPeriodicTask(1, "P", 1, 2, 20, 20);
        k.addPeriodicTask(2, "C", 2, 2, 20, 20);
        Mutex* m = k.createMutex("M");
        Task* p = k.getTask(1); p->release(0);
        Task* c = k.getTask(2); c->release(0);
        bool a = k.mutexLock("M", p);
        bool b = k.mutexLock("M", c);
        addResult("Mutex Lock", a && !b && c->getState() == TaskState::BLOCKED && m->getOwner() == p);
    } catch (...) { addResult("Mutex Lock", false, "exception"); }

    // 10. Priority inheritance
    try {
        Kernel k;
        k.setScheduler(std::make_unique<PriorityScheduler>());
        Task* L = k.addPeriodicTask(1, "Low",    5, 4, 40, 40);
        Task* M = k.addPeriodicTask(2, "Medium", 3, 4, 40, 40);
        Task* H = k.addPeriodicTask(3, "High",   1, 2, 40, 40);
        Mutex* m = k.createMutex("M");
        (void)m; // resource registered by name; pointer unused
        // L acquires mutex at tick 0.
        L->release(0); k.tick();
        // H arrives at tick 1 and tries to lock the mutex.
        H->release(1); k.tick();
        bool inherited = (L->getEffectivePriority() == 1);
        // M arrives at tick 2 — should NOT preempt L because L inherited priority 1.
        M->release(2); k.tick();
        bool noPreemptByM = (L->getState() == TaskState::RUNNING);
        addResult("Priority Inheritance", inherited && noPreemptByM);
    } catch (...) { addResult("Priority Inheritance", false, "exception"); }

    // 11. Message queue
    try {
        Kernel k;
        k.addPeriodicTask(1, "P", 1, 2, 20, 20);
        k.addPeriodicTask(2, "C", 2, 2, 20, 20);
        MessageQueue* q = k.createMessageQueue("Q", 2);
        (void)q; // resource registered by name; pointer unused
        Task* p = k.getTask(1); p->release(0);
        Task* c = k.getTask(2); c->release(0);
        Message m{1, 2, "hello", 0};
        bool sent = k.messageSend("Q", m, p);
        Message out;
        bool recv = k.messageReceive("Q", out, c);
        addResult("Message Queue", sent && recv && out.payload == "hello");
    } catch (...) { addResult("Message Queue", false, "exception"); }

    // 12. Event flags
    try {
        Kernel k;
        k.addPeriodicTask(1, "W", 1, 2, 20, 20);
        EventFlags* e = k.createEventFlags("E");
        (void)e; // resource registered by name; pointer unused
        Task* w = k.getTask(1); w->release(0);
        bool immediate = k.eventWaitAny("E", 0x01, w); // not set yet
        bool blocked = (w->getState() == TaskState::BLOCKED);
        k.eventSet("E", 0x01);
        bool woke = (w->getState() == TaskState::READY);
        addResult("Event Flags", !immediate && blocked && woke);
    } catch (...) { addResult("Event Flags", false, "exception"); }

    // 13. Watchdog
    try {
        Kernel k;
        k.addPeriodicTask(1, "P", 1, 2, 20, 20);
        WatchdogTimer* w = k.createWatchdog("WD", 3);
        w->start(0);
        w->pet(0);
        w->tick(1); bool ok1 = !w->isExpired();
        w->tick(2); bool ok2 = !w->isExpired();
        w->tick(3); bool ok3 =  w->isExpired();
        addResult("Watchdog", ok1 && ok2 && ok3);
    } catch (...) { addResult("Watchdog", false, "exception"); }

    // 14. Deadline miss detection
    try {
        Kernel k;
        // Task needs 5 ticks but deadline is 3 — guaranteed miss.
        k.addPeriodicTask(1, "Over", 1, 5, 20, 3);
        k.run(10);
        Task* t = k.getTask(1);
        addResult("Deadline Miss Detection", t->getMissedDeadlines() >= 1);
    } catch (...) { addResult("Deadline Miss Detection", false, "exception"); }

    // 15. CPU utilization
    try {
        Kernel k;
        k.addPeriodicTask(1, "T", 1, 5, 10, 10);
        k.run(10);
        auto g = k.getStats().getGlobalStats();
        bool ok = (g.totalTicks == 10) && (g.busyTicks == 5) && (g.idleTicks == 5);
        addResult("CPU Utilization", ok);
    } catch (...) { addResult("CPU Utilization", false, "exception"); }

    // 16. Gantt history
    try {
        Kernel k;
        k.addPeriodicTask(1, "T", 1, 3, 10, 10);
        k.run(5);
        auto segs = k.getExecutionLogger().buildGanttSegments();
        bool ok = !segs.empty();
        addResult("Gantt History", ok);
    } catch (...) { addResult("Gantt History", false, "exception"); }

    // 17. Timer expiration (taskDelay)
    try {
        Kernel k;
        k.addPeriodicTask(1, "A", 1, 2, 30, 30);
        Task* a = k.getTask(1); a->release(0);
        k.tick(); // tick 0: A runs
        k.taskDelay(a, 3); // wake at tick 4
        bool blocked = (a->getState() == TaskState::BLOCKED);
        k.tick(); k.tick(); k.tick(); // ticks 1, 2, 3
        bool stillBlocked = (a->getState() == TaskState::BLOCKED);
        k.tick(); // tick 4: wake
        bool ready = (a->getState() == TaskState::READY);
        addResult("Timer Expiration", blocked && stillBlocked && ready);
    } catch (...) { addResult("Timer Expiration", false, "exception"); }
}

} // namespace rtos