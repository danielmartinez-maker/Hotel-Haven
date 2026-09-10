#include "hh/game/RevenueInventory.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool v, const char *m) { if (!v) throw std::runtime_error(m); }

int main() {
  RevenueInventory inv(77);
  inv.setPhysicalCapacity("standard", 10);
  inv.setOverbookingAllowance("standard", 2);
  require(inv.sellableUnits(20, "standard") == 12, "overbooking allowance not reflected in sellable inventory");

  for (int i = 0; i < 12; ++i) {
    BookingRequestInput req;
    req.bookingId = 100 + i;
    req.arrivalDay = 20;
    req.departureDay = 22;
    req.roomCategory = "standard";
    req.rateCents = 15000;
    req.channel = BookingChannel::Direct;
    require(inv.book(req).ok, "valid future booking rejected");
  }
  BookingRequestInput extra{999, 20, 22, "standard", 15000, BookingChannel::Direct};
  require(!inv.book(extra).ok, "inventory went negative beyond explicit allowance");
  require(inv.bookedUnits(20, "standard") == 12, "future inventory conservation failed");

  RevenueInventory replayA(123), replayB(123);
  replayA.setPhysicalCapacity("standard", 20); replayB.setPhysicalCapacity("standard", 20);
  replayA.setCancellationBasisPoints(BookingChannel::Ota, 2500);
  replayB.setCancellationBasisPoints(BookingChannel::Ota, 2500);
  for (int i = 0; i < 10; ++i) {
    BookingRequestInput r{static_cast<std::uint64_t>(i + 1), 30, 32, "standard", 12000, BookingChannel::Ota};
    require(replayA.book(r).ok && replayB.book(r).ok, "fixture booking failed");
  }
  replayA.processDay(29); replayB.processDay(29);
  require(replayA.snapshot() == replayB.snapshot(), "cancellation/no-show processing was not deterministic");
  const auto snap = replayA.snapshot();
  for (const auto &booking : snap.bookings)
    if (booking.channel == BookingChannel::Ota)
      require(booking.commissionBasisPoints > 0, "booking channel commission metadata missing");
}
