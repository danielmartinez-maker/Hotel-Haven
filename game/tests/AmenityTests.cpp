#include "hh/game/Amenities.h"
#include <algorithm>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static AmenityConfig spa() {
  AmenityConfig amenity;
  amenity.id = 10;
  amenity.type = AmenityType::Spa;
  amenity.openMinuteOfDay = 0;
  amenity.closeMinuteOfDay = 24 * 60;
  amenity.capacity = 1;
  amenity.staffCapacity = 1;
  amenity.priceCents = 15000;
  amenity.defaultDurationSeconds = 10;
  return amenity;
}

static void spa_reservation_is_atomic_across_guest_staff_room_and_slot() {
  AmenitiesSystem amenities;
  amenities.addAmenity(spa());
  const AmenityRequest request{10, 100, 30};
  const auto first = amenities.reserveAmenity(1001, request);
  const auto second = amenities.reserveAmenity(1002, request);
  require(first.ok, "first spa reservation failed");
  require(!second.ok, "same spa slot was double-booked");
  require(second.blockReason == AmenityBlockReason::CapacityFull ||
              second.blockReason == AmenityBlockReason::StaffUnavailable,
          "spa collision did not expose a capacity/staff block reason");
}

static void closed_or_dirty_amenity_is_excluded_from_guest_opportunities() {
  AmenitiesSystem amenities;
  auto pool = spa();
  pool.id = 20;
  pool.type = AmenityType::Pool;
  pool.capacity = 20;
  pool.staffCapacity = 1;
  amenities.addAmenity(pool);
  amenities.setCleanliness(20, 0);
  const auto available = amenities.opportunities(2001);
  require(std::find(available.begin(), available.end(), 20) == available.end(),
          "dirty amenity remained available to guest psychology");
}

static void completed_service_posts_outcome_revenue_and_service_demand() {
  AmenitiesSystem amenities;
  auto treatment = spa();
  treatment.cleanliness = 6100;
  treatment.condition = 5100;
  amenities.addAmenity(treatment);
  const auto result = amenities.reserveAmenity(3001, {10, 1, 5});
  require(result.ok, "valid spa reservation failed");
  amenities.tickSeconds(7);
  const auto snapshot = amenities.snapshot();
  require(snapshot.revenueCents == 15000,
          "amenity revenue was not posted exactly once");
  require(snapshot.outcomes.size() == 1,
          "amenity service did not publish one attributed outcome");
  require(snapshot.outcomes.front().guestId == 3001,
          "amenity outcome was not attributed to the guest");
  const auto view = std::find_if(snapshot.amenities.begin(), snapshot.amenities.end(),
                                 [](const auto &value) { return value.id == 10; });
  require(view != snapshot.amenities.end(), "amenity disappeared from snapshot");
  require(view->cleaningRequired || view->maintenanceRequired,
          "amenity condition did not expose cleaning/maintenance demand");
}

static void amenity_state_round_trips_deterministically() {
  AmenitiesSystem amenities;
  amenities.addAmenity(spa());
  require(amenities.reserveAmenity(4001, {10, 5, 10}).ok,
          "reservation failed for persistence test");
  amenities.tickSeconds(8);
  const auto restored = AmenitiesSystem::load(amenities.save());
  require(restored.save() == amenities.save(),
          "amenity save/load changed authoritative state");
}

int main() {
  spa_reservation_is_atomic_across_guest_staff_room_and_slot();
  closed_or_dirty_amenity_is_excluded_from_guest_opportunities();
  completed_service_posts_outcome_revenue_and_service_demand();
  amenity_state_round_trips_deterministically();
}
