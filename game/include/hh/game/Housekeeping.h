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
    bool suppliesPreclaimed{};
  };

  [[nodiscard]] RoomState *room(RoomId id);
  [[nodiscard]] const RoomState *room(RoomId id) const;
  [[nodiscard]] bool beginStage(Job &job);
  void completeStage(Job &job);
  void tickJobSecond(Job &job);
  void rebuildActiveJobs();
  void tickSecondFor(const std::vector<RoomId> &managedRooms,
                     const std::vector<RoomId> &workingRooms);
  [[nodiscard]] static int duration(HousekeepingStage stage);

  LogisticsSystem *logistics_{};
  ServiceId nextId_{1};
  std::int64_t elapsedSeconds_{};
  std::vector<RoomState> rooms_;
  std::vector<Job> jobs_;
  std::vector<std::size_t> activeJobs_;
};

} // namespace hh::game
