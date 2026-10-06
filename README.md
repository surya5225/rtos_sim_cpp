# RTOS Kernel Simulator

An educational, software-only **Real-Time Operating System kernel simulator**
written in modern C++17. It demonstrates core OS, real-time systems and
embedded-systems concepts through a clean, object-oriented architecture.

The simulator uses a **deterministic discrete-time tick model** — no OS threads,
no `sleep()`, no hardware dependencies. Every result is reproducible.

---

## 1. Objectives

* Demonstrate OOP (encapsulation, inheritance, polymorphism, composition,
  strategy pattern, state machines).
* Demonstrate data structures (vector, queue, deque, priority queue / min-heap,
  circular buffer, unordered map).
* Demonstrate real-time scheduling algorithms (Priority, Round-Robin, EDF, RMS).
* Demonstrate IPC (semaphores, mutexes with priority inheritance, message
  queues, event flags).
* Demonstrate timing analysis (deadline miss detection, CPU utilization,
  Gantt chart generation).

---

## 2. Architecture

```
Application (CLI / future GUI)
        |
        v
SimulatorController          ← presentation-layer bridge
        |
        v
Kernel                       ← central simulation engine
   ├── TaskManager           ← owns Task objects (TCBs)
   ├── Scheduler             ← strategy pattern (Priority / RR / EDF / RMS)
   ├── TimerManager          ← min-heap of delayed wake-ups
   ├── ResourceManager       ← mutexes, semaphores, queues, event flags
   ├── WatchdogTimer(s)      ← liveness monitors
   ├── StatisticsAnalyzer    ← per-task and global statistics
   └── ExecutionLogger       ← history used for Gantt charts & CSV export
```

---

## 3. OOP Concepts Demonstrated

| Concept            | Where                                                                 |
|--------------------|-----------------------------------------------------------------------|
| Encapsulation      | `Task` — private attributes, public state-transition methods          |
| Abstraction        | Abstract `Scheduler` interface                                        |
| Inheritance        | `PriorityScheduler`, `RoundRobinScheduler`, `EDFScheduler`, `RMSScheduler` |
| Polymorphism       | Kernel calls `scheduler_->selectNextTask(...)` without knowing the concrete type |
| Composition        | Kernel *contains* TaskManager, TimerManager, ResourceManager, …       |
| Strategy pattern   | Scheduling algorithm is swappable at runtime                          |
| State machine      | `TaskState` with validated transitions                                |

---

## 4. Data Structures Demonstrated

| Structure        | Used for                                       | Complexity              |
|------------------|------------------------------------------------|-------------------------|
| `std::vector`    | Execution history, ready-task snapshot         | O(n) scan               |
| `std::queue`     | Semaphore / message-queue wait queues          | O(1) push / pop         |
| `std::priority_queue` | Mutex waiters (priority-ordered)          | O(log n) push / pop     |
| min-heap         | TimerManager, delayed-task wake-ups            | O(log n) insert / extract-min |
| circular buffer  | `MessageQueue`                                 | O(1) send / receive     |
| `std::unordered_map` | Task lookup, resource lookup               | O(1) average            |

---

## 5. Scheduling Algorithms

* **Fixed Priority** — smaller priority number = higher priority.
* **Round Robin** — FIFO queue with a configurable time quantum.
* **Earliest Deadline First** — smallest absolute deadline runs first.
* **Rate Monotonic** — shorter period = higher priority.

All schedulers use a **deterministic tie-breaker**:
criterion → effective priority → release time → task id.

---

## 6. IPC Mechanisms

* **Semaphore** — counting semaphore with FIFO wait queue.
* **Mutex** — ownership, with **priority inheritance** to prevent unbounded
  priority inversion.
* **Message Queue** — bounded FIFO circular buffer; senders block when full,
  receivers block when empty.
* **Event Flags** — 32-bit mask; tasks can wait for ANY or ALL of a set of bits.

---

## 7. Priority Inheritance

```
Low  (priority 5) locks mutex M.
High (priority 1) tries to lock M → blocks.
Low inherits priority 1, preempts Medium.
Low unlocks M → priority restored to 5, High becomes READY.
```

The event log explicitly records `PRIORITY_INHERITANCE` and `PRIORITY_RESTORED`.

---

## 8. Watchdog Timer

A watchdog must be *petted* within its timeout. If not, it expires and the
kernel logs `WATCHDOG_EXPIRED` and increments the watchdog-fault counter.

---

## 9. Deadline Analysis

At every tick the kernel checks every active job:
if `currentTick >= absoluteDeadline` and the job has not completed, a
`DEADLINE_MISS` event is logged, the task's miss counter is incremented,
and the current job is aborted.

---

## 10. CPU Utilization

```
CPU Utilization = busy_ticks / total_ticks * 100
Theoretical U   = Σ (Ci / Ti)   (for periodic task sets)
```

Both are reported in the statistics summary.

---

## 11. Build

```bash
make            # builds ./rtos_simulator
make run        # builds and runs
make clean      # removes build artifacts
```

Requires `g++` with C++17 support.

---

## 12. Run

```bash
./rtos_simulator
```

Menu:

```
1. Run All Demonstration Scenarios
2. Configure & Run Custom Simulation
3. Run Unit Tests
4. Show Last Gantt Chart
5. Show Last Statistics
6. Show Last Event Log
0. Exit
```

---

## 13. Demonstration Scenarios

1. Basic fixed-priority scheduling
2. Earliest Deadline First
3. Rate Monotonic
4. Round Robin (quantum = 2)
5. Priority inversion + inheritance
6. Message queue producer/consumer
7. Semaphore producer/consumer
8. Event flags
9. Watchdog
10. Overloaded task set (deadline misses)

---

## 14. Unit Tests

Built-in tests cover:

* Task creation / deletion / state transitions
* All four schedulers
* Semaphore, Mutex, Priority inheritance
* Message Queue, Event Flags, Watchdog
* Deadline miss detection
* CPU utilization, Gantt history, Timer expiration

Run with menu option **3**.

---

## 15. File Output

After each simulation, the kernel writes into `./output/`:

* `execution_history.csv` — tick-by-tick record
* `gantt_data.csv` — compressed Gantt segments
* `simulation_log.txt` — event log
* `statistics.txt` — global + per-task statistics

---

## 16. Future GUI Architecture

The `SimulatorController` class is the **only** entry point the GUI needs.
It exposes:

```cpp
createPeriodicTask(...)
deleteTask(id)
selectScheduler(policy, quantum)
run(ticks) / step() / reset()
getTasks() / getGanttData() / getStatistics() / getEvents()
```

Suggested GUI stacks:

* **Qt (C++)** — `QTableWidget`, `QChart`, `QComboBox`, `QPushButton`.
* **HTML/JS dashboard** — served by a thin C++ REST layer that calls
  `SimulatorController` and returns JSON.
* **Python (PySide6)** — bind `SimulatorController` with `pybind11`.

The kernel itself remains UI-agnostic.

---

## 17. Complexity Summary

| Operation              | Complexity |
|------------------------|------------|
| Task lookup by id      | O(1)       |
| Task lookup by name    | O(n)       |
| Scheduler selection    | O(n)       |
| Timer insert / extract | O(log n)   |
| Message send / receive | O(1)       |
| Semaphore wait/signal  | O(1)       |
| Mutex lock / unlock    | O(log n)   |

---

## 18. License

Educational project — use freely for teaching and learning.