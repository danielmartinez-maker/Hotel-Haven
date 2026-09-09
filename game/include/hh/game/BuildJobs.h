#pragma once

#include "hh/game/Construction.h"
#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

using BuildJobId = EntityId;

struct ConstructionMaterials {
  int lumber{};
  int drywall{};
  int electrical{};
  int plumbing{};
  int hardware{};
  bool operator==(const ConstructionMaterials &) const = default;
};

enum class BuildJobState {
  WaitingForMaterials,
  ReadyForLabor,
  Building,
  Blocked,
  Completed,
  Cancelled
};

struct BuildPlan {
  ConstructionCommand construction;
  int workSeconds{300};
};

struct BuildQueueResult {
  bool ok{};
  BuildJobId jobId{};
  ConstructionReason error{ConstructionReason::None};
  std::string message;
  explicit operator bool() const noexcept { return ok; }
};

struct BuildJobSnapshot {
  BuildJobId id{};
  BuildJobState state{BuildJobState::WaitingForMaterials};
  ConstructionCommand construction;
  std::int64_t reservedCashCents{};
  ConstructionMaterials reservedMaterials;
  bool materialsConsumed{};
  EntityId taskId{};
  std::string blockedReason;
  bool operator==(const BuildJobSnapshot &) const = default;
};

struct ConstructionSnapshot {
  std::vector<ConstructionObjectView> objects;
  std::vector<BuildJobSnapshot> buildJobs;
  ConstructionMaterials availableMaterials;
  ConstructionMaterials reservedMaterials;
  bool operator==(const ConstructionSnapshot &) const = default;
};

namespace detail {
[[nodiscard]] ConstructionMaterials
constructionMaterialsFor(const ConstructionCommand &command);
[[nodiscard]] bool hasMaterials(const ConstructionMaterials &available,
                                const ConstructionMaterials &required) noexcept;
void addMaterials(ConstructionMaterials &target,
                  const ConstructionMaterials &value) noexcept;
void subtractMaterials(ConstructionMaterials &target,
                       const ConstructionMaterials &value) noexcept;
[[nodiscard]] bool validMaterials(const ConstructionMaterials &value) noexcept;
} // namespace detail

} // namespace hh::game
