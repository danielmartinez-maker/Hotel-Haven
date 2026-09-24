#include "hh/game/Housekeeping.h"
#include <algorithm>

namespace hh::game {

HousekeepingSystem::HousekeepingSystem(LogisticsSystem &logistics)
    : logistics_(&logistics) {}

void HousekeepingSystem::registerRoom(RoomId id, ServiceRoomStatus status) {
  if (id == 0 || room(id))
    return;
  const auto index = rooms_.size();
  rooms_.push_back({id, status});
  roomIndex_.emplace(id, index);
}

HousekeepingSystem::RoomState *HousekeepingSystem::room(RoomId id) {
  const auto found = roomIndex_.find(id);
  return found == roomIndex_.end() ? nullptr : &rooms_[found->second];
}

const HousekeepingSystem::RoomState *HousekeepingSystem::room(RoomId id) const {
  const auto found = roomIndex_.find(id);
  return found == roomIndex_.end() ? nullptr : &rooms_[found->second];
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
  for (const auto index : activeJobs_)
    if (jobs_[index].roomId == roomId)
      return jobs_[index].id;
  roomState->status = ServiceRoomStatus::Dirty;
  const auto id = nextId_++;
  const auto index = jobs_.size();
  jobs_.push_back({id, roomId, HousekeepingStage::StripLinen,
                   duration(HousekeepingStage::StripLinen), BlockReason::None,
                   false, false});
  activeJobs_.push_back(index);
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
    if (!job.suppliesPreclaimed &&
        !logistics_->consumeUsable("cleaning_chemical", 1)) {
      job.blockedReason = BlockReason::MissingChemicals;
      return false;
    }
    break;
  case HousekeepingStage::ReplaceLinen:
    if (job.suppliesPreclaimed)
      break;
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
    if (!job.suppliesPreclaimed &&
        !logistics_->consumeUsable("amenity_kit", 1)) {
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

void HousekeepingSystem::tickJobSecond(Job &job) {
  if (job.stage == HousekeepingStage::Completed)
    return;
  if (!job.stageStarted && !beginStage(job)) {
    if (auto *state = room(job.roomId))
      state->status = ServiceRoomStatus::Blocked;
    return;
  }
  if (job.remainingSeconds > 0)
    --job.remainingSeconds;
  if (job.remainingSeconds == 0)
    completeStage(job);
}

void HousekeepingSystem::rebuildActiveJobs() {
  roomIndex_.clear();
  roomIndex_.reserve(rooms_.size());
  for (std::size_t index = 0; index < rooms_.size(); ++index)
    roomIndex_.emplace(rooms_[index].id, index);
  activeJobs_.clear();
  activeJobs_.reserve(jobs_.size());
  for (std::size_t index = 0; index < jobs_.size(); ++index)
    if (jobs_[index].stage != HousekeepingStage::Completed)
      activeJobs_.push_back(index);
}

void HousekeepingSystem::tickSecond() {
  ++elapsedSeconds_;
  for (const auto index : activeJobs_)
    tickJobSecond(jobs_[index]);
  activeJobs_.erase(
      std::remove_if(activeJobs_.begin(), activeJobs_.end(),
                     [&](std::size_t index) {
                       return jobs_[index].stage == HousekeepingStage::Completed;
                     }),
      activeJobs_.end());
}

void HousekeepingSystem::tickSecondFor(
    const std::vector<RoomId> &managedRooms,
    const std::vector<RoomId> &workingRooms) {
  ++elapsedSeconds_;
  const bool managedSorted =
      std::is_sorted(managedRooms.begin(), managedRooms.end());
  const bool workingSorted =
      std::is_sorted(workingRooms.begin(), workingRooms.end());
  for (const auto index : activeJobs_) {
    auto &job = jobs_[index];
    const bool managed =
        managedSorted
            ? std::binary_search(managedRooms.begin(), managedRooms.end(),
                                 job.roomId)
            : std::find(managedRooms.begin(), managedRooms.end(), job.roomId) !=
                  managedRooms.end();
    const bool working =
        workingSorted
            ? std::binary_search(workingRooms.begin(), workingRooms.end(),
                                 job.roomId)
            : std::find(workingRooms.begin(), workingRooms.end(), job.roomId) !=
                  workingRooms.end();
    if (!managed || working)
      tickJobSecond(job);
  }
  activeJobs_.erase(
      std::remove_if(activeJobs_.begin(), activeJobs_.end(),
                     [&](std::size_t index) {
                       return jobs_[index].stage == HousekeepingStage::Completed;
                     }),
      activeJobs_.end());
}

void HousekeepingSystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

HousekeepingSnapshot HousekeepingSystem::snapshot(bool includeHistory) const {
  HousekeepingSnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  if (includeHistory) {
    out.jobs.reserve(jobs_.size());
    for (const auto &job : jobs_)
      out.jobs.push_back({job.id, job.roomId, job.stage, job.remainingSeconds,
                          job.blockedReason});
  } else {
    out.jobs.reserve(activeJobs_.size());
    for (const auto index : activeJobs_) {
      const auto &job = jobs_[index];
      out.jobs.push_back({job.id, job.roomId, job.stage, job.remainingSeconds,
                          job.blockedReason});
    }
  }
  return out;
}

} // namespace hh::game
