#pragma once

#include "hh/game/Simulation.h"
#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

enum class UtilityKind { Power, Water };
enum class InfrastructureKind { Egress, Accessibility, Fire, Security };
enum class BuildingSystemReason {
  None,
  RoomNotFound,
  NoPower,
  NoWater,
  NoEgress,
  NoAccessibleRoute
};

enum class ElevatorKind { Passenger, Service };
enum class ElevatorState {
  Idle,
  MovingToPickup,
  Boarding,
  MovingToDestination,
  Alighting
};
enum class ElevatorDirection { Idle, Up, Down };

struct UtilityNodeSnapshot {
  EntityId id{};
  UtilityKind kind{UtilityKind::Power};
  EntityId roomId{};
  bool source{};
  int capacity{};
  int load{};
  bool operator==(const UtilityNodeSnapshot &) const = default;
};

struct UtilityEdgeSnapshot {
  EntityId from{};
  EntityId to{};
  bool operator==(const UtilityEdgeSnapshot &) const = default;
};

struct RoomSystemSnapshot {
  EntityId roomId{};
  bool powerConnected{};
  bool waterConnected{};
  bool egress{};
  bool accessible{};
  bool fireCovered{};
  bool securityCovered{};
  bool operator==(const RoomSystemSnapshot &) const = default;
};

struct ElevatorRequestSnapshot {
  EntityId id{};
  int pickupFloor{};
  int destinationFloor{};
  bool boarded{};
  EntityId assignedElevatorId{};
  std::int64_t requestedAtSeconds{};
  std::int64_t boardedAtSeconds{};
  bool operator==(const ElevatorRequestSnapshot &) const = default;
};

struct ElevatorSnapshot {
  EntityId id{};
  ElevatorKind kind{ElevatorKind::Passenger};
  int minFloor{};
  int maxFloor{};
  int currentFloor{};
  int targetFloor{};
  int capacity{8};
  int travelSecondsPerFloor{4};
  int doorSeconds{2};
  ElevatorState state{ElevatorState::Idle};
  int phaseSecondsRemaining{};
  EntityId activeRequestId{};
  std::vector<ElevatorRequestSnapshot> requests;
  ElevatorDirection direction{ElevatorDirection::Idle};
  int onboardCount{};
  std::uint64_t completedTrips{};
  std::int64_t cumulativeWaitSeconds{};
  std::int64_t cumulativeRideSeconds{};
  bool operator==(const ElevatorSnapshot &) const = default;
};

struct ElevatorTripSnapshot {
  EntityId requestId{};
  EntityId elevatorId{};
  ElevatorKind kind{ElevatorKind::Passenger};
  int pickupFloor{};
  int destinationFloor{};
  std::int64_t waitSeconds{};
  std::int64_t rideSeconds{};
  bool operator==(const ElevatorTripSnapshot &) const = default;
};

struct ElevatorSpec {
  ElevatorKind kind{ElevatorKind::Passenger};
  int minFloor{};
  int maxFloor{};
  int startFloor{};
  int capacity{8};
  int travelSecondsPerFloor{4};
  int doorSeconds{2};
};

struct BuildingSystemsSnapshot {
  std::vector<UtilityNodeSnapshot> utilityNodes;
  std::vector<UtilityEdgeSnapshot> utilityEdges;
  std::vector<RoomSystemSnapshot> rooms;
  std::vector<ElevatorSnapshot> elevators;
  std::vector<ElevatorTripSnapshot> completedElevatorTrips;
  bool operator==(const BuildingSystemsSnapshot &) const = default;
};

struct RoomSaleValidation {
  bool sellable{};
  std::vector<BuildingSystemReason> reasons;
};

namespace detail {
[[nodiscard]] bool utilityConnected(const BuildingSystemsSnapshot &systems,
                                    EntityId roomId,
                                    UtilityKind kind) noexcept;
void refreshRoomUtilityFlags(BuildingSystemsSnapshot &systems) noexcept;
[[nodiscard]] EntityId
selectElevatorForRequest(const BuildingSystemsSnapshot &systems,
                         ElevatorKind kind, int pickupFloor,
                         int destinationFloor) noexcept;
void tickElevator(ElevatorSnapshot &elevator) noexcept;
[[nodiscard]] RoomSaleValidation
validateRoomSystems(const BuildingSystemsSnapshot &systems,
                    EntityId roomId) noexcept;
} // namespace detail

} // namespace hh::game
