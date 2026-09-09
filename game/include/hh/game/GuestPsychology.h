#pragma once

#include "hh/game/Simulation.h"
#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <unordered_map>
#include <vector>

namespace hh::game {

using GuestId = EntityId;

enum class ExperienceCategory {
  Room,
  Service,
  Cleanliness,
  Food,
  Amenities,
  Quiet,
  Convenience,
  Value,
  ArrivalDeparture
};

enum class ExperienceEventType {
  FastCheckIn,
  LongCheckInQueue,
  RoomNotReady,
  FreeUpgrade,
  DirtyBathroom,
  ExcellentRoomCleanliness,
  BrokenAC,
  QuickMaintenanceRecovery,
  GreatMeal,
  SlowRoomService,
  ElevatorDelay,
  NoiseDisturbance,
  StaffRudeness,
  StaffExceptionalService
};

enum class ComplaintType {
  CheckInDelay,
  RoomReadiness,
  Cleanliness,
  Maintenance,
  FoodService,
  ElevatorDelay,
  Noise,
  StaffConduct,
  ServiceFailure
};

enum class ComplaintUrgency { Low, Medium, High, Critical };

struct GuestNeedState {
  int energy{100};
  int hunger{100};
  int hygiene{100};
  int comfort{100};
  int entertainment{100};
  int social{100};
  int privacy{100};
  int safety{100};
  bool operator==(const GuestNeedState &) const = default;
};

struct GuestOperationalPerception {
  int serviceConfidence{70};
  int cleanlinessConfidence{70};
  int environmentComfort{70};
  int valuePerception{70};
  bool operator==(const GuestOperationalPerception &) const = default;
};

struct GuestExpectationState {
  int room{70};
  int cleanliness{70};
  int service{70};
  int food{70};
  int amenities{70};
  int quiet{70};
  int convenience{70};
  int value{70};
  bool operator==(const GuestExpectationState &) const = default;
};

struct SatisfactionBreakdown {
  int room{70};
  int service{70};
  int cleanliness{70};
  int food{70};
  int amenities{70};
  int convenience{70};
  int value{70};
  int arrivalDeparture{70};
  int checkIn{70};
  int noise{70};
  int waits{70};
  int expectationFit{70};
  int overall{70};
  bool operator==(const SatisfactionBreakdown &) const = default;
};

struct ExperienceEvent {
  ExperienceEventType type{ExperienceEventType::FastCheckIn};
  std::int64_t timestampSeconds{};
  EntityId locationId{};
  EntityId sourceEntityId{};
  ExperienceCategory category{ExperienceCategory::Service};
  int observedValue{};
  int expectedValue{};
  int rawImpact{};
  int memorySalience{10000};
  int memoryHalfLifeHours{24};
  bool resolved{};
  bool complaintEligible{};
  bool operator==(const ExperienceEvent &) const = default;
};

struct GuestMemory {
  ExperienceEventType type{ExperienceEventType::FastCheckIn};
  std::int64_t timestampSeconds{};
  EntityId locationId{};
  EntityId sourceEntityId{};
  ExperienceCategory category{ExperienceCategory::Service};
  int valence{};
  int magnitude{};
  int salience{10000};
  int halfLifeHours{24};
  bool resolved{};
  bool operator==(const GuestMemory &) const = default;
};

struct Complaint {
  ComplaintType type{ComplaintType::ServiceFailure};
  ComplaintUrgency urgency{ComplaintUrgency::Low};
  std::int64_t timestampSeconds{};
  EntityId locationId{};
  EntityId sourceEntityId{};
  ExperienceEventType sourceEvent{ExperienceEventType::LongCheckInQueue};
  int magnitude{};
  bool resolved{};
  bool operator==(const Complaint &) const = default;
};

struct GuestPsychologySnapshot {
  GuestId guestId{};
  GuestProfileView profile;
  GuestNeedState needs;
  GuestOperationalPerception operational;
  GuestExpectationState expectations;
  SatisfactionBreakdown satisfaction;
  std::vector<GuestMemory> memories;
  std::vector<Complaint> complaints;
  bool operator==(const GuestPsychologySnapshot &) const = default;
};

[[nodiscard]] int
calculateRepeatIntent(const GuestPsychologySnapshot &psychology,
                      std::int64_t nowSeconds) noexcept;

class GuestPsychology {
public:
  explicit GuestPsychology(std::uint64_t campaignSeed) noexcept;

  [[nodiscard]] GuestProfileView generateGuestProfile(GuestId guestId) const;
  [[nodiscard]] static bool validProfile(const GuestProfileView &profile) noexcept;

  void initializeGuest(GuestId guestId, const GuestProfileView &profile);
  void restoreGuest(const GuestPsychologySnapshot &snapshot);
  [[nodiscard]] std::optional<GuestPsychologySnapshot>
  snapshot(GuestId guestId) const;
  void updateNeeds(GuestId guestId, std::int64_t seconds, bool sleeping);
  void recordExperience(GuestId guestId, const ExperienceEvent &event);

  [[nodiscard]] static double
  memoryContribution(const GuestMemory &memory,
                     std::int64_t nowSeconds) noexcept;

private:
  struct Record {
    GuestPsychologySnapshot snapshot;
    std::array<std::int64_t, 8> needRemainders{};
  };

  std::uint64_t campaignSeed_{};
  std::unordered_map<GuestId, Record> guests_;
};

namespace detail {
[[nodiscard]] GuestProfileView
 generateGuestProfileFromRandom(std::mt19937_64 &random);
[[nodiscard]] bool validGuestProfile(const GuestProfileView &profile) noexcept;
[[nodiscard]] int queueToleranceFor(const GuestProfileView &profile,
                                    double baseSeconds = 480) noexcept;
} // namespace detail

} // namespace hh::game
