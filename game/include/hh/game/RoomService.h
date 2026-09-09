#pragma once

#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <vector>

namespace hh::game {

struct RoomServiceOrder {
  int itemCount{1};
  int promisedSeconds{35 * 60};
};

enum class RoomServiceStage : std::uint8_t {
  AwaitingProduction,
  RunnerPickup,
  InTransit,
  GuestHandoff,
  AwaitingTrayPickup,
  TrayReturn,
  Completed
};

struct RoomServiceOrderView {
  RoomServiceOrderId id{};
  GuestId guestId{};
  RoomServiceStage stage{RoomServiceStage::AwaitingProduction};
  int remainingSeconds{};
  int ageSeconds{};
  int promisedSeconds{};
  BlockReason blockedReason{BlockReason::None};
};

struct RoomServiceSnapshot {
  std::int64_t elapsedSeconds{};
  std::vector<RoomServiceOrderView> orders;
};

class RoomServiceSystem {
public:
  [[nodiscard]] RoomServiceOrderId placeRoomServiceOrder(
      GuestId guest, const RoomServiceOrder &order);
  [[nodiscard]] bool markProductionReady(RoomServiceOrderId id);
  [[nodiscard]] bool requestTrayPickup(RoomServiceOrderId id);
  [[nodiscard]] RoomServiceStage stage(RoomServiceOrderId id) const;
  [[nodiscard]] std::vector<RoomServiceStage> history(RoomServiceOrderId id) const;
  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] RoomServiceSnapshot snapshot() const;

private:
  struct ActiveOrder {
    RoomServiceOrderId id{};
    GuestId guestId{};
    RoomServiceStage stage{RoomServiceStage::AwaitingProduction};
    int remainingSeconds{};
    int ageSeconds{};
    int promisedSeconds{};
    BlockReason blockedReason{BlockReason::AwaitingProduction};
    std::vector<RoomServiceStage> stageHistory;
  };
  [[nodiscard]] ActiveOrder *order(RoomServiceOrderId id);
  [[nodiscard]] const ActiveOrder *order(RoomServiceOrderId id) const;
  void enter(ActiveOrder &order, RoomServiceStage stage, int seconds,
             BlockReason blocked = BlockReason::None);

  ServiceId nextId_{1};
  std::int64_t elapsedSeconds_{};
  std::vector<ActiveOrder> orders_;
};

} // namespace hh::game
