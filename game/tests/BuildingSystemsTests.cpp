#include "hh/game/BuildingSystems.h"
#include "hh/game/Simulation.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

EntityId buildRoom(Simulation &sim) {
  for (int x = 0; x < 10; ++x)
    require(sim.buildTile({0, x, 0}, x == 0 ? TileKind::Entrance
                                            : TileKind::Floor)
                .ok,
            "building-system corridor failed");
  const auto room = sim.buildFurnishedRoom(
      {"101", 0, 3, 1, 4, 4, {0, 3, 1}, 1, 1, 120});
  require(room.ok, "building-system room failed");
  return room.id;
}

bool hasReason(const RoomSaleValidation &validation,
               BuildingSystemReason reason) {
  return std::find(validation.reasons.begin(), validation.reasons.end(), reason) !=
         validation.reasons.end();
}

void guest_room_requires_power_water_egress_and_accessible_path() {
  Simulation sim(3001, 16, 10, 1);
  const auto roomId = buildRoom(sim);
  require(sim.setRoomUtility(roomId, UtilityKind::Power, false).ok &&
              sim.setRoomUtility(roomId, UtilityKind::Water, false).ok &&
              sim.setRoomInfrastructure(roomId, InfrastructureKind::Egress,
                                        false)
                  .ok &&
              sim.setRoomInfrastructure(roomId,
                                        InfrastructureKind::Accessibility,
                                        false)
                  .ok,
          "room systems could not be disconnected for validation fixture");
  const auto blocked = sim.validateRoomForSale(roomId);
  require(!blocked.sellable && hasReason(blocked, BuildingSystemReason::NoPower) &&
              hasReason(blocked, BuildingSystemReason::NoWater) &&
              hasReason(blocked, BuildingSystemReason::NoEgress) &&
              hasReason(blocked, BuildingSystemReason::NoAccessibleRoute),
          "room sale validation did not attribute all required blockers");

  require(sim.setRoomUtility(roomId, UtilityKind::Power, true).ok &&
              sim.setRoomUtility(roomId, UtilityKind::Water, true).ok &&
              sim.setRoomInfrastructure(roomId, InfrastructureKind::Egress,
                                        true)
                  .ok &&
              sim.setRoomInfrastructure(roomId,
                                        InfrastructureKind::Accessibility,
                                        true)
                  .ok,
          "room systems could not be restored");
  require(sim.validateRoomForSale(roomId).sellable,
          "fully serviced guest room remained blocked from sale");
}

void connected_utility_component_enforces_source_capacity() {
  BuildingSystemsSnapshot systems;
  systems.utilityNodes = {
      {1, UtilityKind::Power, 0, true, 1, 0},
      {2, UtilityKind::Power, 101, false, 0, 1},
      {3, UtilityKind::Power, 102, false, 0, 1},
  };
  systems.utilityEdges = {{1, 2}, {1, 3}};
  systems.rooms = {{101}, {102}};

  detail::refreshRoomUtilityFlags(systems);
  require(!systems.rooms[0].powerConnected &&
              !systems.rooms[1].powerConnected,
          "overloaded utility component remained connected despite insufficient capacity");

  systems.utilityNodes[0].capacity = 2;
  detail::refreshRoomUtilityFlags(systems);
  require(systems.rooms[0].powerConnected && systems.rooms[1].powerConnected,
          "adequately sized utility component did not restore connectivity");
}

void fire_and_security_coverage_are_authoritative_snapshot_state() {
  Simulation sim(3002, 16, 10, 1);
  const auto roomId = buildRoom(sim);
  require(sim.setRoomInfrastructure(roomId, InfrastructureKind::Fire, true).ok &&
              sim.setRoomInfrastructure(roomId, InfrastructureKind::Security,
                                        true)
                  .ok,
          "fire/security infrastructure could not be installed");
  const auto systems = sim.buildingSystemsSnapshot();
  const auto it = std::find_if(systems.rooms.begin(), systems.rooms.end(),
                               [&](const RoomSystemSnapshot &room) {
                                 return room.roomId == roomId;
                               });
  require(it != systems.rooms.end() && it->fireCovered && it->securityCovered,
          "fire/security coverage was not exposed by immutable snapshot");
}

void elevator_dispatch_is_deterministic_for_equal_requests() {
  Simulation a(3003, 8, 8, 5);
  Simulation b(3003, 8, 8, 5);
  ElevatorSpec spec;
  spec.kind = ElevatorKind::Passenger;
  spec.minFloor = 0;
  spec.maxFloor = 4;
  spec.startFloor = 0;
  const auto ea = a.installElevator(spec);
  const auto eb = b.installElevator(spec);
  require(ea.ok && eb.ok && ea.id == eb.id,
          "matching elevator installation diverged");
  require(a.requestElevator(ea.id, 0, 4).ok &&
              b.requestElevator(eb.id, 0, 4).ok,
          "matching elevator request was rejected");
  a.step(600);
  b.step(600);
  require(a.buildingSystemsSnapshot().elevators ==
              b.buildingSystemsSnapshot().elevators,
          "same-seed elevator dispatch diverged");
}

void elevator_bank_selects_best_car_with_stable_tie_breaking() {
  Simulation sim(3010, 8, 8, 8);
  ElevatorSpec spec;
  spec.minFloor = 0;
  spec.maxFloor = 7;
  spec.startFloor = 0;
  const auto low = sim.installElevator(spec);
  spec.startFloor = 6;
  const auto high = sim.installElevator(spec);
  require(low.ok && high.ok, "elevator bank fixture install failed");

  const auto request = sim.requestElevator(ElevatorKind::Passenger, 6, 1);
  require(request.ok, "bank request was rejected");
  sim.step(1);
  const auto snapshot = sim.buildingSystemsSnapshot();
  const auto highCar = std::find_if(snapshot.elevators.begin(), snapshot.elevators.end(),
                                    [&](const ElevatorSnapshot &e) { return e.id == high.id; });
  require(highCar != snapshot.elevators.end(), "selected elevator disappeared");
  require(highCar->requests.size() == 1 &&
              highCar->requests.front().id == request.id &&
              highCar->requests.front().assignedElevatorId == high.id,
          "bank request did not select the nearest deterministic car");
}

void elevator_capacity_batches_same_floor_same_direction_requests() {
  Simulation sim(3011, 8, 8, 6);
  ElevatorSpec spec;
  spec.minFloor = 0;
  spec.maxFloor = 5;
  spec.startFloor = 0;
  spec.capacity = 2;
  spec.travelSecondsPerFloor = 4;
  spec.doorSeconds = 2;
  const auto elevator = sim.installElevator(spec);
  require(elevator.ok, "capacity fixture install failed");
  require(sim.requestElevator(ElevatorKind::Passenger, 0, 4).ok &&
              sim.requestElevator(ElevatorKind::Passenger, 0, 3).ok &&
              sim.requestElevator(ElevatorKind::Passenger, 0, 5).ok,
          "capacity fixture request failed");

  sim.step(3);
  const auto snapshot = sim.buildingSystemsSnapshot();
  const auto &car = snapshot.elevators.front();
  const auto boarded = std::count_if(car.requests.begin(), car.requests.end(),
                                     [](const ElevatorRequestSnapshot &r) {
                                       return r.boarded;
                                     });
  require(car.onboardCount == 2 && boarded == 2,
          "elevator did not board up to capacity in one door cycle");
  require(car.requests.size() == 3,
          "capacity-saturated rider was lost instead of remaining queued");
}

void elevator_banks_are_kind_isolated_and_validate_requests() {
  Simulation sim(3012, 8, 8, 6);
  ElevatorSpec passenger;
  passenger.kind = ElevatorKind::Passenger;
  passenger.minFloor = 0;
  passenger.maxFloor = 5;
  passenger.startFloor = 0;
  ElevatorSpec service = passenger;
  service.kind = ElevatorKind::Service;
  service.startFloor = 5;
  const auto p = sim.installElevator(passenger);
  const auto s = sim.installElevator(service);
  require(p.ok && s.ok, "bank isolation fixture install failed");

  const auto request = sim.requestElevator(ElevatorKind::Service, 5, 0);
  require(request.ok, "service bank request failed");
  const auto snapshot = sim.buildingSystemsSnapshot();
  const auto passengerCar = std::find_if(snapshot.elevators.begin(), snapshot.elevators.end(),
                                         [&](const ElevatorSnapshot &e) { return e.id == p.id; });
  const auto serviceCar = std::find_if(snapshot.elevators.begin(), snapshot.elevators.end(),
                                       [&](const ElevatorSnapshot &e) { return e.id == s.id; });
  require(passengerCar->requests.empty() && serviceCar->requests.size() == 1,
          "service request crossed into passenger bank");
  require(!sim.requestElevator(ElevatorKind::Passenger, 2, 2).ok,
          "degenerate elevator request was accepted");
  require(!sim.requestElevator(ElevatorKind::Passenger, 0, 7).ok,
          "unserved-floor elevator request was accepted");
}
} // namespace

int main() {
  try {
    guest_room_requires_power_water_egress_and_accessible_path();
    connected_utility_component_enforces_source_capacity();
    fire_and_security_coverage_are_authoritative_snapshot_state();
    elevator_dispatch_is_deterministic_for_equal_requests();
    elevator_bank_selects_best_car_with_stable_tie_breaking();
    elevator_capacity_batches_same_floor_same_direction_requests();
    elevator_banks_are_kind_isolated_and_validate_requests();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All building-system tests passed\n";
  return 0;
}
