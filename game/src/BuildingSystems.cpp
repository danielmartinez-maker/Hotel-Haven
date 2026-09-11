#include "hh/game/BuildingSystems.h"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace hh::game::detail {
namespace {
ElevatorDirection requestDirection(const ElevatorRequestSnapshot &request) noexcept {
  if (request.destinationFloor > request.pickupFloor)
    return ElevatorDirection::Up;
  if (request.destinationFloor < request.pickupFloor)
    return ElevatorDirection::Down;
  return ElevatorDirection::Idle;
}

bool directionCompatible(const ElevatorRequestSnapshot &request,
                         ElevatorDirection direction) noexcept {
  return direction == ElevatorDirection::Idle ||
         requestDirection(request) == direction;
}

int onboardCount(const ElevatorSnapshot &elevator) noexcept {
  return static_cast<int>(std::count_if(
      elevator.requests.begin(), elevator.requests.end(),
      [](const ElevatorRequestSnapshot &request) { return request.boarded; }));
}

std::vector<ElevatorRequestSnapshot>::iterator
oldestWaiting(ElevatorSnapshot &elevator) noexcept {
  return std::min_element(
      elevator.requests.begin(), elevator.requests.end(),
      [](const ElevatorRequestSnapshot &a, const ElevatorRequestSnapshot &b) {
        if (a.boarded != b.boarded)
          return !a.boarded && b.boarded;
        return a.id < b.id;
      });
}

std::vector<ElevatorRequestSnapshot>::iterator
nextOnboardStop(ElevatorSnapshot &elevator) noexcept {
  auto best = elevator.requests.end();
  for (auto it = elevator.requests.begin(); it != elevator.requests.end(); ++it) {
    if (!it->boarded)
      continue;
    if (elevator.direction == ElevatorDirection::Up &&
        it->destinationFloor < elevator.currentFloor)
      continue;
    if (elevator.direction == ElevatorDirection::Down &&
        it->destinationFloor > elevator.currentFloor)
      continue;
    if (best == elevator.requests.end()) {
      best = it;
      continue;
    }
    const int distance = std::abs(it->destinationFloor - elevator.currentFloor);
    const int bestDistance =
        std::abs(best->destinationFloor - elevator.currentFloor);
    if (std::tie(distance, it->id) < std::tie(bestDistance, best->id))
      best = it;
  }
  if (best != elevator.requests.end())
    return best;
  for (auto it = elevator.requests.begin(); it != elevator.requests.end(); ++it)
    if (it->boarded &&
        (best == elevator.requests.end() || it->id < best->id))
      best = it;
  return best;
}

void refreshDerivedState(ElevatorSnapshot &elevator) noexcept {
  for (auto &request : elevator.requests)
    if (request.assignedElevatorId == 0)
      request.assignedElevatorId = elevator.id;
  elevator.onboardCount = onboardCount(elevator);
}
} // namespace

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

EntityId selectElevatorForRequest(const BuildingSystemsSnapshot &systems,
                                  ElevatorKind kind, int pickupFloor,
                                  int destinationFloor) noexcept {
  if (pickupFloor == destinationFloor)
    return 0;
  EntityId bestId = 0;
  std::tuple<std::int64_t, std::size_t, EntityId> bestKey{
      std::numeric_limits<std::int64_t>::max(),
      std::numeric_limits<std::size_t>::max(),
      std::numeric_limits<EntityId>::max()};
  for (const auto &elevator : systems.elevators) {
    if (elevator.kind != kind || pickupFloor < elevator.minFloor ||
        pickupFloor > elevator.maxFloor || destinationFloor < elevator.minFloor ||
        destinationFloor > elevator.maxFloor)
      continue;
    const int projectedFloor = elevator.state == ElevatorState::Idle
                                   ? elevator.currentFloor
                                   : elevator.targetFloor;
    const std::int64_t eta =
        std::max(0, elevator.phaseSecondsRemaining) +
        static_cast<std::int64_t>(std::abs(projectedFloor - pickupFloor)) *
            elevator.travelSecondsPerFloor +
        static_cast<std::int64_t>(elevator.requests.size()) *
            elevator.doorSeconds;
    const auto key = std::make_tuple(eta, elevator.requests.size(), elevator.id);
    if (key < bestKey) {
      bestKey = key;
      bestId = elevator.id;
    }
  }
  return bestId;
}

void tickElevator(ElevatorSnapshot &elevator) noexcept {
  refreshDerivedState(elevator);

  const auto beginPickup = [&]() {
    auto request = oldestWaiting(elevator);
    if (request == elevator.requests.end() || request->boarded) {
      elevator.state = ElevatorState::Idle;
      elevator.activeRequestId = 0;
      elevator.targetFloor = elevator.currentFloor;
      elevator.direction = ElevatorDirection::Idle;
      elevator.phaseSecondsRemaining = 0;
      return;
    }
    elevator.activeRequestId = request->id;
    elevator.direction = requestDirection(*request);
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

  const auto beginDestination = [&]() {
    auto next = nextOnboardStop(elevator);
    if (next == elevator.requests.end()) {
      elevator.state = ElevatorState::Idle;
      elevator.activeRequestId = 0;
      elevator.targetFloor = elevator.currentFloor;
      elevator.direction = ElevatorDirection::Idle;
      elevator.phaseSecondsRemaining = 0;
      beginPickup();
      return;
    }
    elevator.activeRequestId = next->id;
    elevator.targetFloor = next->destinationFloor;
    if (elevator.currentFloor == elevator.targetFloor) {
      elevator.state = ElevatorState::Alighting;
      elevator.phaseSecondsRemaining = elevator.doorSeconds;
    } else {
      elevator.state = ElevatorState::MovingToDestination;
      elevator.phaseSecondsRemaining =
          std::abs(elevator.targetFloor - elevator.currentFloor) *
          elevator.travelSecondsPerFloor;
    }
  };

  if (elevator.state == ElevatorState::Idle) {
    beginPickup();
    return;
  }
  if (elevator.phaseSecondsRemaining > 0)
    --elevator.phaseSecondsRemaining;
  if (elevator.phaseSecondsRemaining > 0)
    return;

  switch (elevator.state) {
  case ElevatorState::MovingToPickup:
    elevator.currentFloor = elevator.targetFloor;
    elevator.state = ElevatorState::Boarding;
    elevator.phaseSecondsRemaining = elevator.doorSeconds;
    break;
  case ElevatorState::Boarding: {
    auto active = std::find_if(elevator.requests.begin(), elevator.requests.end(),
                               [&](const ElevatorRequestSnapshot &request) {
                                 return request.id == elevator.activeRequestId;
                               });
    if (active == elevator.requests.end() || active->boarded) {
      elevator.state = ElevatorState::Idle;
      beginPickup();
      break;
    }
    elevator.direction = requestDirection(*active);
    std::vector<ElevatorRequestSnapshot *> candidates;
    for (auto &request : elevator.requests)
      if (!request.boarded && request.pickupFloor == elevator.currentFloor &&
          directionCompatible(request, elevator.direction))
        candidates.push_back(&request);
    std::sort(candidates.begin(), candidates.end(),
              [](const ElevatorRequestSnapshot *a,
                 const ElevatorRequestSnapshot *b) { return a->id < b->id; });
    int available = std::max(0, elevator.capacity - onboardCount(elevator));
    for (auto *request : candidates) {
      if (available == 0)
        break;
      request->boarded = true;
      --available;
    }
    refreshDerivedState(elevator);
    beginDestination();
    break;
  }
  case ElevatorState::MovingToDestination:
    elevator.currentFloor = elevator.targetFloor;
    elevator.state = ElevatorState::Alighting;
    elevator.phaseSecondsRemaining = elevator.doorSeconds;
    break;
  case ElevatorState::Alighting:
    elevator.requests.erase(
        std::remove_if(elevator.requests.begin(), elevator.requests.end(),
                       [&](const ElevatorRequestSnapshot &request) {
                         return request.boarded &&
                                request.destinationFloor == elevator.currentFloor;
                       }),
        elevator.requests.end());
    refreshDerivedState(elevator);
    if (elevator.onboardCount > 0)
      beginDestination();
    else {
      elevator.direction = ElevatorDirection::Idle;
      elevator.state = ElevatorState::Idle;
      elevator.activeRequestId = 0;
      elevator.targetFloor = elevator.currentFloor;
      beginPickup();
    }
    break;
  case ElevatorState::Idle:
    beginPickup();
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
