#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "hh/optimization/MilpRequest.h"
#include "hh/optimization/Optimizer.h"
#include "hh/optimization/PlanValidator.h"

namespace {

int failures = 0;

void check(bool condition, const char* message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

hh::optimization::OptimizerSnapshot baseSnapshot() {
    using namespace hh::optimization;
    OptimizerSnapshot snapshot{};
    snapshot.optimizationEpoch = 17;
    snapshot.simulationSecond = 50'000;
    snapshot.employees = {
        Employee{.id = 20, .available = true, .fatigue = 25, .regularWageMinorPerHour = 2200, .overtimeWageMinorPerHour = 3300},
        Employee{.id = 10, .available = true, .fatigue = 25, .regularWageMinorPerHour = 2200, .overtimeWageMinorPerHour = 3300},
    };
    snapshot.tasks = {
        Task{.id = 200, .state = TaskState::Blocked, .priority = 150, .deadlineSecond = 50'300, .estimatedWorkSeconds = 300},
        Task{.id = 100, .state = TaskState::Ready, .priority = 85, .deadlineSecond = 50'600, .estimatedWorkSeconds = 600},
    };
    snapshot.serviceStations = {
        ServiceStation{.id = 300, .guestImpactWeight = 5, .minimumCoverage = 1, .demandSecondsByBucket = {300, 600, 300, 0}},
    };
    snapshot.stationCandidates = {
        StationCandidate{.employeeId = 20, .stationId = 300, .eligible = true, .serviceCapacitySecondsPerBucket = 300, .zonePenaltySeconds = 0},
        StationCandidate{.employeeId = 10, .stationId = 300, .eligible = true, .serviceCapacitySecondsPerBucket = 300, .zonePenaltySeconds = 0},
    };
    snapshot.candidates = {
        Candidate{.employeeId = 20, .taskId = 100, .eligible = true, .travelSeconds = 120, .effectiveWorkSeconds = 600, .fatiguePenaltySeconds = 0, .zonePenaltySeconds = 0, .taskSwitchPenaltySeconds = 0, .skillBonusSeconds = 0},
        Candidate{.employeeId = 10, .taskId = 100, .eligible = true, .travelSeconds = 120, .effectiveWorkSeconds = 600, .fatiguePenaltySeconds = 0, .zonePenaltySeconds = 0, .taskSwitchPenaltySeconds = 0, .skillBonusSeconds = 0},
    };
    return snapshot;
}

void testCanonicalFingerprintIgnoresInputOrdering() {
    using namespace hh::optimization;
    auto a = baseSnapshot();
    auto b = baseSnapshot();
    std::reverse(b.employees.begin(), b.employees.end());
    std::reverse(b.tasks.begin(), b.tasks.end());
    std::reverse(b.candidates.begin(), b.candidates.end());
    std::reverse(b.serviceStations.begin(), b.serviceStations.end());
    std::reverse(b.stationCandidates.begin(), b.stationCandidates.end());
    check(snapshotFingerprint(a) == snapshotFingerprint(b), "snapshot fingerprint must be canonical");
}

void testFallbackUsesPriorityEligibilityAndStableTieBreak() {
    using namespace hh::optimization;
    auto snapshot = baseSnapshot();
    DeterministicFallbackOptimizer optimizer;
    const auto plan = optimizer.optimize(snapshot);
    check(plan.assignments.size() == 1, "fallback should assign only ready task");
    if (plan.assignments.size() == 1) {
        check(plan.assignments[0].taskId == 100, "blocked task must never be assigned");
        check(plan.assignments[0].employeeId == 10, "equal bid must use lowest stable employee id");
    }
    check(plan.snapshotFingerprint == snapshotFingerprint(snapshot), "plan must carry snapshot fingerprint");
}

void testPlanValidatorRejectsStaleAndOverlappingPlans() {
    using namespace hh::optimization;
    auto snapshot = baseSnapshot();
    SchedulerPlan stale{};
    stale.optimizationEpoch = snapshot.optimizationEpoch;
    stale.snapshotFingerprint = snapshotFingerprint(snapshot) + 1;
    stale.assignments.push_back(Assignment{.taskId = 100, .employeeId = 10, .startBucket = 0, .durationBuckets = 2});
    check(!validatePlan(snapshot, stale).ok, "stale plan fingerprint must be rejected");

    snapshot.tasks.push_back(Task{.id = 101, .state = TaskState::Ready, .priority = 60, .deadlineSecond = 51'000, .estimatedWorkSeconds = 600});
    snapshot.candidates.push_back(Candidate{.employeeId = 10, .taskId = 101, .eligible = true, .travelSeconds = 0, .effectiveWorkSeconds = 600});
    SchedulerPlan overlap{};
    overlap.optimizationEpoch = snapshot.optimizationEpoch;
    overlap.snapshotFingerprint = snapshotFingerprint(snapshot);
    overlap.assignments = {
        Assignment{.taskId = 100, .employeeId = 10, .startBucket = 0, .durationBuckets = 2},
        Assignment{.taskId = 101, .employeeId = 10, .startBucket = 1, .durationBuckets = 2},
    };
    check(!validatePlan(snapshot, overlap).ok, "overlapping employee assignments must be rejected");
}

void testPlanValidatorRejectsTaskStationOverlap() {
    using namespace hh::optimization;
    auto snapshot = baseSnapshot();
    SchedulerPlan plan{};
    plan.optimizationEpoch = snapshot.optimizationEpoch;
    plan.snapshotFingerprint = snapshotFingerprint(snapshot);
    plan.assignments = {Assignment{.taskId = 100, .employeeId = 10, .startBucket = 0, .durationBuckets = 2}};
    plan.stationAssignments = {StationAssignment{.stationId = 300, .employeeId = 10, .bucket = 1}};
    check(!validatePlan(snapshot, plan).ok, "employee cannot cover a station while executing a task");
}

void testMilpRequestIncludesStationCoverageAndClampsOverdueDeadline() {
    using namespace hh::optimization;
    auto snapshot = baseSnapshot();
    snapshot.tasks[1].deadlineSecond = snapshot.simulationSecond - 2'000;
    const auto json = buildMilpRequestJson(snapshot, MilpRequestOptions{.bucketMinutes = 5, .horizonBuckets = 4});
    check(json.find("\"service_stations\"") != std::string::npos, "request must include service stations");
    check(json.find("\"station_candidates\"") != std::string::npos, "request must include station candidates");
    check(json.find("\"deadline_bucket\":0") != std::string::npos, "overdue deadlines must clamp to bucket zero");
}

void testMilpRequestIsDeterministicAndDeclaresLexicographicObjectives() {
    using namespace hh::optimization;
    auto a = baseSnapshot();
    auto b = baseSnapshot();
    std::reverse(b.employees.begin(), b.employees.end());
    std::reverse(b.tasks.begin(), b.tasks.end());
    std::reverse(b.candidates.begin(), b.candidates.end());
    std::reverse(b.serviceStations.begin(), b.serviceStations.end());
    std::reverse(b.stationCandidates.begin(), b.stationCandidates.end());
    const auto jsonA = buildMilpRequestJson(a, MilpRequestOptions{.bucketMinutes = 5, .horizonBuckets = 12});
    const auto jsonB = buildMilpRequestJson(b, MilpRequestOptions{.bucketMinutes = 5, .horizonBuckets = 12});
    check(jsonA == jsonB, "MILP request must be byte-stable for equivalent snapshots");
    check(jsonA.find("critical_service") != std::string::npos, "request must declare critical-service objective");
    check(jsonA.find("guest_wait_lateness") != std::string::npos, "request must declare waiting/lateness objective");
    check(jsonA.find("labor_travel_cost") != std::string::npos, "request must declare labor/travel objective");
    check(jsonA.find("fatigue_zone_switch") != std::string::npos, "request must declare fatigue/zone/switch objective");
}

}  // namespace

int main() {
    testCanonicalFingerprintIgnoresInputOrdering();
    testFallbackUsesPriorityEligibilityAndStableTieBreak();
    testPlanValidatorRejectsStaleAndOverlappingPlans();
    testPlanValidatorRejectsTaskStationOverlap();
    testMilpRequestIncludesStationCoverageAndClampsOverdueDeadline();
    testMilpRequestIsDeterministicAndDeclaresLexicographicObjectives();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "optimizer tests passed\n";
    return EXIT_SUCCESS;
}
