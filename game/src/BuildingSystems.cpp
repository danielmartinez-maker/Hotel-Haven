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
enum class Direction { Idle, Up, Down };

Direction requestDirection(const ElevatorRequestSnapshot &request) noexcept {
  if (request.destinationFloor > request.pickupFloor)
    return Direction::Up;
  if (request.destinationFloor < request.pickupFloor)
    return Direction::Down;
  return Direction::Idle;
}

Direction sweepDirection(const ElevatorSnapshot &elevator) noexcept {
  for (const auto &request : elevator.requests)
    if (request.boarded)
      return requestDirection(request);
  const auto active = std::find_if(
      elevator.requests.begin(), elevator.requests.end(),
      [&](const ElevatorRequestSnapshot &request) {
        return request.id == elevator.activeRequestId;
      });
  return active == elevator.requests.end() ? Direction::Idle
                                           : requestDirection(*active);
}

bool directionCompatible(const ElevatorRequestSnapshot &request,
                         Direction direction) noexcept {
  return direction == Direction::Idle || requestDirection(request) == direction;
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
  const auto direction = sweepDirection(elevator);
  auto best = elevator.requests.end();
  for (auto it = elevator.requests.begin(); it != elevator.requests.end(); ++it) {
    if (!it->boarded)
      continue;
    if (direction == Direction::Up &&
        it->destinationFloor < elevator.currentFloor)
      continue;
    if (direction == Direction::Down &&
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

struct DispatchProjection {
  int floor{};
  std::int64_t seconds{};
};

DispatchProjection projectAfterOnboardSweep(
    const ElevatorSnapshot &elevator) noexcept {
  DispatchProjection projection;
  projection.floor = elevator.state == ElevatorState::Idle
                         ? elevator.currentFloor
                         : elevator.targetFloor;
  projection.seconds = std::max(0, elevator.phaseSecondsRemaining);

  const auto direction = sweepDirection(elevator);
  if (direction == Direction::Idle || onboardCount(elevator) == 0)
    return projection;

  int sweepEnd = projection.floor;
  std::vector<int> stops;
  for (const auto &request : elevator.requests) {
    if (!request.boarded)
      continue;
    if (std::find(stops.begin(), stops.end(), request.destinationFloor) ==
        stops.end())
      stops.push_back(request.destinationFloor);
    if (direction == Direction::Up)
      sweepEnd = std::max(sweepEnd, request.destinationFloor);
    else
      sweepEnd = std::min(sweepEnd, request.destinationFloor);
  }

  projection.seconds +=
      static_cast<std::int64_t>(std::abs(sweepEnd - projection.floor)) *
      elevator.travelSecondsPerFloor;
  for (const int stop : stops) {
    const bool currentAlightingStop =
        elevator.state == ElevatorState::Alighting &&
        stop == elevator.currentFloor;
    if (!currentAlightingStop)
      projection.seconds += elevator.doorSeconds;
  }
  projection.floor = sweepEnd;
  return projection;
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
    const auto projection = projectAfterOnboardSweep(elevator);
    const auto waitingRequests = static_cast<std::size_t>(std::count_if(
        elevator.requests.begin(), elevator.requests.end(),
        [](const ElevatorRequestSnapshot &request) { return !request.boarded; }));
    const std::int64_t eta =
        projection.seconds +
        static_cast<std::int64_t>(std::abs(projection.floor - pickupFloor)) *
            elevator.travelSecondsPerFloor +
        static_cast<std::int64_t>(waitingRequests) * elevator.doorSeconds;
    const auto key = std::make_tuple(eta, elevator.requests.size(), elevator.id);
    if (key < bestKey) {
      bestKey = key;
      bestId = elevator.id;
    }
  }
  return bestId;
}

void tickElevator(ElevatorSnapshot &elevator) noexcept {
  const auto beginPickup = [&]() {
    auto request = oldestWaiting(elevator);
    if (request == elevator.requests.end() || request->boarded) {
      elevator.state = ElevatorState::Idle;
      elevator.activeRequestId = 0;
      elevator.targetFloor = elevator.currentFloor;
      elevator.phaseSecondsRemaining = 0;
      return;
    }
    elevator.activeRequestId = request->id;
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
    const auto direction = requestDirection(*active);
    std::vector<ElevatorRequestSnapshot *> candidates;
    for (auto &request : elevator.requests)
      if (!request.boarded && request.pickupFloor == elevator.currentFloor &&
          directionCompatible(request, direction))
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
    if (onboardCount(elevator) > 0)
      beginDestination();
    else {
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
