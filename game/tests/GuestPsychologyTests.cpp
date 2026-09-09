#include "hh/game/GuestGoals.h"
#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

bool conflicts(std::uint32_t flags, GuestTrait a, GuestTrait b) {
  return (flags & guestTraitFlag(a)) != 0 &&
         (flags & guestTraitFlag(b)) != 0;
}

const PersonView &firstGuest(const SimulationView &view) {
  for (const auto &person : view.people)
    if (person.kind == PersonKind::Guest)
      return person;
  throw std::runtime_error("guest psychology fixture produced no guest");
}

void same_seed_and_guest_id_produce_identical_profile() {
  GuestPsychology a(42), b(42);
  const auto pa = a.generateGuestProfile(1001);
  const auto pb = b.generateGuestProfile(1001);
  require(pa == pb, "same seed and guest id produced different psychology");
  require(GuestPsychology::validProfile(pa),
          "deterministic generated profile was outside valid bounds");
}

void all_thirteen_archetypes_are_reachable_and_profiles_are_bounded() {
  GuestPsychology psychology(20260909);
  std::set<GuestArchetype> archetypes;
  for (GuestId id = 1; id <= 5000; ++id) {
    const auto profile = psychology.generateGuestProfile(id);
    archetypes.insert(profile.archetype);
    require(GuestPsychology::validProfile(profile),
            "generated guest profile violated bounds");
    require(std::popcount(profile.traitFlags) <= 3,
            "generated guest received more than three traits");
    require(!conflicts(profile.traitFlags, GuestTrait::Patient,
                       GuestTrait::Impatient) &&
                !conflicts(profile.traitFlags, GuestTrait::Neat,
                           GuestTrait::Messy) &&
                !conflicts(profile.traitFlags, GuestTrait::LightSleeper,
                           GuestTrait::HeavySleeper) &&
                !conflicts(profile.traitFlags, GuestTrait::Social,
                           GuestTrait::Private) &&
                !conflicts(profile.traitFlags, GuestTrait::EarlyRiser,
                           GuestTrait::NightOwl),
            "generated guest received conflicting traits");
  }
  require(archetypes.size() == 13,
          "not all thirteen baseline guest archetypes were reachable");
}

void profile_generation_is_keyed_by_stable_guest_identity() {
  GuestPsychology psychology(77);
  const auto first = psychology.generateGuestProfile(9001);
  for (int i = 0; i < 100; ++i)
    (void)psychology.generateGuestProfile(static_cast<GuestId>(10000 + i));
  const auto again = psychology.generateGuestProfile(9001);
  require(first == again,
          "guest profile changed after unrelated profile generation");
}

void awake_and_sleeping_need_updates_use_hmg_rates() {
  GuestPsychology psychology(88);
  const GuestId id = 2001;
  psychology.initializeGuest(id, psychology.generateGuestProfile(id));
  const auto initial = psychology.snapshot(id);
  require(initial.has_value(), "guest psychology state was not initialized");

  psychology.updateNeeds(id, 3600, false);
  auto awake = psychology.snapshot(id);
  require(awake.has_value() &&
              awake->needs.hunger == initial->needs.hunger - 10 &&
              awake->needs.energy == initial->needs.energy - 5,
          "awake need decay did not use HMG-010 hourly rates");

  psychology.updateNeeds(id, 3600, true);
  auto sleeping = psychology.snapshot(id);
  require(sleeping.has_value() &&
              sleeping->needs.energy ==
                  std::min(100, awake->needs.energy + 22) &&
              sleeping->needs.hunger == awake->needs.hunger,
          "sleeping need recovery did not use HMG-010 energy rate");
}

void negative_service_experience_creates_attributed_memory_and_complaint() {
  GuestPsychology psychology(89);
  const GuestId id = 2002;
  auto profile = psychology.generateGuestProfile(id);
  profile.serviceSensitivity = 1.0;
  psychology.initializeGuest(id, profile);

  ExperienceEvent event;
  event.type = ExperienceEventType::LongCheckInQueue;
  event.timestampSeconds = 3600;
  event.category = ExperienceCategory::ArrivalDeparture;
  event.observedValue = 45 * 60;
  event.expectedValue = 8 * 60;
  event.rawImpact = -30;
  event.memorySalience = 10000;
  event.memoryHalfLifeHours = 24;
  event.complaintEligible = true;
  psychology.recordExperience(id, event);

  const auto state = psychology.snapshot(id);
  require(state.has_value(), "guest psychology state disappeared");
  require(state->satisfaction.checkIn < 50 &&
              state->satisfaction.arrivalDeparture < 50,
          "long check-in wait was not attributed to arrival satisfaction");
  require(state->memories.size() == 1 &&
              state->memories.front().type ==
                  ExperienceEventType::LongCheckInQueue &&
              state->memories.front().magnitude == 30 &&
              !state->memories.front().resolved,
          "negative service event did not create an attributable memory");
  require(state->complaints.size() == 1 &&
              state->complaints.front().type == ComplaintType::CheckInDelay &&
              state->complaints.front().urgency == ComplaintUrgency::Low,
          "complaint-eligible check-in delay did not create the required complaint");
}

void memory_contribution_halves_at_configured_half_life() {
  GuestMemory memory;
  memory.valence = -1;
  memory.magnitude = 60;
  memory.salience = 10000;
  memory.timestampSeconds = 10 * 3600;
  memory.halfLifeHours = 12;
  const double now = GuestPsychology::memoryContribution(memory, 10 * 3600);
  const double later = GuestPsychology::memoryContribution(memory, 22 * 3600);
  require(std::abs(later - now * .5) < 1e-9,
          "guest memory did not halve after one configured half-life");
}

void complaint_threshold_respects_magnitude_and_guest_sensitivity() {
  GuestPsychology psychology(90);
  const GuestId id = 2003;
  auto profile = psychology.generateGuestProfile(id);
  profile.serviceSensitivity = .20;
  profile.traitFlags &= ~guestTraitFlag(GuestTrait::ComplaintProne);
  psychology.initializeGuest(id, profile);

  ExperienceEvent event;
  event.type = ExperienceEventType::LongCheckInQueue;
  event.timestampSeconds = 100;
  event.category = ExperienceCategory::ArrivalDeparture;
  event.rawImpact = -30;
  event.memorySalience = 10000;
  event.memoryHalfLifeHours = 24;
  event.complaintEligible = true;
  psychology.recordExperience(id, event);
  require(psychology.snapshot(id)->complaints.empty(),
          "low-sensitivity guest complained below HMG-010 magnitude override");

  event.timestampSeconds = 200;
  event.rawImpact = -50;
  psychology.recordExperience(id, event);
  require(psychology.snapshot(id)->complaints.size() == 1,
          "magnitude-50 complaint override was not honored");
}

void live_simulation_exposes_authoritative_psychology_and_updates_needs() {
  auto sim = Simulation::tutorial(169);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "live psychology fixture definitions rejected");
  sim.step(3600);
  const auto guest = firstGuest(sim.view());
  const auto before = sim.guestPsychology(guest.reservationId);
  require(before.guestId == guest.reservationId && before.profile == guest.profile,
          "live guest did not expose matching authoritative psychology");

  sim.step(3600);
  const auto after = sim.guestPsychology(guest.reservationId);
  require(after.needs.hunger <= before.needs.hunger - 9 &&
              after.needs.energy <= before.needs.energy - 4,
          "live one-second simulation did not advance authoritative guest needs");
}

void severe_live_check_in_wait_creates_retained_memory_and_complaint() {
  auto sim = Simulation::tutorial(169);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "wait psychology fixture definitions rejected");
  EntityId receptionist{};
  for (const auto &person : sim.view().people)
    if (person.kind == PersonKind::Receptionist) {
      receptionist = person.id;
      break;
    }
  require(receptionist && sim.fireStaff(receptionist).ok,
          "wait psychology fixture could not remove receptionist");
  sim.step(3600);
  const auto guestId = firstGuest(sim.view()).reservationId;
  sim.step(1900);
  const auto psychology = sim.guestPsychology(guestId);
  const bool hasWaitMemory = std::any_of(
      psychology.memories.begin(), psychology.memories.end(),
      [](const GuestMemory &memory) {
        return memory.type == ExperienceEventType::LongCheckInQueue;
      });
  const bool hasDelayComplaint = std::any_of(
      psychology.complaints.begin(), psychology.complaints.end(),
      [](const Complaint &complaint) {
        return complaint.type == ComplaintType::CheckInDelay;
      });
  require(hasWaitMemory && hasDelayComplaint,
          "severe live check-in wait lacked attributed memory/complaint");
}

void simulation_exposes_deterministic_goal_selection_interface() {
  auto sim = Simulation::tutorial(171);
  GuestOpportunitySnapshot opportunities;
  GoalOpportunity relax;
  relax.stableGoalId = 20;
  relax.goal = GuestGoalClass::Relax;
  relax.needScore = 30;
  GoalOpportunity eat = relax;
  eat.stableGoalId = 10;
  eat.goal = GuestGoalClass::Eat;
  opportunities.opportunities = {relax, eat};
  const auto selected = sim.chooseGuestGoal(55, opportunities);
  require(selected.valid && selected.stableGoalId == 10,
          "Simulation guest-goal interface did not preserve stable tie-break");
}

void live_guest_psychology_survives_save_round_trip() {
  auto sim = Simulation::tutorial(172);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "psychology save fixture definitions rejected");
  sim.step(3600);
  const auto guestId = firstGuest(sim.view()).reservationId;
  sim.step(733);
  const auto before = sim.guestPsychology(guestId);
  const auto loaded = Simulation::load(sim.save());
  const auto after = loaded.guestPsychology(guestId);
  require(before == after,
          "authoritative guest psychology did not survive save/load exactly");
  require(loaded.save() == sim.save(),
          "psychology save was not byte-stable after round trip");
}

void repeat_intent_responds_to_expectation_adjusted_satisfaction() {
  GuestPsychologySnapshot high;
  high.satisfaction.overall = 92;
  high.satisfaction.expectationFit = 90;
  high.operational.valuePerception = 85;
  high.expectations.room = high.expectations.cleanliness =
      high.expectations.service = high.expectations.food =
          high.expectations.amenities = high.expectations.quiet =
              high.expectations.convenience = high.expectations.value = 80;
  auto low = high;
  low.satisfaction.overall = 55;
  low.satisfaction.expectationFit = 45;
  low.operational.valuePerception = 50;
  require(calculateRepeatIntent(high, 0) > calculateRepeatIntent(low, 0),
          "repeat intent ignored expectation-adjusted satisfaction");
}

void material_memories_influence_repeat_intent() {
  GuestPsychologySnapshot positive;
  positive.satisfaction.overall = 80;
  positive.satisfaction.expectationFit = 80;
  positive.operational.valuePerception = 80;
  GuestMemory good;
  good.valence = 1;
  good.magnitude = 50;
  good.salience = 10000;
  good.halfLifeHours = 48;
  positive.memories.push_back(good);
  auto negative = positive;
  negative.memories.front().valence = -1;
  require(calculateRepeatIntent(positive, 0) >
              calculateRepeatIntent(negative, 0),
          "repeat intent ignored material memory sentiment");
}

void material_experience_persists_in_versioned_save() {
  auto sim = Simulation::tutorial(174);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "material experience fixture definitions rejected");
  sim.step(3600);
  const auto guestId = firstGuest(sim.view()).reservationId;
  ExperienceEvent meal;
  meal.type = ExperienceEventType::GreatMeal;
  meal.timestampSeconds = sim.view().elapsedSeconds;
  meal.category = ExperienceCategory::Food;
  meal.observedValue = 95;
  meal.expectedValue = 75;
  meal.rawImpact = 25;
  meal.memorySalience = 10000;
  meal.memoryHalfLifeHours = 48;
  require(sim.recordGuestExperience(guestId, meal).ok,
          "simulation rejected material guest experience");
  const auto before = sim.guestPsychology(guestId);
  require(std::any_of(before.memories.begin(), before.memories.end(),
                      [](const GuestMemory &memory) {
                        return memory.type == ExperienceEventType::GreatMeal;
                      }),
          "material guest experience was not retained before save");
  const auto saved = sim.save();
  require(saved.rfind("HHGS 12 ", 0) == 0,
          "psychology schema change did not advance save version to 12");
  const auto loaded = Simulation::load(saved);
  require(loaded.guestPsychology(guestId) == before,
          "material psychology history changed across v12 save/load");
  require(loaded.save() == saved,
          "v12 psychology save was not byte-stable after round trip");
}
} // namespace

int main() {
  try {
    same_seed_and_guest_id_produce_identical_profile();
    all_thirteen_archetypes_are_reachable_and_profiles_are_bounded();
    profile_generation_is_keyed_by_stable_guest_identity();
    awake_and_sleeping_need_updates_use_hmg_rates();
    negative_service_experience_creates_attributed_memory_and_complaint();
    memory_contribution_halves_at_configured_half_life();
    complaint_threshold_respects_magnitude_and_guest_sensitivity();
    live_simulation_exposes_authoritative_psychology_and_updates_needs();
    severe_live_check_in_wait_creates_retained_memory_and_complaint();
    simulation_exposes_deterministic_goal_selection_interface();
    live_guest_psychology_survives_save_round_trip();
    repeat_intent_responds_to_expectation_adjusted_satisfaction();
    material_memories_influence_repeat_intent();
    material_experience_persists_in_versioned_save();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest psychology tests passed\n";
  return 0;
}
