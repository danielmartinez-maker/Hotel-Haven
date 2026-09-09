#include "hh/game/GuestPsychology.h"
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
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest psychology tests passed\n";
  return 0;
}
