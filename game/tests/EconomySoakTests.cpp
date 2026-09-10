#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  EconomyRuntime runtime(36506);
  runtime.setPhysicalRoomCapacity("standard", 120);
  runtime.setPhysicalRoomCapacity("suite", 30);
  runtime.setPlayerHotelOffer({1, 17000, 76, 4, 80, 80, 80, true});
  runtime.setCompetitors({{2, "Rival One", 16500, 74, 4, 78, 75, 76},
                          {3, "Rival Two", 19500, 82, 4, 84, 82, 80},
                          {4, "Budget", 12000, 62, 3, 62, 65, 60}});
  runtime.runDays(365);
  const auto snapshot = runtime.revenueManagementSnapshot();
  require(snapshot.inventory.minimumAvailableUnits >= 0,
          "365-day campaign exceeded explicit inventory allowance");
  require(snapshot.inventory.hotPathBookingCount < 5000,
          "completed historical bookings leaked into hot-path inventory scans");
  require(runtime.financialSnapshot().economics.reconciled,
          "365-day accounting did not reconcile");
  require(runtime.marketSnapshot().physicalCompetitorGuests == 0,
          "365-day market created physical competitor guests");
  require(runtime.estimatedStateBytes() < 8 * 1024 * 1024,
          "365-day market/economy state growth was unbounded");
}
