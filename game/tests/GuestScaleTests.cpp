#include "hh/game/Simulation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace hh::game;

namespace {
constexpr std::size_t GuestCount = 1000;
constexpr std::size_t CandidatesPerGuest = 8;
std::size_t reportedSaveBytes{};
std::size_t reportedMemoryCount{};
std::size_t reportedComplaintCount{};

void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

struct DecisionReport {
  GuestGoalEvaluationMetrics metrics;
  std::uint64_t fingerprint{};
  std::size_t selected{};
};

DecisionReport runGuestDecisionSoak(std::uint64_t seed) {
  const auto definitions = defaultGuestModelDefinitions();
  DecisionReport report;
  report.fingerprint = 14695981039346656037ULL;
  for (std::size_t index = 0; index < GuestCount; ++index) {
    const auto guestId = static_cast<GuestId>(index + 1);
    auto profile = generateGuestProfile(seed, guestId, definitions);
    GuestNeedState needs;
    needs.values.fill(55.0);
    const bool hungry = guestId % 2 == 0;
    const auto need = hungry ? GuestNeed::Hunger : GuestNeed::Comfort;
    auto &needValue = needs.values[static_cast<std::size_t>(need)];
    needValue = 20.0 + static_cast<double>(drawGuestRandom(profile.randomState) % 61);

    GuestGoalCandidate candidate;
    candidate.goal = hungry ? GuestGoal::Eat : GuestGoal::Relax;
    candidate.targetId = 100000 + guestId;
    candidate.requiredNeed = need;
    candidate.preference =
        0.5 + static_cast<double>(drawGuestRandom(profile.randomState) % 501) /
                  1000.0;
    candidate.expectedWaitMinutes = 5.0;
    candidate.queueToleranceMinutes = 30.0;

    std::array<GuestGoalCandidate, CandidatesPerGuest> candidates;
    for (auto &entry : candidates)
      entry = candidate;
    candidates[0].available = false;
    candidates[1].reachable = false;
    candidates[2].budgetCompatible = false;
    candidates[3].timeCompatible = false;
    candidates[4].groupCompatible = false;
    candidates[5].expectedWaitMinutes = 31.0;
    candidates[6].availabilityFactor =
        std::numeric_limits<double>::quiet_NaN();

    GuestGoalEvaluationMetrics guestMetrics;
    const auto selection =
        selectGuestGoal(profile, needs, candidates, std::nullopt, &guestMetrics);
    report.metrics.candidatesVisited += guestMetrics.candidatesVisited;
    report.metrics.utilityEvaluations += guestMetrics.utilityEvaluations;
    require(selection && selection->targetId == candidate.targetId,
            "guest selector did not retain the only viable candidate");
    ++report.selected;

    const std::array<std::uint64_t, 3> values = {
        selection->targetId, static_cast<std::uint64_t>(selection->goal),
        static_cast<std::uint64_t>(std::llround(selection->utility * 1000000.0))};
    for (const auto value : values) {
      report.fingerprint ^= value;
      report.fingerprint *= 1099511628211ULL;
    }
  }
  return report;
}

static void one_thousand_guest_decisions_use_filtered_candidates() {
  const auto report = runGuestDecisionSoak(0x484D47303130ULL);
  require(report.selected == GuestCount,
          "the 1,000-guest decision soak lost a valid selection");
  require(report.metrics.candidatesVisited ==
              GuestCount * CandidatesPerGuest,
          "the soak did not visit its fixed candidate set");
  require(report.metrics.utilityEvaluations == GuestCount,
          "filtered candidates reached full utility evaluation");
}

static void memories_and_complaints_remain_within_save_limits() {
  auto simulation = Simulation::tutorial(0x484D47303131ULL);
  require(simulation.loadDefinitions(R"({"baseDemand":100})").ok,
          "guest-scale save fixture definitions were rejected");
  simulation.step(2 * 3600.0);
  auto view = simulation.view();
  require(!view.guests.empty(), "guest-scale save fixture has no guest");
  const auto guestId = view.guests.front().profile.id;
  for (std::size_t index = 0; index < GuestCount; ++index) {
    GuestExperienceEvent event;
    event.guestId = guestId;
    event.type = GuestExperienceEventType::StaffRudeness;
    event.category = GuestCategory::Service;
    event.observedValue = 5.0;
    event.expectedValue = 80.0;
    event.rawImpact = -80.0;
    event.salience = 1.0;
    event.complaintEligible = true;
    event.reviewStatement = "The service response was poor.";
    const auto result = simulation.reportGuestExperience(event);
    require(result.ok,
            "guest-scale event was rejected at record " +
                std::to_string(index));
  }

  const auto saved = simulation.save();
  const auto restored = Simulation::load(saved);
  const auto roundTrip = restored.view();
  const auto guest = std::find_if(
      roundTrip.guests.begin(), roundTrip.guests.end(),
      [guestId](const auto &candidate) {
        return candidate.profile.id == guestId;
      });
  require(guest != roundTrip.guests.end(),
          "guest-scale save lost its selected guest");
  reportedMemoryCount = guest->experience.memories.size();
  reportedComplaintCount = guest->experience.complaints.size();
  reportedSaveBytes = saved.size();
  require(reportedMemoryCount == GuestCount,
          "v10 save lost guest memories in the 1,000-event fixture");
  require(reportedComplaintCount == GuestCount,
          "v10 save lost guest complaints in the 1,000-event fixture");
  require(reportedMemoryCount <= 100000 && reportedComplaintCount <= 100000,
          "guest history crossed a documented save collection limit");
  require(restored.save() == saved,
          "guest-scale v10 save changed during deterministic round-trip");
}

static void guest_model_soak_is_repeatable_for_fixed_seed() {
  const auto first = runGuestDecisionSoak(0x484D47303130ULL);
  const auto second = runGuestDecisionSoak(0x484D47303130ULL);
  require(first.selected == second.selected &&
              first.metrics.candidatesVisited ==
                  second.metrics.candidatesVisited &&
              first.metrics.utilityEvaluations ==
                  second.metrics.utilityEvaluations &&
              first.fingerprint == second.fingerprint,
          "fixed-seed guest decisions were not repeatable");
}
} // namespace

int main() {
  try {
    one_thousand_guest_decisions_use_filtered_candidates();
    memories_and_complaints_remain_within_save_limits();
    guest_model_soak_is_repeatable_for_fixed_seed();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  const auto report = runGuestDecisionSoak(0x484D47303130ULL);
  std::cout << "GuestScale: decisions=" << report.selected
            << " candidatesVisited=" << report.metrics.candidatesVisited
            << " utilityEvaluations=" << report.metrics.utilityEvaluations
            << " memories=" << reportedMemoryCount
            << " complaints=" << reportedComplaintCount
            << " saveBytes=" << reportedSaveBytes << '\n';
}
