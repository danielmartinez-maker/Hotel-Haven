#include "hh/game/Housekeeping.h"

namespace hh::game {

HousekeepingSystem::HousekeepingSystem(LogisticsSystem &logistics)
    : logistics_(&logistics) {}

void HousekeepingSystem::registerRoom(RoomId id, ServiceRoomStatus status) {
  if (id == 0 || room(id))
    return;
  rooms_.push_back({id, status});
}

HousekeepingSystem::RoomState *HousekeepingSystem::room(RoomId id) {
  for (auto &entry : rooms_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

const HousekeepingSystem::RoomState *HousekeepingSystem::room(RoomId id) const {
  for (const auto &entry : rooms_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

HousekeepingSystem::Job *HousekeepingSystem::job(TaskId id) {
  for (auto &entry : jobs_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

const HousekeepingSystem::Job *HousekeepingSystem::job(TaskId id) const {
  for (const auto &entry : jobs_)
    if (entry.id == id)
      return &entry;
  return nullptr;
}

int HousekeepingSystem::duration(HousekeepingStage stage) {
  switch (stage) {
  case HousekeepingStage::StripLinen: return 120;
  case HousekeepingStage::CollectTrash: return 120;
  case HousekeepingStage::CleanBathroom: return 300;
  case HousekeepingStage::CleanSurfacesFloor: return 420;
  case HousekeepingStage::ReplaceLinen: return 180;
  case HousekeepingStage::ReplenishAmenities: return 120;
  case HousekeepingStage::Inspect: return 120;
  case HousekeepingStage::Completed: return 0;
  }
  return 0;
}

TaskId HousekeepingSystem::requestRoomTurn(RoomId roomId) {
  auto *roomState = room(roomId);
  if (!roomState)
    return 0;
  for (const auto &entry : jobs_)
    if (entry.roomId == roomId && entry.stage != HousekeepingStage::Completed)
      return entry.id;
  roomState->status = ServiceRoomStatus::Dirty;
  const auto id = nextId_++;
  jobs_.push_back({id, roomId, HousekeepingStage::StripLinen,
                   duration(HousekeepingStage::StripLinen), BlockReason::None,
                   false});
  return id;
}

ServiceRoomStatus HousekeepingSystem::roomStatus(RoomId id) const {
  const auto *entry = room(id);
  return entry ? entry->status : ServiceRoomStatus::Blocked;
}

TaskId HousekeepingSystem::latestJob(RoomId roomId) const {
  for (auto it = jobs_.rbegin(); it != jobs_.rend(); ++it)
    if (it->roomId == roomId)
      return it->id;
  return 0;
}

bool HousekeepingSystem::beginStage(Job &entry) {
  entry.blockedReason = BlockReason::None;
  switch (entry.stage) {
  case HousekeepingStage::StripLinen:
    if (!logistics_->canAddToKind(StorageKind::DirtyLinen, 1)) {
      entry.blockedReason = BlockReason::MissingDirtyStorage;
      return false;
    }
    break;
  case HousekeepingStage::CleanBathroom:
    if (!logistics_->consumeUsable("cleaning_chemical", 1)) {
      entry.blockedReason = BlockReason::MissingChemicals;
      return false;
    }
    break;
  case HousekeepingStage::ReplaceLinen:
    if (logistics_->inventoryUsable("clean_linen_set") < 1) {
      entry.blockedReason = BlockReason::MissingCleanLinen;
      return false;
    }
    if (logistics_->inventoryUsable("towel_unit") < 2) {
      entry.blockedReason = BlockReason::MissingTowels;
      return false;
    }
    if (!logistics_->consumeUsable("clean_linen_set", 1)) {
      entry.blockedReason = BlockReason::MissingCleanLinen;
      return false;
    }
    if (!logistics_->consumeUsable("towel_unit", 2)) {
      entry.blockedReason = BlockReason::MissingTowels;
      return false;
    }
    break;
  case HousekeepingStage::ReplenishAmenities:
    if (!logistics_->consumeUsable("amenity_kit", 1)) {
      entry.blockedReason = BlockReason::MissingAmenities;
      return false;
    }
    break;
  default:
    break;
  }
  entry.stageStarted = true;
  if (auto *state = room(entry.roomId))
    state->status = ServiceRoomStatus::Turning;
  return true;
}

void HousekeepingSystem::completeStage(Job &entry) {
  if (entry.stage == HousekeepingStage::StripLinen) {
    if (!logistics_->addToKind(StorageKind::DirtyLinen, "dirty_linen_set", 1)) {
      entry.blockedReason = BlockReason::MissingDirtyStorage;
      entry.stageStarted = false;
      if (auto *state = room(entry.roomId))
        state->status = ServiceRoomStatus::Blocked;
      return;
    }
  } else if (entry.stage == HousekeepingStage::CollectTrash) {
    logistics_->produceWaste(1);
  }

  if (entry.stage == HousekeepingStage::Inspect) {
    entry.stage = HousekeepingStage::Completed;
    entry.remainingSeconds = 0;
    entry.stageStarted = true;
    entry.blockedReason = BlockReason::None;
    if (auto *state = room(entry.roomId))
      state->status = ServiceRoomStatus::Ready;
    return;
  }

  entry.stage = static_cast<HousekeepingStage>(
      static_cast<unsigned>(entry.stage) + 1U);
  entry.remainingSeconds = duration(entry.stage);
  entry.stageStarted = false;
  entry.blockedReason = BlockReason::None;
}

ServiceWorkResult HousekeepingSystem::workSecond(TaskId id) {
  auto *entry = job(id);
  if (!entry)
    return {};
  if (entry->stage == HousekeepingStage::Completed)
    return {true, false, true, BlockReason::None};

  if (!entry->stageStarted && !beginStage(*entry)) {
    if (auto *state = room(entry->roomId))
      state->status = ServiceRoomStatus::Blocked;
    return {true, false, false, entry->blockedReason};
  }

  if (entry->remainingSeconds > 0)
    --entry->remainingSeconds;
  if (entry->remainingSeconds == 0)
    completeStage(*entry);

  const bool completed = entry->stage == HousekeepingStage::Completed;
  return {true, true, completed, entry->blockedReason};
}

void HousekeepingSystem::tickSecond() { ++elapsedSeconds_; }

void HousekeepingSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

HousekeepingSnapshot HousekeepingSystem::snapshot() const {
  HousekeepingSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  for (const auto &entry : jobs_)
    out.jobs.push_back({entry.id, entry.roomId, entry.stage,
                        entry.remainingSeconds, entry.blockedReason});
  return out;
}

} // namespace hh::game
