#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hh::optimization {

using EntityId = std::uint64_t;

enum class TaskState : std::uint8_t {
    Created,
    Blocked,
    Ready,
    Assigned,
    Traveling,
    Executing,
    Completed,
    Cancelled,
    Failed,
};

struct Employee {
    EntityId id{};
    bool available{false};
    std::int32_t fatigue{};
    std::int64_t regularWageMinorPerHour{};
    std::int64_t overtimeWageMinorPerHour{};
    bool overtime{false};
    bool needsBreak{false};
    std::int32_t availableFromBucket{0};
    std::int32_t availableUntilBucket{2'147'483'647};
};

struct Task {
    EntityId id{};
    TaskState state{TaskState::Created};
    std::int32_t priority{};
    std::int64_t deadlineSecond{-1};
    std::int32_t estimatedWorkSeconds{};
    std::int32_t guestImpactPoints{};
    std::int32_t revenueImpactPoints{};
};

struct ServiceStation {
    EntityId id{};
    std::int32_t guestImpactWeight{1};
    std::int32_t minimumCoverage{};
    std::vector<std::int32_t> demandSecondsByBucket;
};

struct StationCandidate {
    EntityId employeeId{};
    EntityId stationId{};
    bool eligible{false};
    std::int32_t serviceCapacitySecondsPerBucket{};
    std::int32_t zonePenaltySeconds{};
};

struct Candidate {
    EntityId employeeId{};
    EntityId taskId{};
    bool eligible{false};
    std::int32_t travelSeconds{};
    std::int32_t effectiveWorkSeconds{};
    std::int32_t fatiguePenaltySeconds{};
    std::int32_t zonePenaltySeconds{};
    std::int32_t taskSwitchPenaltySeconds{};
    std::int32_t skillBonusSeconds{};
};

struct OptimizerSnapshot {
    std::uint64_t optimizationEpoch{};
    std::int64_t simulationSecond{};
    std::vector<Employee> employees;
    std::vector<Task> tasks;
    std::vector<Candidate> candidates;
    std::vector<ServiceStation> serviceStations;
    std::vector<StationCandidate> stationCandidates;
};

struct Assignment {
    EntityId taskId{};
    EntityId employeeId{};
    std::int32_t startBucket{};
    std::int32_t durationBuckets{1};
};

struct StationAssignment {
    EntityId stationId{};
    EntityId employeeId{};
    std::int32_t bucket{};
};

enum class PlanSource : std::uint8_t {
    DeterministicFallback,
    CuOpt,
};

struct ObjectiveValue {
    std::string name;
    std::int64_t value{};
};

struct SchedulerPlan {
    std::uint64_t optimizationEpoch{};
    std::int64_t simulationSecond{};
    std::uint64_t snapshotFingerprint{};
    PlanSource source{PlanSource::DeterministicFallback};
    std::int32_t bucketMinutes{5};
    std::int32_t horizonBuckets{12};
    std::vector<Assignment> assignments;
    std::vector<StationAssignment> stationAssignments;
    std::vector<ObjectiveValue> objectives;
};

}  // namespace hh::optimization
