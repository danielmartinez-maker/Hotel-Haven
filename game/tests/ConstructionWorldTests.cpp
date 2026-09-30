#include "hh/game/ConstructionWorld.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace hh::game;

namespace {

void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

bool contains(const std::vector<Position> &positions, Position target) {
  return std::find(positions.begin(), positions.end(), target) !=
         positions.end();
}

bool contains(const std::vector<GridEdge> &edges, GridEdge target) {
  return std::find(edges.begin(), edges.end(), target) != edges.end();
}

void placeFloorRect(ConstructionWorld &world, int floor, int x, int y,
                    int width, int height) {
  for (int row = y; row < y + height; ++row)
    for (int col = x; col < x + width; ++col)
      require(world.setTile({floor, col, row}, TileKind::Floor).ok,
              "floor fixture tile was rejected");
}

void placeWall(ConstructionWorld &world, GridEdge edge) {
  require(world.setWall(edge, true).ok, "wall fixture edge was rejected");
}

void closeRect(ConstructionWorld &world, int floor, int x, int y, int width,
               int height, std::optional<GridEdge> door = std::nullopt) {
  for (int col = x; col < x + width; ++col) {
    placeWall(world, edgeForSide({floor, col, y}, GridSide::North));
    placeWall(world,
              edgeForSide({floor, col, y + height - 1}, GridSide::South));
  }
  for (int row = y; row < y + height; ++row) {
    placeWall(world, edgeForSide({floor, x, row}, GridSide::West));
    placeWall(world,
              edgeForSide({floor, x + width - 1, row}, GridSide::East));
  }
  if (door)
    require(world.setDoor(*door, true).ok,
            "door fixture did not replace its wall");
}

RoomId allocateId(RoomId &nextId) { return nextId++; }

std::vector<Position> rectTiles(int floor, int x, int y, int width, int height) {
  std::vector<Position> tiles;
  for (int row = y; row < y + height; ++row)
    for (int col = x; col < x + width; ++col)
      tiles.push_back({floor, col, row});
  return tiles;
}

const ConstructionRegion *regionContaining(const ConstructionWorld &world,
                                           Position tile) {
  for (const auto &region : world.regions())
    if (contains(region.tiles, tile))
      return &region;
  return nullptr;
}

void canonical_edge_has_one_identity_from_either_cell() {
  const GridEdge east = edgeForSide({0, 3, 4}, GridSide::East);
  const GridEdge west = edgeForSide({0, 4, 4}, GridSide::West);
  require(east == west, "opposite sides did not produce one canonical edge");
}

void perimeter_edges_are_valid_and_outside_edges_rejected() {
  const ConstructionWorld world(10, 8, 1);
  require(world.validEdge(edgeForSide({0, 0, 0}, GridSide::North)),
          "north perimeter edge was rejected");
  require(world.validEdge(edgeForSide({0, 9, 4}, GridSide::East)),
          "east perimeter edge was rejected");
  require(!world.validEdge({0, 11, 4, EdgeAxis::Vertical}),
          "vertical edge past the perimeter was accepted");
  require(!world.validEdge({0, 4, 9, EdgeAxis::Horizontal}),
          "horizontal edge past the perimeter was accepted");
}

void rotated_footprint_and_interaction_nodes_are_canonical() {
  const ConstructionObject bed{1, ConstructionObjectKind::SingleBed,
                               {0, 3, 3}, 1};
  const auto footprint = footprintTiles(bed);
  const auto nodes = interactionNodes(bed);
  require(footprint.size() == 2, "rotated single bed is not two tiles");
  require(contains(footprint, {0, 3, 3}) && contains(footprint, {0, 4, 3}),
          "quarter-turn did not rotate the 1x2 bed into a 2x1 footprint");
  require(nodes.size() == 1, "functional bed must expose one interaction node");
  require(!contains(footprint, nodes.front()),
          "bed interaction node overlaps its footprint");
  int nearestDistance = 100;
  for (const Position tile : footprint) {
    const int distance = std::abs(nodes.front().x - tile.x) +
                         std::abs(nodes.front().y - tile.y);
    nearestDistance = std::min(nearestDistance, distance);
  }
  require(nearestDistance == 1,
          "bed interaction node is not immediately outside the footprint");
}

void closed_boundary_detects_one_room_and_door_remains_an_entrance() {
  ConstructionWorld world(10, 10, 1);
  placeFloorRect(world, 0, 2, 2, 3, 3);
  const GridEdge door = edgeForSide({0, 3, 2}, GridSide::North);
  closeRect(world, 0, 2, 2, 3, 3, door);
  RoomId nextId = 100;
  world.rebuildRegions([&] { return allocateId(nextId); });

  require(world.regions().size() == 1,
          "closed floor boundary did not create exactly one room");
  const auto &room = world.regions().front();
  require(room.tiles == rectTiles(0, 2, 2, 3, 3),
          "detected room tiles are not exact and row-major sorted");
  require(room.boundaryEdges.size() == 12,
          "detected room omitted boundary edge geometry");
  require(room.doorEntrances == std::vector<GridEdge>{door},
          "door did not remain a room entrance");
  require(world.setDoor(door, false).ok,
          "door could not be disabled back to a solid wall");
  require(world.doors().empty() && contains(world.walls(), door),
          "disabling a door did not restore its solid wall segment");
}

void open_boundary_does_not_report_an_enclosed_room() {
  ConstructionWorld world(8, 8, 1);
  placeFloorRect(world, 0, 2, 2, 2, 2);
  RoomId nextId = 100;
  world.rebuildRegions([&] { return allocateId(nextId); });
  require(world.regions().empty(),
          "open floor boundary was incorrectly reported as a room");
}

void split_keeps_old_id_on_larger_region() {
  ConstructionWorld world(10, 9, 1);
  placeFloorRect(world, 0, 2, 2, 5, 3);
  closeRect(world, 0, 2, 2, 5, 3);
  RoomId nextId = 100;
  world.rebuildRegions([&] { return allocateId(nextId); });
  require(world.regions().size() == 1 && world.regions().front().id == 100,
          "initial room ID was not allocated deterministically");

  for (int row = 2; row < 5; ++row)
    placeWall(world, {0, 5, row, EdgeAxis::Vertical});
  world.rebuildRegions([&] { return allocateId(nextId); });
  const auto *larger = regionContaining(world, {0, 2, 2});
  const auto *smaller = regionContaining(world, {0, 5, 2});
  require(larger && larger->tiles == rectTiles(0, 2, 2, 3, 3) &&
              larger->id == 100,
          "old room ID did not follow the larger split region");
  require(smaller && smaller->tiles == rectTiles(0, 5, 2, 2, 3) &&
              smaller->id == 101,
          "new split region did not receive the next stable room ID");
}

void split_id_tie_uses_first_tile_order() {
  ConstructionWorld world(9, 9, 1);
  placeFloorRect(world, 0, 2, 2, 4, 3);
  closeRect(world, 0, 2, 2, 4, 3);
  RoomId nextId = 200;
  world.rebuildRegions([&] { return allocateId(nextId); });
  for (int row = 2; row < 5; ++row)
    placeWall(world, {0, 4, row, EdgeAxis::Vertical});
  world.rebuildRegions([&] { return allocateId(nextId); });

  const auto *first = regionContaining(world, {0, 2, 2});
  const auto *second = regionContaining(world, {0, 4, 2});
  require(first && first->id == 200 &&
              first->tiles == rectTiles(0, 2, 2, 2, 3),
          "split tie did not keep the old ID on the first tile region");
  require(second && second->id == 201 &&
              second->tiles == rectTiles(0, 4, 2, 2, 3),
          "split tie assigned the new ID to the wrong region");
}

void merge_keeps_oldest_room_id() {
  ConstructionWorld world(10, 9, 1);
  placeFloorRect(world, 0, 2, 2, 4, 3);
  closeRect(world, 0, 2, 2, 4, 3);
  for (int row = 2; row < 5; ++row)
    placeWall(world, {0, 4, row, EdgeAxis::Vertical});
  RoomId nextId = 300;
  world.rebuildRegions([&] { return allocateId(nextId); });
  require(world.regions().size() == 2 && world.regions()[0].id == 300 &&
              world.regions()[1].id == 301,
          "adjacent enclosed rooms did not receive ordered IDs");

  for (int row = 2; row < 5; ++row)
    require(world.removeEdge({0, 4, row, EdgeAxis::Vertical}).ok,
            "shared wall could not be removed");
  world.rebuildRegions([&] { return allocateId(nextId); });
  require(world.regions().size() == 1 && world.regions().front().id == 300 &&
              world.regions().front().tiles == rectTiles(0, 2, 2, 4, 3),
          "merged room did not keep the oldest contributing ID");
}

void edit_preserves_other_floor_room_ids() {
  ConstructionWorld world(10, 10, 2);
  placeFloorRect(world, 0, 2, 2, 3, 3);
  closeRect(world, 0, 2, 2, 3, 3);
  placeFloorRect(world, 1, 2, 2, 3, 3);
  closeRect(world, 1, 2, 2, 3, 3);
  RoomId nextId = 400;
  world.rebuildRegions([&] { return allocateId(nextId); });
  const auto *upper = regionContaining(world, {1, 2, 2});
  require(upper && upper->id == 401,
          "second floor room did not get the second stable ID");

  for (int row = 2; row < 5; ++row)
    placeWall(world, {0, 3, row, EdgeAxis::Vertical});
  world.rebuildRegions([&] { return allocateId(nextId); });
  upper = regionContaining(world, {1, 2, 2});
  require(upper && upper->id == 401 && upper->tiles == rectTiles(1, 2, 2, 3, 3),
          "editing another floor changed an unrelated room ID");
}

void object_footprints_validate_and_reject_atomically() {
  ConstructionWorld world(8, 8, 1);
  placeFloorRect(world, 0, 1, 1, 5, 5);
  const ConstructionObject bed{1, ConstructionObjectKind::SingleBed,
                               {0, 2, 2}, 0};
  require(world.placeObject(bed).ok, "valid bed footprint was rejected");
  require(!world.setTile({0, 2, 2}, TileKind::Lobby).ok &&
              world.tileAt({0, 2, 2}) == TileKind::Floor,
          "tile edit overwrote a placed object's footprint");
  const auto before = world.objects();
  require(!world.placeObject({2, ConstructionObjectKind::Chair, {0, 2, 2}, 0})
               .ok,
          "overlapping object footprint was accepted");
  require(world.objects() == before,
          "rejected object placement changed the placed objects");
  require(!world.placeObject(
               {3, ConstructionObjectKind::SingleBed, {0, 2, 7}, 0})
               .ok,
          "out-of-bounds object footprint was accepted");
  require(world.objects() == before,
          "rejected out-of-bounds placement changed the placed objects");
}

} // namespace

int main() {
  try {
    canonical_edge_has_one_identity_from_either_cell();
    perimeter_edges_are_valid_and_outside_edges_rejected();
    rotated_footprint_and_interaction_nodes_are_canonical();
    closed_boundary_detects_one_room_and_door_remains_an_entrance();
    open_boundary_does_not_report_an_enclosed_room();
    split_keeps_old_id_on_larger_region();
    split_id_tie_uses_first_tile_order();
    merge_keeps_oldest_room_id();
    edit_preserves_other_floor_room_ids();
    object_footprints_validate_and_reject_atomically();
    std::cout << "ConstructionWorld: 10 tests passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "ConstructionWorld test failure: " << error.what() << '\n';
    return 1;
  }
}
