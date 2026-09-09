#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

namespace hh::game {
namespace {
struct GuestArchetypeDefaults {
  GuestArchetype archetype;
  int weight;
  std::int64_t budgetPerNightCents;
  double priceSensitivity;
  double serviceSensitivity;
  double cleanlinessSensitivity;
  double noiseSensitivity;
  double privacySensitivity;
  double safetySensitivity;
  double comfortSensitivity;
  double foodSensitivity;
  double patience;
};

constexpr std::array<GuestArchetypeDefaults, 13> archetypes{{
    {GuestArchetype::BudgetLeisure, 13, 9000, .90, .45, .55, .40, .35, .55,
     .55, .45, .55},
    {GuestArchetype::Backpacker, 8, 7000, .95, .35, .40, .35, .25, .45, .35,
     .35, .70},
    {GuestArchetype::BusinessTraveler, 18, 18000, .45, .75, .80, .85, .55,
     .65, .70, .70, .45},
    {GuestArchetype::ExecutiveBusiness, 7, 26000, .25, .90, .90, .85, .80,
     .80, .90, .75, .40},
    {GuestArchetype::CoupleLeisure, 14, 17000, .55, .60, .70, .65, .60, .65,
     .75, .70, .65},
    {GuestArchetype::FamilyLeisure, 12, 19000, .65, .70, .85, .65, .35, .85,
     .75, .75, .55},
    {GuestArchetype::LuxuryLeisure, 5, 30000, .20, .95, .95, .85, .90, .90,
     .98, .90, .45},
    {GuestArchetype::ConferenceDelegate, 7, 16000, .50, .70, .75, .75, .50,
     .65, .65, .70, .50},
    {GuestArchetype::GroupTourTraveler, 5, 11000, .80, .45, .60, .50, .30,
     .65, .50, .55, .60},
    {GuestArchetype::AirportTransitTraveler, 4, 13000, .70, .55, .70, .65,
     .40, .70, .60, .45, .35},
    {GuestArchetype::WellnessTraveler, 3, 22000, .35, .80, .85, .75, .80,
     .75, .85, .75, .75},
    {GuestArchetype::VipCelebrity, 2, 30000, .10, .98, .98, .95, .98, .95,
     1.0, .90, .25},
    {GuestArchetype::CriticReviewer, 2, 22000, .35, 1.0, 1.0, .90, .85, .85,
     .95, .95, .35},
}};
static_assert([] {
  int total = 0;
  for (const auto &archetype : archetypes)
    total += archetype.weight;
  return total == 100;
}());

double randomUnit(std::mt19937_64 &random) noexcept {
  return static_cast<double>(random() >> 11) *
         (1.0 / static_cast<double>(std::uint64_t{1} << 53));
}

bool traitConflicts(std::uint32_t flags, GuestTrait candidate) noexcept {
  const auto has = [&](GuestTrait trait) {
    return (flags & guestTraitFlag(trait)) != 0;
  };
  switch (candidate) {
  case GuestTrait::Patient:
    return has(GuestTrait::Impatient);
  case GuestTrait::Impatient:
    return has(GuestTrait::Patient);
  case GuestTrait::Neat:
    return has(GuestTrait::Messy);
  case GuestTrait::Messy:
    return has(GuestTrait::Neat);
  case GuestTrait::LightSleeper:
    return has(GuestTrait::HeavySleeper);
  case GuestTrait::HeavySleeper:
    return has(GuestTrait::LightSleeper);
  case GuestTrait::Social:
    return has(GuestTrait::Private);
  case GuestTrait::Private:
    return has(GuestTrait::Social);
  case GuestTrait::EarlyRiser:
    return has(GuestTrait::NightOwl);
  case GuestTrait::NightOwl:
    return has(GuestTrait::EarlyRiser);
  default:
    return false;
  }
}

std::uint64_t mix(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}
} // namespace

GuestPsychology::GuestPsychology(std::uint64_t campaignSeed) noexcept
    : campaignSeed_(campaignSeed) {}

GuestProfileView GuestPsychology::generateGuestProfile(GuestId guestId) const {
  std::mt19937_64 random(
      mix(campaignSeed_ ^ mix(static_cast<std::uint64_t>(guestId)) ^
          0x4755455354505359ULL));
  return detail::generateGuestProfileFromRandom(random);
}

bool GuestPsychology::validProfile(const GuestProfileView &profile) noexcept {
  return detail::validGuestProfile(profile);
}

namespace detail {
GuestProfileView generateGuestProfileFromRandom(std::mt19937_64 &random) {
  int roll = static_cast<int>(random() % 100);
  const GuestArchetypeDefaults *defaults = &archetypes.back();
  for (const auto &candidate : archetypes) {
    if (roll < candidate.weight) {
      defaults = &candidate;
      break;
    }
    roll -= candidate.weight;
  }
  const auto varied = [&](double mean) {
    return std::clamp(mean + (randomUnit(random) * 2.0 - 1.0) * .08, 0.0,
                      1.0);
  };
  GuestProfileView profile;
  profile.archetype = defaults->archetype;
  profile.budgetPerNightCents = static_cast<std::int64_t>(std::llround(
      defaults->budgetPerNightCents * (.90 + randomUnit(random) * .20)));
  profile.priceSensitivity = varied(defaults->priceSensitivity);
  profile.serviceSensitivity = varied(defaults->serviceSensitivity);
  profile.cleanlinessSensitivity = varied(defaults->cleanlinessSensitivity);
  profile.noiseSensitivity = varied(defaults->noiseSensitivity);
  profile.privacySensitivity = varied(defaults->privacySensitivity);
  profile.safetySensitivity = varied(defaults->safetySensitivity);
  profile.comfortSensitivity = varied(defaults->comfortSensitivity);
  profile.foodSensitivity = varied(defaults->foodSensitivity);
  profile.patience = varied(defaults->patience);

  const int traitCount = static_cast<int>(random() % 4);
  for (int attempts = 0;
       std::popcount(profile.traitFlags) < traitCount && attempts < 64;
       ++attempts) {
    const auto trait = static_cast<GuestTrait>(random() % 17);
    const auto flag = guestTraitFlag(trait);
    if ((profile.traitFlags & flag) == 0 &&
        !traitConflicts(profile.traitFlags, trait))
      profile.traitFlags |= flag;
  }
  const auto has = [&](GuestTrait trait) {
    return (profile.traitFlags & guestTraitFlag(trait)) != 0;
  };
  if (has(GuestTrait::Neat))
    profile.cleanlinessSensitivity =
        std::min(1.0, profile.cleanlinessSensitivity + .12);
  if (has(GuestTrait::Messy))
    profile.cleanlinessSensitivity =
        std::max(0.0, profile.cleanlinessSensitivity - .10);
  if (has(GuestTrait::LightSleeper))
    profile.noiseSensitivity = std::min(1.0, profile.noiseSensitivity + .15);
  if (has(GuestTrait::HeavySleeper))
    profile.noiseSensitivity = std::max(0.0, profile.noiseSensitivity - .15);
  if (has(GuestTrait::Foodie))
    profile.foodSensitivity = std::min(1.0, profile.foodSensitivity + .20);
  if (has(GuestTrait::Workaholic))
    profile.serviceSensitivity = std::min(1.0, profile.serviceSensitivity + .10);
  if (has(GuestTrait::Social))
    profile.privacySensitivity = std::max(0.0, profile.privacySensitivity - .15);
  if (has(GuestTrait::Private))
    profile.privacySensitivity = std::min(1.0, profile.privacySensitivity + .20);
  if (has(GuestTrait::Frugal)) {
    profile.priceSensitivity = std::min(1.0, profile.priceSensitivity + .20);
    profile.budgetPerNightCents = profile.budgetPerNightCents * 9 / 10;
  }
  if (has(GuestTrait::StatusConscious)) {
    profile.serviceSensitivity = std::min(1.0, profile.serviceSensitivity + .15);
    profile.comfortSensitivity = std::min(1.0, profile.comfortSensitivity + .15);
  }
  if (has(GuestTrait::FitnessFocused))
    profile.comfortSensitivity = std::min(1.0, profile.comfortSensitivity + .08);
  if (has(GuestTrait::EarlyRiser))
    profile.patience = std::min(1.0, profile.patience + .05);
  if (has(GuestTrait::NightOwl))
    profile.patience = std::max(0.0, profile.patience - .03);
  if (has(GuestTrait::ComplaintProne))
    profile.serviceSensitivity = std::min(1.0, profile.serviceSensitivity + .18);
  if (has(GuestTrait::Forgiving))
    profile.serviceSensitivity = std::max(0.0, profile.serviceSensitivity - .10);
  return profile;
}

bool validGuestProfile(const GuestProfileView &profile) noexcept {
  const auto archetype = static_cast<int>(profile.archetype);
  const auto normalized = [](double value) {
    return std::isfinite(value) && value >= 0 && value <= 1;
  };
  constexpr auto validTraitFlags =
      (guestTraitFlag(GuestTrait::Forgiving) << 1) - 1;
  if (archetype < 0 || archetype >= 13 || profile.budgetPerNightCents < 4000 ||
      profile.budgetPerNightCents > 1'000'000 ||
      !normalized(profile.priceSensitivity) ||
      !normalized(profile.serviceSensitivity) ||
      !normalized(profile.cleanlinessSensitivity) ||
      !normalized(profile.noiseSensitivity) ||
      !normalized(profile.privacySensitivity) ||
      !normalized(profile.safetySensitivity) ||
      !normalized(profile.comfortSensitivity) ||
      !normalized(profile.foodSensitivity) || !normalized(profile.patience) ||
      (profile.traitFlags & ~validTraitFlags) != 0 ||
      std::popcount(profile.traitFlags) > 3)
    return false;
  for (GuestTrait trait : {GuestTrait::Patient, GuestTrait::Impatient,
                           GuestTrait::Neat, GuestTrait::Messy,
                           GuestTrait::LightSleeper, GuestTrait::HeavySleeper,
                           GuestTrait::Social, GuestTrait::Private,
                           GuestTrait::EarlyRiser, GuestTrait::NightOwl})
    if ((profile.traitFlags & guestTraitFlag(trait)) != 0 &&
        traitConflicts(profile.traitFlags & ~guestTraitFlag(trait), trait))
      return false;
  return true;
}

int queueToleranceFor(const GuestProfileView &profile,
                      double baseSeconds) noexcept {
  double segmentModifier = 1.0;
  switch (profile.archetype) {
  case GuestArchetype::FamilyLeisure:
    segmentModifier = 1.15;
    break;
  case GuestArchetype::GroupTourTraveler:
    segmentModifier = 1.20;
    break;
  case GuestArchetype::BusinessTraveler:
    segmentModifier = .90;
    break;
  case GuestArchetype::ExecutiveBusiness:
    segmentModifier = .85;
    break;
  case GuestArchetype::AirportTransitTraveler:
    segmentModifier = .70;
    break;
  case GuestArchetype::VipCelebrity:
    segmentModifier = .60;
    break;
  case GuestArchetype::CriticReviewer:
    segmentModifier = .75;
    break;
  default:
    break;
  }
  double traitModifier = 1.0;
  if (profile.traitFlags & guestTraitFlag(GuestTrait::Patient))
    traitModifier *= 1.30;
  if (profile.traitFlags & guestTraitFlag(GuestTrait::Impatient))
    traitModifier *= .65;
  return static_cast<int>(std::clamp(
      std::lround(baseSeconds * (.5 + profile.patience) * segmentModifier *
                  traitModifier),
      120L, 1200L));
}
} // namespace detail
} // namespace hh::game
