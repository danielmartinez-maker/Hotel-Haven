#include "hh/game/EconomyRuntime.h"

#include <stdexcept>
#include <utility>

namespace hh::game {

void EconomyRuntime::postExternalTransaction(int day, EconomicCategory category,
                                             std::int64_t amountCents,
                                             std::uint64_t sourceId,
                                             std::string memo) {
  if (day < 0 || day < currentDay_)
    throw std::invalid_argument("external economic transaction moved backwards in time");
  if (amountCents == 0)
    return;
  currentDay_ = day;
  economics_.post({nextTransactionId_++, day, category, amountCents, sourceId,
                   std::move(memo)});
}

void EconomyRuntime::synchronizeExternalMetrics(
    int currentDay, std::int64_t cumulativeSellableRoomNights,
    std::int64_t cumulativeOccupiedRoomNights) {
  if (currentDay < 0 || currentDay < currentDay_ ||
      cumulativeSellableRoomNights < 0 || cumulativeOccupiedRoomNights < 0 ||
      cumulativeOccupiedRoomNights > cumulativeSellableRoomNights)
    throw std::invalid_argument("invalid external economy metric synchronization");
  currentDay_ = currentDay;
  cumulativeSellableRoomNights_ = cumulativeSellableRoomNights;
  cumulativeOccupiedRoomNights_ = cumulativeOccupiedRoomNights;
}

} // namespace hh::game
