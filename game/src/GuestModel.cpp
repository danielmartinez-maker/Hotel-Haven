#include "hh/game/GuestModel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace hh::game {
namespace {

constexpr double kUnitScale = 1.0 / 9007199254740992.0;
constexpr std::uint64_t kIncrement = 0x9e3779b97f4a7c15ULL;

std::uint64_t mix(std::uint64_t value) noexcept {
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

double unitDraw(std::uint64_t &state) noexcept {
  return static_cast<double>(drawGuestRandom(state) >> 11) * kUnitScale;
}

double drawBetween(std::uint64_t &state, double low, double high) noexcept {
  return low + (high - low) * unitDraw(state);
}

std::uint64_t drawBounded(std::uint64_t &state,
                          std::uint64_t bound) noexcept {
  if (bound <= 1)
    return 0;
  const auto threshold = static_cast<std::uint64_t>(-bound) % bound;
  for (;;) {
    const auto value = drawGuestRandom(state);
    if (value >= threshold)
      return value % bound;
  }
}

bool isUnit(double value) noexcept {
  return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool validPurpose(GuestTravelPurpose value) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(GuestTravelPurpose::Count);
}

bool validWealth(GuestWealthBand value) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(GuestWealthBand::Count);
}

GuestSensitivityProfile sensitivities(double price, double service,
                                      double cleanliness, double noise,
                                      double privacy, double safety,
                                      double comfort, double food) {
  return {price, service, cleanliness, noise, privacy, safety, comfort, food};
}

bool finiteSensitivity(const GuestSensitivityProfile &s) noexcept {
  return isUnit(s.price) && isUnit(s.service) && isUnit(s.cleanliness) &&
         isUnit(s.noise) && isUnit(s.privacy) && isUnit(s.safety) &&
         isUnit(s.comfort) && isUnit(s.food);
}

bool traitHasEffect(const GuestTraitEffects &e) noexcept {
  const auto nonzero = [](double value) { return value != 0.0; };
  if (nonzero(e.sensitivityDelta.price) ||
      nonzero(e.sensitivityDelta.service) ||
      nonzero(e.sensitivityDelta.cleanliness) ||
      nonzero(e.sensitivityDelta.noise) ||
      nonzero(e.sensitivityDelta.privacy) ||
      nonzero(e.sensitivityDelta.safety) ||
      nonzero(e.sensitivityDelta.comfort) ||
      nonzero(e.sensitivityDelta.food) || nonzero(e.patienceDelta) ||
      nonzero(e.socialPreferenceDelta) || e.queueToleranceMultiplier != 1.0 ||
      e.negativeMemoryReviewWeightMultiplier != 1.0 ||
      e.lightSleepModifier != 1.0 || e.hygieneDecayMultiplier != 1.0 ||
      nonzero(e.complaintThresholdDelta) ||
      nonzero(e.morningPreferenceDelta) ||
      nonzero(e.eveningPreferenceDelta))
    return true;
  for (const auto value : e.activityPreferenceDelta)
    if (nonzero(value))
      return true;
  for (const auto value : e.roomPreferenceDelta)
    if (nonzero(value))
      return true;
  for (const auto value : e.expectationDelta)
    if (nonzero(value))
      return true;
  return false;
}

template <std::size_t N>
void addAndClamp(std::array<double, N> &values,
                 const std::array<double, N> &delta, double low, double high) {
  for (std::size_t i = 0; i < N; ++i)
    values[i] = std::clamp(values[i] + delta[i], low, high);
}

void addAndClamp(GuestSensitivityProfile &value,
                 const GuestSensitivityProfile &delta) {
  value.price = std::clamp(value.price + delta.price, 0.0, 1.0);
  value.service = std::clamp(value.service + delta.service, 0.0, 1.0);
  value.cleanliness =
      std::clamp(value.cleanliness + delta.cleanliness, 0.0, 1.0);
  value.noise = std::clamp(value.noise + delta.noise, 0.0, 1.0);
  value.privacy = std::clamp(value.privacy + delta.privacy, 0.0, 1.0);
  value.safety = std::clamp(value.safety + delta.safety, 0.0, 1.0);
  value.comfort = std::clamp(value.comfort + delta.comfort, 0.0, 1.0);
  value.food = std::clamp(value.food + delta.food, 0.0, 1.0);
}

void setActivity(GuestArchetypeDefinition &definition,
                 GuestActivityPreference preference, double value) {
  definition.activityPreferenceMean[static_cast<std::size_t>(preference)] =
      value;
}

void setRoom(GuestArchetypeDefinition &definition,
             GuestRoomPreference preference, double value) {
  definition.roomPreferenceMean[static_cast<std::size_t>(preference)] = value;
}

void setExpectation(GuestArchetypeDefinition &definition,
                    GuestCategory category, double value) {
  definition.expectationMean[static_cast<std::size_t>(category)] = value;
}

bool validNeedTuning(const GuestNeedTuning &tuning) noexcept {
  const std::array rates{
      tuning.awakeEnergyDecayPerHour,
      tuning.awakeHungerDecayPerHour,
      tuning.hygieneDecayPerHour,
      tuning.idleEntertainmentDecayPerHour,
      tuning.socialGuestDecayPerHour,
      tuning.privateGuestDecayPerHour,
      tuning.sleepingEnergyRecoveryPerHour,
      tuning.exerciseHygienePenaltyPerHour};
  for (const auto rate : rates)
    if (!std::isfinite(rate) || rate < 0.0 || rate > 1000.0)
      return false;
  return std::isfinite(tuning.sleepNoiseThresholdDb) &&
         tuning.sleepNoiseThresholdDb >= 0.0 &&
         tuning.sleepNoiseThresholdDb <= 100.0 &&
         std::isfinite(tuning.noiseSensitivityThresholdOffsetDb) &&
         tuning.noiseSensitivityThresholdOffsetDb >= 0.0 &&
         tuning.noiseSensitivityThresholdOffsetDb <= 100.0 &&
         std::isfinite(tuning.sleepNoiseSampleIntervalSeconds) &&
         tuning.sleepNoiseSampleIntervalSeconds > 0.0 &&
         tuning.sleepNoiseSampleIntervalSeconds <= 3600.0 &&
         std::isfinite(tuning.sleepNoiseDisruptionSeconds) &&
         tuning.sleepNoiseDisruptionSeconds > 0.0 &&
         tuning.sleepNoiseDisruptionSeconds <= 3600.0 &&
         std::isfinite(tuning.noiseComplaintWindowSeconds) &&
         tuning.noiseComplaintWindowSeconds > 0.0 &&
         tuning.noiseComplaintWindowSeconds <= 86400.0 &&
         tuning.noiseDisruptionsForComplaint >= 1 &&
         tuning.noiseDisruptionsForComplaint <= 3;
}

double boundedNeed(double value) noexcept {
  if (!std::isfinite(value))
    return 0.0;
  return std::clamp(value, 0.0, 100.0);
}

bool validEnum(GuestNeed value) noexcept {
  return static_cast<std::size_t>(value) < GuestNeedCount;
}

bool validEnum(GuestGoal value) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(GuestGoal::Count);
}

bool validEnum(GuestActivity value) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(GuestActivity::Count);
}

bool validLifecycleState(GuestLifecycleState value) noexcept {
  return static_cast<std::size_t>(value) <
         static_cast<std::size_t>(GuestLifecycleState::Count);
}

bool lifecycleGoal(GuestGoal goal) noexcept {
  return goal == GuestGoal::ReachHotel || goal == GuestGoal::CheckIn ||
         goal == GuestGoal::ReachRoom || goal == GuestGoal::Checkout ||
         goal == GuestGoal::LeaveHotel;
}

void setPerception(double &value, const std::optional<double> &evidence) noexcept {
  value = boundedNeed(value);
  if (evidence && std::isfinite(*evidence))
    value = std::clamp(*evidence, 0.0, 100.0);
}

} // namespace

std::uint64_t deriveGuestRandomState(std::uint64_t simulationSeed,
                                     GuestId guestId) noexcept {
  auto state = mix(simulationSeed ^ mix(guestId ^ 0xd1b54a32d192ed03ULL) ^
                   0xa0761d6478bd642fULL);
  return state == 0 ? kIncrement : state;
}

std::uint64_t drawGuestRandom(std::uint64_t &state) noexcept {
  state += kIncrement;
  return mix(state);
}

GuestModelDefinitions defaultGuestModelDefinitions() {
  GuestModelDefinitions definitions;
  const auto define = [&](GuestArchetype archetype, GuestTravelPurpose purpose,
                          GuestWealthBand wealth, std::int64_t budget,
                          GuestSensitivityProfile sensitivity,
                          double patience, double social, double childChance,
                          double teenChance, double seniorChance) {
    auto &entry =
        definitions.archetypes[static_cast<std::size_t>(archetype)];
    entry.archetype = archetype;
    entry.weight = 1.0;
    entry.travelPurpose = purpose;
    entry.wealthBand = wealth;
    entry.budgetPerNightCents = budget;
    entry.budgetSpread = 0.20;
    entry.childProbability = childChance;
    entry.teenProbability = teenChance;
    entry.seniorProbability = seniorChance;
    entry.meanSensitivities = sensitivity;
    entry.meanPatience = patience;
    entry.meanSocialPreference = social;
    entry.activityPreferenceMean.fill(0.30);
    entry.roomPreferenceMean.fill(0.35);
    entry.expectationMean.fill(70.0);
  };

  define(GuestArchetype::BudgetLeisure, GuestTravelPurpose::Leisure,
         GuestWealthBand::Budget, 7000,
         sensitivities(0.82, 0.48, 0.62, 0.42, 0.28, 0.46, 0.45, 0.40),
         0.55, 0.42, 0.0, 0.04, 0.08);
  define(GuestArchetype::Backpacker, GuestTravelPurpose::Leisure,
         GuestWealthBand::Budget, 4000,
         sensitivities(0.90, 0.36, 0.44, 0.35, 0.22, 0.38, 0.35, 0.34),
         0.62, 0.72, 0.0, 0.08, 0.02);
  define(GuestArchetype::BusinessTraveler, GuestTravelPurpose::Business,
         GuestWealthBand::Standard, 15000,
         sensitivities(0.45, 0.75, 0.80, 0.85, 0.48, 0.60, 0.66, 0.55),
         0.45, 0.38, 0.0, 0.01, 0.12);
  define(GuestArchetype::ExecutiveBusiness, GuestTravelPurpose::Business,
         GuestWealthBand::Affluent, 30000,
         sensitivities(0.26, 0.91, 0.90, 0.90, 0.74, 0.79, 0.86, 0.72),
         0.50, 0.35, 0.0, 0.0, 0.16);
  define(GuestArchetype::CoupleLeisure, GuestTravelPurpose::Leisure,
         GuestWealthBand::Standard, 12000,
         sensitivities(0.48, 0.60, 0.66, 0.72, 0.46, 0.55, 0.74, 0.60),
         0.58, 0.58, 0.0, 0.0, 0.08);
  define(GuestArchetype::FamilyLeisure, GuestTravelPurpose::Leisure,
         GuestWealthBand::Standard, 16000,
         sensitivities(0.60, 0.62, 0.74, 0.52, 0.38, 0.72, 0.66, 0.68),
         0.68, 0.62, 0.28, 0.08, 0.04);
  define(GuestArchetype::LuxuryLeisure, GuestTravelPurpose::Leisure,
         GuestWealthBand::Luxury, 45000,
         sensitivities(0.18, 0.90, 0.88, 0.84, 0.62, 0.68, 0.92, 0.72),
         0.48, 0.44, 0.0, 0.0, 0.10);
  define(GuestArchetype::ConferenceDelegate, GuestTravelPurpose::Conference,
         GuestWealthBand::Standard, 13000,
         sensitivities(0.52, 0.72, 0.72, 0.72, 0.44, 0.56, 0.58, 0.48),
         0.42, 0.56, 0.0, 0.0, 0.08);
  define(GuestArchetype::GroupTourTraveler, GuestTravelPurpose::Group,
         GuestWealthBand::Budget, 8000,
         sensitivities(0.70, 0.52, 0.56, 0.42, 0.28, 0.54, 0.50, 0.48),
         0.72, 0.78, 0.08, 0.06, 0.06);
  define(GuestArchetype::AirportTransitTraveler, GuestTravelPurpose::Transit,
         GuestWealthBand::Standard, 7000,
         sensitivities(0.66, 0.68, 0.64, 0.76, 0.42, 0.56, 0.54, 0.44),
         0.36, 0.30, 0.0, 0.0, 0.05);
  define(GuestArchetype::WellnessTraveler, GuestTravelPurpose::Wellness,
         GuestWealthBand::Affluent, 20000,
         sensitivities(0.34, 0.70, 0.82, 0.90, 0.60, 0.65, 0.92, 0.66),
         0.62, 0.46, 0.0, 0.0, 0.18);
  define(GuestArchetype::VipCelebrity, GuestTravelPurpose::Leisure,
         GuestWealthBand::Luxury, 80000,
         sensitivities(0.12, 0.96, 0.91, 0.86, 0.98, 0.96, 0.90, 0.68),
         0.42, 0.28, 0.0, 0.0, 0.08);
  define(GuestArchetype::CriticReviewer, GuestTravelPurpose::Review,
         GuestWealthBand::Affluent, 12000,
         sensitivities(0.36, 0.94, 0.96, 0.91, 0.58, 0.74, 0.78, 0.84),
         0.40, 0.42, 0.0, 0.0, 0.10);

  auto &budget = definitions.archetypes[0];
  setActivity(budget, GuestActivityPreference::Breakfast, 0.62);
  setRoom(budget, GuestRoomPreference::Single, 0.66);
  setExpectation(budget, GuestCategory::Value, 78.0);
  setExpectation(budget, GuestCategory::Room, 62.0);

  auto &backpacker = definitions.archetypes[1];
  setActivity(backpacker, GuestActivityPreference::Social, 0.82);
  setRoom(backpacker, GuestRoomPreference::Single, 0.78);
  setExpectation(backpacker, GuestCategory::Value, 80.0);
  setExpectation(backpacker, GuestCategory::Room, 60.0);

  auto &business = definitions.archetypes[2];
  setActivity(business, GuestActivityPreference::Wifi, 0.95);
  setActivity(business, GuestActivityPreference::Desk, 0.80);
  setActivity(business, GuestActivityPreference::Breakfast, 0.70);
  setActivity(business, GuestActivityPreference::Spa, 0.10);
  setActivity(business, GuestActivityPreference::Pool, 0.15);
  setExpectation(business, GuestCategory::Service, 77.0);
  setExpectation(business, GuestCategory::Quiet, 82.0);

  auto &executive = definitions.archetypes[3];
  setActivity(executive, GuestActivityPreference::Wifi, 0.98);
  setActivity(executive, GuestActivityPreference::Desk, 0.92);
  setActivity(executive, GuestActivityPreference::Breakfast, 0.78);
  setRoom(executive, GuestRoomPreference::Suite, 0.72);
  setExpectation(executive, GuestCategory::Room, 84.0);
  setExpectation(executive, GuestCategory::Service, 88.0);
  setExpectation(executive, GuestCategory::Value, 74.0);

  auto &couple = definitions.archetypes[4];
  setActivity(couple, GuestActivityPreference::Spa, 0.70);
  setActivity(couple, GuestActivityPreference::Social, 0.64);
  setActivity(couple, GuestActivityPreference::Event, 0.58);
  setRoom(couple, GuestRoomPreference::Double, 0.84);
  setRoom(couple, GuestRoomPreference::Quiet, 0.62);
  setExpectation(couple, GuestCategory::Amenities, 78.0);

  auto &family = definitions.archetypes[5];
  setActivity(family, GuestActivityPreference::Pool, 0.84);
  setActivity(family, GuestActivityPreference::Breakfast, 0.78);
  setActivity(family, GuestActivityPreference::Event, 0.68);
  setRoom(family, GuestRoomPreference::Double, 0.90);
  setRoom(family, GuestRoomPreference::Accessible, 0.40);
  setExpectation(family, GuestCategory::Convenience, 82.0);
  setExpectation(family, GuestCategory::Amenities, 76.0);

  auto &luxury = definitions.archetypes[6];
  setActivity(luxury, GuestActivityPreference::Spa, 0.86);
  setActivity(luxury, GuestActivityPreference::Pool, 0.62);
  setActivity(luxury, GuestActivityPreference::Breakfast, 0.72);
  setRoom(luxury, GuestRoomPreference::Suite, 0.90);
  setRoom(luxury, GuestRoomPreference::HighFloor, 0.64);
  setExpectation(luxury, GuestCategory::Room, 90.0);
  setExpectation(luxury, GuestCategory::Service, 88.0);
  setExpectation(luxury, GuestCategory::Value, 68.0);

  auto &conference = definitions.archetypes[7];
  setActivity(conference, GuestActivityPreference::Wifi, 0.90);
  setActivity(conference, GuestActivityPreference::Work, 0.76);
  setActivity(conference, GuestActivityPreference::Event, 0.78);
  setRoom(conference, GuestRoomPreference::Single, 0.64);
  setExpectation(conference, GuestCategory::Convenience, 82.0);

  auto &group = definitions.archetypes[8];
  setActivity(group, GuestActivityPreference::Social, 0.88);
  setActivity(group, GuestActivityPreference::Event, 0.78);
  setRoom(group, GuestRoomPreference::Double, 0.68);
  setExpectation(group, GuestCategory::Convenience, 76.0);

  auto &transit = definitions.archetypes[9];
  setActivity(transit, GuestActivityPreference::Wifi, 0.72);
  setRoom(transit, GuestRoomPreference::Quiet, 0.78);
  setExpectation(transit, GuestCategory::Convenience, 84.0);
  setExpectation(transit, GuestCategory::ArrivalDeparture, 84.0);

  auto &wellness = definitions.archetypes[10];
  setActivity(wellness, GuestActivityPreference::Spa, 0.94);
  setActivity(wellness, GuestActivityPreference::Fitness, 0.88);
  setActivity(wellness, GuestActivityPreference::Pool, 0.72);
  setRoom(wellness, GuestRoomPreference::Quiet, 0.88);
  setExpectation(wellness, GuestCategory::Cleanliness, 86.0);
  setExpectation(wellness, GuestCategory::Quiet, 88.0);

  auto &vip = definitions.archetypes[11];
  setActivity(vip, GuestActivityPreference::Spa, 0.80);
  setRoom(vip, GuestRoomPreference::Suite, 0.96);
  setRoom(vip, GuestRoomPreference::Quiet, 0.92);
  setRoom(vip, GuestRoomPreference::HighFloor, 0.72);
  setExpectation(vip, GuestCategory::Service, 94.0);
  setExpectation(vip, GuestCategory::Quiet, 88.0);
  setExpectation(vip, GuestCategory::ArrivalDeparture, 92.0);

  auto &critic = definitions.archetypes[12];
  setActivity(critic, GuestActivityPreference::Breakfast, 0.78);
  setActivity(critic, GuestActivityPreference::Wifi, 0.72);
  setExpectation(critic, GuestCategory::Room, 82.0);
  setExpectation(critic, GuestCategory::Cleanliness, 90.0);
  setExpectation(critic, GuestCategory::Service, 88.0);

  const auto setTrait = [&](GuestTrait trait, GuestTraitEffects effects) {
    definitions.traits[static_cast<std::size_t>(trait)] = {trait, effects};
  };
  {
    GuestTraitEffects e;
    e.patienceDelta = 0.18;
    e.queueToleranceMultiplier = 1.40;
    setTrait(GuestTrait::Patient, e);
  }
  {
    GuestTraitEffects e;
    e.patienceDelta = -0.18;
    e.queueToleranceMultiplier = 0.65;
    setTrait(GuestTrait::Impatient, e);
  }
  {
    GuestTraitEffects e;
    e.sensitivityDelta.cleanliness = 0.10;
    e.hygieneDecayMultiplier = 0.85;
    setTrait(GuestTrait::Neat, e);
  }
  {
    GuestTraitEffects e;
    e.sensitivityDelta.cleanliness = -0.10;
    e.hygieneDecayMultiplier = 1.20;
    setTrait(GuestTrait::Messy, e);
  }
  {
    GuestTraitEffects e;
    e.lightSleepModifier = 1.50;
    e.sensitivityDelta.noise = 0.10;
    setTrait(GuestTrait::LightSleeper, e);
  }
  {
    GuestTraitEffects e;
    e.lightSleepModifier = 0.50;
    e.sensitivityDelta.noise = -0.10;
    setTrait(GuestTrait::HeavySleeper, e);
  }
  {
    GuestTraitEffects e;
    e.sensitivityDelta.food = 0.15;
    e.activityPreferenceDelta[static_cast<std::size_t>(
        GuestActivityPreference::Breakfast)] = 0.15;
    setTrait(GuestTrait::Foodie, e);
  }
  {
    GuestTraitEffects e;
    e.activityPreferenceDelta[static_cast<std::size_t>(
        GuestActivityPreference::Work)] = 0.20;
    setTrait(GuestTrait::Workaholic, e);
  }
  {
    GuestTraitEffects e;
    e.socialPreferenceDelta = 0.18;
    e.activityPreferenceDelta[static_cast<std::size_t>(
        GuestActivityPreference::Social)] = 0.20;
    setTrait(GuestTrait::Social, e);
  }
  {
    GuestTraitEffects e;
    e.socialPreferenceDelta = -0.16;
    e.sensitivityDelta.privacy = 0.15;
    setTrait(GuestTrait::Private, e);
  }
  {
    GuestTraitEffects e;
    e.sensitivityDelta.price = 0.18;
    e.expectationDelta[static_cast<std::size_t>(GuestCategory::Value)] = 6.0;
    setTrait(GuestTrait::Frugal, e);
  }
  {
    GuestTraitEffects e;
    e.expectationDelta[static_cast<std::size_t>(GuestCategory::Room)] = 7.0;
    e.expectationDelta[static_cast<std::size_t>(GuestCategory::Service)] = 6.0;
    e.roomPreferenceDelta[static_cast<std::size_t>(
        GuestRoomPreference::Suite)] = 0.20;
    setTrait(GuestTrait::StatusConscious, e);
  }
  {
    GuestTraitEffects e;
    e.activityPreferenceDelta[static_cast<std::size_t>(
        GuestActivityPreference::Fitness)] = 0.25;
    setTrait(GuestTrait::FitnessFocused, e);
  }
  {
    GuestTraitEffects e;
    e.morningPreferenceDelta = 0.25;
    setTrait(GuestTrait::EarlyRiser, e);
  }
  {
    GuestTraitEffects e;
    e.eveningPreferenceDelta = 0.25;
    setTrait(GuestTrait::NightOwl, e);
  }
  {
    GuestTraitEffects e;
    e.complaintThresholdDelta = -10.0;
    setTrait(GuestTrait::ComplaintProne, e);
  }
  {
    GuestTraitEffects e;
    e.negativeMemoryReviewWeightMultiplier = 0.80;
    setTrait(GuestTrait::Forgiving, e);
  }

  definitions.categoryWeights = {0.28, 0.16, 0.24, 0.10, 0.08,
                                 0.08, 0.06, 0.05, 0.03};
  const double categoryWeightTotal = std::accumulate(
      definitions.categoryWeights.begin(), definitions.categoryWeights.end(),
      0.0);
  for (auto &weight : definitions.categoryWeights)
    weight /= categoryWeightTotal;
  return definitions;
}

bool validateGuestModelDefinitions(const GuestModelDefinitions &definitions,
                                   std::string *error) {
  if (error)
    error->clear();
  const auto fail = [&](const char *message) {
    if (error)
      *error = message;
    return false;
  };

  std::array<bool, GuestArchetypeCount> archetypesSeen{};
  double totalArchetypeWeight = 0.0;
  for (const auto &entry : definitions.archetypes) {
    const auto index = static_cast<std::size_t>(entry.archetype);
    if (index >= GuestArchetypeCount || archetypesSeen[index])
      return fail("guest archetype definitions are missing or duplicated");
    archetypesSeen[index] = true;
    if (!std::isfinite(entry.weight) || entry.weight < 0.0)
      return fail("guest archetype weight must be finite and nonnegative");
    totalArchetypeWeight += entry.weight;
    if (!validPurpose(entry.travelPurpose) || !validWealth(entry.wealthBand))
      return fail("guest archetype purpose or wealth band is unknown");
    if (entry.budgetPerNightCents <= 0 ||
        entry.budgetPerNightCents >
            std::numeric_limits<std::int64_t>::max() / 2 ||
        !std::isfinite(entry.budgetSpread) || entry.budgetSpread < 0.0 ||
        entry.budgetSpread > 0.75)
      return fail("guest archetype budget or spread is out of range");
    if (!isUnit(entry.childProbability) || !isUnit(entry.teenProbability) ||
        !isUnit(entry.seniorProbability) ||
        entry.childProbability + entry.teenProbability +
                entry.seniorProbability >
            1.0)
      return fail("guest archetype age probabilities are invalid");
    if (!finiteSensitivity(entry.meanSensitivities) ||
        !isUnit(entry.meanPatience) || !isUnit(entry.meanSocialPreference))
      return fail("guest archetype profile means are invalid");
    for (const auto value : entry.activityPreferenceMean)
      if (!isUnit(value))
        return fail("guest activity preference is outside 0..1");
    for (const auto value : entry.roomPreferenceMean)
      if (!isUnit(value))
        return fail("guest room preference is outside 0..1");
    for (const auto value : entry.expectationMean)
      if (!std::isfinite(value) || value < 0.0 || value > 100.0)
        return fail("guest expectation is outside 0..100");
  }
  for (const bool seen : archetypesSeen)
    if (!seen)
      return fail("guest archetype definitions are incomplete");
  if (!std::isfinite(totalArchetypeWeight) || totalArchetypeWeight <= 0.0)
    return fail("guest archetype weights must have a positive finite sum");

  std::array<bool, GuestTraitCount> traitsSeen{};
  for (std::size_t i = 0; i < definitions.traits.size(); ++i) {
    const auto &definition = definitions.traits[i];
    const auto index = static_cast<std::size_t>(definition.trait);
    if (index >= GuestTraitCount || index != i || traitsSeen[index])
      return fail("guest trait definitions are missing or duplicated");
    traitsSeen[index] = true;
    const auto &e = definition.effects;
    const auto finiteBounded = [](double value, double bound) {
      return std::isfinite(value) && std::abs(value) <= bound;
    };
    if (!finiteBounded(e.sensitivityDelta.price, 1.0) ||
        !finiteBounded(e.sensitivityDelta.service, 1.0) ||
        !finiteBounded(e.sensitivityDelta.cleanliness, 1.0) ||
        !finiteBounded(e.sensitivityDelta.noise, 1.0) ||
        !finiteBounded(e.sensitivityDelta.privacy, 1.0) ||
        !finiteBounded(e.sensitivityDelta.safety, 1.0) ||
        !finiteBounded(e.sensitivityDelta.comfort, 1.0) ||
        !finiteBounded(e.sensitivityDelta.food, 1.0) ||
        !finiteBounded(e.patienceDelta, 1.0) ||
        !finiteBounded(e.socialPreferenceDelta, 1.0) ||
        !std::isfinite(e.queueToleranceMultiplier) ||
        e.queueToleranceMultiplier < 0.0 ||
        e.queueToleranceMultiplier > 4.0 ||
        !std::isfinite(e.negativeMemoryReviewWeightMultiplier) ||
        e.negativeMemoryReviewWeightMultiplier < 0.0 ||
        e.negativeMemoryReviewWeightMultiplier > 4.0 ||
        !std::isfinite(e.lightSleepModifier) || e.lightSleepModifier < 0.0 ||
        e.lightSleepModifier > 4.0 ||
        !std::isfinite(e.hygieneDecayMultiplier) ||
        e.hygieneDecayMultiplier < 0.0 || e.hygieneDecayMultiplier > 4.0 ||
        !finiteBounded(e.complaintThresholdDelta, 100.0) ||
        !finiteBounded(e.morningPreferenceDelta, 1.0) ||
        !finiteBounded(e.eveningPreferenceDelta, 1.0))
      return fail("guest trait contains an invalid numeric modifier");
    for (const auto value : e.activityPreferenceDelta)
      if (!finiteBounded(value, 1.0))
        return fail("guest trait activity modifier is out of range");
    for (const auto value : e.roomPreferenceDelta)
      if (!finiteBounded(value, 1.0))
        return fail("guest trait room modifier is out of range");
    for (const auto value : e.expectationDelta)
      if (!finiteBounded(value, 100.0))
        return fail("guest trait expectation modifier is out of range");
    if (!traitHasEffect(e))
      return fail("every guest trait must have a numeric effect");
  }
  for (const bool seen : traitsSeen)
    if (!seen)
      return fail("guest trait definitions are incomplete");

  double categoryWeightTotal = 0.0;
  for (const auto weight : definitions.categoryWeights) {
    if (!std::isfinite(weight) || weight < 0.0)
      return fail("guest category weight must be finite and nonnegative");
    categoryWeightTotal += weight;
  }
  if (!std::isfinite(categoryWeightTotal) || categoryWeightTotal <= 0.0)
    return fail("guest category weights must have a positive finite sum");
  if (!validNeedTuning(definitions.needTuning))
    return fail("guest need and sleep tuning is out of range");
  return true;
}

bool canTransitionGuest(GuestLifecycleState from,
                        GuestLifecycleState to) noexcept {
  if (!validLifecycleState(from) || !validLifecycleState(to))
    return false;
  switch (from) {
  case GuestLifecycleState::Prospective:
    return to == GuestLifecycleState::Reserved;
  case GuestLifecycleState::Reserved:
    return to == GuestLifecycleState::TravelingToHotel ||
           to == GuestLifecycleState::Cancelled ||
           to == GuestLifecycleState::NoShow;
  case GuestLifecycleState::TravelingToHotel:
    return to == GuestLifecycleState::Arriving;
  case GuestLifecycleState::Arriving:
    return to == GuestLifecycleState::AwaitingCheckIn;
  case GuestLifecycleState::AwaitingCheckIn:
    return to == GuestLifecycleState::CheckedIn ||
           to == GuestLifecycleState::WalkedRelocated;
  case GuestLifecycleState::CheckedIn:
    return to == GuestLifecycleState::InStay;
  case GuestLifecycleState::InStay:
    return to == GuestLifecycleState::PreparingCheckout;
  case GuestLifecycleState::PreparingCheckout:
    return to == GuestLifecycleState::AwaitingCheckout;
  case GuestLifecycleState::AwaitingCheckout:
    return to == GuestLifecycleState::Departing;
  case GuestLifecycleState::Departing:
    return to == GuestLifecycleState::CompletedStay;
  case GuestLifecycleState::CompletedStay:
  case GuestLifecycleState::Cancelled:
  case GuestLifecycleState::NoShow:
  case GuestLifecycleState::WalkedRelocated:
  case GuestLifecycleState::Count:
    return false;
  }
  return false;
}

double guestNeedPressure(double satisfiedScore) noexcept {
  if (!std::isfinite(satisfiedScore))
    return 0.0;
  const double unsatisfied = (100.0 - std::clamp(satisfiedScore, 0.0, 100.0)) /
                             100.0;
  return unsatisfied * unsatisfied * 2.0;
}

GuestNeedState updateGuestNeeds(GuestNeedState current,
                                const GuestProfile &profile,
                                GuestActivity activity,
                                double elapsedSimulationSeconds,
                                const GuestModelDefinitions &definitions) {
  for (auto &value : current.values)
    value = boundedNeed(value);
  current.perceptions = updateGuestPerceptions(current.perceptions, {});
  if (!std::isfinite(current.energyPauseRemainingSeconds) ||
      current.energyPauseRemainingSeconds < 0.0)
    current.energyPauseRemainingSeconds = 0.0;
  if (!std::isfinite(elapsedSimulationSeconds) ||
      elapsedSimulationSeconds <= 0.0 ||
      !validNeedTuning(definitions.needTuning) || !validEnum(activity))
    return current;

  const auto addDelta = [](double value, double delta) {
    const double result = value + delta;
    if (!std::isfinite(result))
      return delta > 0.0 ? 100.0 : 0.0;
    return std::clamp(result, 0.0, 100.0);
  };
  const double hours = elapsedSimulationSeconds / 3600.0;
  const bool sleeping = activity == GuestActivity::Sleeping;
  const double pauseBeforeUpdate = current.energyPauseRemainingSeconds;
  current.energyPauseRemainingSeconds =
      std::max(0.0, pauseBeforeUpdate - elapsedSimulationSeconds);

  auto &energy = current.values[static_cast<std::size_t>(GuestNeed::Energy)];
  if (sleeping) {
    const double recoverySeconds =
        std::max(0.0, elapsedSimulationSeconds - pauseBeforeUpdate);
    energy = addDelta(energy, definitions.needTuning.sleepingEnergyRecoveryPerHour *
                                  (recoverySeconds / 3600.0));
  } else {
    energy = addDelta(energy,
                      -definitions.needTuning.awakeEnergyDecayPerHour * hours);
    auto &hunger =
        current.values[static_cast<std::size_t>(GuestNeed::Hunger)];
    hunger = addDelta(hunger,
                      -definitions.needTuning.awakeHungerDecayPerHour * hours);
  }

  double hygieneMultiplier = profile.hygieneDecayMultiplier;
  if (!std::isfinite(hygieneMultiplier) || hygieneMultiplier < 0.0)
    hygieneMultiplier = 1.0;
  hygieneMultiplier = std::min(hygieneMultiplier, 16.0);
  double hygieneDecay = definitions.needTuning.hygieneDecayPerHour;
  if (activity == GuestActivity::Exercise || activity == GuestActivity::Swim)
    hygieneDecay += definitions.needTuning.exerciseHygienePenaltyPerHour;
  auto &hygiene = current.values[static_cast<std::size_t>(GuestNeed::Hygiene)];
  hygiene = addDelta(hygiene, -hygieneDecay * hygieneMultiplier * hours);

  if (activity == GuestActivity::Idle) {
    auto &entertainment =
        current.values[static_cast<std::size_t>(GuestNeed::Entertainment)];
    entertainment = addDelta(
        entertainment,
        -definitions.needTuning.idleEntertainmentDecayPerHour * hours);
  }
  if (!sleeping) {
    const double socialPreference =
        std::isfinite(profile.socialPreference)
            ? std::clamp(profile.socialPreference, 0.0, 1.0)
            : 0.5;
    const double socialDecay =
        socialPreference >= 0.5
            ? definitions.needTuning.socialGuestDecayPerHour
            : definitions.needTuning.privateGuestDecayPerHour;
    auto &social = current.values[static_cast<std::size_t>(GuestNeed::Social)];
    social = addDelta(social, -socialDecay * hours);
  }
  return current;
}

GuestOperationalPerceptions updateGuestPerceptions(
    GuestOperationalPerceptions current,
    const GuestPerceptionEvidence &evidence) noexcept {
  setPerception(current.serviceConfidence, evidence.serviceConfidence);
  setPerception(current.cleanlinessConfidence,
                evidence.cleanlinessConfidence);
  setPerception(current.environmentComfort, evidence.environmentComfort);
  setPerception(current.valuePerception, evidence.valuePerception);
  return current;
}

bool guestCanPerformGoalIndependently(const GuestProfile &profile,
                                      GuestGoal goal) noexcept {
  if (!validEnum(goal) ||
      static_cast<std::size_t>(profile.ageBand) >=
          static_cast<std::size_t>(GuestAgeBand::Count))
    return false;
  return !(profile.ageBand == GuestAgeBand::Child && lifecycleGoal(goal));
}

bool groupAcceptsGoal(double proposedUtility,
                      double bestAlternativeUtility) noexcept {
  return std::isfinite(proposedUtility) && proposedUtility >= 0.0 &&
         std::isfinite(bestAlternativeUtility) &&
         bestAlternativeUtility >= 0.0 &&
         proposedUtility >= bestAlternativeUtility * 0.70;
}

std::optional<GuestGoalSelection>
selectGuestGoal(const GuestProfile &profile, const GuestNeedState &needs,
                std::span<const GuestGoalCandidate> candidates,
                std::optional<GuestGoal> mandatoryGoal) {
  if (mandatoryGoal) {
    if (!validEnum(*mandatoryGoal) ||
        !guestCanPerformGoalIndependently(profile, *mandatoryGoal))
      return std::nullopt;
    GuestGoalSelection selection;
    selection.goal = *mandatoryGoal;
    selection.mandatory = true;
    return selection;
  }

  constexpr double kMaximumFactor = 1.5;
  const auto validFactor = [=](double factor) {
    return std::isfinite(factor) && factor >= 0.0 &&
           factor <= kMaximumFactor;
  };
  std::optional<GuestGoalSelection> best;
  for (const auto &candidate : candidates) {
    if (!validEnum(candidate.goal) || !validEnum(candidate.requiredNeed) ||
        !guestCanPerformGoalIndependently(profile, candidate.goal) ||
        !candidate.available || !candidate.reachable ||
        !candidate.budgetCompatible || !candidate.timeCompatible ||
        !candidate.groupCompatible ||
        !validFactor(candidate.preference) ||
        !validFactor(candidate.availabilityFactor) ||
        !validFactor(candidate.timeCompatibility) ||
        !validFactor(candidate.budgetCompatibility) ||
        !validFactor(candidate.groupCompatibility) ||
        !validFactor(candidate.distanceUtility) ||
        !validFactor(candidate.moodModifier) ||
        !std::isfinite(candidate.expectedWaitMinutes) ||
        candidate.expectedWaitMinutes < 0.0 ||
        !std::isfinite(candidate.queueToleranceMinutes) ||
        candidate.queueToleranceMinutes < 0.0 ||
        candidate.expectedWaitMinutes > candidate.queueToleranceMinutes)
      continue;

    const auto needIndex = static_cast<std::size_t>(candidate.requiredNeed);
    const double pressure = guestNeedPressure(needs.values[needIndex]);
    const double utility = pressure * candidate.preference *
                           candidate.availabilityFactor *
                           candidate.timeCompatibility *
                           candidate.budgetCompatibility *
                           candidate.groupCompatibility *
                           candidate.distanceUtility * candidate.moodModifier;
    if (!std::isfinite(utility) || utility <= 0.0)
      continue;

    GuestGoalSelection selection;
    selection.goal = candidate.goal;
    selection.targetId = candidate.targetId;
    selection.utility = utility;
    selection.needPressure = pressure;
    selection.preference = candidate.preference;
    selection.availabilityFactor = candidate.availabilityFactor;
    selection.timeCompatibility = candidate.timeCompatibility;
    selection.budgetCompatibility = candidate.budgetCompatibility;
    selection.groupCompatibility = candidate.groupCompatibility;
    selection.distanceUtility = candidate.distanceUtility;
    selection.moodModifier = candidate.moodModifier;
    selection.expectedWaitMinutes = candidate.expectedWaitMinutes;

    const auto isBetter = [&] {
      if (!best || selection.utility > best->utility)
        return true;
      if (selection.utility < best->utility)
        return false;
      if (selection.goal != best->goal)
        return static_cast<std::size_t>(selection.goal) <
               static_cast<std::size_t>(best->goal);
      return selection.targetId < best->targetId;
    };
    if (isBetter())
      best = selection;
  }
  return best;
}

double guestQueueToleranceMinutes(double baseToleranceMinutes,
                                  const GuestProfile &profile,
                                  double segmentModifier,
                                  double urgencyModifier) noexcept {
  if (!std::isfinite(baseToleranceMinutes) || baseToleranceMinutes < 0.0 ||
      !std::isfinite(segmentModifier) || segmentModifier < 0.0 ||
      !std::isfinite(urgencyModifier) || urgencyModifier < 0.0 ||
      !std::isfinite(profile.queueToleranceMultiplier) ||
      profile.queueToleranceMultiplier < 0.0)
    return 0.0;
  const double patience = std::isfinite(profile.patience)
                               ? std::clamp(profile.patience, 0.0, 1.0)
                               : 0.0;
  const double tolerance = baseToleranceMinutes * (0.5 + patience) *
                           segmentModifier * urgencyModifier *
                           profile.queueToleranceMultiplier;
  return std::isfinite(tolerance) ? tolerance : 0.0;
}

GuestSleepNoiseUpdate updateGuestSleepingNoise(
    GuestNeedState &state, const GuestProfile &profile,
    std::optional<double> effectiveNoiseDb, double elapsedSimulationSeconds,
    std::uint64_t &guestRandomState,
    const GuestNeedTuning &tuning) noexcept {
  GuestSleepNoiseUpdate result;
  if (!validNeedTuning(tuning) ||
      !std::isfinite(elapsedSimulationSeconds) ||
      elapsedSimulationSeconds <= 0.0)
    return result;

  const bool measured = effectiveNoiseDb && std::isfinite(*effectiveNoiseDb) &&
                        *effectiveNoiseDb >= 0.0 && *effectiveNoiseDb <= 140.0;
  const double interval = tuning.sleepNoiseSampleIntervalSeconds;
  if (!std::isfinite(state.noiseSampleElapsedSeconds) ||
      state.noiseSampleElapsedSeconds < 0.0)
    state.noiseSampleElapsedSeconds = 0.0;
  state.noiseSampleElapsedSeconds =
      std::fmod(state.noiseSampleElapsedSeconds, interval);
  if (!std::isfinite(state.energyPauseRemainingSeconds) ||
      state.energyPauseRemainingSeconds < 0.0)
    state.energyPauseRemainingSeconds = 0.0;
  state.recentNoiseDisruptionCount =
      std::min(state.recentNoiseDisruptionCount,
               state.recentNoiseDisruptionAgesSeconds.size());

  const double sensitivity =
      std::isfinite(profile.sensitivities.noise)
          ? std::clamp(profile.sensitivities.noise, 0.0, 1.0)
          : 0.0;
  const double lightSleepModifier =
      std::isfinite(profile.lightSleepModifier)
          ? std::clamp(profile.lightSleepModifier, 0.0, 16.0)
          : 1.0;
  const double threshold = std::clamp(
      tuning.sleepNoiseThresholdDb -
          sensitivity * tuning.noiseSensitivityThresholdOffsetDb,
      0.0, 140.0);
  const double disruptionProbability =
      measured ? std::clamp((*effectiveNoiseDb - threshold) / 50.0, 0.0, 1.0) *
                     lightSleepModifier
               : 0.0;

  const auto advanceTime = [&](double seconds) {
    std::size_t kept = 0;
    for (std::size_t i = 0; i < state.recentNoiseDisruptionCount; ++i) {
      const double age = state.recentNoiseDisruptionAgesSeconds[i] + seconds;
      if (std::isfinite(age) && age <= tuning.noiseComplaintWindowSeconds)
        state.recentNoiseDisruptionAgesSeconds[kept++] = age;
    }
    state.recentNoiseDisruptionCount = kept;
  };

  double remaining = elapsedSimulationSeconds;
  while (remaining > 0.0) {
    const double untilSample =
        std::max(0.0, interval - state.noiseSampleElapsedSeconds);
    const double step = std::min(remaining, untilSample);
    if (step > 0.0) {
      advanceTime(step);
      state.noiseSampleElapsedSeconds += step;
      remaining -= step;
    }
    if (state.noiseSampleElapsedSeconds + 1e-9 < interval)
      break;
    state.noiseSampleElapsedSeconds =
        std::max(0.0, state.noiseSampleElapsedSeconds - interval);
    if (measured) {
      result.sampled = true;
      ++result.sampleCount;
      if (unitDraw(guestRandomState) < disruptionProbability) {
        result.disrupted = true;
        result.negativeMemoryAdded = true;
        ++result.disruptionCount;
        state.energyPauseRemainingSeconds =
            std::max(state.energyPauseRemainingSeconds,
                     tuning.sleepNoiseDisruptionSeconds);
        if (state.recentNoiseDisruptionCount ==
            state.recentNoiseDisruptionAgesSeconds.size()) {
          std::move(state.recentNoiseDisruptionAgesSeconds.begin() + 1,
                    state.recentNoiseDisruptionAgesSeconds.end(),
                    state.recentNoiseDisruptionAgesSeconds.begin());
          --state.recentNoiseDisruptionCount;
        }
        state.recentNoiseDisruptionAgesSeconds[
            state.recentNoiseDisruptionCount++] = 0.0;
        if (state.recentNoiseDisruptionCount >=
            tuning.noiseDisruptionsForComplaint)
          result.complaintEligible = true;
      }
    }
    if (step == 0.0 && remaining > 0.0) {
      // The elapsed sample value was at the boundary; the next iteration
      // must consume time before another sample can be reached.
      const double nextStep = std::min(remaining, interval);
      advanceTime(nextStep);
      state.noiseSampleElapsedSeconds += nextStep;
      remaining -= nextStep;
    }
  }
  result.energyGainPaused = state.energyPauseRemainingSeconds > 0.0;
  return result;
}

GuestProfile generateGuestProfile(std::uint64_t simulationSeed, GuestId guestId,
                                  const GuestModelDefinitions &definitions) {
  std::string error;
  if (!validateGuestModelDefinitions(definitions, &error))
    throw std::invalid_argument(error);
  if (guestId == 0)
    throw std::invalid_argument("guest id must be nonzero");

  GuestProfile profile;
  profile.id = guestId;
  auto state = deriveGuestRandomState(simulationSeed, guestId);

  double totalWeight = 0.0;
  for (const auto &entry : definitions.archetypes)
    totalWeight += entry.weight;
  const double roll = unitDraw(state) * totalWeight;
  double cumulative = 0.0;
  const GuestArchetypeDefinition *selected = nullptr;
  for (const auto &entry : definitions.archetypes) {
    cumulative += entry.weight;
    if (entry.weight > 0.0 && roll < cumulative) {
      selected = &entry;
      break;
    }
  }
  if (!selected) {
    for (auto it = definitions.archetypes.rbegin();
         it != definitions.archetypes.rend(); ++it) {
      if (it->weight > 0.0) {
        selected = &*it;
        break;
      }
    }
  }
  if (!selected)
    throw std::logic_error("validated guest archetype distribution is empty");

  profile.archetype = selected->archetype;
  profile.travelPurpose = selected->travelPurpose;
  profile.wealthBand = selected->wealthBand;
  const long double budget =
      static_cast<long double>(selected->budgetPerNightCents) *
      (1.0 + drawBetween(state, -selected->budgetSpread,
                         selected->budgetSpread));
  profile.budgetPerNightCents = static_cast<std::int64_t>(std::clamp(
      std::round(budget), 1.0L,
      static_cast<long double>(std::numeric_limits<std::int64_t>::max())));

  const double ageRoll = unitDraw(state);
  if (ageRoll < selected->childProbability)
    profile.ageBand = GuestAgeBand::Child;
  else if (ageRoll < selected->childProbability + selected->teenProbability)
    profile.ageBand = GuestAgeBand::Teen;
  else if (ageRoll < selected->childProbability + selected->teenProbability +
                         selected->seniorProbability)
    profile.ageBand = GuestAgeBand::Senior;
  else
    profile.ageBand = GuestAgeBand::Adult;

  const auto varyUnit = [&](double mean, double radius) {
    return std::clamp(drawBetween(state, mean - radius, mean + radius), 0.0,
                      1.0);
  };
  profile.sensitivities = {
      varyUnit(selected->meanSensitivities.price, 0.08),
      varyUnit(selected->meanSensitivities.service, 0.08),
      varyUnit(selected->meanSensitivities.cleanliness, 0.08),
      varyUnit(selected->meanSensitivities.noise, 0.08),
      varyUnit(selected->meanSensitivities.privacy, 0.08),
      varyUnit(selected->meanSensitivities.safety, 0.08),
      varyUnit(selected->meanSensitivities.comfort, 0.08),
      varyUnit(selected->meanSensitivities.food, 0.08)};
  profile.patience = varyUnit(selected->meanPatience, 0.10);
  profile.socialPreference = varyUnit(selected->meanSocialPreference, 0.10);
  for (std::size_t i = 0; i < GuestActivityPreferenceCount; ++i)
    profile.activityPreferences[i] =
        varyUnit(selected->activityPreferenceMean[i], 0.12);
  for (std::size_t i = 0; i < GuestRoomPreferenceCount; ++i)
    profile.roomPreferences[i] =
        varyUnit(selected->roomPreferenceMean[i], 0.12);
  for (std::size_t i = 0; i < GuestCategoryCount; ++i)
    profile.expectations[i] = std::clamp(
        drawBetween(state, selected->expectationMean[i] - 3.0,
                    selected->expectationMean[i] + 3.0),
        0.0, 100.0);

  std::array<std::size_t, GuestTraitCount> traitOrder{};
  std::iota(traitOrder.begin(), traitOrder.end(), 0);
  if (profile.ageBand == GuestAgeBand::Adult ||
      profile.ageBand == GuestAgeBand::Senior) {
    const auto traitCount = static_cast<std::size_t>(drawBounded(state, 4));
    profile.traits.reserve(traitCount);
    for (std::size_t i = 0; i < traitCount; ++i) {
      const auto index = i + static_cast<std::size_t>(
                                  drawBounded(state, GuestTraitCount - i));
      std::swap(traitOrder[i], traitOrder[index]);
      const auto traitIndex = traitOrder[i];
      profile.traits.push_back(static_cast<GuestTrait>(traitIndex));
      const auto &e = definitions.traits[traitIndex].effects;
      addAndClamp(profile.sensitivities, e.sensitivityDelta);
      profile.patience =
          std::clamp(profile.patience + e.patienceDelta, 0.0, 1.0);
      profile.socialPreference =
          std::clamp(profile.socialPreference + e.socialPreferenceDelta, 0.0,
                     1.0);
      addAndClamp(profile.activityPreferences, e.activityPreferenceDelta, 0.0,
                  1.0);
      addAndClamp(profile.roomPreferences, e.roomPreferenceDelta, 0.0, 1.0);
      addAndClamp(profile.expectations, e.expectationDelta, 0.0, 100.0);
      profile.queueToleranceMultiplier = std::clamp(
          profile.queueToleranceMultiplier * e.queueToleranceMultiplier, 0.0,
          16.0);
      profile.negativeMemoryReviewWeightMultiplier = std::clamp(
          profile.negativeMemoryReviewWeightMultiplier *
              e.negativeMemoryReviewWeightMultiplier,
          0.0, 16.0);
      profile.lightSleepModifier =
          std::clamp(profile.lightSleepModifier * e.lightSleepModifier, 0.0,
                     16.0);
      profile.hygieneDecayMultiplier =
          std::clamp(profile.hygieneDecayMultiplier * e.hygieneDecayMultiplier,
                     0.0, 16.0);
      profile.complaintThresholdDelta =
          std::clamp(profile.complaintThresholdDelta + e.complaintThresholdDelta,
                     -100.0, 100.0);
      profile.morningPreference =
          std::clamp(profile.morningPreference + e.morningPreferenceDelta, -1.0,
                     1.0);
      profile.eveningPreference =
          std::clamp(profile.eveningPreference + e.eveningPreferenceDelta, -1.0,
                     1.0);
    }
  }

  profile.randomState = state;
  return profile;
}

} // namespace hh::game
