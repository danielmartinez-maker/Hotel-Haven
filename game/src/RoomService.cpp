#include "hh/game/RoomService.h"

namespace hh::game {

RoomServiceSystem::ActiveOrder *RoomServiceSystem::order(RoomServiceOrderId id) {
  for (auto &entry : orders_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

const RoomServiceSystem::ActiveOrder *RoomServiceSystem::order(RoomServiceOrderId id) const {
  for (const auto &entry : orders_)
    if (entry.id == id)
      return &entry;
  return nullptr;
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
  orders_.push_back(entry);
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
  for (auto &entry : orders_) {
    if (entry.stage == RoomServiceStage::Completed)
      continue;
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
}

void RoomServiceSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

RoomServiceSnapshot RoomServiceSystem::snapshot() const {
  RoomServiceSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  for (const auto &entry : orders_)
    out.orders.push_back({entry.id, entry.guestId, entry.stage,
                          entry.remainingSeconds, entry.ageSeconds,
                          entry.promisedSeconds, entry.blockedReason});
  return out;
}

} // namespace hh::game
