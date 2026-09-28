#include "hh/game/Engineering.h"
#include <algorithm>

namespace hh::game {

EngineeringSystem::EngineeringSystem(LogisticsSystem &logistics, std::uint64_t seed)
    : logistics_(&logistics), rng_(seed) {}

void EngineeringSystem::registerAsset(AssetId id, int condition) {
  if (id == 0 || asset(id))
    return;
  assets_.push_back({id, std::clamp(condition, 0, 10000), 0, false});
}

EngineeringSystem::Asset *EngineeringSystem::asset(AssetId id) {
  for (auto &entry : assets_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

const EngineeringSystem::Asset *EngineeringSystem::asset(AssetId id) const {
  for (const auto &entry : assets_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

WorkOrderId EngineeringSystem::createWorkOrder(AssetId assetId,
                                               WorkOrderType type) {
  if (!asset(assetId))
    return 0;
  for (const auto &order : workOrders_)
    if (order.assetId == assetId && order.type == type &&
        order.stage != WorkOrderStage::Completed)
      return order.id;
  const auto id = nextId_++;
  const int work = type == WorkOrderType::Preventive ? 15 * 60 : 25 * 60;
  workOrders_.push_back({id, assetId, type, WorkOrderStage::Queued, work,
                         BlockReason::None, false});
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
      target->failurePressure = 0;
      target->failed = false;
    }
  }
  order.stage = WorkOrderStage::Completed;
  order.blockedReason = BlockReason::None;
}

void EngineeringSystem::tickReliabilitySecond(int conditionLossPerHour,
                                             bool generateFailures) {
  if (elapsedSeconds_ % 3600 != 0 || conditionLossPerHour <= 0)
    return;
  for (auto &entry : assets_) {
    entry.condition = std::max(0, entry.condition - conditionLossPerHour);
    if (!generateFailures)
      continue;
    const int pressureGain = std::max(1, (7000 - entry.condition) / 8);
    entry.failurePressure =
        std::clamp(entry.failurePressure + pressureGain, 0, 9500);
    const auto draw = static_cast<int>(rng_() % 10000ULL);
    if (!entry.failed && draw < entry.failurePressure) {
      ++failures_;
      entry.failed = true;
    }
  }
}

void EngineeringSystem::tickSecond() {
  ++elapsedSeconds_;
  for (auto &order : workOrders_)
    tickWorkOrderSecond(order);
  tickReliabilitySecond();
}

void EngineeringSystem::tickSecondFor(
    const std::vector<AssetId> &managedAssets,
    const std::vector<AssetId> &workingAssets,
    int conditionLossPerHour) {
  ++elapsedSeconds_;
  for (auto &order : workOrders_) {
    const bool managed =
        std::find(managedAssets.begin(), managedAssets.end(), order.assetId) !=
        managedAssets.end();
    const bool working =
        std::find(workingAssets.begin(), workingAssets.end(), order.assetId) !=
        workingAssets.end();
    if (!managed || working)
      tickWorkOrderSecond(order);
  }
  // Reliability/condition remains authoritative in FINAL-04 for both
  // standalone and integrated execution. Simulation supplies labor gating for
  // work orders, then mirrors this hourly state into physical RoomView state.
  tickReliabilitySecond(conditionLossPerHour, false);
}

void EngineeringSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

EngineeringSnapshot EngineeringSystem::snapshot() const {
  EngineeringSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  out.failures = failures_;
  for (const auto &entry : assets_)
    out.assets.push_back({entry.id, entry.condition, entry.failurePressure,
                          entry.failed});
  for (const auto &order : workOrders_)
    out.workOrders.push_back({order.id, order.assetId, order.type, order.stage,
                              order.remainingSeconds, order.blockedReason});
  return out;
}

} // namespace hh::game
