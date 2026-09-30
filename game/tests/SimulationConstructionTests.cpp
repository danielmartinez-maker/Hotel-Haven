#include "hh/game/Simulation.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace hh::game;

namespace {

void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

EntityId nextIdFromSave(const Simulation &simulation) {
  std::istringstream input(simulation.save());
  std::string magic;
  int version{}, width{}, height{}, floors{};
  std::uint64_t seed{}, nextId{};
  std::int64_t elapsed{}, remainder{};
  input >> magic >> version >> seed >> width >> height >> floors >> elapsed >>
      remainder >> nextId;
  require(input && magic == "HHGS", "serialized save header is invalid");
  return nextId;
}

Simulation hotelWithOneRoom(RoomBlueprint blueprint, std::uint64_t seed = 71) {
  Simulation simulation(seed, 16, 12, 1);
  require(simulation.buildTile({0, 1, 1}, TileKind::Entrance).ok,
          "hotel entrance build failed");
  for (int x = 2; x <= 6; ++x)
    require(simulation.buildTile({0, x, 1}, TileKind::Floor).ok,
            "hotel approach corridor build failed");
  require(simulation.buildFurnishedRoom(blueprint).ok,
          "guest-ready room blueprint failed");
  return simulation;
}

void wall_edge_blocks_shared_path_and_door_edge_restores_it() {
  Simulation simulation(70, 8, 6, 1);
  for (int x = 1; x <= 5; ++x)
    require(simulation
                .buildTile({0, x, 2}, x == 1 ? TileKind::Entrance
                                              : TileKind::Floor)
                .ok,
            "path fixture tile build failed");
  const GridEdge edge = edgeForSide({0, 3, 2}, GridSide::East);
  require(simulation.isReachable({0, 1, 2}, {0, 5, 2}),
          "open path fixture is not reachable");
  require(simulation.setConstructionWall(edge, true).ok,
          "wall edge placement failed");
  require(!simulation.isReachable({0, 1, 2}, {0, 5, 2}),
          "wall edge did not block the shared path transition");
  require(simulation.setConstructionDoor(edge, true).ok,
          "door did not replace its wall edge");
  require(simulation.isReachable({0, 1, 2}, {0, 5, 2}),
          "door edge did not restore the path transition");
  require(simulation.setConstructionDoor(edge, false).ok &&
              !simulation.isReachable({0, 1, 2}, {0, 5, 2}),
          "disabling a door did not restore its wall barrier");
  require(simulation.removeConstructionEdge(edge).ok &&
              simulation.isReachable({0, 1, 2}, {0, 5, 2}),
          "removing the edge did not reopen the path");
}

void blueprint_creates_same_guest_ready_profile_and_capacity() {
  const RoomBlueprint blueprint{"101", 0, 2, 2, 6, 6, {0, 4, 2}, 2, 1,
                                140.0};
  Simulation simulation = hotelWithOneRoom(blueprint, 71);
  const auto view = simulation.view();
  require(view.rooms.size() == 1, "blueprint did not create one detected room");
  const auto &room = view.rooms.front();
  require(room.name == "101" && room.beds == 2 && room.baths == 1,
          "blueprint did not preserve guest-ready capacity");
  if (!room.reachable || room.status != RoomStatus::VacantReady)
    throw std::runtime_error(
        "blueprint not ready: reachable=" + std::to_string(room.reachable) +
        " status=" + std::to_string(static_cast<int>(room.status)) +
        " beds=" + std::to_string(room.beds) +
        " baths=" + std::to_string(room.baths));
  require(room.nightlyRateCents == 14000,
          "blueprint room rate changed during construction integration");
  require(view.economy.constructionCostCents == 6 * 6 * 15000 + 6 * 500,
          "blueprint aggregate construction charge changed");
}

void rejected_build_preserves_economy_and_ids() {
  const RoomBlueprint blueprint{"101", 0, 2, 2, 6, 6, {0, 4, 2}, 1, 1,
                                120.0};
  Simulation simulation = hotelWithOneRoom(blueprint, 72);
  const auto before = simulation.view().economy;
  const auto beforeNextId = nextIdFromSave(simulation);
  const auto rejected = simulation.setConstructionDoor(
      {0, 17, 5, EdgeAxis::Vertical}, true);
  require(!rejected.ok &&
              rejected.message == "Construction edge is outside the property",
          "out-of-property edge did not return the exact diagnostic");
  const auto after = simulation.view().economy;
  require(after.cashCents == before.cashCents &&
              after.constructionCostCents == before.constructionCostCents,
          "rejected edge placement changed cash or construction ledger");
  require(nextIdFromSave(simulation) == beforeNextId,
          "rejected edge placement consumed an entity ID");
}

void occupied_or_reserved_room_geometry_cannot_be_invalidated() {
  auto simulation = Simulation::tutorial(28);
  require(simulation.loadDefinitions(R"({"baseDemand":100})").ok,
          "reserved-room definitions were rejected");
  simulation.step(3600);
  const RoomView *reserved = nullptr;
  const auto before = simulation.view();
  for (const auto &room : before.rooms)
    if (room.reservationId != 0) {
      reserved = &room;
      break;
    }
  require(reserved != nullptr, "fixture did not reserve a guest room");
  const GridEdge perimeter =
      edgeForSide({reserved->floor, reserved->x + 1, reserved->y},
                  GridSide::North);
  const auto result = simulation.removeConstructionEdge(perimeter);
  require(!result.ok, "reserved room geometry was allowed to disappear");
  const auto after = simulation.view();
  const auto retained = std::find_if(
      after.rooms.begin(), after.rooms.end(),
      [&](const auto &room) { return room.id == reserved->id; });
  require(after.rooms.size() == before.rooms.size() &&
              retained != after.rooms.end() &&
              retained->reservationId == reserved->reservationId &&
              (retained->status == RoomStatus::Reserved ||
               retained->status == RoomStatus::Occupied),
          "rejected demolition changed the reserved room state");
}

void unreachable_required_object_blocks_room_readiness() {
  const RoomBlueprint blueprint{"101", 0, 2, 2, 6, 6, {0, 4, 2}, 1, 1,
                                120.0};
  auto simulation = hotelWithOneRoom(blueprint, 73);
  const Position objectTile{0, 7, 7};
  const GridEdge blockedInteraction =
      edgeForSide(objectTile, GridSide::North);
  require(simulation.setConstructionWall(blockedInteraction, true).ok,
          "object interaction barrier fixture failed");
  const auto result = simulation.placeConstructionObject(
      ConstructionObjectKind::Toilet, objectTile);
  require(!result.ok && result.message == "Required object is unreachable",
          "unreachable required object did not return the exact diagnostic");
}

} // namespace

int main() {
  try {
    wall_edge_blocks_shared_path_and_door_edge_restores_it();
    blueprint_creates_same_guest_ready_profile_and_capacity();
    rejected_build_preserves_economy_and_ids();
    occupied_or_reserved_room_geometry_cannot_be_invalidated();
    unreachable_required_object_blocks_room_readiness();
    std::cout << "SimulationConstruction: 5 tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "SimulationConstruction test failure: " << error.what()
              << '\n';
    return 1;
  }
}
