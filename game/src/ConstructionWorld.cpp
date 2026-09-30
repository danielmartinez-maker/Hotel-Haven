#include "hh/game/ConstructionWorld.h"

#include <algorithm>
#include <array>
#include <deque>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace hh::game {
namespace {

constexpr std::int64_t kObjectCostPerTileCents = 15000;

ConstructionObjectDefinition definition(ConstructionObjectKind kind, int width,
                                        int height, int bedCapacity = 0,
                                        bool toilet = false, bool sink = false,
                                        bool bathing = false, bool light = false,
                                        bool decor = false) {
  std::vector<Position> interactions;
  if (!decor)
    interactions.push_back({0, 0, -1});
  return {kind,
          width,
          height,
          bedCapacity,
          kObjectCostPerTileCents * width * height,
          toilet,
          sink,
          bathing,
          light,
          decor,
          std::move(interactions)};
}

const std::array<ConstructionObjectDefinition, 11> &definitions() {
  static const std::array<ConstructionObjectDefinition, 11> values{
      definition(ConstructionObjectKind::SingleBed, 1, 2, 1),
      definition(ConstructionObjectKind::DoubleBed, 2, 2, 2),
      definition(ConstructionObjectKind::Toilet, 1, 1, 0, true),
      definition(ConstructionObjectKind::Sink, 1, 1, 0, false, true),
      definition(ConstructionObjectKind::Shower, 1, 1, 0, false, false, true),
      definition(ConstructionObjectKind::Bath, 1, 1, 0, false, false, true),
      definition(ConstructionObjectKind::Light, 1, 1, 0, false, false, false,
                 true),
      definition(ConstructionObjectKind::Desk, 1, 1),
      definition(ConstructionObjectKind::Chair, 1, 1),
      definition(ConstructionObjectKind::Nightstand, 1, 1),
      definition(ConstructionObjectKind::Plant, 1, 1, 0, false, false, false,
                 false, true)};
  return values;
}

Position orientedOffset(const ConstructionObjectDefinition &definition,
                        Position anchor, Position offset, int quarterTurns) {
  const int turn = ((quarterTurns % 4) + 4) % 4;
  int dx = offset.x;
  int dy = offset.y;
  switch (turn) {
  case 1:
    return {anchor.floor, anchor.x + definition.height - 1 - dy,
            anchor.y + dx};
  case 2:
    return {anchor.floor, anchor.x + definition.width - 1 - dx,
            anchor.y + definition.height - 1 - dy};
  case 3:
    return {anchor.floor, anchor.x + dy,
            anchor.y + definition.width - 1 - dx};
  default:
    return {anchor.floor, anchor.x + dx, anchor.y + dy};
  }
}

bool tileLess(Position lhs, Position rhs) {
  return std::tie(lhs.floor, lhs.y, lhs.x) <
         std::tie(rhs.floor, rhs.y, rhs.x);
}

void sortPositions(std::vector<Position> &positions) {
  std::sort(positions.begin(), positions.end(), tileLess);
}

bool containsPosition(const std::vector<Position> &positions,
                      Position target) {
  return std::binary_search(positions.begin(), positions.end(), target,
                            tileLess);
}

bool roomFloorTile(TileKind kind) {
  return kind == TileKind::Floor || kind == TileKind::Bathroom ||
         kind == TileKind::StaffRoom || kind == TileKind::Lobby;
}

ConstructionResult success(std::optional<EntityId> id = std::nullopt) {
  return {true, {}, id};
}

ConstructionResult failure(std::string message) {
  return {false, std::move(message), std::nullopt};
}

bool edgeIsIn(const std::vector<GridEdge> &edges, GridEdge edge) {
  return std::binary_search(edges.begin(), edges.end(), edge);
}

void insertEdge(std::vector<GridEdge> &edges, GridEdge edge) {
  const auto it = std::lower_bound(edges.begin(), edges.end(), edge);
  if (it == edges.end() || *it != edge)
    edges.insert(it, edge);
}

bool eraseEdge(std::vector<GridEdge> &edges, GridEdge edge) {
  const auto it = std::lower_bound(edges.begin(), edges.end(), edge);
  if (it == edges.end() || *it != edge)
    return false;
  edges.erase(it);
  return true;
}

bool regionFirstTileLess(const ConstructionRegion &lhs,
                         const ConstructionRegion &rhs) {
  if (lhs.tiles.empty())
    return !rhs.tiles.empty();
  if (rhs.tiles.empty())
    return false;
  const auto key = [](const Position &position) {
    return std::tuple{position.floor, position.y, position.x};
  };
  return key(lhs.tiles.front()) < key(rhs.tiles.front());
}

} // namespace

GridEdge edgeForSide(Position tile, GridSide side) noexcept {
  switch (side) {
  case GridSide::North:
    return {tile.floor, tile.x, tile.y, EdgeAxis::Horizontal};
  case GridSide::East:
    return {tile.floor, tile.x + 1, tile.y, EdgeAxis::Vertical};
  case GridSide::South:
    return {tile.floor, tile.x, tile.y + 1, EdgeAxis::Horizontal};
  case GridSide::West:
    return {tile.floor, tile.x, tile.y, EdgeAxis::Vertical};
  }
  return {};
}

const ConstructionObjectDefinition &
objectDefinition(ConstructionObjectKind kind) noexcept {
  const auto index = static_cast<std::size_t>(kind);
  const auto &all = definitions();
  if (index < all.size() && all[index].kind == kind)
    return all[index];
  static const ConstructionObjectDefinition invalid{};
  return invalid;
}

std::vector<Position> footprintTiles(const ConstructionObject &object) {
  const auto &definition = objectDefinition(object.kind);
  if (definition.width <= 0 || definition.height <= 0)
    return {};

  const int turn = ((object.quarterTurns % 4) + 4) % 4;
  std::vector<Position> result;
  result.reserve(static_cast<std::size_t>(definition.width * definition.height));
  for (int y = 0; y < definition.height; ++y)
    for (int x = 0; x < definition.width; ++x)
      result.push_back(orientedOffset(definition, object.anchor, {0, x, y},
                                      turn));
  sortPositions(result);
  return result;
}

std::vector<Position> interactionNodes(const ConstructionObject &object) {
  const auto &definition = objectDefinition(object.kind);
  std::vector<Position> result;
  result.reserve(definition.interactionOffsets.size());
  for (const Position offset : definition.interactionOffsets)
    result.push_back(orientedOffset(definition, object.anchor, offset,
                                    object.quarterTurns));
  sortPositions(result);
  return result;
}

ConstructionWorld::ConstructionWorld(int width, int height, int floors)
    : width_(width), height_(height), floors_(floors), dirtyFloors_(floors, 0) {
  if (width <= 0 || height <= 0 || floors <= 0)
    throw std::invalid_argument("construction world dimensions must be positive");
  const auto count = static_cast<std::size_t>(width) *
                     static_cast<std::size_t>(height) *
                     static_cast<std::size_t>(floors);
  tiles_.resize(count, TileKind::Empty);
  objectOccupancy_.resize(count, 0);
}

bool ConstructionWorld::contains(Position position) const noexcept {
  return position.floor >= 0 && position.floor < floors_ && position.x >= 0 &&
         position.x < width_ && position.y >= 0 && position.y < height_;
}

TileKind ConstructionWorld::tileAt(Position position) const noexcept {
  if (!contains(position))
    return TileKind::Empty;
  const auto index = (static_cast<std::size_t>(position.floor) * height_ +
                      static_cast<std::size_t>(position.y)) *
                         width_ +
                     static_cast<std::size_t>(position.x);
  return tiles_[index];
}

bool ConstructionWorld::occupiedByObject(Position position) const noexcept {
  if (!contains(position))
    return false;
  const auto index = (static_cast<std::size_t>(position.floor) * height_ +
                      static_cast<std::size_t>(position.y)) *
                         width_ +
                     static_cast<std::size_t>(position.x);
  return objectOccupancy_[index] != 0;
}

bool ConstructionWorld::validEdge(GridEdge edge) const noexcept {
  if (edge.floor < 0 || edge.floor >= floors_)
    return false;
  switch (edge.axis) {
  case EdgeAxis::Vertical:
    return edge.x >= 0 && edge.x <= width_ && edge.y >= 0 &&
           edge.y < height_;
  case EdgeAxis::Horizontal:
    return edge.x >= 0 && edge.x < width_ && edge.y >= 0 &&
           edge.y <= height_;
  }
  return false;
}

ConstructionResult ConstructionWorld::setTile(Position position,
                                                TileKind kind) {
  if (!contains(position))
    return failure("Construction tile is outside the property");
  if (kind == TileKind::Wall || kind == TileKind::Door)
    return failure("Walls and doors are placed on edges");

  const TileKind oldKind = tileAt(position);
  if (oldKind == kind)
    return success();
  if (occupiedByObject(position))
    return failure("Construction tile overlaps a placed object");

  const auto index = (static_cast<std::size_t>(position.floor) * height_ +
                      static_cast<std::size_t>(position.y)) *
                         width_ +
                     static_cast<std::size_t>(position.x);
  tiles_[index] = kind;
  dirtyFloors_[static_cast<std::size_t>(position.floor)] = 1;
  return success();
}

ConstructionResult ConstructionWorld::setWall(GridEdge edge, bool enabled) {
  if (!validEdge(edge))
    return failure("Construction edge is outside the property");
  if (enabled) {
    if (edgeIsIn(doors_, edge))
      return failure("A door already occupies this edge");
    if (!edgeIsIn(walls_, edge)) {
      insertEdge(walls_, edge);
      dirtyFloors_[static_cast<std::size_t>(edge.floor)] = 1;
    }
  } else if (eraseEdge(walls_, edge)) {
    dirtyFloors_[static_cast<std::size_t>(edge.floor)] = 1;
  }
  return success();
}

ConstructionResult ConstructionWorld::setDoor(GridEdge edge, bool enabled) {
  if (!validEdge(edge))
    return failure("Construction edge is outside the property");
  if (enabled) {
    if (edgeIsIn(doors_, edge))
      return success();
    if (!eraseEdge(walls_, edge))
      return failure("Place a wall before adding a door");
    insertEdge(doors_, edge);
    dirtyFloors_[static_cast<std::size_t>(edge.floor)] = 1;
  } else if (eraseEdge(doors_, edge)) {
    insertEdge(walls_, edge);
    dirtyFloors_[static_cast<std::size_t>(edge.floor)] = 1;
  }
  return success();
}

ConstructionResult ConstructionWorld::removeEdge(GridEdge edge) {
  if (!validEdge(edge))
    return failure("Construction edge is outside the property");
  const bool removedWall = eraseEdge(walls_, edge);
  const bool removedDoor = eraseEdge(doors_, edge);
  if (removedWall || removedDoor)
    dirtyFloors_[static_cast<std::size_t>(edge.floor)] = 1;
  return success();
}

ConstructionResult ConstructionWorld::placeObject(ConstructionObject object) {
  if (object.id == 0)
    return failure("Construction object needs a stable ID");
  if (std::any_of(objects_.begin(), objects_.end(), [&](const auto &placed) {
        return placed.id == object.id;
      }))
    return failure("Construction object ID already exists");

  const auto &definition = objectDefinition(object.kind);
  if (definition.width <= 0 || definition.height <= 0)
    return failure("Construction object kind is invalid");
  object.quarterTurns = ((object.quarterTurns % 4) + 4) % 4;
  if (object.legacyBaseline) {
    if (!contains(object.anchor) || !roomFloorTile(tileAt(object.anchor)))
      return failure("Legacy baseline object anchor must be on a room floor");
    objects_.push_back(object);
    std::sort(objects_.begin(), objects_.end(),
              [](const auto &lhs, const auto &rhs) {
                return lhs.id < rhs.id;
              });
    return success(object.id);
  }
  const auto footprint = footprintTiles(object);
  if (footprint.empty())
    return failure("Construction object footprint is empty");
  for (const Position tile : footprint) {
    if (!contains(tile))
      return failure("Object footprint is outside the property");
    if (!roomFloorTile(tileAt(tile)))
      return failure("Object footprint must be placed on floor tiles");
    if (occupiedByObject(tile))
      return failure("Object footprint overlaps existing construction");
  }
  for (const Position tile : footprint) {
    const auto index = (static_cast<std::size_t>(tile.floor) * height_ +
                        static_cast<std::size_t>(tile.y)) *
                           width_ +
                       static_cast<std::size_t>(tile.x);
    objectOccupancy_[index] = object.id;
  }
  objects_.push_back(object);
  std::sort(objects_.begin(), objects_.end(),
            [](const auto &lhs, const auto &rhs) { return lhs.id < rhs.id; });
  return success(object.id);
}

ConstructionResult ConstructionWorld::removeObject(EntityId id) {
  const auto it = std::find_if(objects_.begin(), objects_.end(),
                               [id](const auto &object) {
                                 return object.id == id;
                               });
  if (it == objects_.end())
    return failure("Construction object does not exist");
  if (!it->legacyBaseline)
    for (const Position tile : footprintTiles(*it)) {
      const auto index = (static_cast<std::size_t>(tile.floor) * height_ +
                          static_cast<std::size_t>(tile.y)) *
                             width_ +
                         static_cast<std::size_t>(tile.x);
      objectOccupancy_[index] = 0;
    }
  objects_.erase(it);
  return success(id);
}

void ConstructionWorld::rebuildRegions(std::function<RoomId()> allocateRoomId) {
  if (std::none_of(dirtyFloors_.begin(), dirtyFloors_.end(),
                   [](std::uint8_t dirty) { return dirty != 0; }))
    return;

  std::vector<ConstructionRegion> rebuilt;
  for (const auto &region : regions_)
    if (region.tiles.empty() ||
        !dirtyFloors_[static_cast<std::size_t>(region.tiles.front().floor)])
      rebuilt.push_back(region);

  const std::array<GridSide, 4> sides{GridSide::North, GridSide::East,
                                      GridSide::South, GridSide::West};
  for (int floor = 0; floor < floors_; ++floor) {
    if (!dirtyFloors_[static_cast<std::size_t>(floor)])
      continue;

    std::vector<ConstructionRegion> oldRegions;
    for (const auto &region : regions_)
      if (!region.tiles.empty() && region.tiles.front().floor == floor)
        oldRegions.push_back(region);

    const auto planeSize = static_cast<std::size_t>(width_) * height_;
    std::vector<int> componentAt(planeSize, -1);
    std::vector<ConstructionRegion> candidates;
    int nextComponentId = 0;
    for (int y = 0; y < height_; ++y) {
      for (int x = 0; x < width_; ++x) {
        const Position start{floor, x, y};
        const auto startIndex = static_cast<std::size_t>(y) * width_ + x;
        if (componentAt[startIndex] != -1 || !roomFloorTile(tileAt(start)))
          continue;

        const int componentId = nextComponentId++;
        std::deque<Position> queue{start};
        componentAt[startIndex] = componentId;
        ConstructionRegion candidate;
        bool enclosed = true;
        while (!queue.empty()) {
          const Position tile = queue.front();
          queue.pop_front();
          candidate.tiles.push_back(tile);
          for (const GridSide side : sides) {
            const GridEdge edge = edgeForSide(tile, side);
            Position neighbor = tile;
            switch (side) {
            case GridSide::North:
              --neighbor.y;
              break;
            case GridSide::East:
              ++neighbor.x;
              break;
            case GridSide::South:
              ++neighbor.y;
              break;
            case GridSide::West:
              --neighbor.x;
              break;
            }

            const bool blocked = edgeIsIn(walls_, edge) ||
                                 edgeIsIn(doors_, edge);
            if (contains(neighbor)) {
              const auto neighborIndex =
                  static_cast<std::size_t>(neighbor.y) * width_ + neighbor.x;
              if (!blocked && roomFloorTile(tileAt(neighbor)) &&
                  componentAt[neighborIndex] == -1) {
                componentAt[neighborIndex] = componentId;
                queue.push_back(neighbor);
              }
            }
          }
        }

        sortPositions(candidate.tiles);
        for (const Position tile : candidate.tiles) {
          for (const GridSide side : sides) {
            const GridEdge edge = edgeForSide(tile, side);
            Position neighbor = tile;
            switch (side) {
            case GridSide::North:
              --neighbor.y;
              break;
            case GridSide::East:
              ++neighbor.x;
              break;
            case GridSide::South:
              ++neighbor.y;
              break;
            case GridSide::West:
              --neighbor.x;
              break;
            }
            if (contains(neighbor)) {
              const auto neighborIndex =
                  static_cast<std::size_t>(neighbor.y) * width_ + neighbor.x;
              if (componentAt[neighborIndex] == componentId)
                continue;
            }

            candidate.boundaryEdges.push_back(edge);
            if (edgeIsIn(doors_, edge))
              candidate.doorEntrances.push_back(edge);
            if (!edgeIsIn(walls_, edge) && !edgeIsIn(doors_, edge))
              enclosed = false;
          }
        }
        std::sort(candidate.boundaryEdges.begin(),
                  candidate.boundaryEdges.end());
        std::sort(candidate.doorEntrances.begin(),
                  candidate.doorEntrances.end());
        candidate.boundaryEdges.erase(
            std::unique(candidate.boundaryEdges.begin(),
                        candidate.boundaryEdges.end()),
            candidate.boundaryEdges.end());
        candidate.doorEntrances.erase(
            std::unique(candidate.doorEntrances.begin(),
                        candidate.doorEntrances.end()),
            candidate.doorEntrances.end());
        if (enclosed)
          candidates.push_back(std::move(candidate));
      }
    }

    std::sort(candidates.begin(), candidates.end(), regionFirstTileLess);
    std::vector<std::vector<RoomId>> proposals(candidates.size());
    for (const auto &oldRegion : oldRegions) {
      std::size_t bestIndex = candidates.size();
      std::size_t bestOverlap = 0;
      for (std::size_t candidateIndex = 0;
           candidateIndex < candidates.size(); ++candidateIndex) {
        std::size_t overlap = 0;
        for (const Position tile : oldRegion.tiles)
          overlap += containsPosition(candidates[candidateIndex].tiles, tile);
        if (overlap > bestOverlap ||
            (overlap == bestOverlap && overlap > 0 &&
             (bestIndex == candidates.size() ||
              regionFirstTileLess(candidates[candidateIndex],
                                  candidates[bestIndex])))) {
          bestIndex = candidateIndex;
          bestOverlap = overlap;
        }
      }
      if (bestIndex != candidates.size())
        proposals[bestIndex].push_back(oldRegion.id);
    }

    for (std::size_t candidateIndex = 0;
         candidateIndex < candidates.size(); ++candidateIndex) {
      auto &ids = proposals[candidateIndex];
      if (ids.empty()) {
        if (!allocateRoomId)
          throw std::invalid_argument("room ID allocator is required");
        candidates[candidateIndex].id = allocateRoomId();
      } else {
        candidates[candidateIndex].id =
            *std::min_element(ids.begin(), ids.end());
      }
      rebuilt.push_back(std::move(candidates[candidateIndex]));
    }
  }

  std::sort(rebuilt.begin(), rebuilt.end(), regionFirstTileLess);
  regions_ = std::move(rebuilt);
  std::fill(dirtyFloors_.begin(), dirtyFloors_.end(), 0);
}

} // namespace hh::game
