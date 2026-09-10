#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hh::game {

enum class EconomicCategory : std::uint8_t {
  RoomRevenue,
  RestaurantFoodRevenue,
  RestaurantBeverageRevenue,
  BarRevenue,
  RoomServiceRevenue,
  MinibarRevenue,
  EventRevenue,
  SpaRevenue,
  ParkingRevenue,
  PremiumServiceRevenue,
  CancellationFeeRevenue,
  NoShowFeeRevenue,
  LaborCost,
  UtilityCost,
  ChannelCommissionCost,
  ConsumablesCost,
  FoodBeverageCost,
  FixedPeriodicExpense,
  SupplyCost,
  MaintenanceCost,
  MarketingCost,
  CompensationCost,
  DebtService,
  CapitalExpense,
  LoanProceeds,
  LoanOriginationFee
};

enum class EconomicDepartment : std::uint8_t {
  Unassigned,
  Rooms,
  FoodBeverage,
  Events,
  Spa,
  Parking,
  Amenities,
  Undistributed,
  NonOperating
};

struct EconomicTransaction {
  std::uint64_t id{};
  int day{};
  EconomicCategory category{EconomicCategory::RoomRevenue};
  std::int64_t amountCents{};
  std::uint64_t sourceId{};
  std::string memo;
  EconomicDepartment department{EconomicDepartment::Unassigned};
  bool operator==(const EconomicTransaction &) const = default;
};

struct HotelEconomicsSnapshot {
  std::int64_t cashCents{};
  std::int64_t totalRevenueCents{};
  std::int64_t roomRevenueCents{};
  std::int64_t nonRoomRevenueCents{};
  std::int64_t operatingCostCents{};
  std::int64_t debtServiceCents{};
  std::int64_t capitalExpenseCents{};
  std::int64_t gopCents{};
  double occupancy{};
  std::int64_t adrCents{};
  std::int64_t revParCents{};
  std::int64_t tRevParCents{};
  std::uint64_t transactionCount{};
  bool reconciled{};
};

class HotelEconomics {
public:
  explicit HotelEconomics(std::int64_t openingCashCents = 0);

  void post(const EconomicTransaction &transaction);
  [[nodiscard]] HotelEconomicsSnapshot snapshot(int throughDay,
                                                std::int64_t sellableRoomNights,
                                                std::int64_t occupiedRoomNights) const;
  [[nodiscard]] bool reconciles() const;
  [[nodiscard]] const std::vector<EconomicTransaction> &transactions() const noexcept;
  [[nodiscard]] std::int64_t openingCashCents() const noexcept;
  [[nodiscard]] std::string save() const;
  static HotelEconomics load(std::string_view data);

  [[nodiscard]] static EconomicDepartment defaultDepartment(
      EconomicCategory category) noexcept;

private:
  std::int64_t openingCashCents_{};
  std::vector<EconomicTransaction> transactions_;
};

} // namespace hh::game
