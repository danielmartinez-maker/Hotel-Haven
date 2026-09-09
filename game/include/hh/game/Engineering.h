#pragma once

#include "hh/game/Logistics.h"
#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <random>
#include <vector>

namespace hh::game {

enum class WorkOrderType : std::uint8_t { Preventive, Corrective };
enum class WorkOrderStage : std::uint8_t { Queued, Working, Completed };

struct EngineeringAssetView {
  AssetId id{};
  int condition{};
  int failurePressure{};
  bool failed{};
};

struct WorkOrderView {
  WorkOrderId id{};
  AssetId assetId{};
  WorkOrderType type{WorkOrderType::Corrective};
  WorkOrderStage stage{WorkOrderStage::Queued};
  int remainingSeconds{};
  BlockReason blockedReason{BlockReason::None};
};

struct EngineeringSnapshot {
  std::int64_t elapsedSeconds{};
  int failures{};
  std::vector<EngineeringAssetView> assets;
  std::vector<WorkOrderView> workOrders;
};

class EngineeringSystem {
public:
  EngineeringSystem(LogisticsSystem &logistics, std::uint64_t seed = 1);
  void registerAsset(AssetId asset, int condition = 10000);
  [[nodiscard]] WorkOrderId createWorkOrder(AssetId asset, WorkOrderType type);
  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] EngineeringSnapshot snapshot() const;

private:
  friend class ServiceLogisticsRuntime;

  struct Asset {
    AssetId id{};
    int condition{10000};
    int failurePressure{};
    bool failed{};
  };
  struct WorkOrder {
    WorkOrderId id{};
    AssetId assetId{};
    WorkOrderType type{WorkOrderType::Corrective};
    WorkOrderStage stage{WorkOrderStage::Queued};
    int remainingSeconds{};
    BlockReason blockedReason{BlockReason::None};
    bool partClaimed{};
  };
  [[nodiscard]] Asset *asset(AssetId id);
  [[nodiscard]] const Asset *asset(AssetId id) const;

  LogisticsSystem *logistics_{};
  std::mt19937_64 rng_;
  ServiceId nextId_{1};
  std::int64_t elapsedSeconds_{};
  int failures_{};
  std::vector<Asset> assets_;
  std::vector<WorkOrder> workOrders_;
};

} // namespace hh::game
