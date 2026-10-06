// Common.h — Shared enums, structs and constants used across the simulator.
// Centralizing these types avoids circular header dependencies and keeps the
// kernel API consistent.
#pragma once

#include <string>
#include <cstdint>
#include <stdexcept>

namespace rtos {

// ---------- Enumerations ----------
enum class TaskState {
    DORMANT,     // Task created but not yet released
    READY,       // Eligible to run, waiting for CPU
    RUNNING,     // Currently executing on the CPU
    BLOCKED,     // Waiting on a resource / event / delay
    SUSPENDED,   // Administratively suspended by the user
    TERMINATED   // Job completed; waiting for next period or removal
};

enum class SchedulingPolicy {
    PRIORITY,
    ROUND_ROBIN,
    EDF,
    RMS
};

enum class BlockReason {
    NONE,
    SEMAPHORE,
    MUTEX,
    MESSAGE_QUEUE_SEND,
    MESSAGE_QUEUE_RECEIVE,
    EVENT_FLAG,
    DELAY
};

inline const char* toString(TaskState s) {
    switch (s) {
        case TaskState::DORMANT:     return "DORMANT";
        case TaskState::READY:       return "READY";
        case TaskState::RUNNING:     return "RUNNING";
        case TaskState::BLOCKED:     return "BLOCKED";
        case TaskState::SUSPENDED:   return "SUSPENDED";
        case TaskState::TERMINATED:  return "TERMINATED";
    }
    return "UNKNOWN";
}

inline const char* toString(SchedulingPolicy p) {
    switch (p) {
        case SchedulingPolicy::PRIORITY:    return "FIXED PRIORITY";
        case SchedulingPolicy::ROUND_ROBIN: return "ROUND ROBIN";
        case SchedulingPolicy::EDF:         return "EARLIEST DEADLINE FIRST";
        case SchedulingPolicy::RMS:         return "RATE MONOTONIC";
    }
    return "UNKNOWN";
}

inline const char* toString(BlockReason r) {
    switch (r) {
        case BlockReason::NONE:                 return "NONE";
        case BlockReason::SEMAPHORE:            return "SEMAPHORE";
        case BlockReason::MUTEX:                return "MUTEX";
        case BlockReason::MESSAGE_QUEUE_SEND:   return "MQ_SEND";
        case BlockReason::MESSAGE_QUEUE_RECEIVE:return "MQ_RECV";
        case BlockReason::EVENT_FLAG:           return "EVENT_FLAG";
        case BlockReason::DELAY:                return "DELAY";
    }
    return "UNKNOWN";
}

// ---------- Shared data structures ----------
struct Message {
    int         senderTaskId{-1};
    int         receiverTaskId{-1};
    std::string payload;
    int         timestamp{0};
};

struct ExecutionRecord {
    int         tick{0};
    int         taskId{-1};       // -1 means IDLE
    std::string taskName{"IDLE"};
    TaskState   state{TaskState::DORMANT};
    std::string event;
};

struct GanttSegment {
    int         startTick{0};
    int         endTick{0};       // exclusive
    int         taskId{-1};
    std::string taskName{"IDLE"};
};

// ---------- Exception types ----------
class RtosException : public std::runtime_error {
public:
    explicit RtosException(const std::string& msg) : std::runtime_error(msg) {}
};

class DuplicateTaskId : public RtosException {
public:
    explicit DuplicateTaskId(int id)
        : RtosException("Duplicate task id: " + std::to_string(id)) {}
};

class InvalidTaskId : public RtosException {
public:
    explicit InvalidTaskId(int id)
        : RtosException("Invalid task id: " + std::to_string(id)) {}
};

class InvalidParameter : public RtosException {
public:
    explicit InvalidParameter(const std::string& msg)
        : RtosException("Invalid parameter: " + msg) {}
};

class ResourceError : public RtosException {
public:
    explicit ResourceError(const std::string& msg)
        : RtosException("Resource error: " + msg) {}
};

} // namespace rtos