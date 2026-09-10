#include "hh/game/EconomyRuntime.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace hh::game {
namespace {

bool isOperatingCostCategory(EconomicCategory category) {
  return category >= EconomicCategory::LaborCost &&
         category <= EconomicCategory::CompensationCost;
}

bool isFoodRevenueCategory(EconomicCategory category) {
  return category == EconomicCategory::RestaurantFoodRevenue ||
         category == EconomicCategory::RestaurantBeverageRevenue ||
         category == EconomicCategory::BarRevenue ||
         category == EconomicCategory::RoomServiceRevenue ||
         category == EconomicCategory::MinibarRevenue;
}

} // namespace

EconomyDiagnostics EconomyRuntime::diagnostics() const {
  EconomyDiagnostics out;
  const auto financial = financialSnapshot();
  const auto inventory = inventory_.snapshot();
  const auto market = market_.snapshot();

  out.adrCents = financial.economics.adrCents;
  out.revParCents = financial.economics.revParCents;
  out.roomRevenueCents = financial.economics.roomRevenueCents;
  out.totalRevenueCents = financial.economics.totalRevenueCents;
  out.channelCommissionCents = inventory.channelCommissionCents;
  out.competitorMedianRateCents = market.comparableMedianRateCents;
  out.outstandingPrincipalCents = financial.financing.outstandingPrincipalCents;
  out.nextDebtServiceCents = financial.financing.nextDebtServiceCents;
  out.nextDebtPaymentDay = financial.financing.nextPaymentDay;

  const int capacity = physicalCapacityTotal();
  const auto occupancyBasisPoints = [&](int requestedDays) {
    if (capacity <= 0 || currentDay_ <= 0 || requestedDays <= 0)
      return 0;
    const int startDay = std::max(0, currentDay_ - requestedDays);
    const int days = currentDay_ - startDay;
    std::int64_t occupiedRoomNights = 0;
    for (const auto &booking : inventory.bookings) {
      if (booking.state == BookingState::Cancelled ||
          booking.state == BookingState::NoShow)
        continue;
      const int first = std::max(startDay, booking.arrivalDay);
      const int last = std::min(currentDay_, booking.departureDay);
      if (last > first)
        occupiedRoomNights += last - first;
    }
    const auto availableRoomNights =
        static_cast<std::int64_t>(capacity) * days;
    if (availableRoomNights <= 0)
      return 0;
    return static_cast<int>(std::clamp<std::int64_t>(
        (occupiedRoomNights * 10000 + availableRoomNights / 2) /
            availableRoomNights,
        0, 10000));
  };
  out.occupancyTodayBasisPoints = occupancyBasisPoints(1);
  out.occupancy7DayBasisPoints = occupancyBasisPoints(7);
  out.occupancy30DayBasisPoints = occupancyBasisPoints(30);

  for (const auto &booking : inventory.bookings) {
    if (booking.state == BookingState::Cancelled)
      ++out.cancellations;
    if (booking.state == BookingState::NoShow)
      ++out.noShows;
    if (booking.state != BookingState::Cancelled)
      ++out.channelBookings[booking.channel];
    if (booking.state == BookingState::Confirmed &&
        booking.arrivalDay >= currentDay_)
      ++out.bookingPaceByArrivalDay[booking.arrivalDay];
  }
  for (const auto &request : market.requests)
    ++out.demandBySegment[request.segment];

  std::int64_t foodRevenueCents = 0;
  std::int64_t recentOperatingCostCents = 0;
  const int recentStartDay = std::max(0, currentDay_ - 30);
  for (const auto &transaction : economics_.transactions()) {
    if (transaction.category == EconomicCategory::LaborCost)
      out.laborCostCents -= transaction.amountCents;
    if (transaction.category == EconomicCategory::UtilityCost)
      out.utilityCostCents -= transaction.amountCents;
    if (transaction.category == EconomicCategory::FoodBeverageCost)
      out.foodBeverageCostCents -= transaction.amountCents;
    if (isFoodRevenueCategory(transaction.category))
      foodRevenueCents += transaction.amountCents;
    if (transaction.day >= recentStartDay &&
        transaction.day <= currentDay_ &&
        isOperatingCostCategory(transaction.category))
      recentOperatingCostCents -= transaction.amountCents;
  }

  if (out.totalRevenueCents > 0)
    out.laborCostShareBasisPoints = static_cast<int>(std::clamp<std::int64_t>(
        (out.laborCostCents * 10000 + out.totalRevenueCents / 2) /
            out.totalRevenueCents,
        0, std::numeric_limits<int>::max()));
  if (foodRevenueCents > 0)
    out.foodCostShareBasisPoints = static_cast<int>(std::clamp<std::int64_t>(
        (out.foodBeverageCostCents * 10000 + foodRevenueCents / 2) /
            foodRevenueCents,
        0, std::numeric_limits<int>::max()));

  const int recentDays = std::max(1, currentDay_ - recentStartDay);
  const long double averageDailyOperatingCost =
      static_cast<long double>(recentOperatingCostCents) /
      static_cast<long double>(recentDays);
  if (averageDailyOperatingCost > 0.0L) {
    out.cashRunwayDays = static_cast<double>(
        static_cast<long double>(financial.economics.cashCents) /
        averageDailyOperatingCost);
  } else {
    out.cashRunwayDays = 0.0;
  }

  return out;
}

} // namespace hh::game
