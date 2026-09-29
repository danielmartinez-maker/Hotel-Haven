#include "hh/game/GuestModel.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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
      effect.complaintThresholdDelta != 0.0)
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
              first.priceSensitivity != other.priceSensitivity ||
              first.noiseSensitivity != other.noiseSensitivity ||
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
}

int main() {
  try {
    profile_generation_is_repeatable_for_seed_and_guest_id();
    profiles_vary_without_shared_rng_state();
    profile_sampling_covers_all_13_archetypes();
    trait_count_is_zero_to_three_and_every_trait_has_a_numeric_effect();
    profile_definitions_reject_invalid_ranges();
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}

