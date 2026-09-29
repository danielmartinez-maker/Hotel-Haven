#pragma once

#include "hh/game/ServiceTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

enum class GuestArchetype : std::uint8_t {
  BudgetLeisure,
  Backpacker,
  BusinessTraveler,
  ExecutiveBusiness,
  CoupleLeisure,
  FamilyLeisure,
  LuxuryLeisure,
  ConferenceDelegate,
  GroupTourTraveler,
  AirportTransitTraveler,
  WellnessTraveler,
  VipCelebrity,
  CriticReviewer,
  Count
};
inline constexpr std::size_t GuestArchetypeCount =
    static_cast<std::size_t>(GuestArchetype::Count);

enum class GuestAgeBand : std::uint8_t { Child, Teen, Adult, Senior, Count };
enum class GuestTravelPurpose : std::uint8_t {
  Leisure,
  Business,
  Conference,
  Transit,
  Wellness,
  Group,
  Review,
  Count
};
enum class GuestWealthBand : std::uint8_t {
  Budget,
  Standard,
  Affluent,
  Luxury,
  Count
};

enum class GuestActivityPreference : std::uint8_t {
  Wifi,
  Desk,
  Breakfast,
  Spa,
  Pool,
  Fitness,
  Work,
  Social,
  Event,
  Count
};
inline constexpr std::size_t GuestActivityPreferenceCount =
    static_cast<std::size_t>(GuestActivityPreference::Count);

enum class GuestRoomPreference : std::uint8_t {
  Single,
  Double,
  Suite,
  Quiet,
  HighFloor,
  Accessible,
  Count
};
inline constexpr std::size_t GuestRoomPreferenceCount =
    static_cast<std::size_t>(GuestRoomPreference::Count);

// HMG-010 names Quiet among the expectation categories and
// ArrivalDeparture among the overall review weights. Keep both explicit.
enum class GuestCategory : std::uint8_t {
  Room,
  Cleanliness,
  Service,
  Food,
  Amenities,
  Quiet,
  Convenience,
  Value,
  ArrivalDeparture,
  Count
};
inline constexpr std::size_t GuestCategoryCount =
    static_cast<std::size_t>(GuestCategory::Count);

enum class GuestTrait : std::uint8_t {
  Patient,
  Impatient,
  Neat,
  Messy,
  LightSleeper,
  HeavySleeper,
  Foodie,
  Workaholic,
  Social,
  Private,
  Frugal,
  StatusConscious,
  FitnessFocused,
  EarlyRiser,
  NightOwl,
  ComplaintProne,
  Forgiving,
  Count
};
inline constexpr std::size_t GuestTraitCount =
    static_cast<std::size_t>(GuestTrait::Count);

struct GuestSensitivityProfile {
  double price{};
  double service{};
  double cleanliness{};
  double noise{};
  double privacy{};
  double safety{};
  double comfort{};
  double food{};
  friend bool operator==(const GuestSensitivityProfile &,
                         const GuestSensitivityProfile &) = default;
};

struct GuestTraitEffects {
  GuestSensitivityProfile sensitivityDelta{};
  std::array<double, GuestActivityPreferenceCount> activityPreferenceDelta{};
  std::array<double, GuestRoomPreferenceCount> roomPreferenceDelta{};
  std::array<double, GuestCategoryCount> expectationDelta{};
  double patienceDelta{};
  double socialPreferenceDelta{};
  double queueToleranceMultiplier{1.0};
  double negativeMemoryReviewWeightMultiplier{1.0};
  double lightSleepModifier{1.0};
  double hygieneDecayMultiplier{1.0};
  double complaintThresholdDelta{};
  double morningPreferenceDelta{};
  double eveningPreferenceDelta{};
  friend bool operator==(const GuestTraitEffects &,
                         const GuestTraitEffects &) = default;
};

struct GuestTraitDefinition {
  GuestTrait trait{GuestTrait::Patient};
  GuestTraitEffects effects{};
  friend bool operator==(const GuestTraitDefinition &,
                         const GuestTraitDefinition &) = default;
};

struct GuestArchetypeDefinition {
  GuestArchetype archetype{GuestArchetype::BudgetLeisure};
  double weight{1.0};
  GuestTravelPurpose travelPurpose{GuestTravelPurpose::Leisure};
  GuestWealthBand wealthBand{GuestWealthBand::Standard};
  std::int64_t budgetPerNightCents{10000};
  double budgetSpread{0.15};
  double childProbability{};
  double teenProbability{};
  double seniorProbability{};
  GuestSensitivityProfile meanSensitivities{};
  double meanPatience{0.5};
  double meanSocialPreference{0.5};
  std::array<double, GuestActivityPreferenceCount> activityPreferenceMean{};
  std::array<double, GuestRoomPreferenceCount> roomPreferenceMean{};
  std::array<double, GuestCategoryCount> expectationMean{};
  friend bool operator==(const GuestArchetypeDefinition &,
                         const GuestArchetypeDefinition &) = default;
};

struct GuestModelDefinitions {
  std::array<GuestArchetypeDefinition, GuestArchetypeCount> archetypes{};
  std::array<GuestTraitDefinition, GuestTraitCount> traits{};
  std::array<double, GuestCategoryCount> categoryWeights{};
  friend bool operator==(const GuestModelDefinitions &,
                         const GuestModelDefinitions &) = default;
};

struct GuestProfile {
  GuestId id{};
  GuestArchetype archetype{GuestArchetype::BudgetLeisure};
  GuestAgeBand ageBand{GuestAgeBand::Adult};
  GuestTravelPurpose travelPurpose{GuestTravelPurpose::Leisure};
  GuestWealthBand wealthBand{GuestWealthBand::Standard};
  std::int64_t budgetPerNightCents{};
  GuestSensitivityProfile sensitivities{};
  double patience{0.5};
  double socialPreference{0.5};
  std::array<double, GuestActivityPreferenceCount> activityPreferences{};
  std::array<double, GuestRoomPreferenceCount> roomPreferences{};
  std::array<double, GuestCategoryCount> expectations{};
  std::vector<GuestTrait> traits;
  double queueToleranceMultiplier{1.0};
  double negativeMemoryReviewWeightMultiplier{1.0};
  double lightSleepModifier{1.0};
  double hygieneDecayMultiplier{1.0};
  double complaintThresholdDelta{};
  double morningPreference{};
  double eveningPreference{};
  std::uint64_t randomState{};
  friend bool operator==(const GuestProfile &, const GuestProfile &) = default;
};

GuestModelDefinitions defaultGuestModelDefinitions();
bool validateGuestModelDefinitions(const GuestModelDefinitions &definitions,
                                   std::string *error = nullptr);
GuestProfile generateGuestProfile(std::uint64_t simulationSeed, GuestId guestId,
                                  const GuestModelDefinitions &definitions);

// These fixed integer operations make guest-local draws portable and
// independent of standard-library distribution implementations.
std::uint64_t deriveGuestRandomState(std::uint64_t simulationSeed,
                                     GuestId guestId) noexcept;
std::uint64_t drawGuestRandom(std::uint64_t &state) noexcept;

} // namespace hh::game
