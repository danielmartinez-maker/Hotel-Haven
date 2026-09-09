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
} // namespace

int main() {
  try {
    guest_room_requires_power_water_egress_and_accessible_path();
    fire_and_security_coverage_are_authoritative_snapshot_state();
    elevator_dispatch_is_deterministic_for_equal_requests();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All building-system tests passed\n";
  return 0;
}
