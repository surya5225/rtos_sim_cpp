// main.cpp — Educational CLI for the RTOS Kernel Simulator.
#include "SimulatorController.h"
#include "TestRunner.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>
#include <filesystem>

using namespace rtos;

// ---------- Demonstration scenarios ----------
namespace scenarios {

void scenario1_BasicScheduling(SimulatorController& c) {
    std::cout << "\n=== Scenario 1: Basic Fixed-Priority Scheduling ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::PRIORITY);
    c.createPeriodicTask(1, "Sensor",     1, 2, 10, 10);
    c.createPeriodicTask(2, "Controller", 2, 3, 15, 15);
    c.createPeriodicTask(3, "Logger",     4, 2, 20, 20);
    c.run(50);
}

void scenario2_EDF(SimulatorController& c) {
    std::cout << "\n=== Scenario 2: Earliest Deadline First ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::EDF);
    c.createPeriodicTask(1, "T1", 3, 2, 20, 20);
    c.createPeriodicTask(2, "T2", 2, 2, 15, 15);
    c.createPeriodicTask(3, "T3", 1, 2, 25, 25);
    c.run(50);
}

void scenario3_RMS(SimulatorController& c) {
    std::cout << "\n=== Scenario 3: Rate Monotonic ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::RMS);
    c.createPeriodicTask(1, "Fast",   5, 1, 5,  5);
    c.createPeriodicTask(2, "Medium", 4, 2, 10, 10);
    c.createPeriodicTask(3, "Slow",   3, 3, 25, 25);
    c.run(50);
}

void scenario4_RoundRobin(SimulatorController& c) {
    std::cout << "\n=== Scenario 4: Round Robin (quantum=2) ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::ROUND_ROBIN, 2);
    c.createPeriodicTask(1, "A", 1, 6, 30, 30);
    c.createPeriodicTask(2, "B", 1, 6, 30, 30);
    c.createPeriodicTask(3, "C", 1, 6, 30, 30);
    c.run(30);
}

void scenario5_PriorityInversion(SimulatorController& c) {
    std::cout << "\n=== Scenario 5: Priority Inheritance ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::PRIORITY);
    c.createPeriodicTask(1, "Low",    5, 6, 60, 60);
    c.createPeriodicTask(2, "Medium", 3, 4, 60, 60);
    c.createPeriodicTask(3, "High",   1, 2, 60, 60);
    c.kernel().createMutex("SHARED");

    Kernel& k = c.kernel();
    // Manually script the classic priority-inversion scenario.
    Task* L = k.getTask(1);
    Task* M = k.getTask(2);
    Task* H = k.getTask(3);

    L->release(0);  k.tick(); // tick 0: L runs, acquires mutex
    k.mutexLock("SHARED", L);
    H->release(1);  k.tick(); // tick 1: H arrives, blocks on mutex -> L inherits priority 1
    M->release(2);  k.tick(); // tick 2: M cannot preempt L (L has effective priority 1)
    // L finishes its job and releases the mutex.
    while (!L->isCompleted() && k.getCurrentTick() < 10) k.tick();
    k.mutexUnlock("SHARED", L);
    k.run(20); // let H and M finish
}

void scenario6_MessageQueue(SimulatorController& c) {
    std::cout << "\n=== Scenario 6: Message Queue ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::PRIORITY);
    c.createPeriodicTask(1, "Producer", 1, 1, 5, 5);
    c.createPeriodicTask(2, "Consumer", 2, 1, 5, 5);
    c.kernel().createMessageQueue("MQ", 4);

    Kernel& k = c.kernel();
    Task* p = k.getTask(1);
    Task* cs = k.getTask(2);
    for (int i = 0; i < 20; ++i) {
        Message m{1, 2, "sample-" + std::to_string(i), k.getCurrentTick()};
        k.messageSend("MQ", m, p);
        Message out;
        k.messageReceive("MQ", out, cs);
        k.tick();
    }
}

void scenario7_Semaphore(SimulatorController& c) {
    std::cout << "\n=== Scenario 7: Semaphore Producer/Consumer ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::PRIORITY);
    c.createPeriodicTask(1, "Prod", 1, 1, 4, 4);
    c.createPeriodicTask(2, "Cons", 2, 1, 4, 4);
    c.kernel().createSemaphore("S", 0);

    Kernel& k = c.kernel();
    Task* p = k.getTask(1);
    Task* cs = k.getTask(2);
    for (int i = 0; i < 10; ++i) {
        k.semaphoreSignal("S");
        k.semaphoreWait("S", cs);
        k.tick();
    }
}

void scenario8_EventFlags(SimulatorController& c) {
    std::cout << "\n=== Scenario 8: Event Flags ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::PRIORITY);
    c.createPeriodicTask(1, "Waiter", 1, 2, 20, 20);
    c.kernel().createEventFlags("EVT");

    Kernel& k = c.kernel();
    Task* w = k.getTask(1);
    w->release(0);
    k.eventWaitAny("EVT", 0x03, w); // wait for bits 0 and 1
    k.tick(); // waiter blocks
    k.eventSet("EVT", 0x01);
    k.tick(); // still waiting for bit 1
    k.eventSet("EVT", 0x02);
    k.run(10); // waiter wakes and runs
}

void scenario9_Watchdog(SimulatorController& c) {
    std::cout << "\n=== Scenario 9: Watchdog ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::PRIORITY);
    c.createPeriodicTask(1, "Pet",    1, 1, 5, 5);
    c.createPeriodicTask(2, "Slacker",2, 1, 5, 5);
    WatchdogTimer* wd = c.kernel().createWatchdog("WD", 4);
    wd->start(0);

    Kernel& k = c.kernel();
    Task* pet = k.getTask(1);
    for (int i = 0; i < 20; ++i) {
        if (i % 3 == 0) wd->pet(k.getCurrentTick()); // pet regularly
        k.tick();
    }
}

void scenario10_DeadlineMiss(SimulatorController& c) {
    std::cout << "\n=== Scenario 10: Overloaded Task Set (Deadline Miss) ===\n";
    c.reset();
    c.selectScheduler(SchedulingPolicy::EDF);
    // Total utilization > 1 — guaranteed misses.
    c.createPeriodicTask(1, "A", 1, 6, 10, 10);
    c.createPeriodicTask(2, "B", 2, 6, 10, 10);
    c.run(30);
}

void runAllScenarios() {
    SimulatorController c;
    scenario1_BasicScheduling(c);
    scenario2_EDF(c);
    scenario3_RMS(c);
    scenario4_RoundRobin(c);
    scenario5_PriorityInversion(c);
    scenario6_MessageQueue(c);
    scenario7_Semaphore(c);
    scenario8_EventFlags(c);
    scenario9_Watchdog(c);
    scenario10_DeadlineMiss(c);

    std::cout << "\n==================== FINAL SUMMARY ====================\n";
    std::cout << c.getStatistics().totalTicks << " ticks simulated in last scenario.\n";
    std::cout << "\nEvents from last scenario:\n";
    for (const auto& e : c.getEvents()) {
        std::cout << "  [" << e.first << "] " << e.second << "\n";
    }
    std::cout << "\nGantt chart (last scenario):\n";
    auto segs = c.getGanttData();
    for (const auto& s : segs) {
        std::cout << "  " << s.startTick << "-" << (s.endTick - 1) << "  " << s.taskName << "\n";
    }

    std::filesystem::create_directories("output");
    c.kernel().exportFiles("output");
    std::cout << "\nFiles exported to ./output/\n";
}

// ---------- CLI ----------
void printHeader() {
    std::cout << "\n===========================================\n";
    std::cout << "        RTOS KERNEL SIMULATOR\n";
    std::cout << "===========================================\n";
}

void printMenu() {
    std::cout << "\n 1. Run All Demonstration Scenarios\n";
    std::cout << " 2. Configure & Run Custom Simulation\n";
    std::cout << " 3. Run Unit Tests\n";
    std::cout << " 4. Show Last Gantt Chart\n";
    std::cout << " 5. Show Last Statistics\n";
    std::cout << " 6. Show Last Event Log\n";
    std::cout << " 0. Exit\n";
    std::cout << "\n> ";
}

void runCustomSimulation() {
    SimulatorController c;
    std::cout << "Select scheduler (1=Priority 2=RR 3=EDF 4=RMS): ";
    int s; std::cin >> s;
    SchedulingPolicy pol = SchedulingPolicy::PRIORITY;
    int quantum = 2;
    if (s == 2) { pol = SchedulingPolicy::ROUND_ROBIN; std::cout << "Quantum: "; std::cin >> quantum; }
    else if (s == 3) pol = SchedulingPolicy::EDF;
    else if (s == 4) pol = SchedulingPolicy::RMS;
    c.selectScheduler(pol, quantum);

    std::cout << "Simulation ticks: "; int ticks; std::cin >> ticks;
    std::cout << "Number of tasks: "; int n; std::cin >> n;
    for (int i = 0; i < n; ++i) {
        int id, prio, exec, period, deadline;
        std::string name;
        std::cout << "Task " << (i+1) << " id name priority exec period deadline: ";
        std::cin >> id >> name >> prio >> exec >> period >> deadline;
        c.createPeriodicTask(id, name, prio, exec, period, deadline);
    }
    c.run(ticks);

    std::cout << "\n--- Gantt ---\n";
    for (const auto& seg : c.getGanttData())
        std::cout << seg.startTick << "-" << (seg.endTick - 1) << "  " << seg.taskName << "\n";

    std::cout << "\n--- Statistics ---\n";
    auto g = c.getStatistics();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "CPU Utilization : " << g.cpuUtilization << " %\n";
    std::cout << "Context Switches: " << g.contextSwitches << "\n";
    std::cout << "Deadline Misses : " << g.deadlineMisses << "\n";

    std::filesystem::create_directories("output");
    c.kernel().exportFiles("output");
    std::cout << "Files exported to ./output/\n";
}

int main() {
    printHeader();
    std::string choice;
    SimulatorController lastController;
    bool haveLast = false;

    while (true) {
        printMenu();
        std::cin >> choice;
        if (choice == "1") {
            runAllScenarios();
        } else if (choice == "2") {
            runCustomSimulation();
        } else if (choice == "3") {
            TestRunner tr;
            tr.runAll();
            tr.printReport();
        } else if (choice == "0") {
            std::cout << "Goodbye.\n";
            break;
        } else {
            std::cout << "Unknown option.\n";
        }
    }
    return 0;
}
}