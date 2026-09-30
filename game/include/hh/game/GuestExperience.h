#pragma once

#include "hh/game/GuestModel.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hh::game {

enum class GuestExperienceEventType : std::uint8_t {
  FastCheckIn,
  LongCheckInQueue,
  RoomNotReady,
  FreeUpgrade,
  DirtyBathroom,
  ExcellentRoomCleanliness,
  BrokenAc,
  QuickMaintenanceRecovery,
  GreatMeal,
  SlowRoomService,
  ElevatorDelay,
  NoiseDisturbance,
  StaffRudeness,
  StaffExceptionalService,
  SecurityIncident,
  Recovery,
  Count
};
inline constexpr std::size_t GuestExperienceEventTypeCount =
    static_cast<std::size_t>(GuestExperienceEventType::Count);

enum class GuestComplaintUrgency : std::uint8_t { Low, Medium, High, Critical, Count };
enum class GuestRecoveryOption : std::uint8_t {
  ApologyOnly,
  FreeDrink,
  PartialRoomRefund,
  RoomUpgrade,
  FullNightRefund,
  Count
};
inline constexpr std::size_t GuestRecoveryOptionCount =
    static_cast<std::size_t>(GuestRecoveryOption::Count);

struct GuestExperienceEvent {
  std::uint64_t eventId{};
  GuestId guestId{};
  EntityId incidentId{};
  GuestExperienceEventType type{GuestExperienceEventType::FastCheckIn};
  std::int64_t timestampSeconds{};
  EntityId locationId{};
  std::optional<EntityId> sourceEntityId;
  GuestCategory category{GuestCategory::Service};
  double observedValue{};
  double expectedValue{70.0};
  double rawImpact{};
  double salience{1.0};
  bool complaintEligible{};
  bool critical{};
  bool resolved{};
  std::optional<EntityId> resolvesIncidentId;
  double resolvedMagnitudeReduction{};
  std::string reviewStatement;
};

struct GuestMemory {
  EntityId id{};
  std::uint64_t eventId{};
  EntityId incidentId{};
  GuestExperienceEventType type{GuestExperienceEventType::FastCheckIn};
  std::int64_t timestampSeconds{};
  EntityId locationId{};
  std::optional<EntityId> sourceEntityId;
  GuestCategory category{GuestCategory::Service};
  double valence{};
  double magnitude{};
  double salience{1.0};
  double decayHalfLifeHours{12.0};
  bool critical{};
  bool resolved{};
  std::string reviewStatement;
  friend bool operator==(const GuestMemory &, const GuestMemory &) = default;
};

struct GuestComplaint {
  EntityId id{};
  EntityId incidentId{};
  EntityId memoryId{};
  GuestComplaintUrgency urgency{GuestComplaintUrgency::Low};
  bool open{true};
  friend bool operator==(const GuestComplaint &, const GuestComplaint &) =
      default;
};

struct GuestExperienceState {
  std::array<double, GuestCategoryCount> categoryStartingSatisfaction{
      70.0, 70.0, 70.0, 70.0, 70.0, 70.0, 70.0, 70.0, 70.0};
  std::array<double, GuestCategoryCount> categorySatisfaction{
      70.0, 70.0, 70.0, 70.0, 70.0, 70.0, 70.0, 70.0, 70.0};
  std::vector<GuestExperienceEvent> events;
  std::vector<GuestMemory> memories;
  std::vector<GuestComplaint> complaints;
  bool isGroupLeader{true};
};

struct GuestExperienceDefinitions {
  std::array<double, GuestCategoryCount> categoryWeights{};
  std::array<double, GuestExperienceEventTypeCount> memoryHalfLifeHours{};
  std::array<double, GuestRecoveryOptionCount> recoveryMagnitudeReduction{};
  double complaintMagnitudeThreshold{25.0};
  double reviewGenerationProbability{0.65};
  double lowSatisfactionReviewBonus{0.20};
  double highSatisfactionReviewBonus{0.10};
  double reviewScoreNoiseRange{0.30};
  double criticReputationImpactMultiplier{5.0};
  friend bool operator==(const GuestExperienceDefinitions &,
                         const GuestExperienceDefinitions &) = default;
};

struct GuestExperienceResult {
  bool accepted{};
  bool duplicate{};
  bool memoryAdded{};
  bool complaintCreated{};
  bool memoryResolved{};
  bool complaintResolved{};
  EntityId memoryId{};
  EntityId complaintId{};
  double overallSatisfaction{70.0};
  GuestComplaintUrgency complaintUrgency{GuestComplaintUrgency::Low};
};

struct GuestRecoveryContext {
  double staffHospitality{1.0};
  double guestForgiveness{1.0};
  double responseMinutes{};
  GuestComplaintUrgency complaintUrgency{GuestComplaintUrgency::Low};
  bool issueFixed{};
  bool superiorRoomAvailable{};
};

struct GuestRecoveryOutcome {
  double effectiveness{};
  double responseSpeedFactor{};
  double magnitudeReduction{};
  double remainingNegativeMagnitude{};
  friend bool operator==(const GuestRecoveryOutcome &,
                         const GuestRecoveryOutcome &) = default;
};

struct GuestReview {
  bool generated{};
  int completedDay{};
  double rating{1.0};
  double overallSatisfaction{70.0};
  double generationProbability{};
  double reputationImpactMultiplier{1.0};
  std::vector<EntityId> memoryIds;
  std::string text;
  friend bool operator==(const GuestReview &, const GuestReview &) = default;
};

GuestExperienceDefinitions defaultGuestExperienceDefinitions();
bool validateGuestExperienceDefinitions(
    const GuestExperienceDefinitions &definitions,
    std::string *error = nullptr);
double guestMemoryContribution(const GuestMemory &memory,
                               double ageHours) noexcept;
double guestOverallSatisfaction(const GuestExperienceState &state,
                                std::int64_t currentTimestampSeconds,
                                const GuestExperienceDefinitions &definitions)
    noexcept;
GuestExperienceResult applyGuestExperience(
    const GuestProfile &profile, GuestExperienceState &state,
    const GuestExperienceEvent &event,
    const GuestExperienceDefinitions &definitions, EntityId memoryId,
    EntityId complaintId);
std::optional<GuestRecoveryOutcome> calculateGuestRecovery(
    const GuestMemory &memory, GuestRecoveryOption option,
    const GuestRecoveryContext &context,
    const GuestExperienceDefinitions &definitions);
GuestReview generateGuestReview(const GuestProfile &profile,
                                const GuestExperienceState &state,
                                int completedDay,
                                std::uint64_t &guestRandomState,
                                const GuestExperienceDefinitions &definitions);

} // namespace hh::game
