#include "hh/optimization/PlanValidator.h"

#include <algorithm>
#include <cstdint>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "hh/optimization/Optimizer.h"

namespace hh::optimization {

ValidationResult validatePlan(const OptimizerSnapshot& snapshot, const SchedulerPlan& plan) {
    ValidationResult result{};
    auto fail = [&](std::string message) {
        result.ok = false;
        result.errors.push_back(std::move(message));
    };

    if (plan.optimizationEpoch != snapshot.optimizationEpoch) fail("optimization epoch mismatch");
    if (plan.snapshotFingerprint != snapshotFingerprint(snapshot)) fail("snapshot fingerprint mismatch");
    if (plan.bucketMinutes <= 0) fail("plan bucket size must be positive");
    if (plan.horizonBuckets <= 0) fail("plan horizon must be positive");
    if (plan.source == PlanSource::CuOpt &&
        (plan.bucketMinutes != kLivePlanningBucketMinutes || plan.horizonBuckets != kLivePlanningHorizonBuckets)) {
        fail("cuOpt plan planning window mismatch");
    }

    const std::int64_t bucketSeconds = plan.bucketMinutes > 0
        ? static_cast<std::int64_t>(plan.bucketMinutes) * 60
        : 0;
    const std::int64_t horizonBuckets = std::max<std::int64_t>(0, plan.horizonBuckets);

    std::set<EntityId> seenTasks;
    std::unordered_map<EntityId, std::vector<std::pair<std::int64_t, std::int64_t>>> intervals;

    for (const auto& assignment : plan.assignments) {
        if (assignment.startBucket < 0 || assignment.durationBuckets <= 0) {
            fail("assignment has invalid bucket interval");
            continue;
        }
        if (!seenTasks.insert(assignment.taskId).second) fail("task assigned more than once");

        const std::int64_t start = assignment.startBucket;
        const std::int64_t duration = assignment.durationBuckets;
        const std::int64_t end = start + duration;
        if (plan.horizonBuckets > 0 && end > horizonBuckets) fail("assignment outside plan horizon");

        const auto task = std::find_if(snapshot.tasks.begin(), snapshot.tasks.end(), [&](const Task& value) { return value.id == assignment.taskId; });
        if (task == snapshot.tasks.end()) {
            fail("assignment references unknown task");
        } else if (task->state != TaskState::Ready) {
            fail("assignment references non-ready task");
        }

        const auto employee = std::find_if(snapshot.employees.begin(), snapshot.employees.end(), [&](const Employee& value) { return value.id == assignment.employeeId; });
        if (employee == snapshot.employees.end()) {
            fail("assignment references unknown employee");
        } else if (!employee->available) {
            fail("assignment references unavailable employee");
        } else if (start < static_cast<std::int64_t>(employee->availableFromBucket) || end > static_cast<std::int64_t>(employee->availableUntilBucket)) {
            fail("assignment outside employee availability window");
        }

        const auto candidate = std::find_if(snapshot.candidates.begin(), snapshot.candidates.end(), [&](const Candidate& value) {
            return value.taskId == assignment.taskId && value.employeeId == assignment.employeeId;
        });
        if (candidate == snapshot.candidates.end() || !candidate->eligible) {
            fail("assignment violates task eligibility");
        } else if (bucketSeconds > 0) {
            const std::int64_t travelSeconds = std::max<std::int64_t>(0, candidate->travelSeconds);
            const std::int64_t workSeconds = std::max<std::int64_t>(0, candidate->effectiveWorkSeconds);
            const std::int64_t totalSeconds = travelSeconds + workSeconds;
            const std::int64_t requiredBuckets = std::max<std::int64_t>(1, (totalSeconds + bucketSeconds - 1) / bucketSeconds);
            if (duration < requiredBuckets) fail("assignment interval shorter than candidate travel plus work duration");
        }

        auto& employeeIntervals = intervals[assignment.employeeId];
        for (const auto& [existingStart, existingEnd] : employeeIntervals) {
            if (start < existingEnd && existingStart < end) {
                fail("employee assignments overlap");
                break;
            }
        }
        employeeIntervals.emplace_back(start, end);
    }

    std::set<std::tuple<EntityId, EntityId, std::int32_t>> seenStationCoverage;
    for (const auto& stationAssignment : plan.stationAssignments) {
        if (stationAssignment.bucket < 0) {
            fail("station assignment has invalid bucket");
            continue;
        }
        if (plan.horizonBuckets > 0 && static_cast<std::int64_t>(stationAssignment.bucket) >= horizonBuckets) {
            fail("station assignment outside plan horizon");
        }
        if (!seenStationCoverage.emplace(stationAssignment.stationId, stationAssignment.employeeId, stationAssignment.bucket).second) {
            fail("station assignment duplicated");
        }
        const auto station = std::find_if(snapshot.serviceStations.begin(), snapshot.serviceStations.end(), [&](const ServiceStation& value) {
            return value.id == stationAssignment.stationId;
        });
        if (station == snapshot.serviceStations.end()) fail("station assignment references unknown station");

        const auto employee = std::find_if(snapshot.employees.begin(), snapshot.employees.end(), [&](const Employee& value) {
            return value.id == stationAssignment.employeeId;
        });
        if (employee == snapshot.employees.end()) {
            fail("station assignment references unknown employee");
        } else if (!employee->available) {
            fail("station assignment references unavailable employee");
        } else if (stationAssignment.bucket < employee->availableFromBucket || stationAssignment.bucket >= employee->availableUntilBucket) {
            fail("station assignment outside employee availability window");
        }

        const auto candidate = std::find_if(snapshot.stationCandidates.begin(), snapshot.stationCandidates.end(), [&](const StationCandidate& value) {
            return value.stationId == stationAssignment.stationId && value.employeeId == stationAssignment.employeeId;
        });
        if (candidate == snapshot.stationCandidates.end() || !candidate->eligible) fail("station assignment violates eligibility");

        auto& employeeIntervals = intervals[stationAssignment.employeeId];
        const std::int64_t start = stationAssignment.bucket;
        const std::int64_t end = start + 1;
        for (const auto& [existingStart, existingEnd] : employeeIntervals) {
            if (start < existingEnd && existingStart < end) {
                fail("employee station/task assignments overlap");
                break;
            }
        }
        employeeIntervals.emplace_back(start, end);
    }
    return result;
}

}  // namespace hh::optimization
