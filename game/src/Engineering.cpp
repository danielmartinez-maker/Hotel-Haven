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

EngineeringSystem::WorkOrder *EngineeringSystem::workOrder(WorkOrderId id) {
  for (auto &entry : workOrders_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

const EngineeringSystem::WorkOrder *
EngineeringSystem::workOrder(WorkOrderId id) const {
  for (const auto &entry : workOrders_)
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

WorkOrderId EngineeringSystem::latestWorkOrder(AssetId assetId,
                                               WorkOrderType type) const {
  for (auto it = workOrders_.rbegin(); it != workOrders_.rend(); ++it)
    if (it->assetId == assetId && it->type == type)
      return it->id;
  return 0;
}

ServiceWorkResult EngineeringSystem::workSecond(WorkOrderId id) {
  auto *order = workOrder(id);
  if (!order)
    return {};
  if (order->stage == WorkOrderStage::Completed)
    return {true, false, true, BlockReason::None};

  if (!order->partClaimed) {
    if (!logistics_->consumeUsable("maintenance_part", 1)) {
      order->blockedReason = BlockReason::AwaitingPart;
      return {true, false, false, order->blockedReason};
    }
    order->partClaimed = true;
    order->stage = WorkOrderStage::Working;
    order->blockedReason = BlockReason::None;
  }

  if (order->remainingSeconds > 0)
    --order->remainingSeconds;
  if (order->remainingSeconds > 0)
    return {true, true, false, BlockReason::None};

  auto *target = asset(order->assetId);
  if (target) {
    if (order->type == WorkOrderType::Preventive) {
      target->condition = std::min(10000, target->condition + 3000);
      target->failurePressure /= 4;
    } else {
      target->condition = std::max(target->condition, 8000);
      target->failurePressure /= 2;
      target->failed = false;
    }
  }
  order->stage = WorkOrderStage::Completed;
  order->blockedReason = BlockReason::None;
  return {true, true, true, BlockReason::None};
}

void EngineeringSystem::tickSecond() {
  ++elapsedSeconds_;
  if (elapsedSeconds_ % 3600 != 0)
    return;

  for (auto &entry : assets_) {
    entry.condition = std::max(0, entry.condition - 10);
    const int pressureGain = std::max(1, (7000 - entry.condition) / 8);
    entry.failurePressure =
        std::clamp(entry.failurePressure + pressureGain, 0, 9500);
    if (entry.failed)
      continue;
    const auto draw = static_cast<int>(rng_() % 10000ULL);
    if (draw < entry.failurePressure) {
      ++failures_;
      entry.failed = true;
    }
  }
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
