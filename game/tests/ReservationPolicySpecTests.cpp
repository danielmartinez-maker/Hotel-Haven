#include "hh/game/RevenueInventory.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  RevenueInventory inventory(909);
  inventory.setPhysicalCapacity("standard", 5);
  inventory.setCancellationBasisPoints(BookingChannel::Ota, 10'000);
  inventory.setNoShowBasisPoints(BookingChannel::Ota, 10'000);

  BookingRequestInput protectedBooking;
  protectedBooking.bookingId = 1;
  protectedBooking.arrivalDay = 20;
  protectedBooking.departureDay = 22;
  protectedBooking.roomCategory = "standard";
  protectedBooking.rateCents = 20'000;
  protectedBooking.channel = BookingChannel::Ota;
  protectedBooking.cancellationBasisPoints = 0;
  protectedBooking.noShowBasisPoints = 0;
  protectedBooking.cancellationDeadlineDaysBeforeArrival = 2;
  protectedBooking.cancellationPenaltyCents = 7'500;
  require(inventory.book(protectedBooking).ok,
          "valid reservation-specific policy was rejected");

  BookingRequestInput cancellable = protectedBooking;
  cancellable.bookingId = 2;
  cancellable.cancellationBasisPoints = 10'000;
  cancellable.noShowBasisPoints = 0;
  cancellable.cancellationDeadlineDaysBeforeArrival = 2;
  cancellable.cancellationPenaltyCents = 9'000;
  require(inventory.book(cancellable).ok,
          "deterministic cancellation fixture booking rejected");

  BookingRequestInput walkIn;
  walkIn.bookingId = 3;
  walkIn.arrivalDay = 18;
  walkIn.departureDay = 19;
  walkIn.roomCategory = "standard";
  walkIn.rateCents = 15'000;
  walkIn.channel = BookingChannel::WalkIn;
  walkIn.cancellationBasisPoints = 0;
  walkIn.noShowBasisPoints = 0;
  walkIn.cancellationDeadlineDaysBeforeArrival = 0;
  require(inventory.book(walkIn).ok, "walk-in booking channel rejected");

  inventory.processDay(18);
  const auto snapshot = inventory.snapshot();
  const BookingView *protectedView = nullptr;
  const BookingView *cancelledView = nullptr;
  const BookingView *walkInView = nullptr;
  for (const auto &booking : snapshot.bookings) {
    if (booking.bookingId == 1) protectedView = &booking;
    if (booking.bookingId == 2) cancelledView = &booking;
    if (booking.bookingId == 3) walkInView = &booking;
  }
  require(protectedView && protectedView->state == BookingState::Confirmed,
          "per-reservation cancellation override did not override OTA default");
  require(cancelledView && cancelledView->state == BookingState::Cancelled &&
              cancelledView->cancellationPenaltyCents == 9'000,
          "reservation did not cancel at its configured deadline/penalty");
  require(walkInView && walkInView->commissionBasisPoints == 0 &&
              walkInView->cancellationBasisPoints == 0 &&
              walkInView->noShowBasisPoints == 0,
          "walk-in channel did not retain zero-commission same-day policy metadata");

  const auto saved = inventory.save();
  const auto restored = RevenueInventory::load(saved);
  require(restored.save() == saved,
          "reservation-specific policy metadata did not round-trip");
  require(restored.snapshot() == snapshot,
          "reservation policy snapshot changed across save/load");
}
