#include "hh/game/BuildingSystems.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace hh::game::detail {
bool utilityConnected(const BuildingSystemsSnapshot &systems, EntityId roomId,
                      UtilityKind kind) noexcept {
  std::unordered_map<EntityId, const UtilityNodeSnapshot *> nodes;
  std::vector<EntityId> roomNodes;
  for (const auto &node : systems.utilityNodes)
    if (node.kind == kind) {
      nodes.emplace(node.id, &node);
      if (!node.source && node.roomId == roomId && node.load > 0)
        roomNodes.push_back(node.id);
    }
  if (roomNodes.empty())
    return false;

  std::unordered_map<EntityId, std::vector<EntityId>> adjacency;
  for (const auto &edge : systems.utilityEdges)
    if (nodes.contains(edge.from) && nodes.contains(edge.to)) {
      adjacency[edge.from].push_back(edge.to);
      adjacency[edge.to].push_back(edge.from);
    }

  for (const auto roomNode : roomNodes) {
    std::queue<EntityId> frontier;
    std::unordered_set<EntityId> visited;
    frontier.push(roomNode);
    visited.insert(roomNode);
    std::int64_t capacity = 0;
    std::int64_t load = 0;
    while (!frontier.empty()) {
      const auto current = frontier.front();
      frontier.pop();
      const auto *node = nodes.at(current);
      if (node->source)
        capacity += std::max(0, node->capacity);
      else
        load += std::max(0, node->load);
      const auto found = adjacency.find(current);
      if (found == adjacency.end())
        continue;
      for (const auto next : found->second)
        if (visited.insert(next).second)
          frontier.push(next);
    }
    if (capacity > 0 && capacity >= load)
      return true;
  }
  return false;
}

void refreshRoomUtilityFlags(BuildingSystemsSnapshot &systems) noexcept {
  for (auto &room : systems.rooms) {
    room.powerConnected = utilityConnected(systems, room.roomId,
                                           UtilityKind::Power);
    room.waterConnected = utilityConnected(systems, room.roomId,
                                           UtilityKind::Water);
  }
}

void tickElevator(ElevatorSnapshot &elevator) noexcept {
  const auto startNext = [&]() {
    if (elevator.requests.empty()) {
      elevator.state = ElevatorState::Idle;
      elevator.activeRequestId = 0;
      elevator.targetFloor = elevator.currentFloor;
      elevator.phaseSecondsRemaining = 0;
      return;
    }
    auto request = std::min_element(
        elevator.requests.begin(), elevator.requests.end(),
        [](const ElevatorRequestSnapshot &a, const ElevatorRequestSnapshot &b) {
          return a.id < b.id;
        });
    elevator.activeRequestId = request->id;
    request->boarded = false;
    if (elevator.currentFloor == request->pickupFloor) {
      elevator.state = ElevatorState::Boarding;
      elevator.targetFloor = elevator.currentFloor;
      elevator.phaseSecondsRemaining = elevator.doorSeconds;
    } else {
      elevator.state = ElevatorState::MovingToPickup;
      elevator.targetFloor = request->pickupFloor;
      elevator.phaseSecondsRemaining =
          std::abs(elevator.targetFloor - elevator.currentFloor) *
          elevator.travelSecondsPerFloor;
    }
  };

  if (elevator.state == ElevatorState::Idle) {
    startNext();
    return;
  }
  if (elevator.phaseSecondsRemaining > 0)
    --elevator.phaseSecondsRemaining;
  if (elevator.phaseSecondsRemaining > 0)
    return;

  auto active = std::find_if(
      elevator.requests.begin(), elevator.requests.end(),
      [&](const ElevatorRequestSnapshot &request) {
        return request.id == elevator.activeRequestId;
      });
  if (active == elevator.requests.end()) {
    elevator.state = ElevatorState::Idle;
    startNext();
    return;
  }

  switch (elevator.state) {
  case ElevatorState::MovingToPickup:
    elevator.currentFloor = elevator.targetFloor;
    elevator.state = ElevatorState::Boarding;
    elevator.phaseSecondsRemaining = elevator.doorSeconds;
    break;
  case ElevatorState::Boarding:
    active->boarded = true;
    elevator.targetFloor = active->destinationFloor;
    if (elevator.currentFloor == elevator.targetFloor) {
      elevator.state = ElevatorState::Alighting;
      elevator.phaseSecondsRemaining = elevator.doorSeconds;
    } else {
      elevator.state = ElevatorState::MovingToDestination;
      elevator.phaseSecondsRemaining =
          std::abs(elevator.targetFloor - elevator.currentFloor) *
          elevator.travelSecondsPerFloor;
    }
    break;
  case ElevatorState::MovingToDestination:
    elevator.currentFloor = elevator.targetFloor;
    elevator.state = ElevatorState::Alighting;
    elevator.phaseSecondsRemaining = elevator.doorSeconds;
    break;
  case ElevatorState::Alighting:
    elevator.requests.erase(active);
    elevator.activeRequestId = 0;
    elevator.state = ElevatorState::Idle;
    elevator.targetFloor = elevator.currentFloor;
    startNext();
    break;
  case ElevatorState::Idle:
    startNext();
    break;
  }
}

RoomSaleValidation validateRoomSystems(const BuildingSystemsSnapshot &systems,
                                       EntityId roomId) noexcept {
  RoomSaleValidation result;
  const auto found = std::find_if(
      systems.rooms.begin(), systems.rooms.end(),
      [&](const RoomSystemSnapshot &room) { return room.roomId == roomId; });
  if (found == systems.rooms.end()) {
    result.reasons.push_back(BuildingSystemReason::RoomNotFound);
    return result;
  }
  if (!found->powerConnected)
    result.reasons.push_back(BuildingSystemReason::NoPower);
  if (!found->waterConnected)
    result.reasons.push_back(BuildingSystemReason::NoWater);
  if (!found->egress)
    result.reasons.push_back(BuildingSystemReason::NoEgress);
  if (!found->accessible)
    result.reasons.push_back(BuildingSystemReason::NoAccessibleRoute);
  result.sellable = result.reasons.empty();
  return result;
}

} // namespace hh::game::detail
