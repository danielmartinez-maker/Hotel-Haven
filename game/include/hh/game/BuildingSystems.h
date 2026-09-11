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
  // Derived from containment in the authoritative elevator request vector.
  EntityId assignedElevatorId{};
  bool operator==(const ElevatorRequestSnapshot &other) const noexcept {
    return id == other.id && pickupFloor == other.pickupFloor &&
           destinationFloor == other.destinationFloor && boarded == other.boarded;
  }
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
  // Direction/count/counter fields are deterministic diagnostics derived from
  // the HHGS 11 authoritative car/request state and are intentionally omitted
  // from persistence equality.
  ElevatorDirection direction{ElevatorDirection::Idle};
  int onboardCount{};
  std::uint64_t completedTrips{};
  bool operator==(const ElevatorSnapshot &other) const noexcept {
    return id == other.id && kind == other.kind && minFloor == other.minFloor &&
           maxFloor == other.maxFloor && currentFloor == other.currentFloor &&
           targetFloor == other.targetFloor && capacity == other.capacity &&
           travelSecondsPerFloor == other.travelSecondsPerFloor &&
           doorSeconds == other.doorSeconds && state == other.state &&
           phaseSecondsRemaining == other.phaseSecondsRemaining &&
           activeRequestId == other.activeRequestId && requests == other.requests;
  }
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
