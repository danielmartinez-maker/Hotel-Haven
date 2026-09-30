#pragma once

#include "hh/game/ServiceTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
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

enum class GuestLifecycleState : std::uint8_t {
  Prospective,
  Reserved,
  TravelingToHotel,
  Arriving,
  AwaitingCheckIn,
  CheckedIn,
  InStay,
  PreparingCheckout,
  AwaitingCheckout,
  Departing,
  CompletedStay,
  Cancelled,
  NoShow,
  WalkedRelocated,
  Count
};

enum class GuestNeed : std::uint8_t {
  Energy,
  Hunger,
  Hygiene,
  Comfort,
  Entertainment,
  Social,
  Privacy,
  Safety,
  Count
};
inline constexpr std::size_t GuestNeedCount =
    static_cast<std::size_t>(GuestNeed::Count);

enum class GuestActivity : std::uint8_t {
  Idle,
  Awake,
  Sleeping,
  Eating,
  Drinking,
  Bathing,
  Working,
  Exercise,
  Swim,
  Socializing,
  Relaxing,
  AttendEvent,
  Count
};

enum class GuestGoal : std::uint8_t {
  ReachHotel,
  CheckIn,
  ReachRoom,
  Sleep,
  Eat,
  Drink,
  Bathe,
  Work,
  Exercise,
  Swim,
  Socialize,
  Relax,
  AttendEvent,
  RequestService,
  ResolveComplaint,
  Checkout,
  LeaveHotel,
  Count
};

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

struct GuestNeedTuning {
  double awakeEnergyDecayPerHour{5.0};
  double awakeHungerDecayPerHour{10.0};
  double hygieneDecayPerHour{2.0};
  double idleEntertainmentDecayPerHour{4.0};
  double socialGuestDecayPerHour{3.0};
  double privateGuestDecayPerHour{1.0};
  double sleepingEnergyRecoveryPerHour{22.0};
  double exerciseHygienePenaltyPerHour{5.0};
  double sleepNoiseThresholdDb{75.0};
  double noiseSensitivityThresholdOffsetDb{45.0};
  double sleepNoiseSampleIntervalSeconds{300.0};
  double sleepNoiseDisruptionSeconds{300.0};
  double noiseComplaintWindowSeconds{3600.0};
  std::size_t noiseDisruptionsForComplaint{3};
  friend bool operator==(const GuestNeedTuning &,
                         const GuestNeedTuning &) = default;
};

struct GuestModelDefinitions {
  std::array<GuestArchetypeDefinition, GuestArchetypeCount> archetypes{};
  std::array<GuestTraitDefinition, GuestTraitCount> traits{};
  std::array<double, GuestCategoryCount> categoryWeights{};
  GuestNeedTuning needTuning{};
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

struct GuestOperationalPerceptions {
  double serviceConfidence{50.0};
  double cleanlinessConfidence{50.0};
  double environmentComfort{50.0};
  double valuePerception{50.0};
  friend bool operator==(const GuestOperationalPerceptions &,
                         const GuestOperationalPerceptions &) = default;
};

struct GuestPerceptionEvidence {
  std::optional<double> serviceConfidence;
  std::optional<double> cleanlinessConfidence;
  std::optional<double> environmentComfort;
  std::optional<double> valuePerception;
};

struct GuestNeedState {
  std::array<double, GuestNeedCount> values{100.0, 100.0, 100.0, 100.0,
                                           100.0, 100.0, 100.0, 100.0};
  GuestOperationalPerceptions perceptions{};
  double noiseSampleElapsedSeconds{};
  double energyPauseRemainingSeconds{};
  std::array<double, 3> recentNoiseDisruptionAgesSeconds{};
  std::size_t recentNoiseDisruptionCount{};
};

struct GuestSleepNoiseUpdate {
  bool sampled{};
  bool disrupted{};
  bool negativeMemoryAdded{};
  bool energyGainPaused{};
  bool complaintEligible{};
  std::size_t sampleCount{};
  std::size_t disruptionCount{};
};

struct GuestGoalCandidate {
  GuestGoal goal{GuestGoal::Relax};
  GuestId targetId{};
  GuestNeed requiredNeed{GuestNeed::Comfort};
  double preference{1.0};
  double availabilityFactor{1.0};
  double timeCompatibility{1.0};
  double budgetCompatibility{1.0};
  double groupCompatibility{1.0};
  double distanceUtility{1.0};
  double moodModifier{1.0};
  double expectedWaitMinutes{};
  double queueToleranceMinutes{60.0};
  bool available{true};
  bool reachable{true};
  bool budgetCompatible{true};
  bool timeCompatible{true};
  bool groupCompatible{true};
};

struct GuestGoalSelection {
  GuestGoal goal{GuestGoal::Relax};
  GuestId targetId{};
  double utility{};
  double needPressure{};
  double preference{};
  double availabilityFactor{};
  double timeCompatibility{};
  double budgetCompatibility{};
  double groupCompatibility{};
  double distanceUtility{};
  double moodModifier{};
  double expectedWaitMinutes{};
  bool mandatory{};
};

struct GuestGoalEvaluationMetrics {
  std::size_t candidatesVisited{};
  std::size_t utilityEvaluations{};
};

GuestModelDefinitions defaultGuestModelDefinitions();
bool validateGuestModelDefinitions(const GuestModelDefinitions &definitions,
                                   std::string *error = nullptr);
GuestProfile generateGuestProfile(std::uint64_t simulationSeed, GuestId guestId,
                                  const GuestModelDefinitions &definitions);

bool canTransitionGuest(GuestLifecycleState from,
                        GuestLifecycleState to) noexcept;
double guestNeedPressure(double satisfiedScore) noexcept;
GuestNeedState updateGuestNeeds(GuestNeedState current,
                                const GuestProfile &profile,
                                GuestActivity activity,
                                double elapsedSimulationSeconds,
                                const GuestModelDefinitions &definitions);
GuestOperationalPerceptions updateGuestPerceptions(
    GuestOperationalPerceptions current,
    const GuestPerceptionEvidence &evidence) noexcept;
bool guestCanPerformGoalIndependently(const GuestProfile &profile,
                                      GuestGoal goal) noexcept;
bool groupAcceptsGoal(double proposedUtility,
                      double bestAlternativeUtility) noexcept;
std::optional<GuestGoalSelection>
selectGuestGoal(const GuestProfile &profile, const GuestNeedState &needs,
                std::span<const GuestGoalCandidate> candidates,
                std::optional<GuestGoal> mandatoryGoal,
                GuestGoalEvaluationMetrics *metrics = nullptr);
double guestQueueToleranceMinutes(double baseToleranceMinutes,
                                  const GuestProfile &profile,
                                  double segmentModifier,
                                  double urgencyModifier) noexcept;
// Call after updateGuestNeeds for the same simulation interval. Need updates
// own the five-minute recovery-pause countdown; a new disruption pauses the
// following interval.
GuestSleepNoiseUpdate updateGuestSleepingNoise(
    GuestNeedState &state, const GuestProfile &profile,
    std::optional<double> effectiveNoiseDb, double elapsedSimulationSeconds,
    std::uint64_t &guestRandomState,
    const GuestNeedTuning &tuning) noexcept;

// These fixed integer operations make guest-local draws portable and
// independent of standard-library distribution implementations.
std::uint64_t deriveGuestRandomState(std::uint64_t simulationSeed,
                                     GuestId guestId) noexcept;
std::uint64_t drawGuestRandom(std::uint64_t &state) noexcept;

} // namespace hh::game
