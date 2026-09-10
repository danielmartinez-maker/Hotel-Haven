#pragma once

#include "hh/game/Logistics.h"
#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <vector>

namespace hh::game {

enum class ServiceRoomStatus : std::uint8_t { Ready, Dirty, Turning, Blocked };
enum class HousekeepingStage : std::uint8_t {
  StripLinen,
  CollectTrash,
  CleanBathroom,
  CleanSurfacesFloor,
  ReplaceLinen,
  ReplenishAmenities,
  Inspect,
  Completed
};

struct HousekeepingJobView {
  TaskId id{};
  RoomId roomId{};
  HousekeepingStage stage{HousekeepingStage::StripLinen};
  int remainingSeconds{};
  BlockReason blockedReason{BlockReason::None};
};

struct HousekeepingSnapshot {
  std::int64_t elapsedSeconds{};
  std::vector<HousekeepingJobView> jobs;
};

class HousekeepingSystem {
public:
  explicit HousekeepingSystem(LogisticsSystem &logistics);
  void registerRoom(RoomId room, ServiceRoomStatus status = ServiceRoomStatus::Ready);
  [[nodiscard]] TaskId requestRoomTurn(RoomId room);
  [[nodiscard]] ServiceRoomStatus roomStatus(RoomId room) const;
  [[nodiscard]] TaskId latestJob(RoomId room) const;
  [[nodiscard]] ServiceWorkResult workSecond(TaskId job);
  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] HousekeepingSnapshot snapshot() const;

private:
  friend class ServiceLogisticsRuntime;

  struct RoomState { RoomId id{}; ServiceRoomStatus status{ServiceRoomStatus::Ready}; };
  struct Job {
    TaskId id{};
    RoomId roomId{};
    HousekeepingStage stage{HousekeepingStage::StripLinen};
    int remainingSeconds{};
    BlockReason blockedReason{BlockReason::None};
    bool stageStarted{};
  };

  [[nodiscard]] RoomState *room(RoomId id);
  [[nodiscard]] const RoomState *room(RoomId id) const;
  [[nodiscard]] Job *job(TaskId id);
  [[nodiscard]] const Job *job(TaskId id) const;
  [[nodiscard]] bool beginStage(Job &job);
  void completeStage(Job &job);
  [[nodiscard]] static int duration(HousekeepingStage stage);

  LogisticsSystem *logistics_{};
  ServiceId nextId_{1};
  std::int64_t elapsedSeconds_{};
  std::vector<RoomState> rooms_;
  std::vector<Job> jobs_;
};

} // namespace hh::game
