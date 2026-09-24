#include "hh/game/RoomService.h"
#include <algorithm>

namespace hh::game {

RoomServiceSystem::ActiveOrder *RoomServiceSystem::order(RoomServiceOrderId id) {
  const auto found = orderIndex_.find(id);
  return found == orderIndex_.end() ? nullptr : &orders_[found->second];
}

const RoomServiceSystem::ActiveOrder *RoomServiceSystem::order(RoomServiceOrderId id) const {
  const auto found = orderIndex_.find(id);
  return found == orderIndex_.end() ? nullptr : &orders_[found->second];
}

void RoomServiceSystem::rebuildDerivedState() {
  orderIndex_.clear();
  activeOrders_.clear();
  orderIndex_.reserve(orders_.size());
  activeOrders_.reserve(orders_.size());
  for (std::size_t index = 0; index < orders_.size(); ++index) {
    orderIndex_.emplace(orders_[index].id, index);
    if (orders_[index].stage != RoomServiceStage::Completed)
      activeOrders_.push_back(index);
  }
}

void RoomServiceSystem::enter(ActiveOrder &entry, RoomServiceStage stage,
                              int seconds, BlockReason blocked) {
  entry.stage = stage;
  entry.remainingSeconds = seconds;
  entry.blockedReason = blocked;
  entry.stageHistory.push_back(stage);
}

RoomServiceOrderId RoomServiceSystem::placeRoomServiceOrder(
    GuestId guest, const RoomServiceOrder &spec) {
  if (guest == 0 || spec.itemCount <= 0 || spec.promisedSeconds <= 0)
    return 0;
  const auto id = nextId_++;
  ActiveOrder entry;
  entry.id = id;
  entry.guestId = guest;
  entry.promisedSeconds = spec.promisedSeconds;
  entry.blockedReason = BlockReason::AwaitingProduction;
  entry.stageHistory.push_back(RoomServiceStage::AwaitingProduction);
  const auto index = orders_.size();
  orders_.push_back(entry);
  orderIndex_.emplace(id, index);
  activeOrders_.push_back(index);
  return id;
}

bool RoomServiceSystem::markProductionReady(RoomServiceOrderId id) {
  auto *entry = order(id);
  if (!entry || entry->stage != RoomServiceStage::AwaitingProduction)
    return false;
  enter(*entry, RoomServiceStage::RunnerPickup, 2 * 60);
  return true;
}

bool RoomServiceSystem::requestTrayPickup(RoomServiceOrderId id) {
  auto *entry = order(id);
  if (!entry || entry->stage != RoomServiceStage::AwaitingTrayPickup)
    return false;
  enter(*entry, RoomServiceStage::TrayReturn, 5 * 60);
  return true;
}

RoomServiceStage RoomServiceSystem::stage(RoomServiceOrderId id) const {
  const auto *entry = order(id);
  return entry ? entry->stage : RoomServiceStage::Completed;
}

std::vector<RoomServiceStage> RoomServiceSystem::history(RoomServiceOrderId id) const {
  const auto *entry = order(id);
  return entry ? entry->stageHistory : std::vector<RoomServiceStage>{};
}

void RoomServiceSystem::tickSecond() {
  ++elapsedSeconds_;
  for (const auto index : activeOrders_) {
    auto &entry = orders_[index];
    ++entry.ageSeconds;
    if (entry.stage == RoomServiceStage::AwaitingProduction ||
        entry.stage == RoomServiceStage::AwaitingTrayPickup)
      continue;
    if (entry.remainingSeconds > 0)
      --entry.remainingSeconds;
    if (entry.remainingSeconds > 0)
      continue;
    switch (entry.stage) {
    case RoomServiceStage::RunnerPickup:
      enter(entry, RoomServiceStage::InTransit, 5 * 60);
      break;
    case RoomServiceStage::InTransit:
      enter(entry, RoomServiceStage::GuestHandoff, 60);
      break;
    case RoomServiceStage::GuestHandoff:
      enter(entry, RoomServiceStage::AwaitingTrayPickup, 0);
      break;
    case RoomServiceStage::TrayReturn:
      enter(entry, RoomServiceStage::Completed, 0);
      break;
    default:
      break;
    }
  }
  activeOrders_.erase(
      std::remove_if(activeOrders_.begin(), activeOrders_.end(),
                     [&](std::size_t index) {
                       return orders_[index].stage == RoomServiceStage::Completed;
                     }),
      activeOrders_.end());
}

void RoomServiceSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

RoomServiceSnapshot RoomServiceSystem::snapshot() const {
  RoomServiceSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  out.orders.reserve(orders_.size());
  for (const auto &entry : orders_)
    out.orders.push_back({entry.id, entry.guestId, entry.stage,
                          entry.remainingSeconds, entry.ageSeconds,
                          entry.promisedSeconds, entry.blockedReason});
  return out;
}

} // namespace hh::game
