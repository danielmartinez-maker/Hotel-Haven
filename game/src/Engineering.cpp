#include "hh/game/Engineering.h"
#include <algorithm>

namespace hh::game {

EngineeringSystem::EngineeringSystem(LogisticsSystem &logistics, std::uint64_t seed)
    : logistics_(&logistics), rng_(seed) {}

void EngineeringSystem::registerAsset(AssetId id, int condition) {
  if (id == 0 || asset(id))
    return;
  const auto index = assets_.size();
  assets_.push_back({id, std::clamp(condition, 0, 10000), 0, false});
  assetIndex_.emplace(id, index);
}

EngineeringSystem::Asset *EngineeringSystem::asset(AssetId id) {
  const auto found = assetIndex_.find(id);
  return found == assetIndex_.end() ? nullptr : &assets_[found->second];
}

const EngineeringSystem::Asset *EngineeringSystem::asset(AssetId id) const {
  const auto found = assetIndex_.find(id);
  return found == assetIndex_.end() ? nullptr : &assets_[found->second];
}

WorkOrderId EngineeringSystem::createWorkOrder(AssetId assetId,
                                               WorkOrderType type) {
  if (!asset(assetId))
    return 0;
  for (const auto index : activeWorkOrders_) {
    const auto &order = workOrders_[index];
    if (order.assetId == assetId && order.type == type)
      return order.id;
  }
  const auto id = nextId_++;
  const int work = type == WorkOrderType::Preventive ? 15 * 60 : 25 * 60;
  const auto index = workOrders_.size();
  workOrders_.push_back({id, assetId, type, WorkOrderStage::Queued, work,
                         BlockReason::None, false});
  activeWorkOrders_.push_back(index);
  return id;
}

std::optional<WorkOrderStage>
EngineeringSystem::latestWorkOrderStage(AssetId assetId,
                                        WorkOrderType type) const {
  for (auto it = workOrders_.rbegin(); it != workOrders_.rend(); ++it)
    if (it->assetId == assetId && it->type == type)
      return it->stage;
  return std::nullopt;
}

void EngineeringSystem::tickWorkOrderSecond(WorkOrder &order) {
  if (order.stage == WorkOrderStage::Completed)
    return;
  if (!order.partClaimed) {
    if (!logistics_->consumeUsable("maintenance_part", 1)) {
      order.blockedReason = BlockReason::AwaitingPart;
      return;
    }
    order.partClaimed = true;
    order.stage = WorkOrderStage::Working;
    order.blockedReason = BlockReason::None;
  }
  if (order.remainingSeconds > 0)
    --order.remainingSeconds;
  if (order.remainingSeconds > 0)
    return;
  auto *target = asset(order.assetId);
  if (target) {
    if (order.type == WorkOrderType::Preventive) {
      target->condition = std::min(10000, target->condition + 3000);
      target->failurePressure /= 4;
    } else {
      target->condition = std::max(target->condition, 8000);
      target->failurePressure /= 2;
      target->failed = false;
    }
  }
  order.stage = WorkOrderStage::Completed;
  order.blockedReason = BlockReason::None;
}

void EngineeringSystem::tickReliabilitySecond() {
  if (elapsedSeconds_ % 3600 != 0)
    return;
  for (auto &entry : assets_) {
    entry.condition = std::max(0, entry.condition - 10);
    const int pressureGain = std::max(1, (7000 - entry.condition) / 8);
    entry.failurePressure =
        std::clamp(entry.failurePressure + pressureGain, 0, 9500);
    const auto draw = static_cast<int>(rng_() % 10000ULL);
    if (draw < entry.failurePressure) {
      ++failures_;
      entry.failed = true;
    }
  }
}

void EngineeringSystem::rebuildActiveWorkOrders() {
  assetIndex_.clear();
  assetIndex_.reserve(assets_.size());
  for (std::size_t index = 0; index < assets_.size(); ++index)
    assetIndex_.emplace(assets_[index].id, index);
  activeWorkOrders_.clear();
  activeWorkOrders_.reserve(workOrders_.size());
  for (std::size_t index = 0; index < workOrders_.size(); ++index)
    if (workOrders_[index].stage != WorkOrderStage::Completed)
      activeWorkOrders_.push_back(index);
}

void EngineeringSystem::tickSecond() {
  ++elapsedSeconds_;
  for (const auto index : activeWorkOrders_)
    tickWorkOrderSecond(workOrders_[index]);
  activeWorkOrders_.erase(
      std::remove_if(activeWorkOrders_.begin(), activeWorkOrders_.end(),
                     [&](std::size_t index) {
                       return workOrders_[index].stage ==
                              WorkOrderStage::Completed;
                     }),
      activeWorkOrders_.end());
  tickReliabilitySecond();
}

void EngineeringSystem::tickSecondFor(
    const std::vector<AssetId> &managedAssets,
    const std::vector<AssetId> &workingAssets) {
  ++elapsedSeconds_;
  const bool managedSorted =
      std::is_sorted(managedAssets.begin(), managedAssets.end());
  const bool workingSorted =
      std::is_sorted(workingAssets.begin(), workingAssets.end());
  for (const auto index : activeWorkOrders_) {
    auto &order = workOrders_[index];
    const bool managed =
        managedSorted
            ? std::binary_search(managedAssets.begin(), managedAssets.end(),
                                 order.assetId)
            : std::find(managedAssets.begin(), managedAssets.end(),
                        order.assetId) != managedAssets.end();
    const bool working =
        workingSorted
            ? std::binary_search(workingAssets.begin(), workingAssets.end(),
                                 order.assetId)
            : std::find(workingAssets.begin(), workingAssets.end(),
                        order.assetId) != workingAssets.end();
    if (!managed || working)
      tickWorkOrderSecond(order);
  }
  activeWorkOrders_.erase(
      std::remove_if(activeWorkOrders_.begin(), activeWorkOrders_.end(),
                     [&](std::size_t index) {
                       return workOrders_[index].stage ==
                              WorkOrderStage::Completed;
                     }),
      activeWorkOrders_.end());
  // Integrated Simulation owns room wear/failure generation. Standalone
  // EngineeringSystem::tickSecond() retains FINAL-04 reliability behavior.
}

void EngineeringSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

EngineeringSnapshot EngineeringSystem::snapshot(bool includeHistory) const {
  EngineeringSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  out.failures = failures_;
  out.assets.reserve(assets_.size());
  out.workOrders.reserve(includeHistory ? workOrders_.size()
                                        : activeWorkOrders_.size());
  for (const auto &entry : assets_)
    out.assets.push_back({entry.id, entry.condition, entry.failurePressure,
                          entry.failed});
  if (includeHistory) {
    for (const auto &order : workOrders_)
      out.workOrders.push_back({order.id, order.assetId, order.type, order.stage,
                                order.remainingSeconds, order.blockedReason});
  } else {
    for (const auto index : activeWorkOrders_) {
      const auto &order = workOrders_[index];
      out.workOrders.push_back({order.id, order.assetId, order.type, order.stage,
                                order.remainingSeconds, order.blockedReason});
    }
  }
  return out;
}

} // namespace hh::game
