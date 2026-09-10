#include "hh/game/HotelEconomics.h"
#include <cmath>
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  HotelEconomics ledger(1000000);
  ledger.post({1, 1, EconomicCategory::RoomRevenue, 30000, 101, "room stay"});
  ledger.post({2, 1, EconomicCategory::RestaurantFoodRevenue, 10000, 201, "restaurant"});
  ledger.post({3, 1, EconomicCategory::LaborCost, -8000, 301, "payroll"});
  ledger.post({4, 1, EconomicCategory::UtilityCost, -2000, 0, "utilities"});
  const auto snap = ledger.snapshot(1, 10, 8);
  require(snap.roomRevenueCents == 30000, "room revenue attribution failed");
  require(snap.nonRoomRevenueCents == 10000, "non-room revenue attribution failed");
  require(snap.operatingCostCents == 10000, "operating cost attribution failed");
  require(snap.gopCents == 30000, "GOP reconciliation failed");
  require(snap.cashCents == 1030000, "cash did not reconcile to exact minor unit");
  require(std::abs(snap.occupancy - 0.8) < 1e-9, "occupancy calculation failed");
  require(snap.adrCents == 3750, "ADR calculation failed");
  require(snap.revParCents == 3000, "RevPAR calculation failed");
  require(snap.tRevParCents == 4000, "TRevPAR calculation failed");
  require(ledger.reconciles(), "authoritative economic ledger did not reconcile");
}
