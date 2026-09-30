#pragma once

#include "hh/game/ServiceTypes.h"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace hh::game {

struct Position {
  int floor{};
  int x{};
  int y{};
  friend auto operator<=>(const Position &, const Position &) = default;
};

enum class TileKind {
  Empty,
  Floor,
  Wall,
  Door,
  Entrance,
  FrontDesk,
  SupplyCloset,
  Stairs,
  Bathroom,
  StaffRoom,
  Lobby
};

enum class GridSide { North, East, South, West };
enum class EdgeAxis { Vertical, Horizontal };

struct GridEdge {
  int floor{};
  int x{};
  int y{};
  EdgeAxis axis{EdgeAxis::Vertical};
  friend auto operator<=>(const GridEdge &, const GridEdge &) = default;
};

enum class ConstructionObjectKind {
  SingleBed,
  DoubleBed,
  Toilet,
  Sink,
  Shower,
  Bath,
  Light,
  Desk,
  Chair,
  Nightstand,
  Plant
};

struct ConstructionObjectDefinition {
  ConstructionObjectKind kind{};
  int width{};
  int height{};
  int bedCapacity{};
  std::int64_t purchaseCostCents{};
  bool providesToilet{};
  bool providesSink{};
  bool providesBathing{};
  bool providesLight{};
  bool decor{};
  std::vector<Position> interactionOffsets;
};

struct ConstructionObject {
  EntityId id{};
  ConstructionObjectKind kind{};
  Position anchor;
  int quarterTurns{};
  bool legacyBaseline{};
  friend bool operator==(const ConstructionObject &,
                         const ConstructionObject &) = default;
};

struct ConstructionRegion {
  RoomId id{};
  std::vector<Position> tiles;
  std::vector<GridEdge> boundaryEdges;
  std::vector<GridEdge> doorEntrances;
};

struct ConstructionResult {
  bool ok{};
  std::string message;
  std::optional<EntityId> id;
};

GridEdge edgeForSide(Position tile, GridSide side) noexcept;

const ConstructionObjectDefinition &
objectDefinition(ConstructionObjectKind kind) noexcept;
std::vector<Position> footprintTiles(const ConstructionObject &object);
std::vector<Position> interactionNodes(const ConstructionObject &object);

} // namespace hh::game
