#pragma once

#include "hh/game/Logistics.h"
#include "hh/game/ServiceTypes.h"
#include <cstdint>
#include <vector>

namespace hh::game {

struct LaundryStations {
  int washers{};
  int dryers{};
  int foldingStations{};
};

enum class LaundryStage : std::uint8_t {
  AwaitingWasher,
  Washing,
  AwaitingDryer,
  Drying,
  AwaitingFold,
  Folding,
  Completed
};

struct LaundryBatchView {
  LaundryBatchId id{};
  int quantity{};
  LaundryStage stage{LaundryStage::AwaitingWasher};
  int remainingSeconds{};
  BlockReason blockedReason{BlockReason::None};
};

struct LaundrySnapshot {
  std::int64_t elapsedSeconds{};
  LaundryStations stations;
  std::vector<LaundryBatchView> batches;
};

class LaundrySystem {
public:
  LaundrySystem(LogisticsSystem &logistics, LaundryStations stations);
  [[nodiscard]] LaundryBatchId requestBatch(int quantity);
  void tickSecond();
  void tickSeconds(std::int64_t seconds);
  [[nodiscard]] int totalLinenUnits() const;
  [[nodiscard]] LaundrySnapshot snapshot() const;

private:
  struct Batch {
    LaundryBatchId id{};
    int quantity{};
    LaundryStage stage{LaundryStage::AwaitingWasher};
    int remainingSeconds{};
    BlockReason blockedReason{BlockReason::None};
  };
  [[nodiscard]] int activeIn(LaundryStage stage) const;

  LogisticsSystem *logistics_{};
  LaundryStations stations_{};
  ServiceId nextId_{1};
  std::int64_t elapsedSeconds_{};
  std::vector<Batch> batches_;
};

} // namespace hh::game
