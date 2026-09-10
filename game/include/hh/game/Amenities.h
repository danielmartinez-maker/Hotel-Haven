#pragma once

#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

using AmenityId = ServiceId;
using AmenityReservationId = ServiceId;

enum class AmenityType : std::uint8_t { Gym, Spa, Pool };
enum class AmenityBlockReason : std::uint8_t {
  None,
  InvalidRequest,
  NotFound,
  Closed,
  Dirty,
  OutOfCondition,
  CapacityFull,
  StaffUnavailable,
  GuestConflict
};
enum class AmenityStage : std::uint8_t { Reserved, InService, Completed, Cancelled };

struct AmenityConfig {
  AmenityId id{};
  AmenityType type{AmenityType::Gym};
  int openMinuteOfDay{};
  int closeMinuteOfDay{24 * 60};
  int capacity{1};
  int staffCapacity{};
  int cleanliness{10000};
  int condition{10000};
  std::int64_t priceCents{};
  int defaultDurationSeconds{1800};
  bool enabled{true};
};

struct AmenityRequest {
  AmenityId amenityId{};
  std::int64_t startSecond{};
  int durationSeconds{};
};

struct AmenityReservationResult {
  bool ok{};
  AmenityReservationId reservationId{};
  AmenityBlockReason blockReason{AmenityBlockReason::None};
};

struct AmenityView {
  AmenityId id{};
  AmenityType type{AmenityType::Gym};
  int capacity{};
  int active{};
  int cleanliness{};
  int condition{};
  bool open{};
  bool cleaningRequired{};
  bool maintenanceRequired{};
  std::int64_t priceCents{};
};

struct AmenityReservationView {
  AmenityReservationId id{};
  GuestId guestId{};
  AmenityId amenityId{};
  std::int64_t startSecond{};
  std::int64_t endSecond{};
  AmenityStage stage{AmenityStage::Reserved};
};

struct AmenityServiceOutcome {
  AmenityReservationId reservationId{};
  GuestId guestId{};
  AmenityId amenityId{};
  int satisfactionDelta{};
  std::int64_t revenueCents{};
};

struct AmenitiesSnapshot {
  std::int64_t elapsedSeconds{};
  std::vector<AmenityView> amenities;
  std::vector<AmenityReservationView> reservations;
  std::vector<AmenityServiceOutcome> outcomes;
  std::int64_t revenueCents{};
};

class AmenitiesSystem {
public:
  AmenitiesSystem();
  ~AmenitiesSystem();
  AmenitiesSystem(AmenitiesSystem &&) noexcept;
  AmenitiesSystem &operator=(AmenitiesSystem &&) noexcept;
  AmenitiesSystem(const AmenitiesSystem &);
  AmenitiesSystem &operator=(const AmenitiesSystem &);

  void addAmenity(const AmenityConfig &amenity);
  void setElapsedSeconds(std::int64_t elapsedSeconds);
  void setCleanliness(AmenityId id, int cleanliness);
  void setCondition(AmenityId id, int condition);
  [[nodiscard]] AmenityReservationResult reserveAmenity(
      GuestId guest, const AmenityRequest &request);
  [[nodiscard]] std::vector<AmenityId> opportunities(GuestId guest) const;
  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] AmenitiesSnapshot snapshot() const;
  [[nodiscard]] std::string save() const;
  static AmenitiesSystem load(std::string_view data);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace hh::game
