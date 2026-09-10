#include "StressHarness.h"
#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
#include "hh/game/GuestPsychologyArchive.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace hh::game;

std::size_t populationBudget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return 5'000;
  case hh::stress::Scale::Extended:
    return 50'000;
  case hh::stress::Scale::Exhaustive:
    return 250'000;
  }
  std::abort();
}

bool in100(int value) { return value >= 0 && value <= 100; }
bool in10000(int value) { return value >= 0 && value <= 10'000; }

void assertGuest(const GuestPsychologySnapshot &guest,
                 hh::stress::RunContext &ctx, std::uint64_t checkpoint) {
  if (guest.guestId == 0 || !GuestPsychology::validProfile(guest.profile))
    ctx.fail("invalid guest identity/profile", checkpoint);
  for (const int value : {guest.needs.energy, guest.needs.hunger,
                          guest.needs.hygiene, guest.needs.comfort,
                          guest.needs.entertainment, guest.needs.social,
                          guest.needs.privacy, guest.needs.safety})
    if (!in100(value))
      ctx.fail("guest need out of range", checkpoint);
  for (const int value : {guest.preferences.wifi, guest.preferences.desk,
                          guest.preferences.breakfast, guest.preferences.spa,
                          guest.preferences.pool, guest.preferences.fitness,
                          guest.preferences.social, guest.preferences.quietRoom,
                          guest.preferences.roomQuality})
    if (!in10000(value))
      ctx.fail("guest preference out of range", checkpoint);
  for (const int value : {guest.operational.serviceConfidence,
                          guest.operational.cleanlinessConfidence,
                          guest.operational.environmentComfort,
                          guest.operational.valuePerception,
                          guest.expectations.room, guest.expectations.cleanliness,
                          guest.expectations.service, guest.expectations.food,
                          guest.expectations.amenities, guest.expectations.quiet,
                          guest.expectations.convenience, guest.expectations.value,
                          guest.satisfaction.room, guest.satisfaction.service,
                          guest.satisfaction.cleanliness, guest.satisfaction.food,
                          guest.satisfaction.amenities,
                          guest.satisfaction.convenience, guest.satisfaction.value,
                          guest.satisfaction.arrivalDeparture,
                          guest.satisfaction.checkIn, guest.satisfaction.noise,
                          guest.satisfaction.waits,
                          guest.satisfaction.expectationFit,
                          guest.satisfaction.overall})
    if (!in100(value))
      ctx.fail("guest perception/satisfaction out of range", checkpoint);
  for (const auto &memory : guest.memories)
    if ((memory.valence != -1 && memory.valence != 1) || memory.magnitude < 0 ||
        memory.magnitude > 100 || memory.salience < 0 || memory.salience > 10'000 ||
        memory.halfLifeHours < 0)
      ctx.fail("guest memory out of range", checkpoint);
  for (const auto &complaint : guest.complaints)
    if (complaint.magnitude < 0 || complaint.magnitude > 100)
      ctx.fail("guest complaint out of range", checkpoint);
  const auto repeatIntent = calculateRepeatIntent(guest, checkpoint);
  if (!in100(repeatIntent))
    ctx.fail("repeat intent out of range", checkpoint);
}

GuestOpportunitySnapshot opportunitiesFor(std::uint64_t guestId,
                                          hh::stress::Rng &rng) {
  GuestOpportunitySnapshot snapshot;
  const std::array<GuestGoalClass, 8> goals{
      GuestGoalClass::Eat,       GuestGoalClass::Work,
      GuestGoalClass::Exercise,  GuestGoalClass::Swim,
      GuestGoalClass::Socialize, GuestGoalClass::Relax,
      GuestGoalClass::RequestService, GuestGoalClass::ResolveComplaint};
  for (std::size_t index = 0; index < goals.size(); ++index) {
    GoalOpportunity opportunity;
    opportunity.stableGoalId = static_cast<std::uint32_t>(guestId % 1000 * 16 + index + 1);
    opportunity.goal = goals[index];
    opportunity.needScore = static_cast<int>(rng.bounded(101));
    opportunity.preference = 5'000 + static_cast<int>(rng.bounded(10'001));
    opportunity.availability = (index % 5 == 0) ? 0 : 10'000;
    opportunity.timeCompatibility = 7'500 + static_cast<int>(rng.bounded(7'501));
    opportunity.budgetCompatibility = 7'500 + static_cast<int>(rng.bounded(7'501));
    opportunity.groupCompatibility = 7'500 + static_cast<int>(rng.bounded(7'501));
    opportunity.moodModifier = 7'500 + static_cast<int>(rng.bounded(7'501));
    opportunity.expectedTravelSeconds = static_cast<int>(rng.bounded(1'801));
    opportunity.expectedWaitSeconds = static_cast<int>(rng.bounded(1'801));
    snapshot.opportunities.push_back(opportunity);
  }
  return snapshot;
}

std::string runPopulation(std::uint64_t seed, std::size_t population,
                          hh::stress::RunContext &ctx) {
  GuestPsychology psychology(seed);
  hh::stress::Rng rng{seed ^ 0x589965CC75374CC3ULL};
  std::set<GuestArchetype> archetypes;
  std::ostringstream canonical;

  for (std::size_t index = 0; index < population; ++index) {
    const GuestId id = static_cast<GuestId>(index + 1);
    auto profile = psychology.generateGuestProfile(id);
    profile.archetype = static_cast<GuestArchetype>(index % 13);
    if (!GuestPsychology::validProfile(profile))
      ctx.fail("cycled archetype profile became invalid", index);
    archetypes.insert(profile.archetype);
    psychology.initializeGuest(id, profile);

    psychology.updateNeeds(id, 3600 + static_cast<std::int64_t>(rng.bounded(12 * 3600)),
                           (index % 7) == 0);
    GuestExpectationInputs expectations;
    expectations.segmentBase = {70, 70, 70, 70, 70, 70, 70, 70};
    expectations.starClassModifier = {5, 5, 5, 5, 5, 5, 5, 5};
    psychology.updateExpectations(id, expectations);

    ExperienceEvent event;
    event.type = (index % 2) ? ExperienceEventType::GreatMeal
                             : ExperienceEventType::LongCheckInQueue;
    event.timestampSeconds = static_cast<std::int64_t>(index * 60);
    event.locationId = 100 + index % 10;
    event.sourceEntityId = 200 + index % 20;
    event.category = (index % 2) ? ExperienceCategory::Food
                                 : ExperienceCategory::ArrivalDeparture;
    event.rawImpact = (index % 2) ? 25 : -40;
    event.memorySalience = 10'000;
    event.memoryHalfLifeHours = 24;
    event.complaintEligible = event.rawImpact < 0;
    psychology.recordExperience(id, event);

    const auto guest = psychology.snapshot(id);
    if (!guest)
      ctx.fail("guest state disappeared", index);
    assertGuest(*guest, ctx, index);

    const auto rawOpportunities = opportunitiesFor(id, rng);
    const auto personalized = applyGuestPreferences(guest->preferences, rawOpportunities);
    const auto first = chooseGuestGoal(id, personalized);
    const auto second = chooseGuestGoal(id, personalized);
    if (!first.valid || first != second)
      ctx.fail("guest goal selection was invalid or nondeterministic", index);

    canonical << detail::serializeGuestPsychology(*guest) << '|'
              << first.stableGoalId << ':' << static_cast<int>(first.goal) << ':'
              << first.utility << '\n';
  }

  if (archetypes.size() != 13)
    ctx.fail("stress population did not cover all 13 archetypes", population);
  return canonical.str();
}

void stressMemoryPressure(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"final02-guest", config};
  ctx.phase = "memory_pressure";
  GuestPsychology psychology(config.seed ^ 0xDB4F0B9175AE2165ULL);
  constexpr GuestId id = 900'001;
  auto profile = psychology.generateGuestProfile(id);
  psychology.initializeGuest(id, profile);

  for (int cycle = 0; cycle < 8; ++cycle) {
    const auto base = static_cast<std::int64_t>(cycle) * 48 * 3600;
    for (int eventIndex = 0; eventIndex < 64; ++eventIndex) {
      ExperienceEvent event;
      event.type = ExperienceEventType::ElevatorDelay;
      event.timestampSeconds = base + eventIndex;
      event.category = ExperienceCategory::Convenience;
      event.rawImpact = -10;
      event.memorySalience = 10'000;
      event.memoryHalfLifeHours = 1;
      psychology.recordExperience(id, event);
    }
    const auto state = psychology.snapshot(id);
    if (!state)
      ctx.fail("memory-pressure guest disappeared", cycle);
    if (cycle >= 2 && state->memories.size() > 128)
      ctx.fail("decayed long-stay memories grow without a steady bound", cycle,
               std::to_string(state->memories.size()));
  }
}

void stressComplaintDedup(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"final02-guest", config};
  ctx.phase = "complaint_pressure";
  GuestPsychology psychology(config.seed ^ 0x9E3779B97F4A7C15ULL);
  constexpr GuestId id = 900'002;
  auto profile = psychology.generateGuestProfile(id);
  profile.serviceSensitivity = 1.0;
  psychology.initializeGuest(id, profile);
  ExperienceEvent event;
  event.type = ExperienceEventType::LongCheckInQueue;
  event.timestampSeconds = 1234;
  event.locationId = 77;
  event.sourceEntityId = 88;
  event.category = ExperienceCategory::ArrivalDeparture;
  event.rawImpact = -60;
  event.memorySalience = 10'000;
  event.memoryHalfLifeHours = 24;
  event.complaintEligible = true;
  for (int i = 0; i < 1'000; ++i)
    psychology.recordExperience(id, event);
  const auto state = psychology.snapshot(id);
  if (!state || state->complaints.size() != 1)
    ctx.fail("same complaint incident was duplicated", 1'000,
             state ? std::to_string(state->complaints.size()) : "missing");
}

void stressGroupAndFallback(const hh::stress::Config &config) {
  hh::stress::RunContext ctx{"final02-guest", config};
  ctx.phase = "group_reference_churn";
  for (std::uint64_t groupId = 1; groupId <= 5'000; ++groupId) {
    GuestGroup group;
    group.id = groupId;
    group.leader = groupId * 10 + 1;
    group.members = {group.leader, groupId * 10 + 2, groupId * 10 + 3};
    group.cohesion = static_cast<int>(groupId % 10'001);
    group.cohesionRadiusTiles = static_cast<int>(groupId % 64);
    group.sharedItinerary = {GuestGoalClass::Eat, GuestGoalClass::Relax};
    if (!detail::validGuestGroup(group) ||
        detail::deserializeGuestGroup(detail::serializeGuestGroup(group)) != group)
      ctx.fail("guest group reference/archive churn failed", groupId);
  }

  ctx.phase = "unavailable_goal_fallback";
  GuestOpportunitySnapshot unavailable;
  for (std::uint32_t id : {30U, 10U, 20U}) {
    GoalOpportunity opportunity;
    opportunity.stableGoalId = id;
    opportunity.goal = GuestGoalClass::Relax;
    opportunity.needScore = 50;
    opportunity.availability = 0;
    unavailable.opportunities.push_back(opportunity);
  }
  const auto selection = chooseGuestGoal(1, unavailable);
  if (!selection.valid || selection.stableGoalId != 10 || selection.utility != 0)
    ctx.fail("unavailable-goal deterministic fallback changed", 0);
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "mass_arrival" ||
      scenario == "need_pressure" || scenario == "congested_goal_selection" ||
      scenario == "memory_pressure" || scenario == "complaint_pressure" ||
      scenario == "loyalty_evolution" || scenario == "group_reference_churn" ||
      scenario == "unavailable_goal_fallback")
    return;
  throw std::invalid_argument("unknown guest psychology stress scenario");
}
} // namespace

int main() {
  try {
    const auto config = hh::stress::configFromEnvironment(0xC0FFEE1234ABCDEFULL);
    validateScenario(config.scenario);
    const auto population = populationBudget(config.scale);
    hh::stress::RunContext ctx{"final02-guest", config};
    ctx.phase = "mass_arrival";

    if (config.scenario.empty() || config.scenario == "mass_arrival" ||
        config.scenario == "need_pressure" ||
        config.scenario == "congested_goal_selection" ||
        config.scenario == "loyalty_evolution") {
      const auto first = runPopulation(config.seed, population, ctx);
      const auto second = runPopulation(config.seed, population, ctx);
      if (first != second)
        ctx.fail("identical guest population replay diverged", population);
    }
    if (config.scenario.empty() || config.scenario == "memory_pressure")
      stressMemoryPressure(config);
    if (config.scenario.empty() || config.scenario == "complaint_pressure")
      stressComplaintDedup(config);
    if (config.scenario.empty() || config.scenario == "group_reference_churn" ||
        config.scenario == "unavailable_goal_fallback")
      stressGroupAndFallback(config);

    std::cout << "StressGuestPsychology PASS population=" << population << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
