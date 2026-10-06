#include "Task.h"
#include <stdexcept>

namespace rtos {

Task::Task(int id, std::string name, int priority,
           int executionTime, int period, int relativeDeadline)
    : id_(id), name_(std::move(name)),
      basePriority_(priority), effectivePriority_(priority),
      executionTime_(executionTime), remainingExecutionTime_(executionTime),
      period_(period), relativeDeadline_(relativeDeadline),
      absoluteDeadline_(0), releaseTime_(-1),
      nextReleaseTime_(0),
      state_(TaskState::DORMANT), blockReason_(BlockReason::NONE),
      periodic_(true), suspended_(false),
      completedJobs_(0), missedDeadlines_(0),
      totalExecutionTime_(0), totalWaitingTime_(0), responseTime_(0),
      timeSlice_(0), remainingTimeSlice_(0)
{
    if (id < 0) throw InvalidParameter("task id must be >= 0");
    if (executionTime <= 0) throw InvalidParameter("executionTime must be > 0");
    if (period <= 0) throw InvalidParameter("period must be > 0");
    if (relativeDeadline <= 0) throw InvalidParameter("deadline must be > 0");
}

Task::Task(int id, std::string name, int priority, int executionTime)
    : id_(id), name_(std::move(name)),
      basePriority_(priority), effectivePriority_(priority),
      executionTime_(executionTime), remainingExecutionTime_(executionTime),
      period_(0), relativeDeadline_(0), absoluteDeadline_(0),
      releaseTime_(-1), nextReleaseTime_(0),
      state_(TaskState::DORMANT), blockReason_(BlockReason::NONE),
      periodic_(false), suspended_(false),
      completedJobs_(0), missedDeadlines_(0),
      totalExecutionTime_(0), totalWaitingTime_(0), responseTime_(0),
      timeSlice_(0), remainingTimeSlice_(0)
{
    if (id < 0) throw InvalidParameter("task id must be >= 0");
    if (executionTime <= 0) throw InvalidParameter("executionTime must be > 0");
}

void Task::release(int currentTick) {
    // A new job of a periodic task becomes READY.
    releaseTime_ = currentTick;
    absoluteDeadline_ = currentTick + relativeDeadline_;
    remainingExecutionTime_ = executionTime_;
    blockReason_ = BlockReason::NONE;
    state_ = TaskState::READY;
    if (periodic_) nextReleaseTime_ = currentTick + period_;
}

void Task::executeOneTick() {
    if (state_ != TaskState::RUNNING) return;
    --remainingExecutionTime_;
    ++totalExecutionTime_;
    if (remainingTimeSlice_ > 0) --remainingTimeSlice_;
}

bool Task::isCompleted() const {
    return remainingExecutionTime_ <= 0;
}

void Task::resetForNextPeriod() {
    // Prepare for the next job of a periodic task.
    remainingExecutionTime_ = executionTime_;
    state_ = TaskState::DORMANT;
    blockReason_ = BlockReason::NONE;
}

void Task::terminate() {
    state_ = TaskState::TERMINATED;
}

void Task::setState(TaskState s) { state_ = s; }
TaskState Task::getState() const { return state_; }

void Task::block(BlockReason reason) {
    state_ = TaskState::BLOCKED;
    blockReason_ = reason;
}

void Task::unblock() {
    if (state_ == TaskState::BLOCKED) {
        state_ = TaskState::READY;
        blockReason_ = BlockReason::NONE;
    }
}

BlockReason Task::getBlockReason() const { return blockReason_; }

void Task::suspend() {
    suspended_ = true;
    if (state_ != TaskState::RUNNING) state_ = TaskState::SUSPENDED;
}

void Task::resume() {
    if (!suspended_) return;
    suspended_ = false;
    if (state_ == TaskState::SUSPENDED) state_ = TaskState::READY;
}

bool Task::isSuspended() const { return suspended_; }

void Task::setEffectivePriority(int p) { effectivePriority_ = p; }
int  Task::getEffectivePriority() const { return effectivePriority_; }
int  Task::getBasePriority() const { return basePriority_; }

int  Task::getId() const { return id_; }
const std::string& Task::getName() const { return name_; }
int  Task::getExecutionTime() const { return executionTime_; }
int  Task::getRemainingExecutionTime() const { return remainingExecutionTime_; }
int  Task::getPeriod() const { return period_; }
int  Task::getRelativeDeadline() const { return relativeDeadline_; }
int  Task::getAbsoluteDeadline() const { return absoluteDeadline_; }
int  Task::getReleaseTime() const { return releaseTime_; }
int  Task::getNextReleaseTime() const { return nextReleaseTime_; }
bool Task::isPeriodic() const { return periodic_; }

void Task::setTimeSlice(int q) { timeSlice_ = q; remainingTimeSlice_ = q; }
int  Task::getRemainingTimeSlice() const { return remainingTimeSlice_; }
void Task::decrementTimeSlice() { if (remainingTimeSlice_ > 0) --remainingTimeSlice_; }
void Task::resetTimeSlice() { remainingTimeSlice_ = timeSlice_; }

void Task::recordJobCompletion(int currentTick) {
    ++completedJobs_;
    responseTime_ = currentTick - releaseTime_ + 1;
    totalWaitingTime_ += (releaseTime_ >= 0 ? (currentTick - releaseTime_) : 0);
}

void Task::recordDeadlineMiss(int /*currentTick*/) {
    ++missedDeadlines_;
}

void Task::addWaitingTime(int t) { totalWaitingTime_ += t; }

int Task::getCompletedJobs() const { return completedJobs_; }
int Task::getMissedDeadlines() const { return missedDeadlines_; }
int Task::getTotalExecutionTime() const { return totalExecutionTime_; }
int Task::getTotalWaitingTime() const { return totalWaitingTime_; }
int Task::getResponseTime() const { return responseTime_; }

void Task::resetStatistics() {
    completedJobs_ = 0;
    missedDeadlines_ = 0;
    totalExecutionTime_ = 0;
    totalWaitingTime_ = 0;
    responseTime_ = 0;
}

} // namespace rtos