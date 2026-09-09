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
  for (const auto &job : jobs_)
    if (job.roomId == roomId && job.stage != HousekeepingStage::Completed)
      return job.id;
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

bool HousekeepingSystem::beginStage(Job &job) {
  job.blockedReason = BlockReason::None;
  switch (job.stage) {
  case HousekeepingStage::StripLinen:
    if (!logistics_->canAddToKind(StorageKind::DirtyLinen, 1)) {
      job.blockedReason = BlockReason::MissingDirtyStorage;
      return false;
    }
    break;
  case HousekeepingStage::CleanBathroom:
    if (!logistics_->consumeUsable("cleaning_chemical", 1)) {
      job.blockedReason = BlockReason::MissingChemicals;
      return false;
    }
    break;
  case HousekeepingStage::ReplaceLinen:
    if (logistics_->inventoryUsable("clean_linen_set") < 1) {
      job.blockedReason = BlockReason::MissingCleanLinen;
      return false;
    }
    if (logistics_->inventoryUsable("towel_unit") < 2) {
      job.blockedReason = BlockReason::MissingTowels;
      return false;
    }
    if (!logistics_->consumeUsable("clean_linen_set", 1)) {
      job.blockedReason = BlockReason::MissingCleanLinen;
      return false;
    }
    if (!logistics_->consumeUsable("towel_unit", 2)) {
      job.blockedReason = BlockReason::MissingTowels;
      return false;
    }
    break;
  case HousekeepingStage::ReplenishAmenities:
    if (!logistics_->consumeUsable("amenity_kit", 1)) {
      job.blockedReason = BlockReason::MissingAmenities;
      return false;
    }
    break;
  default:
    break;
  }
  job.stageStarted = true;
  if (auto *state = room(job.roomId))
    state->status = ServiceRoomStatus::Turning;
  return true;
}

void HousekeepingSystem::completeStage(Job &job) {
  if (job.stage == HousekeepingStage::StripLinen) {
    if (!logistics_->addToKind(StorageKind::DirtyLinen, "dirty_linen_set", 1)) {
      job.blockedReason = BlockReason::MissingDirtyStorage;
      job.stageStarted = false;
      if (auto *state = room(job.roomId))
        state->status = ServiceRoomStatus::Blocked;
      return;
    }
  } else if (job.stage == HousekeepingStage::CollectTrash) {
    logistics_->produceWaste(1);
  }

  if (job.stage == HousekeepingStage::Inspect) {
    job.stage = HousekeepingStage::Completed;
    job.remainingSeconds = 0;
    job.stageStarted = true;
    job.blockedReason = BlockReason::None;
    if (auto *state = room(job.roomId))
      state->status = ServiceRoomStatus::Ready;
    return;
  }

  job.stage = static_cast<HousekeepingStage>(static_cast<unsigned>(job.stage) + 1U);
  job.remainingSeconds = duration(job.stage);
  job.stageStarted = false;
  job.blockedReason = BlockReason::None;
}

void HousekeepingSystem::tickSecond() {
  ++elapsedSeconds_;
  for (auto &job : jobs_) {
    if (job.stage == HousekeepingStage::Completed)
      continue;
    if (!job.stageStarted && !beginStage(job)) {
      if (auto *state = room(job.roomId))
        state->status = ServiceRoomStatus::Blocked;
      continue;
    }
    if (job.remainingSeconds > 0)
      --job.remainingSeconds;
    if (job.remainingSeconds == 0)
      completeStage(job);
  }
}

void HousekeepingSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

HousekeepingSnapshot HousekeepingSystem::snapshot() const {
  HousekeepingSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  for (const auto &job : jobs_)
    out.jobs.push_back({job.id, job.roomId, job.stage, job.remainingSeconds,
                        job.blockedReason});
  return out;
}

} // namespace hh::game
