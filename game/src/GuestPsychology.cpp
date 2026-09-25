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
  std::uint32_t signatureTraitFlags{};
};

constexpr std::array<GuestArchetypeDefaults, 20> archetypes{{
    {GuestArchetype::BudgetLeisure, 9, 9000, .90, .45, .55, .40, .35, .55,
     .55, .45, .55},
    {GuestArchetype::Backpacker, 5, 7000, .95, .35, .40, .35, .25, .45, .35,
     .35, .70},
    {GuestArchetype::BusinessTraveler, 13, 18000, .45, .75, .80, .85, .55,
     .65, .70, .70, .45},
    {GuestArchetype::ExecutiveBusiness, 5, 26000, .25, .90, .90, .85, .80,
     .80, .90, .75, .40},
    {GuestArchetype::CoupleLeisure, 10, 17000, .55, .60, .70, .65, .60, .65,
     .75, .70, .65},
    {GuestArchetype::FamilyLeisure, 9, 19000, .65, .70, .85, .65, .35, .85,
     .75, .75, .55},
    {GuestArchetype::LuxuryLeisure, 4, 30000, .20, .95, .95, .85, .90, .90,
     .98, .90, .45},
    {GuestArchetype::ConferenceDelegate, 5, 16000, .50, .70, .75, .75, .50,
     .65, .65, .70, .50},
    {GuestArchetype::GroupTourTraveler, 4, 11000, .80, .45, .60, .50, .30,
     .65, .50, .55, .60},
    {GuestArchetype::AirportTransitTraveler, 3, 13000, .70, .55, .70, .65,
     .40, .70, .60, .45, .35},
    {GuestArchetype::WellnessTraveler, 2, 22000, .35, .80, .85, .75, .80,
     .75, .85, .75, .75},
    {GuestArchetype::VipCelebrity, 2, 30000, .10, .98, .98, .95, .98, .95,
     1.0, .90, .25},
    {GuestArchetype::CriticReviewer, 2, 22000, .35, 1.0, 1.0, .90, .85, .85,
     .95, .95, .35},
    {GuestArchetype::DigitalNomad, 5, 17000, .55, .65, .70, .70, .55, .65,
     .70, .55, .65, guestTraitFlag(GuestTrait::Workaholic)},
    {GuestArchetype::BleisureTraveler, 4, 23000, .40, .80, .82, .78, .65, .72,
     .82, .75, .55, guestTraitFlag(GuestTrait::Workaholic)},
    {GuestArchetype::ExtendedStayGuest, 5, 15000, .70, .60, .85, .80, .80, .75,
     .75, .50, .80, guestTraitFlag(GuestTrait::Private)},
    {GuestArchetype::AirlineCrew, 3, 14000, .35, .75, .90, 1.0, .80, .80,
     .85, .55, .45, guestTraitFlag(GuestTrait::LightSleeper)},
    {GuestArchetype::WeddingGuest, 3, 19000, .55, .72, .80, .45, .45, .65,
     .75, .85, .60, guestTraitFlag(GuestTrait::Social)},
    {GuestArchetype::StaycationGuest, 4, 20000, .60, .75, .80, .70, .70, .70,
     .85, .85, .65, guestTraitFlag(GuestTrait::StatusConscious)},
    {GuestArchetype::SportsTeamTraveler, 3, 15000, .65, .55, .70, .40, .35,
     .80, .70, .85, .75, guestTraitFlag(GuestTrait::FitnessFocused)},
}};
static_assert([] {
  int total = 0;
  for (const auto &archetype : archetypes)
    total += archetype.weight;
  return total == 100;
}());

constexpr std::array<GuestPreferenceState, 20> archetypePreferences{{
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
    {10000, 9000, 5000, 2500, 2500, 5500, 6500, 7500, 7500},
    {9500, 8500, 8000, 6000, 4500, 6500, 6500, 8500, 8500},
    {9000, 7500, 5500, 2500, 3500, 5000, 4500, 9000, 8000},
    {6500, 3000, 7500, 3000, 2500, 6500, 3500, 10000, 8500},
    {5000, 1000, 8500, 5500, 4000, 2000, 10000, 4000, 8000},
    {6000, 1500, 9000, 8500, 9000, 5000, 6500, 7500, 9000},
    {6000, 1000, 9500, 3500, 3500, 10000, 9000, 4000, 6000},
}};
static_assert(archetypePreferences.size() == archetypes.size());



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
namespace {
std::int64_t applyWeight(std::int64_t weight, int basisPoints) noexcept {
  const auto bounded = std::clamp(basisPoints, 1000, 30000);
  return std::max<std::int64_t>(
      1, (weight * static_cast<std::int64_t>(bounded) + 5000) / 10000);
}

std::int64_t contextualWeight(const GuestArchetypeDefaults &candidate,
                              const GuestArchetypeContext &context) noexcept {
  std::int64_t weight = static_cast<std::int64_t>(candidate.weight) * 10000;
  const bool hasWeekday = context.weekday >= 0 && context.weekday <= 6;
  const bool weekend = hasWeekday && context.weekday >= 5;

  if (hasWeekday) {
    int multiplier = 10000;
    switch (candidate.archetype) {
    case GuestArchetype::BusinessTraveler:
      multiplier = weekend ? 6500 : 12500;
      break;
    case GuestArchetype::ExecutiveBusiness:
      multiplier = weekend ? 7000 : 12000;
      break;
    case GuestArchetype::ConferenceDelegate:
      multiplier = weekend ? 6500 : 12500;
      break;
    case GuestArchetype::DigitalNomad:
      multiplier = weekend ? 10500 : 11500;
      break;
    case GuestArchetype::BleisureTraveler:
      multiplier = weekend ? 11500 : 12000;
      break;
    case GuestArchetype::ExtendedStayGuest:
      multiplier = weekend ? 10500 : 11250;
      break;
    case GuestArchetype::CoupleLeisure:
      multiplier = weekend ? 13500 : 9000;
      break;
    case GuestArchetype::FamilyLeisure:
      multiplier = weekend ? 14000 : 8500;
      break;
    case GuestArchetype::LuxuryLeisure:
      multiplier = weekend ? 12500 : 9500;
      break;
    case GuestArchetype::WellnessTraveler:
      multiplier = weekend ? 12000 : 9500;
      break;
    case GuestArchetype::WeddingGuest:
      multiplier = weekend ? 15000 : 7500;
      break;
    case GuestArchetype::StaycationGuest:
      multiplier = weekend ? 15500 : 7000;
      break;
    case GuestArchetype::SportsTeamTraveler:
      multiplier = weekend ? 12000 : 9500;
      break;
    default:
      break;
    }
    weight = applyWeight(weight, multiplier);
  }

  if (context.hotelStars > 0) {
    int multiplier = 10000;
    if (context.hotelStars >= 4) {
      switch (candidate.archetype) {
      case GuestArchetype::LuxuryLeisure: multiplier = 13500; break;
      case GuestArchetype::VipCelebrity: multiplier = 13000; break;
      case GuestArchetype::StaycationGuest: multiplier = 12500; break;
      case GuestArchetype::WellnessTraveler: multiplier = 12000; break;
      case GuestArchetype::ExecutiveBusiness: multiplier = 11500; break;
      case GuestArchetype::BudgetLeisure: multiplier = 8500; break;
      case GuestArchetype::Backpacker: multiplier = 8000; break;
      default: break;
      }
    } else if (context.hotelStars <= 2) {
      switch (candidate.archetype) {
      case GuestArchetype::BudgetLeisure: multiplier = 12500; break;
      case GuestArchetype::Backpacker: multiplier = 13000; break;
      case GuestArchetype::GroupTourTraveler: multiplier = 11500; break;
      case GuestArchetype::LuxuryLeisure: multiplier = 4500; break;
      case GuestArchetype::VipCelebrity: multiplier = 3500; break;
      case GuestArchetype::StaycationGuest: multiplier = 7000; break;
      default: break;
      }
    }
    weight = applyWeight(weight, multiplier);
  }

  if (context.hotelReputation >= 0) {
    int multiplier = 10000;
    if (context.hotelReputation >= 85) {
      switch (candidate.archetype) {
      case GuestArchetype::VipCelebrity: multiplier = 12500; break;
      case GuestArchetype::LuxuryLeisure: multiplier = 12000; break;
      case GuestArchetype::ExecutiveBusiness: multiplier = 11500; break;
      case GuestArchetype::CriticReviewer: multiplier = 11250; break;
      default: break;
      }
    } else if (context.hotelReputation < 60) {
      switch (candidate.archetype) {
      case GuestArchetype::VipCelebrity: multiplier = 4000; break;
      case GuestArchetype::LuxuryLeisure: multiplier = 5500; break;
      case GuestArchetype::ExecutiveBusiness: multiplier = 7000; break;
      case GuestArchetype::CriticReviewer: multiplier = 6500; break;
      case GuestArchetype::BudgetLeisure: multiplier = 11000; break;
      default: break;
      }
    }
    weight = applyWeight(weight, multiplier);
  }

  if (context.hasSpa) {
    int multiplier = 10000;
    switch (candidate.archetype) {
    case GuestArchetype::WellnessTraveler: multiplier = 17000; break;
    case GuestArchetype::StaycationGuest: multiplier = 14000; break;
    case GuestArchetype::LuxuryLeisure: multiplier = 12500; break;
    case GuestArchetype::VipCelebrity: multiplier = 11500; break;
    default: break;
    }
    weight = applyWeight(weight, multiplier);
  }
  if (context.hasGym) {
    int multiplier = 10000;
    switch (candidate.archetype) {
    case GuestArchetype::SportsTeamTraveler: multiplier = 16500; break;
    case GuestArchetype::WellnessTraveler: multiplier = 13000; break;
    case GuestArchetype::BusinessTraveler: multiplier = 10500; break;
    case GuestArchetype::BleisureTraveler: multiplier = 11000; break;
    default: break;
    }
    weight = applyWeight(weight, multiplier);
  }
  if (context.hasPool) {
    int multiplier = 10000;
    switch (candidate.archetype) {
    case GuestArchetype::FamilyLeisure: multiplier = 13000; break;
    case GuestArchetype::StaycationGuest: multiplier = 13000; break;
    case GuestArchetype::CoupleLeisure: multiplier = 11000; break;
    case GuestArchetype::WeddingGuest: multiplier = 11000; break;
    default: break;
    }
    weight = applyWeight(weight, multiplier);
  }

  if (context.eventAttendees > 0) {
    const int attendees = std::clamp(context.eventAttendees, 0, 500);
    int multiplier = 10000;
    switch (candidate.archetype) {
    case GuestArchetype::ConferenceDelegate:
      multiplier = 10000 + attendees * 40;
      break;
    case GuestArchetype::WeddingGuest:
      multiplier = 10000 + attendees * 30;
      break;
    case GuestArchetype::GroupTourTraveler:
      multiplier = 10000 + attendees * 10;
      break;
    case GuestArchetype::BleisureTraveler:
      multiplier = 10000 + attendees * 5;
      break;
    default:
      break;
    }
    weight = applyWeight(weight, multiplier);
  }

  // FINAL-06 exposes one market-wide seasonal multiplier. It remains the
  // authority for total demand; here it only tilts the composition of guests
  // who convert. Peak periods skew toward discretionary leisure/event travel,
  // while troughs retain relatively more recurring work and long-stay demand.
  if (context.seasonMultiplierBasisPoints != 10000) {
    const int delta = std::clamp(context.seasonMultiplierBasisPoints - 10000,
                                 -7500, 15000);
    int multiplier = 10000;
    if (delta > 0) {
      switch (candidate.archetype) {
      case GuestArchetype::CoupleLeisure:
      case GuestArchetype::FamilyLeisure:
      case GuestArchetype::StaycationGuest:
        multiplier += delta * 45 / 100;
        break;
      case GuestArchetype::LuxuryLeisure:
      case GuestArchetype::WellnessTraveler:
      case GuestArchetype::WeddingGuest:
        multiplier += delta * 35 / 100;
        break;
      case GuestArchetype::GroupTourTraveler:
      case GuestArchetype::SportsTeamTraveler:
        multiplier += delta * 25 / 100;
        break;
      case GuestArchetype::BusinessTraveler:
      case GuestArchetype::ExecutiveBusiness:
      case GuestArchetype::AirlineCrew:
        multiplier -= delta * 15 / 100;
        break;
      case GuestArchetype::ExtendedStayGuest:
        multiplier -= delta * 10 / 100;
        break;
      default:
        break;
      }
    } else {
      const int trough = -delta;
      switch (candidate.archetype) {
      case GuestArchetype::BusinessTraveler:
      case GuestArchetype::ExecutiveBusiness:
      case GuestArchetype::AirlineCrew:
        multiplier += trough * 25 / 100;
        break;
      case GuestArchetype::ExtendedStayGuest:
      case GuestArchetype::DigitalNomad:
        multiplier += trough * 20 / 100;
        break;
      case GuestArchetype::CoupleLeisure:
      case GuestArchetype::FamilyLeisure:
      case GuestArchetype::StaycationGuest:
        multiplier -= trough * 25 / 100;
        break;
      case GuestArchetype::LuxuryLeisure:
      case GuestArchetype::WellnessTraveler:
      case GuestArchetype::WeddingGuest:
        multiplier -= trough * 20 / 100;
        break;
      default:
        break;
      }
    }
    weight = applyWeight(weight, multiplier);
  }

  // locationScore is the existing FINAL-06 0..100 hotel-location quality
  // signal. It deliberately does not invent an airport/resort geography type:
  // strong locations raise location-sensitive premium/work segments, while
  // weak locations leave a larger share of value and long-stay guests.
  if (context.locationScore >= 0) {
    int multiplier = 10000;
    if (context.locationScore >= 75) {
      switch (candidate.archetype) {
      case GuestArchetype::ExecutiveBusiness:
      case GuestArchetype::LuxuryLeisure:
        multiplier = 12000;
        break;
      case GuestArchetype::BusinessTraveler:
      case GuestArchetype::ConferenceDelegate:
      case GuestArchetype::BleisureTraveler:
      case GuestArchetype::WellnessTraveler:
        multiplier = 11500;
        break;
      case GuestArchetype::VipCelebrity:
      case GuestArchetype::CoupleLeisure:
      case GuestArchetype::StaycationGuest:
        multiplier = 11000;
        break;
      case GuestArchetype::BudgetLeisure:
      case GuestArchetype::Backpacker:
        multiplier = 9000;
        break;
      default:
        break;
      }
    } else if (context.locationScore <= 40) {
      switch (candidate.archetype) {
      case GuestArchetype::BudgetLeisure:
        multiplier = 12000;
        break;
      case GuestArchetype::Backpacker:
        multiplier = 12500;
        break;
      case GuestArchetype::ExtendedStayGuest:
        multiplier = 11500;
        break;
      case GuestArchetype::GroupTourTraveler:
        multiplier = 11000;
        break;
      case GuestArchetype::ExecutiveBusiness:
        multiplier = 7500;
        break;
      case GuestArchetype::LuxuryLeisure:
        multiplier = 6500;
        break;
      case GuestArchetype::VipCelebrity:
        multiplier = 5500;
        break;
      case GuestArchetype::StaycationGuest:
        multiplier = 7500;
        break;
      default:
        break;
      }
    }
    weight = applyWeight(weight, multiplier);
  }

  if (context.roomRateCents > 0 && candidate.budgetPerNightCents > 0) {
    const auto ratioBasisPoints = static_cast<int>(std::clamp<std::int64_t>(
        context.roomRateCents * 10000 / candidate.budgetPerNightCents,
        2500, 30000));
    const int sensitivityBasisPoints =
        static_cast<int>(candidate.priceSensitivity * 10000.0 + .5);
    int multiplier = 10000;
    if (ratioBasisPoints > 10000) {
      const int excess = ratioBasisPoints - 10000;
      const int penalty = static_cast<int>(
          static_cast<std::int64_t>(excess) * sensitivityBasisPoints / 10000);
      multiplier = std::clamp(10000 - penalty, 1500, 10000);
    } else {
      const int discount = 10000 - ratioBasisPoints;
      const int bonus = std::min(
          1500, static_cast<int>(
                    static_cast<std::int64_t>(discount) *
                    sensitivityBasisPoints / 50000));
      multiplier = 10000 + bonus;
    }
    weight = applyWeight(weight, multiplier);
  }

  return weight;
}

GuestProfileView buildProfile(std::mt19937_64 &random,
                              const GuestArchetypeDefaults &defaults) {
  const auto varied = [&](double mean) {
    return std::clamp(mean + (randomUnit(random) * 2.0 - 1.0) * .08, 0.0,
                      1.0);
  };
  GuestProfileView profile;
  profile.archetype = defaults.archetype;
  profile.budgetPerNightCents = static_cast<std::int64_t>(std::llround(
      defaults.budgetPerNightCents * (.90 + randomUnit(random) * .20)));
  profile.priceSensitivity = varied(defaults.priceSensitivity);
  profile.serviceSensitivity = varied(defaults.serviceSensitivity);
  profile.cleanlinessSensitivity = varied(defaults.cleanlinessSensitivity);
  profile.noiseSensitivity = varied(defaults.noiseSensitivity);
  profile.privacySensitivity = varied(defaults.privacySensitivity);
  profile.safetySensitivity = varied(defaults.safetySensitivity);
  profile.comfortSensitivity = varied(defaults.comfortSensitivity);
  profile.foodSensitivity = varied(defaults.foodSensitivity);
  profile.patience = varied(defaults.patience);

  profile.traitFlags = defaults.signatureTraitFlags;
  const int traitCount =
      std::max(static_cast<int>(std::popcount(profile.traitFlags)),
               static_cast<int>(random() % 4));
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
} // namespace

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
  return buildProfile(random, *defaults);
}

GuestProfileView
generateGuestProfileFromRandom(std::mt19937_64 &random,
                               const GuestArchetypeContext &context) {
  std::array<std::int64_t, archetypes.size()> weights{};
  std::int64_t totalWeight{};
  for (std::size_t index = 0; index < archetypes.size(); ++index) {
    weights[index] = contextualWeight(archetypes[index], context);
    totalWeight += weights[index];
  }

  std::uint64_t roll =
      static_cast<std::uint64_t>(random()) %
      static_cast<std::uint64_t>(std::max<std::int64_t>(1, totalWeight));
  const GuestArchetypeDefaults *defaults = &archetypes.back();
  for (std::size_t index = 0; index < archetypes.size(); ++index) {
    const auto weight = static_cast<std::uint64_t>(weights[index]);
    if (roll < weight) {
      defaults = &archetypes[index];
      break;
    }
    roll -= weight;
  }
  return buildProfile(random, *defaults);
}

bool validGuestProfile(const GuestProfileView &profile) noexcept {
  const auto archetype = static_cast<int>(profile.archetype);
  const auto normalized = [](double value) {
    return std::isfinite(value) && value >= 0 && value <= 1;
  };
  constexpr auto validTraitFlags =
      (guestTraitFlag(GuestTrait::Forgiving) << 1) - 1;
  if (archetype < 0 || archetype >= 20 || profile.budgetPerNightCents < 4000 ||
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
  case GuestArchetype::DigitalNomad:
    segmentModifier = 1.05;
    break;
  case GuestArchetype::BleisureTraveler:
    segmentModifier = .90;
    break;
  case GuestArchetype::ExtendedStayGuest:
    segmentModifier = 1.15;
    break;
  case GuestArchetype::AirlineCrew:
    segmentModifier = .65;
    break;
  case GuestArchetype::WeddingGuest:
    segmentModifier = 1.0;
    break;
  case GuestArchetype::StaycationGuest:
    segmentModifier = 1.05;
    break;
  case GuestArchetype::SportsTeamTraveler:
    segmentModifier = 1.20;
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

int stayNightsFor(const GuestProfileView &profile,
                  std::uint64_t randomValue) noexcept {
  switch (profile.archetype) {
  case GuestArchetype::AirportTransitTraveler:
  case GuestArchetype::AirlineCrew:
    return 1;
  case GuestArchetype::ExtendedStayGuest:
    return 7 + static_cast<int>(randomValue % 15);
  case GuestArchetype::DigitalNomad:
    return 3 + static_cast<int>(randomValue % 8);
  case GuestArchetype::BleisureTraveler:
    return 2 + static_cast<int>(randomValue % 4);
  case GuestArchetype::WeddingGuest:
    return 2 + static_cast<int>(randomValue % 2);
  case GuestArchetype::StaycationGuest:
    return 1 + static_cast<int>(randomValue % 2);
  case GuestArchetype::SportsTeamTraveler:
  case GuestArchetype::ConferenceDelegate:
  case GuestArchetype::GroupTourTraveler:
    return 2 + static_cast<int>(randomValue % 3);
  case GuestArchetype::FamilyLeisure:
    return 3 + static_cast<int>(randomValue % 3);
  case GuestArchetype::CoupleLeisure:
    return 2 + static_cast<int>(randomValue % 3);
  case GuestArchetype::LuxuryLeisure:
  case GuestArchetype::VipCelebrity:
  case GuestArchetype::WellnessTraveler:
    return 2 + static_cast<int>(randomValue % 4);
  case GuestArchetype::Backpacker:
    return 2 + static_cast<int>(randomValue % 5);
  default:
    return 1 + static_cast<int>(randomValue % 3);
  }
}
} // namespace detail
} // namespace hh::game
