#include "hh/game/Laundry.h"
#include <algorithm>

namespace hh::game {

LaundrySystem::LaundrySystem(LogisticsSystem &logistics, LaundryStations stations)
    : logistics_(&logistics), stations_(stations) {
  stations_.washers = std::max(0, stations_.washers);
  stations_.dryers = std::max(0, stations_.dryers);
  stations_.foldingStations = std::max(0, stations_.foldingStations);
}

LaundryBatchId LaundrySystem::requestBatch(int quantity) {
  if (quantity <= 0 || quantity > 30 ||
      !logistics_->consumeFromKind(StorageKind::DirtyLinen,
                                   "dirty_linen_set", quantity))
    return 0;
  const auto id = nextId_++;
  batches_.push_back({id, quantity, LaundryStage::AwaitingWasher, 0,
                      BlockReason::None});
  return id;
}

int LaundrySystem::activeIn(LaundryStage stage) const {
  return static_cast<int>(std::count_if(
      batches_.begin(), batches_.end(),
      [stage](const Batch &batch) { return batch.stage == stage; }));
}

void LaundrySystem::tickSecond() {
  ++elapsedSeconds_;
  for (auto &batch : batches_) {
    switch (batch.stage) {
    case LaundryStage::AwaitingWasher:
      if (stations_.washers <= activeIn(LaundryStage::Washing)) {
        batch.blockedReason = BlockReason::MissingWasher;
        break;
      }
      batch.stage = LaundryStage::Washing;
      batch.remainingSeconds = 35 * 60;
      batch.blockedReason = BlockReason::None;
      break;
    case LaundryStage::Washing:
      if (--batch.remainingSeconds <= 0) {
        batch.stage = LaundryStage::AwaitingDryer;
        batch.remainingSeconds = 0;
      }
      break;
    case LaundryStage::AwaitingDryer:
      if (stations_.dryers <= activeIn(LaundryStage::Drying)) {
        batch.blockedReason = BlockReason::MissingDryer;
        break;
      }
      batch.stage = LaundryStage::Drying;
      batch.remainingSeconds = 40 * 60;
      batch.blockedReason = BlockReason::None;
      break;
    case LaundryStage::Drying:
      if (--batch.remainingSeconds <= 0) {
        batch.stage = LaundryStage::AwaitingFold;
        batch.remainingSeconds = 0;
      }
      break;
    case LaundryStage::AwaitingFold:
      if (stations_.foldingStations <= activeIn(LaundryStage::Folding)) {
        batch.blockedReason = BlockReason::MissingFoldingStation;
        break;
      }
      batch.stage = LaundryStage::Folding;
      batch.remainingSeconds = 15 * 60;
      batch.blockedReason = BlockReason::None;
      break;
    case LaundryStage::Folding:
      if (batch.remainingSeconds > 0)
        --batch.remainingSeconds;
      if (batch.remainingSeconds > 0)
        break;
      if (!logistics_->canAddToKind(StorageKind::CleanLinen, batch.quantity)) {
        batch.blockedReason = BlockReason::MissingCleanStorage;
        break;
      }
      if (!logistics_->addToKind(StorageKind::CleanLinen, "clean_linen_set",
                                 batch.quantity)) {
        batch.blockedReason = BlockReason::MissingCleanStorage;
        break;
      }
      batch.stage = LaundryStage::Completed;
      batch.blockedReason = BlockReason::None;
      break;
    case LaundryStage::Completed:
      break;
    }
  }
}

void LaundrySystem::tickSeconds(std::int64_t seconds) {
  for (std::int64_t i = 0; i < seconds; ++i)
    tickSecond();
}

int LaundrySystem::totalLinenUnits() const {
  int total = logistics_->totalInventory("dirty_linen_set") +
              logistics_->totalInventory("clean_linen_set");
  for (const auto &batch : batches_)
    if (batch.stage != LaundryStage::Completed)
      total += batch.quantity;
  return total;
}

LaundrySnapshot LaundrySystem::snapshot() const {
  LaundrySnapshot out;
  out.elapsedSeconds = elapsedSeconds_;
  out.stations = stations_;
  for (const auto &batch : batches_)
    out.batches.push_back({batch.id, batch.quantity, batch.stage,
                           batch.remainingSeconds, batch.blockedReason});
  return out;
}

} // namespace hh::game
