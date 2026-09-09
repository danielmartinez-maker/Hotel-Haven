#include "hh/game/GuestPsychology.h"
#include <bit>
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
} // namespace

int main() {
  try {
    same_seed_and_guest_id_produce_identical_profile();
    all_thirteen_archetypes_are_reachable_and_profiles_are_bounded();
    profile_generation_is_keyed_by_stable_guest_identity();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest psychology profile tests passed\n";
  return 0;
}
