#include "hh/optimization/Optimizer.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <type_traits>
#include <vector>

namespace hh::optimization {
namespace {

class Fnv64 {
public:
    template <typename T>
    void add(T value) noexcept {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        if constexpr (std::is_enum_v<T>) {
            add(static_cast<std::underlying_type_t<T>>(value));
        } else if constexpr (std::is_same_v<std::remove_cv_t<T>, bool>) {
            addByte(value ? 1U : 0U);
        } else {
            using Unsigned = std::make_unsigned_t<T>;
            Unsigned raw = static_cast<Unsigned>(value);
            for (std::size_t i = 0; i < sizeof(Unsigned); ++i) {
                addByte(static_cast<std::uint8_t>((raw >> (i * 8U)) & static_cast<Unsigned>(0xFFU)));
            }
        }
    }

    std::uint64_t value() const noexcept { return hash_; }

private:
    void addByte(std::uint8_t byte) noexcept {
        hash_ ^= byte;
        hash_ *= 1099511628211ULL;
    }

    std::uint64_t hash_{14695981039346656037ULL};
};

auto canonicalEmployees(const OptimizerSnapshot& snapshot) {
    auto values = snapshot.employees;
    std::sort(values.begin(), values.end(), [](const Employee& a, const Employee& b) { return a.id < b.id; });
    return values;
}

auto canonicalTasks(const OptimizerSnapshot& snapshot) {
    auto values = snapshot.tasks;
    std::sort(values.begin(), values.end(), [](const Task& a, const Task& b) { return a.id < b.id; });
    return values;
}

auto canonicalStations(const OptimizerSnapshot& snapshot) {
    auto values = snapshot.serviceStations;
    std::sort(values.begin(), values.end(), [](const ServiceStation& a, const ServiceStation& b) { return a.id < b.id; });
    return values;
}

auto canonicalStationCandidates(const OptimizerSnapshot& snapshot) {
    auto values = snapshot.stationCandidates;
    std::sort(values.begin(), values.end(), [](const StationCandidate& a, const StationCandidate& b) {
        if (a.stationId != b.stationId) return a.stationId < b.stationId;
        return a.employeeId < b.employeeId;
    });
    return values;
}

auto canonicalCandidates(const OptimizerSnapshot& snapshot) {
    auto values = snapshot.candidates;
    std::sort(values.begin(), values.end(), [](const Candidate& a, const Candidate& b) {
        if (a.taskId != b.taskId) return a.taskId < b.taskId;
        return a.employeeId < b.employeeId;
    });
    return values;
}

std::int64_t bidCost(const Candidate& candidate) noexcept {
    return static_cast<std::int64_t>(candidate.travelSeconds)
        + candidate.effectiveWorkSeconds
        + candidate.fatiguePenaltySeconds
        + candidate.zonePenaltySeconds
        + candidate.taskSwitchPenaltySeconds
        - candidate.skillBonusSeconds;
}

std::int32_t durationBucketsForCandidate(const Candidate& candidate, std::int32_t bucketMinutes) noexcept {
    const std::int64_t bucketSeconds = static_cast<std::int64_t>(bucketMinutes) * 60;
    const std::int64_t travelSeconds = std::max<std::int64_t>(0, candidate.travelSeconds);
    const std::int64_t workSeconds = std::max<std::int64_t>(0, candidate.effectiveWorkSeconds);
    const std::int64_t totalSeconds = travelSeconds + workSeconds;
    const std::int64_t buckets = std::max<std::int64_t>(1, (totalSeconds + bucketSeconds - 1) / bucketSeconds);
    return static_cast<std::int32_t>(buckets);
}

}  // namespace

std::uint64_t snapshotFingerprint(const OptimizerSnapshot& snapshot) noexcept {
    Fnv64 hash;
    hash.add(snapshot.optimizationEpoch);
    hash.add(snapshot.simulationSecond);
    const auto employees = canonicalEmployees(snapshot);
    const auto tasks = canonicalTasks(snapshot);
    const auto candidates = canonicalCandidates(snapshot);
    const auto stations = canonicalStations(snapshot);
    const auto stationCandidates = canonicalStationCandidates(snapshot);
    hash.add(static_cast<std::uint64_t>(employees.size()));
    for (const auto& e : employees) {
        hash.add(e.id); hash.add(e.available); hash.add(e.fatigue);
        hash.add(e.regularWageMinorPerHour); hash.add(e.overtimeWageMinorPerHour);
        hash.add(e.overtime); hash.add(e.needsBreak); hash.add(e.availableFromBucket); hash.add(e.availableUntilBucket);
    }
    hash.add(static_cast<std::uint64_t>(tasks.size()));
    for (const auto& t : tasks) {
        hash.add(t.id); hash.add(t.state); hash.add(t.priority); hash.add(t.deadlineSecond);
        hash.add(t.estimatedWorkSeconds); hash.add(t.guestImpactPoints); hash.add(t.revenueImpactPoints);
    }
    hash.add(static_cast<std::uint64_t>(stations.size()));
    for (const auto& station : stations) {
        hash.add(station.id); hash.add(station.guestImpactWeight); hash.add(station.minimumCoverage);
        hash.add(static_cast<std::uint64_t>(station.demandSecondsByBucket.size()));
        for (const auto demand : station.demandSecondsByBucket) hash.add(demand);
    }
    hash.add(static_cast<std::uint64_t>(stationCandidates.size()));
    for (const auto& candidate : stationCandidates) {
        hash.add(candidate.employeeId); hash.add(candidate.stationId); hash.add(candidate.eligible);
        hash.add(candidate.serviceCapacitySecondsPerBucket); hash.add(candidate.zonePenaltySeconds);
    }
    hash.add(static_cast<std::uint64_t>(candidates.size()));
    for (const auto& c : candidates) {
        hash.add(c.employeeId); hash.add(c.taskId); hash.add(c.eligible); hash.add(c.travelSeconds);
        hash.add(c.effectiveWorkSeconds); hash.add(c.fatiguePenaltySeconds); hash.add(c.zonePenaltySeconds);
        hash.add(c.taskSwitchPenaltySeconds); hash.add(c.skillBonusSeconds);
    }
    return hash.value();
}

SchedulerPlan DeterministicFallbackOptimizer::optimize(const OptimizerSnapshot& snapshot) const {
    SchedulerPlan plan{};
    plan.optimizationEpoch = snapshot.optimizationEpoch;
    plan.simulationSecond = snapshot.simulationSecond;
    plan.snapshotFingerprint = snapshotFingerprint(snapshot);
    plan.source = PlanSource::DeterministicFallback;
    plan.bucketMinutes = 5;
    plan.horizonBuckets = 12;

    std::vector<Task> ready;
    for (const auto& task : snapshot.tasks) {
        if (task.state == TaskState::Ready) ready.push_back(task);
    }
    std::sort(ready.begin(), ready.end(), [](const Task& a, const Task& b) {
        if (a.priority != b.priority) return a.priority > b.priority;
        const auto aDeadline = a.deadlineSecond < 0 ? std::numeric_limits<std::int64_t>::max() : a.deadlineSecond;
        const auto bDeadline = b.deadlineSecond < 0 ? std::numeric_limits<std::int64_t>::max() : b.deadlineSecond;
        if (aDeadline != bDeadline) return aDeadline < bDeadline;
        return a.id < b.id;
    });

    std::set<EntityId> usedEmployees;
    for (const auto& task : ready) {
        const Candidate* best = nullptr;
        std::int64_t bestCost = std::numeric_limits<std::int64_t>::max();
        for (const auto& candidate : snapshot.candidates) {
            if (candidate.taskId != task.id || !candidate.eligible || usedEmployees.contains(candidate.employeeId)) continue;
            const auto employee = std::find_if(snapshot.employees.begin(), snapshot.employees.end(), [&](const Employee& e) {
                return e.id == candidate.employeeId && e.available;
            });
            if (employee == snapshot.employees.end()) continue;
            const auto cost = bidCost(candidate);
            if (best == nullptr || cost < bestCost || (cost == bestCost && candidate.employeeId < best->employeeId)) {
                best = &candidate;
                bestCost = cost;
            }
        }
        if (best != nullptr) {
            const auto durationBuckets = durationBucketsForCandidate(*best, plan.bucketMinutes);
            plan.assignments.push_back(Assignment{.taskId = task.id, .employeeId = best->employeeId, .startBucket = 0, .durationBuckets = durationBuckets});
            plan.horizonBuckets = std::max(plan.horizonBuckets, durationBuckets);
            usedEmployees.insert(best->employeeId);
        }
    }
    std::sort(plan.assignments.begin(), plan.assignments.end(), [](const Assignment& a, const Assignment& b) {
        if (a.taskId != b.taskId) return a.taskId < b.taskId;
        return a.employeeId < b.employeeId;
    });
    return plan;
}

}  // namespace hh::optimization
