#include "hh/game/GuestModel.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>

using namespace hh::game;

static void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

static bool hasNumericEffect(const GuestTraitEffects &effect) {
  const auto nonzero = [](double value) { return value != 0.0; };
  if (nonzero(effect.sensitivityDelta.price) ||
      nonzero(effect.sensitivityDelta.service) ||
      nonzero(effect.sensitivityDelta.cleanliness) ||
      nonzero(effect.sensitivityDelta.noise) ||
      nonzero(effect.sensitivityDelta.privacy) ||
      nonzero(effect.sensitivityDelta.safety) ||
      nonzero(effect.sensitivityDelta.comfort) ||
      nonzero(effect.sensitivityDelta.food) ||
      nonzero(effect.patienceDelta) || nonzero(effect.socialPreferenceDelta) ||
      effect.queueToleranceMultiplier != 1.0 ||
      effect.negativeMemoryReviewWeightMultiplier != 1.0 ||
      effect.lightSleepModifier != 1.0 ||
      effect.hygieneDecayMultiplier != 1.0 ||
      effect.complaintThresholdDelta != 0.0)
    return true;
  if (nonzero(effect.morningPreferenceDelta) ||
      nonzero(effect.eveningPreferenceDelta))
    return true;
  for (const auto value : effect.activityPreferenceDelta)
    if (nonzero(value))
      return true;
  for (const auto value : effect.roomPreferenceDelta)
    if (nonzero(value))
      return true;
  for (const auto value : effect.expectationDelta)
    if (nonzero(value))
      return true;
  return false;
}

static void profile_generation_is_repeatable_for_seed_and_guest_id() {
  const auto definitions = defaultGuestModelDefinitions();
  const auto first = generateGuestProfile(0x91a2b3c4ULL, 745, definitions);
  const auto repeated = generateGuestProfile(0x91a2b3c4ULL, 745, definitions);
  require(first == repeated,
          "same seed and guest id generated different guest profiles");
}

static void profiles_vary_without_shared_rng_state() {
  const auto definitions = defaultGuestModelDefinitions();
  const auto first = generateGuestProfile(2026, 1001, definitions);
  const auto other = generateGuestProfile(2026, 1002, definitions);
  const auto afterOtherGuest = generateGuestProfile(2026, 1001, definitions);
  require(first == afterOtherGuest,
          "another guest's profile draw changed a guest-local result");
  require(first.archetype != other.archetype ||
              first.sensitivities.price != other.sensitivities.price ||
              first.sensitivities.noise != other.sensitivities.noise ||
              first.patience != other.patience || first.traits != other.traits,
          "different guest ids produced indistinguishable profiles");
}

static void profile_sampling_covers_all_13_archetypes() {
  const auto definitions = defaultGuestModelDefinitions();
  std::array<bool, GuestArchetypeCount> sampled{};
  for (GuestId guestId = 1; guestId <= 4096; ++guestId) {
    const auto profile = generateGuestProfile(0x10203040ULL, guestId, definitions);
    sampled[static_cast<std::size_t>(profile.archetype)] = true;
  }
  for (const bool found : sampled)
    require(found, "fixed-seed sample omitted a supported guest archetype");
}

static void trait_count_is_zero_to_three_and_every_trait_has_a_numeric_effect() {
  const auto definitions = defaultGuestModelDefinitions();
  for (const auto &trait : definitions.traits)
    require(hasNumericEffect(trait.effects),
            "guest trait has no numeric profile or behavior effect");

  for (GuestId guestId = 1; guestId <= 4096; ++guestId) {
    const auto profile = generateGuestProfile(0x6172636865747970ULL, guestId,
                                             definitions);
    require(profile.traits.size() <= 3,
            "guest received more than three personality traits");
    for (const auto trait : profile.traits) {
      bool defined = false;
      for (const auto &definition : definitions.traits)
        defined |= definition.trait == trait &&
                   hasNumericEffect(definition.effects);
      require(defined, "guest profile contains a trait with no numeric effect");
    }
  }
}

static void profile_definitions_reject_invalid_ranges() {
  auto definitions = defaultGuestModelDefinitions();
  definitions.archetypes.front().weight = -0.25;
  std::string error;
  require(!validateGuestModelDefinitions(definitions, &error) && !error.empty(),
          "negative archetype weight was accepted");

  definitions = defaultGuestModelDefinitions();
  definitions.archetypes.front().meanSensitivities.noise =
      std::numeric_limits<double>::quiet_NaN();
  require(!validateGuestModelDefinitions(definitions, &error),
          "non-finite profile sensitivity was accepted");

  definitions = defaultGuestModelDefinitions();
  definitions.archetypes[1].archetype = definitions.archetypes[0].archetype;
  require(!validateGuestModelDefinitions(definitions, &error),
          "duplicated archetype definition was accepted");

  definitions = defaultGuestModelDefinitions();
  definitions.categoryWeights.fill(0.0);
  require(!validateGuestModelDefinitions(definitions, &error),
          "empty category weight distribution was accepted");

  definitions = defaultGuestModelDefinitions();
  definitions.traits[0].effects.queueToleranceMultiplier = 4.5;
  require(!validateGuestModelDefinitions(definitions, &error),
          "out-of-range guest trait modifier was accepted");
}

static void lifecycle_accepts_only_canonical_transitions() {
  require(canTransitionGuest(GuestLifecycleState::Prospective,
                             GuestLifecycleState::Reserved),
          "prospective guest could not be reserved");
  require(canTransitionGuest(GuestLifecycleState::Reserved,
                             GuestLifecycleState::TravelingToHotel),
          "reserved guest could not begin traveling");
  require(canTransitionGuest(GuestLifecycleState::AwaitingCheckIn,
                             GuestLifecycleState::WalkedRelocated),
          "guest could not relocate from the check-in queue");
  require(canTransitionGuest(GuestLifecycleState::Departing,
                             GuestLifecycleState::CompletedStay),
          "departing guest could not complete the stay");
  require(!canTransitionGuest(GuestLifecycleState::InStay,
                              GuestLifecycleState::Reserved),
          "in-stay guest returned to reserved state");
  require(!canTransitionGuest(GuestLifecycleState::CheckedIn,
                              GuestLifecycleState::CompletedStay),
          "checked-in guest skipped the checkout lifecycle");
  require(!canTransitionGuest(GuestLifecycleState::Cancelled,
                              GuestLifecycleState::TravelingToHotel),
          "cancelled guest resumed travel");
}

static void need_pressure_matches_hmg_formula_and_clamps_input() {
  require(std::abs(guestNeedPressure(0.0) - 2.0) < 1e-12,
          "empty need did not produce maximum pressure");
  require(std::abs(guestNeedPressure(50.0) - 0.5) < 1e-12,
          "half-satisfied need did not follow the HMG pressure curve");
  require(guestNeedPressure(100.0) == 0.0,
          "fully satisfied need produced pressure");
  require(guestNeedPressure(-20.0) == guestNeedPressure(0.0),
          "negative need score was not clamped");
  require(guestNeedPressure(120.0) == guestNeedPressure(100.0),
          "need score above 100 was not clamped");
}

static void needs_decay_and_sleep_recovery_stay_in_range() {
  const auto definitions = defaultGuestModelDefinitions();
  GuestProfile profile;
  profile.socialPreference = 0.8;
  GuestNeedState needs;
  needs.values.fill(100.0);

  const auto idle = updateGuestNeeds(needs, profile, GuestActivity::Idle, 3600.0,
                                     definitions);
  require(idle.values[static_cast<std::size_t>(GuestNeed::Energy)] == 95.0,
          "awake energy did not decay at the configured hourly rate");
  require(idle.values[static_cast<std::size_t>(GuestNeed::Hunger)] == 90.0,
          "hunger did not decay at the configured hourly rate");
  require(idle.values[static_cast<std::size_t>(GuestNeed::Hygiene)] == 98.0,
          "hygiene did not decay at the configured hourly rate");
  require(idle.values[static_cast<std::size_t>(GuestNeed::Entertainment)] ==
              96.0,
          "idle entertainment did not decay at the configured rate");
  require(idle.values[static_cast<std::size_t>(GuestNeed::Social)] == 97.0,
          "social guest need did not decay at its configured rate");

  needs.values[static_cast<std::size_t>(GuestNeed::Energy)] = 50.0;
  const auto sleeping = updateGuestNeeds(needs, profile, GuestActivity::Sleeping,
                                         3600.0, definitions);
  require(sleeping.values[static_cast<std::size_t>(GuestNeed::Energy)] == 72.0,
          "sleep did not restore energy at the configured hourly rate");
  require(sleeping.values[static_cast<std::size_t>(GuestNeed::Energy)] <= 100.0,
          "sleep recovery exceeded the need range");

  needs.values.fill(0.0);
  const auto lowerBounded = updateGuestNeeds(needs, profile, GuestActivity::Idle,
                                             3600.0, definitions);
  for (const auto value : lowerBounded.values)
    require(value >= 0.0 && value <= 100.0,
            "need update escaped the 0..100 range");
}

static void children_cannot_accept_independent_lifecycle_goal() {
  GuestProfile child;
  child.ageBand = GuestAgeBand::Child;
  GuestProfile adult;
  adult.ageBand = GuestAgeBand::Adult;
  require(!guestCanPerformGoalIndependently(child, GuestGoal::CheckIn),
          "child was allowed to perform independent check-in");
  require(!guestCanPerformGoalIndependently(child, GuestGoal::Checkout),
          "child was allowed to perform independent checkout");
  require(guestCanPerformGoalIndependently(adult, GuestGoal::CheckIn),
          "adult guest could not perform check-in");
  require(guestCanPerformGoalIndependently(child, GuestGoal::Sleep),
          "child was incorrectly barred from a discretionary goal");
}

static void group_action_requires_utility_within_30_percent() {
  require(groupAcceptsGoal(70.0, 100.0),
          "member rejected a group action at the 30-percent boundary");
  require(!groupAcceptsGoal(69.9, 100.0),
          "member accepted a group action below the 30-percent boundary");
  require(groupAcceptsGoal(0.0, 0.0),
          "zero-utility group action was rejected with no alternative");
}

static void selector_skips_unavailable_unreachable_and_over_budget_candidates() {
  GuestProfile profile;
  GuestNeedState needs;
  needs.values.fill(100.0);
  needs.values[static_cast<std::size_t>(GuestNeed::Hunger)] = 20.0;

  GuestGoalCandidate unavailable;
  unavailable.goal = GuestGoal::Eat;
  unavailable.targetId = 10;
  unavailable.requiredNeed = GuestNeed::Hunger;
  unavailable.available = false;

  GuestGoalCandidate unreachable;
  unreachable.goal = GuestGoal::Eat;
  unreachable.targetId = 11;
  unreachable.requiredNeed = GuestNeed::Hunger;
  unreachable.reachable = false;

  GuestGoalCandidate overBudget;
  overBudget.goal = GuestGoal::Eat;
  overBudget.targetId = 12;
  overBudget.requiredNeed = GuestNeed::Hunger;
  overBudget.budgetCompatible = false;

  GuestGoalCandidate viable;
  viable.goal = GuestGoal::Eat;
  viable.targetId = 13;
  viable.requiredNeed = GuestNeed::Hunger;
  viable.expectedWaitMinutes = 2.0;
  viable.queueToleranceMinutes = 10.0;

  const std::array candidates{unavailable, unreachable, overBudget, viable};
  const auto selected = selectGuestGoal(profile, needs, candidates, std::nullopt);
  require(selected.has_value(), "selector rejected every valid goal candidate");
  require(selected->targetId == viable.targetId &&
              selected->goal == GuestGoal::Eat,
          "selector chose an unavailable, unreachable, or unaffordable goal");
}

static void mandatory_goal_overrides_discretionary_goal() {
  GuestProfile profile;
  GuestNeedState needs;
  needs.values.fill(0.0);
  GuestGoalCandidate discretionary;
  discretionary.goal = GuestGoal::Eat;
  discretionary.targetId = 50;
  discretionary.requiredNeed = GuestNeed::Hunger;
  const std::array candidates{discretionary};

  const auto selected = selectGuestGoal(profile, needs, candidates,
                                        GuestGoal::Checkout);
  require(selected.has_value() && selected->mandatory,
          "mandatory lifecycle goal was not selected");
  require(selected->goal == GuestGoal::Checkout,
          "discretionary need overrode mandatory checkout");
}

static void queue_tolerance_uses_patience_segment_and_urgency() {
  GuestProfile profile;
  profile.patience = 0.5;
  require(std::abs(guestQueueToleranceMinutes(8.0, profile, 2.0, 0.5) - 8.0) <
              1e-12,
          "queue tolerance did not apply patience, segment, and urgency");
  profile.queueToleranceMultiplier = 0.65;
  require(std::abs(guestQueueToleranceMinutes(8.0, profile, 2.0, 0.5) - 5.2) <
              1e-12,
          "impatient trait modifier did not affect queue tolerance");
}

static void noise_sampling_requires_a_measured_sample_and_disrupts_for_five_minutes() {
  GuestProfile profile;
  profile.sensitivities.noise = 1.0;
  GuestNeedState state;
  std::uint64_t randomState = 42;

  const auto absent = updateGuestSleepingNoise(state, profile, std::nullopt,
                                               300.0, randomState);
  require(!absent.sampled && !absent.disrupted &&
              !absent.negativeMemoryAdded && !absent.energyGainPaused,
          "missing room-noise evidence created a guest experience");

  const auto measured = updateGuestSleepingNoise(state, profile, 100.0, 300.0,
                                                randomState);
  require(measured.sampled,
          "measured room noise was not sampled after five simulation minutes");
  require(measured.disrupted && measured.negativeMemoryAdded,
          "certain high-noise sample did not create a disruption memory");
  require(state.energyPauseRemainingSeconds == 300.0 &&
              measured.energyGainPaused,
          "noise disruption did not pause sleep recovery for five minutes");
}

int main() {
  try {
    profile_generation_is_repeatable_for_seed_and_guest_id();
    profiles_vary_without_shared_rng_state();
    profile_sampling_covers_all_13_archetypes();
    trait_count_is_zero_to_three_and_every_trait_has_a_numeric_effect();
    profile_definitions_reject_invalid_ranges();
    lifecycle_accepts_only_canonical_transitions();
    need_pressure_matches_hmg_formula_and_clamps_input();
    needs_decay_and_sleep_recovery_stay_in_range();
    children_cannot_accept_independent_lifecycle_goal();
    group_action_requires_utility_within_30_percent();
    selector_skips_unavailable_unreachable_and_over_budget_candidates();
    mandatory_goal_overrides_discretionary_goal();
    queue_tolerance_uses_patience_segment_and_urgency();
    noise_sampling_requires_a_measured_sample_and_disrupts_for_five_minutes();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}

