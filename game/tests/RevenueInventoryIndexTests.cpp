#include "hh/game/RevenueInventory.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  RevenueInventory inventory(7301);
  inventory.setPhysicalCapacity("standard", 10);

  // Create campaign history that must not remain in the future lookup index.
  for (std::uint64_t id = 1; id <= 200; ++id) {
    const int arrival = static_cast<int>(id);
    BookingRequestInput historical{id, arrival, arrival + 1, "standard", 10'000,
                                   BookingChannel::Direct};
    require(inventory.book(historical).ok, "historical fixture booking failed");
  }
  inventory.processDay(250);
  require(inventory.snapshot().hotPathBookingCount == 0,
          "completed booking history remained active");
  require(inventory.snapshot().indexedRoomNightCount == 0,
          "completed booking history remained in future inventory index");

  BookingRequestInput first{1001, 300, 302, "standard", 15'000,
                            BookingChannel::Direct};
  BookingRequestInput second{1002, 300, 302, "standard", 15'000,
                             BookingChannel::Ota};
  require(inventory.book(first).ok && inventory.book(second).ok,
          "future fixture booking failed");
  require(inventory.bookedUnits(300, "standard") == 2 &&
              inventory.bookedUnits(301, "standard") == 2,
          "indexed future occupancy is incorrect");
  require(inventory.snapshot().indexedRoomNightCount == 4,
          "future occupancy index did not track active room nights");

  require(inventory.cancel(1001).ok, "cancellation failed");
  require(inventory.bookedUnits(300, "standard") == 1 &&
              inventory.snapshot().indexedRoomNightCount == 2,
          "cancellation did not remove room nights from index");

  const auto saved = inventory.save();
  auto restored = RevenueInventory::load(saved);
  require(restored.save() == saved, "inventory index changed serialized authority");
  require(restored.bookedUnits(300, "standard") == 1 &&
              restored.snapshot().indexedRoomNightCount == 2,
          "future occupancy index was not reconstructed on load");

  restored.processDay(303);
  require(restored.snapshot().indexedRoomNightCount == 0,
          "completed future booking remained in lookup index");
}
