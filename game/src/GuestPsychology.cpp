#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <stdexcept>

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

constexpr std::array<GuestPreferenceState, 13> archetypePreferences{{
    {6000, 3000, 6500, 2000, 5000, 3500, 6000, 4500, 4000},
    {7500, 2500, 6500, 1000, 3000, 4000, 8500, 3000, 3000},
    {9500, 8000, 7000, 1000, 1500, 4500, 3500, 8500, 7000},
    {9800, 9000, 7500, 3000, 2000, 5000, 3000, 9000, 9000},
    {6000, 2000, 7500, 6000, 5500, 3000, 6500, 7000, 7500},
    {6000, 1500, 8500, 2000, 8500, 3000, 7000, 6000, 7000},
    {8000, 3500, 8500, 9500, 8500, 6500, 6000, 9000, 9800},
    {9000, 7500, 8000, 2500, 2000, 4000, 8000, 7000, 6500},
    {5500, 1000, 9000, 2500, 4500, 2500, 9000, 4500, 5000},
    {8000, 3500, 6500, 1000, 1000, 2000, 2500, 8500, 5500},
    {5000, 1500, 8000, 9500, 7000, 9500, 5000, 9000, 8500},
    {9000, 6000, 9000, 9500, 9000, 8000, 5500, 9800, 10000},
    {8000, 5500, 8500, 7000, 6000, 5000, 4500, 9500, 9500},
}};

int clampPreference(int value) noexcept { return std::clamp(value, 0, 10000); }

GuestPreferenceState preferencesFor(const GuestProfileView &profile) noexcept {
  const auto index = static_cast<std::size_t>(profile.archetype);
  GuestPreferenceState preferences =
      index < archetypePreferences.size() ? archetypePreferences[index]
                                          : GuestPreferenceState{};
  const auto has = [&](GuestTrait trait) {
    return (profile.traitFlags & guestTraitFlag(trait)) != 0;
  };
  const auto add = [&](int &value, int delta) {
    value = clampPreference(value + delta);
  };

  if (has(GuestTrait::Workaholic)) {
    add(preferences.desk, 1500);
    add(preferences.wifi, 500);
  }
  if (has(GuestTrait::Foodie))
    add(preferences.breakfast, 1500);
  if (has(GuestTrait::FitnessFocused))
    add(preferences.fitness, 2000);
  if (has(GuestTrait::Social)) {
    add(preferences.social, 1500);
    add(preferences.quietRoom, -1000);
  }
  if (has(GuestTrait::Private)) {
    add(preferences.social, -1000);
    add(preferences.quietRoom, 1500);
  }
  if (has(GuestTrait::LightSleeper))
    add(preferences.quietRoom, 1500);
  if (has(GuestTrait::HeavySleeper))
    add(preferences.quietRoom, -1000);
  if (has(GuestTrait::StatusConscious)) {
    add(preferences.roomQuality, 1500);
    add(preferences.spa, 750);
  }
  if (has(GuestTrait::Frugal)) {
    add(preferences.spa, -1000);
    add(preferences.roomQuality, -750);
    add(preferences.breakfast, 500);
  }
  return preferences;
}

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

int clampScore(int value) noexcept { return std::clamp(value, 0, 100); }

void applyHourlyRate(int &score, std::int64_t &remainder, int pointsPerHour,
                     std::int64_t seconds) noexcept {
  remainder += static_cast<std::int64_t>(pointsPerHour) * seconds;
  const auto wholePoints = remainder / 3600;
  remainder %= 3600;
  score = clampScore(score + static_cast<int>(wholePoints));
  if ((score == 0 && remainder < 0) || (score == 100 && remainder > 0))
    remainder = 0;
}

int sensitivityBasisPoints(const GuestProfileView &profile,
                           ExperienceCategory category) noexcept {
  double sensitivity = profile.serviceSensitivity;
  switch (category) {
  case ExperienceCategory::Room:
    sensitivity = profile.comfortSensitivity;
    break;
  case ExperienceCategory::Service:
  case ExperienceCategory::Convenience:
  case ExperienceCategory::ArrivalDeparture:
    sensitivity = profile.serviceSensitivity;
    break;
  case ExperienceCategory::Cleanliness:
    sensitivity = profile.cleanlinessSensitivity;
    break;
  case ExperienceCategory::Food:
    sensitivity = profile.foodSensitivity;
    break;
  case ExperienceCategory::Amenities:
    sensitivity = profile.comfortSensitivity;
    break;
  case ExperienceCategory::Quiet:
    sensitivity = profile.noiseSensitivity;
    break;
  case ExperienceCategory::Value:
    sensitivity = profile.priceSensitivity;
    break;
  }
  return static_cast<int>(std::lround(std::clamp(sensitivity, 0.0, 1.0) * 10000));
}

ComplaintType complaintTypeFor(ExperienceEventType type) noexcept {
  switch (type) {
  case ExperienceEventType::LongCheckInQueue:
    return ComplaintType::CheckInDelay;
  case ExperienceEventType::RoomNotReady:
    return ComplaintType::RoomReadiness;
  case ExperienceEventType::DirtyBathroom:
    return ComplaintType::Cleanliness;
  case ExperienceEventType::BrokenAC:
    return ComplaintType::Maintenance;
  case ExperienceEventType::SlowRoomService:
  case ExperienceEventType::GreatMeal:
    return ComplaintType::FoodService;
  case ExperienceEventType::ElevatorDelay:
    return ComplaintType::ElevatorDelay;
  case ExperienceEventType::NoiseDisturbance:
    return ComplaintType::Noise;
  case ExperienceEventType::StaffRudeness:
  case ExperienceEventType::StaffExceptionalService:
    return ComplaintType::StaffConduct;
  default:
    return ComplaintType::ServiceFailure;
  }
}

ComplaintUrgency urgencyFor(int magnitude) noexcept {
  if (magnitude >= 80)
    return ComplaintUrgency::Critical;
  if (magnitude >= 60)
    return ComplaintUrgency::High;
  if (magnitude >= 40)
    return ComplaintUrgency::Medium;
  return ComplaintUrgency::Low;
}

std::int64_t personalizedCategoryWeight(int baseWeight,
                                        const GuestProfileView &profile,
                                        ExperienceCategory category) noexcept {
  // A neutral 0.5 sensitivity preserves the documented global category weight.
  // Sensitivity then provides the per-guest modulation required by HMG-010,
  // spanning 0.5x..1.5x before normalization across all categories.
  return static_cast<std::int64_t>(baseWeight) *
         (5000 + sensitivityBasisPoints(profile, category));
}

void recalculateOverall(SatisfactionBreakdown &satisfaction,
                        const GuestProfileView &profile) noexcept {
  const std::int64_t roomWeight =
      personalizedCategoryWeight(28, profile, ExperienceCategory::Room);
  const std::int64_t serviceWeight =
      personalizedCategoryWeight(24, profile, ExperienceCategory::Service);
  const std::int64_t cleanlinessWeight = personalizedCategoryWeight(
      16, profile, ExperienceCategory::Cleanliness);
  const std::int64_t foodWeight =
      personalizedCategoryWeight(10, profile, ExperienceCategory::Food);
  const std::int64_t amenitiesWeight =
      personalizedCategoryWeight(8, profile, ExperienceCategory::Amenities);
  const std::int64_t convenienceWeight = personalizedCategoryWeight(
      6, profile, ExperienceCategory::Convenience);
  const std::int64_t valueWeight =
      personalizedCategoryWeight(5, profile, ExperienceCategory::Value);
  const std::int64_t arrivalWeight = personalizedCategoryWeight(
      3, profile, ExperienceCategory::ArrivalDeparture);
  const std::int64_t totalWeight =
      roomWeight + serviceWeight + cleanlinessWeight + foodWeight +
      amenitiesWeight + convenienceWeight + valueWeight + arrivalWeight;
  const std::int64_t weighted =
      static_cast<std::int64_t>(satisfaction.room) * roomWeight +
      static_cast<std::int64_t>(satisfaction.service) * serviceWeight +
      static_cast<std::int64_t>(satisfaction.cleanliness) * cleanlinessWeight +
      static_cast<std::int64_t>(satisfaction.food) * foodWeight +
      static_cast<std::int64_t>(satisfaction.amenities) * amenitiesWeight +
      static_cast<std::int64_t>(satisfaction.convenience) * convenienceWeight +
      static_cast<std::int64_t>(satisfaction.value) * valueWeight +
      static_cast<std::int64_t>(satisfaction.arrivalDeparture) * arrivalWeight;
  satisfaction.overall = clampScore(static_cast<int>(
      (weighted + totalWeight / 2) / std::max<std::int64_t>(1, totalWeight)));
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

void GuestPsychology::initializeGuest(GuestId guestId,
                                      const GuestProfileView &profile) {
  if (guestId == 0 || !validProfile(profile))
    throw std::invalid_argument("guest psychology identity or profile is invalid");
  Record record;
  record.snapshot.guestId = guestId;
  record.snapshot.profile = profile;
  record.snapshot.preferences = preferencesFor(profile);
  guests_[guestId] = std::move(record);
}

std::optional<GuestPsychologySnapshot>
GuestPsychology::snapshot(GuestId guestId) const {
  const auto found = guests_.find(guestId);
  if (found == guests_.end())
    return std::nullopt;
  return found->second.snapshot;
}

void GuestPsychology::updateNeeds(GuestId guestId, std::int64_t seconds,
                                  bool sleeping) {
  if (seconds < 0)
    throw std::invalid_argument("guest need update duration must be non-negative");
  const auto found = guests_.find(guestId);
  if (found == guests_.end())
    throw std::invalid_argument("guest psychology state not found");
  auto &record = found->second;
  auto &needs = record.snapshot.needs;
  auto &remainders = record.needRemainders;
  const auto flags = record.snapshot.profile.traitFlags;
  const int socialRate =
      (flags & guestTraitFlag(GuestTrait::Social))      ? -3
      : (flags & guestTraitFlag(GuestTrait::Private)) ? -1
                                                       : 0;
  if (sleeping) {
    applyHourlyRate(needs.energy, remainders[0], +22, seconds);
    applyHourlyRate(needs.hygiene, remainders[2], -2, seconds);
    return;
  }
  applyHourlyRate(needs.energy, remainders[0], -5, seconds);
  applyHourlyRate(needs.hunger, remainders[1], -10, seconds);
  applyHourlyRate(needs.hygiene, remainders[2], -2, seconds);
  applyHourlyRate(needs.social, remainders[5], socialRate, seconds);
}

void GuestPsychology::recordExperience(GuestId guestId,
                                       const ExperienceEvent &event) {
  const auto found = guests_.find(guestId);
  if (found == guests_.end())
    throw std::invalid_argument("guest psychology state not found");
  auto &state = found->second.snapshot;
  const int impact = std::clamp(event.rawImpact, -100, 100);
  const int magnitude = std::abs(impact);
  const auto apply = [&](int &score) { score = clampScore(score + impact); };

  switch (event.category) {
  case ExperienceCategory::Room:
    apply(state.satisfaction.room);
    apply(state.operational.environmentComfort);
    break;
  case ExperienceCategory::Service:
    apply(state.satisfaction.service);
    apply(state.operational.serviceConfidence);
    break;
  case ExperienceCategory::Cleanliness:
    apply(state.satisfaction.cleanliness);
    apply(state.operational.cleanlinessConfidence);
    break;
  case ExperienceCategory::Food:
    apply(state.satisfaction.food);
    break;
  case ExperienceCategory::Amenities:
    apply(state.satisfaction.amenities);
    break;
  case ExperienceCategory::Quiet:
    apply(state.satisfaction.noise);
    apply(state.operational.environmentComfort);
    break;
  case ExperienceCategory::Convenience:
    apply(state.satisfaction.convenience);
    apply(state.operational.serviceConfidence);
    break;
  case ExperienceCategory::Value:
    apply(state.satisfaction.value);
    apply(state.operational.valuePerception);
    break;
  case ExperienceCategory::ArrivalDeparture:
    apply(state.satisfaction.arrivalDeparture);
    apply(state.operational.serviceConfidence);
    break;
  }
  if (event.type == ExperienceEventType::LongCheckInQueue ||
      event.type == ExperienceEventType::FastCheckIn) {
    apply(state.satisfaction.checkIn);
    apply(state.satisfaction.waits);
  }
  if (event.type == ExperienceEventType::ElevatorDelay ||
      event.type == ExperienceEventType::SlowRoomService)
    apply(state.satisfaction.waits);
  recalculateOverall(state.satisfaction, state.profile);

  if (magnitude > 0 && event.memorySalience > 0) {
    constexpr double kMemoryRetentionContributionFloor = 0.01;
    state.memories.erase(
        std::remove_if(state.memories.begin(), state.memories.end(),
                       [&](const GuestMemory &existing) {
                         return existing.halfLifeHours > 0 &&
                                std::abs(memoryContribution(
                                    existing, event.timestampSeconds)) <
                                    kMemoryRetentionContributionFloor;
                       }),
        state.memories.end());

    GuestMemory memory;
    memory.type = event.type;
    memory.timestampSeconds = event.timestampSeconds;
    memory.locationId = event.locationId;
    memory.sourceEntityId = event.sourceEntityId;
    memory.category = event.category;
    memory.valence = impact < 0 ? -1 : 1;
    memory.magnitude = magnitude;
    memory.salience = std::clamp(event.memorySalience, 0, 10000);
    memory.halfLifeHours = std::max(0, event.memoryHalfLifeHours);
    memory.resolved = event.resolved;
    state.memories.push_back(memory);
  }

  if (!event.complaintEligible || event.resolved || impact >= 0 ||
      magnitude < 25)
    return;
  const bool sensitivityEligible =
      sensitivityBasisPoints(state.profile, event.category) >= 5500;
  const bool traitEligible =
      (state.profile.traitFlags & guestTraitFlag(GuestTrait::ComplaintProne)) != 0;
  if (!sensitivityEligible && magnitude < 50 && !traitEligible)
    return;
  const bool duplicate = std::any_of(
      state.complaints.begin(), state.complaints.end(),
      [&](const Complaint &complaint) {
        return !complaint.resolved && complaint.sourceEvent == event.type &&
               complaint.timestampSeconds == event.timestampSeconds &&
               complaint.locationId == event.locationId &&
               complaint.sourceEntityId == event.sourceEntityId;
      });
  if (duplicate)
    return;
  Complaint complaint;
  complaint.type = complaintTypeFor(event.type);
  complaint.urgency = urgencyFor(magnitude);
  complaint.timestampSeconds = event.timestampSeconds;
  complaint.locationId = event.locationId;
  complaint.sourceEntityId = event.sourceEntityId;
  complaint.sourceEvent = event.type;
  complaint.magnitude = magnitude;
  state.complaints.push_back(complaint);
}

double GuestPsychology::memoryContribution(const GuestMemory &memory,
                                            std::int64_t nowSeconds) noexcept {
  const auto ageSeconds =
      std::max<std::int64_t>(0, nowSeconds - memory.timestampSeconds);
  const double salience = std::clamp(memory.salience, 0, 10000) / 10000.0;
  double decay = 1.0;
  if (memory.halfLifeHours > 0) {
    const double halfLives = static_cast<double>(ageSeconds) /
                             (static_cast<double>(memory.halfLifeHours) * 3600.0);
    decay = std::pow(.5, halfLives);
  }
  return static_cast<double>(std::clamp(memory.valence, -1, 1)) *
         std::clamp(memory.magnitude, 0, 100) * salience * decay;
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
