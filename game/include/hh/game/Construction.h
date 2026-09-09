#pragma once

#include "hh/game/Simulation.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class ConstructionSupport { Floor, Wall };

enum class ConstructionReason {
  None,
  OutsideProperty,
  OccupiedFootprint,
  InvalidSupport,
  AccessObstructed,
  RoomOccupied,
  InsufficientCash,
  UnknownType,
  ObjectNotFound,
  ActiveDependency,
  InvalidCommand
};

struct ConstructionPlacement {
  std::string typeId;
  Position origin;
  int rotationQuarterTurns{};
  bool operator==(const ConstructionPlacement &) const = default;
};

struct ConstructionCommand {
  std::vector<ConstructionPlacement> placements;
  std::vector<EntityId> removeObjectIds;
  bool operator==(const ConstructionCommand &) const = default;
};

struct ConstructionObjectView {
  EntityId id{};
  std::string typeId;
  Position origin;
  int width{};
  int height{};
  int rotationQuarterTurns{};
  bool blocksMovement{};
  bool operator==(const ConstructionObjectView &) const = default;
};

struct ConstructionPreview {
  bool valid{};
  ConstructionReason reason{ConstructionReason::None};
  std::int64_t costCents{};
  std::string message;
};

struct ConstructionResult {
  bool ok{};
  ConstructionReason error{ConstructionReason::None};
  std::int64_t costCents{};
  std::string message;
  std::vector<EntityId> objectIds;
  explicit operator bool() const noexcept { return ok; }
};

namespace detail {
struct ConstructionDefinition {
  std::string_view typeId;
  int width{};
  int height{};
  ConstructionSupport support{ConstructionSupport::Floor};
  bool blocksMovement{};
  bool requiresAccess{};
  std::int64_t costCents{};
  // lumber, drywall, electrical, plumbing, hardware
  std::array<int, 5> materialUnits{};
};

[[nodiscard]] const ConstructionDefinition *
constructionDefinition(std::string_view typeId) noexcept;
[[nodiscard]] std::vector<Position>
constructionFootprint(const ConstructionPlacement &placement,
                      const ConstructionDefinition &definition);
[[nodiscard]] int normalizedQuarterTurns(int turns) noexcept;
} // namespace detail

} // namespace hh::game
