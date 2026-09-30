#pragma once

#include "hh/game/ConstructionTypes.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace hh::game {

class ConstructionWorld {
public:
  ConstructionWorld(int width, int height, int floors);

  int width() const noexcept { return width_; }
  int height() const noexcept { return height_; }
  int floors() const noexcept { return floors_; }

  bool contains(Position position) const noexcept;
  TileKind tileAt(Position position) const noexcept;
  bool occupiedByObject(Position position) const noexcept;
  bool validEdge(GridEdge edge) const noexcept;

  ConstructionResult setTile(Position position, TileKind kind);
  ConstructionResult setWall(GridEdge edge, bool enabled);
  ConstructionResult setDoor(GridEdge edge, bool enabled);
  ConstructionResult removeEdge(GridEdge edge);
  ConstructionResult placeObject(ConstructionObject object);
  ConstructionResult removeObject(EntityId id);
  void rebuildRegions(std::function<RoomId()> allocateRoomId);

  const std::vector<GridEdge> &walls() const noexcept { return walls_; }
  const std::vector<GridEdge> &doors() const noexcept { return doors_; }
  const std::vector<ConstructionObject> &objects() const noexcept {
    return objects_;
  }
  const std::vector<ConstructionRegion> &regions() const noexcept {
    return regions_;
  }

private:
  int width_{};
  int height_{};
  int floors_{};
  std::vector<TileKind> tiles_;
  std::vector<GridEdge> walls_;
  std::vector<GridEdge> doors_;
  std::vector<ConstructionObject> objects_;
  std::vector<EntityId> objectOccupancy_;
  std::vector<ConstructionRegion> regions_;
  std::vector<std::uint8_t> dirtyFloors_;
};

} // namespace hh::game
