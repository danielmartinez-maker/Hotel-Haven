#include "hh/optimization/MilpRequest.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "hh/optimization/Optimizer.h"

namespace hh::optimization {
namespace {

const char* stateName(TaskState state) noexcept {
    switch (state) {
        case TaskState::Created: return "Created";
        case TaskState::Blocked: return "Blocked";
        case TaskState::Ready: return "Ready";
        case TaskState::Assigned: return "Assigned";
        case TaskState::Traveling: return "Traveling";
        case TaskState::Executing: return "Executing";
        case TaskState::Completed: return "Completed";
        case TaskState::Cancelled: return "Cancelled";
        case TaskState::Failed: return "Failed";
    }
    return "Unknown";
}

}  // namespace

std::string buildMilpRequestJson(const OptimizerSnapshot& snapshot, MilpRequestOptions options) {
    if (options.bucketMinutes <= 0 || options.horizonBuckets <= 0) throw std::invalid_argument("MILP bucket/horizon must be positive");
    const auto bucketSeconds = options.bucketMinutes * 60;
    auto employees = snapshot.employees;
    auto tasks = snapshot.tasks;
    auto candidates = snapshot.candidates;
    auto serviceStations = snapshot.serviceStations;
    auto stationCandidates = snapshot.stationCandidates;
    std::sort(employees.begin(), employees.end(), [](const Employee& a, const Employee& b) { return a.id < b.id; });
    std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) { return a.id < b.id; });
    std::sort(serviceStations.begin(), serviceStations.end(), [](const ServiceStation& a, const ServiceStation& b) { return a.id < b.id; });
    std::sort(stationCandidates.begin(), stationCandidates.end(), [](const StationCandidate& a, const StationCandidate& b) {
        if (a.stationId != b.stationId) return a.stationId < b.stationId;
        return a.employeeId < b.employeeId;
    });
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.taskId != b.taskId) return a.taskId < b.taskId;
        return a.employeeId < b.employeeId;
    });

    std::ostringstream out;
    out << "{";
    out << "\"schema\":1,";
    out << "\"optimization_epoch\":" << snapshot.optimizationEpoch << ',';
    out << "\"simulation_second\":" << snapshot.simulationSecond << ',';
    out << "\"snapshot_fingerprint\":" << snapshotFingerprint(snapshot) << ',';
    out << "\"bucket_minutes\":" << options.bucketMinutes << ',';
    out << "\"horizon_buckets\":" << options.horizonBuckets << ',';
    out << "\"lexicographic_objectives\":[\"critical_service\",\"guest_wait_lateness\",\"labor_travel_cost\",\"fatigue_zone_switch\"],";

    out << "\"employees\":[";
    for (std::size_t i = 0; i < employees.size(); ++i) {
        if (i) out << ',';
        const auto& e = employees[i];
        out << "{\"id\":" << e.id << ",\"available\":" << (e.available ? "true" : "false")
            << ",\"fatigue\":" << e.fatigue
            << ",\"regular_wage_minor_per_hour\":" << e.regularWageMinorPerHour
            << ",\"overtime_wage_minor_per_hour\":" << e.overtimeWageMinorPerHour
            << ",\"overtime\":" << (e.overtime ? "true" : "false")
            << ",\"needs_break\":" << (e.needsBreak ? "true" : "false")
            << ",\"available_from_bucket\":" << e.availableFromBucket
            << ",\"available_until_bucket\":" << e.availableUntilBucket << '}';
    }
    out << "],";

    out << "\"tasks\":[";
    for (std::size_t i = 0; i < tasks.size(); ++i) {
        if (i) out << ',';
        const auto& t = tasks[i];
        std::int64_t deadlineBucket = -1;
        if (t.deadlineSecond >= 0) {
            const auto delta = t.deadlineSecond - snapshot.simulationSecond;
            deadlineBucket = delta <= 0 ? 0 : (delta + bucketSeconds - 1) / bucketSeconds;
        }
        out << "{\"id\":" << t.id << ",\"state\":\"" << stateName(t.state) << "\",\"priority\":" << t.priority
            << ",\"deadline_bucket\":" << deadlineBucket
            << ",\"estimated_work_seconds\":" << t.estimatedWorkSeconds
            << ",\"guest_impact\":" << t.guestImpactPoints
            << ",\"revenue_impact\":" << t.revenueImpactPoints << '}';
    }
    out << "],";

    out << "\"service_stations\":[";
    for (std::size_t i = 0; i < serviceStations.size(); ++i) {
        if (i) out << ',';
        const auto& station = serviceStations[i];
        out << "{\"id\":" << station.id
            << ",\"guest_impact_weight\":" << station.guestImpactWeight
            << ",\"minimum_coverage\":" << station.minimumCoverage
            << ",\"demand_seconds_by_bucket\":[";
        for (std::size_t bucket = 0; bucket < station.demandSecondsByBucket.size(); ++bucket) {
            if (bucket) out << ',';
            out << station.demandSecondsByBucket[bucket];
        }
        out << "]}";
    }
    out << "],";

    out << "\"station_candidates\":[";
    bool firstStationCandidate = true;
    for (const auto& candidate : stationCandidates) {
        if (!candidate.eligible) continue;
        if (!firstStationCandidate) out << ',';
        firstStationCandidate = false;
        out << "{\"employee_id\":" << candidate.employeeId
            << ",\"station_id\":" << candidate.stationId
            << ",\"service_capacity_seconds_per_bucket\":" << candidate.serviceCapacitySecondsPerBucket
            << ",\"zone_penalty_seconds\":" << candidate.zonePenaltySeconds << '}';
    }
    out << "],";

    out << "\"candidates\":[";
    bool first = true;
    for (const auto& c : candidates) {
        if (!c.eligible) continue;
        if (!first) out << ',';
        first = false;
        const auto durationBuckets = std::max(1, (c.travelSeconds + c.effectiveWorkSeconds + bucketSeconds - 1) / bucketSeconds);
        out << "{\"employee_id\":" << c.employeeId << ",\"task_id\":" << c.taskId
            << ",\"travel_seconds\":" << c.travelSeconds
            << ",\"effective_work_seconds\":" << c.effectiveWorkSeconds
            << ",\"duration_buckets\":" << durationBuckets
            << ",\"fatigue_penalty_seconds\":" << c.fatiguePenaltySeconds
            << ",\"zone_penalty_seconds\":" << c.zonePenaltySeconds
            << ",\"task_switch_penalty_seconds\":" << c.taskSwitchPenaltySeconds
            << ",\"skill_bonus_seconds\":" << c.skillBonusSeconds << '}';
    }
    out << "]}";
    return out.str();
}

}  // namespace hh::optimization
