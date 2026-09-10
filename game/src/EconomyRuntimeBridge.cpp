#include "hh/game/EconomyRuntime.h"

#include <stdexcept>
#include <utility>

namespace hh::game {

InventoryCommandResult EconomyRuntime::setInventoryBlock(
    const InventoryBlock &block) {
  return inventory_.setInventoryBlock(block);
}

InventoryCommandResult EconomyRuntime::removeInventoryBlock(
    std::uint64_t blockId) {
  return inventory_.removeInventoryBlock(blockId);
}

RecoveryDecision EconomyRuntime::resolveOverbooking(const RecoveryContext &context,
                                                    std::uint64_t sourceId) {
  const auto decision = overbooking_.chooseRecovery(context);
  if (decision.action == RecoveryAction::CompetitorRelocation &&
      decision.compensationCents > 0)
    post(EconomicCategory::CompensationCost, -decision.compensationCents, sourceId,
         "overbooking relocation compensation");
  return decision;
}

void EconomyRuntime::postExternalTransaction(int day, EconomicCategory category,
                                             std::int64_t amountCents,
                                             std::uint64_t sourceId,
                                             std::string memo) {
  postExternalTransaction(day, category,
                          HotelEconomics::defaultDepartment(category), amountCents,
                          sourceId, std::move(memo));
}

void EconomyRuntime::postExternalTransaction(int day, EconomicCategory category,
                                             EconomicDepartment department,
                                             std::int64_t amountCents,
                                             std::uint64_t sourceId,
                                             std::string memo) {
  if (day < 0 || day < currentDay_)
    throw std::invalid_argument("external economic transaction moved backwards in time");
  if (department == EconomicDepartment::Unassigned)
    throw std::invalid_argument("external economic transaction requires a department");
  if (amountCents == 0)
    return;
  currentDay_ = day;
  EconomicTransaction transaction;
  transaction.id = nextTransactionId_++;
  transaction.day = day;
  transaction.category = category;
  transaction.amountCents = amountCents;
  transaction.sourceId = sourceId;
  transaction.memo = std::move(memo);
  transaction.department = department;
  economics_.post(transaction);
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
