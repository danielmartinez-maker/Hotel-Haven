#include "hh/game/Amenities.h"

namespace hh::game {
struct AmenitiesSystem::Impl {};
AmenitiesSystem::AmenitiesSystem() : impl_(std::make_unique<Impl>()) {}
AmenitiesSystem::~AmenitiesSystem() = default;
AmenitiesSystem::AmenitiesSystem(AmenitiesSystem &&) noexcept = default;
AmenitiesSystem &AmenitiesSystem::operator=(AmenitiesSystem &&) noexcept = default;
AmenitiesSystem::AmenitiesSystem(const AmenitiesSystem &) : impl_(std::make_unique<Impl>()) {}
AmenitiesSystem &AmenitiesSystem::operator=(const AmenitiesSystem &) { return *this; }
void AmenitiesSystem::addAmenity(const AmenityConfig &) {}
void AmenitiesSystem::setElapsedSeconds(std::int64_t) {}
void AmenitiesSystem::setCleanliness(AmenityId, int) {}
void AmenitiesSystem::setCondition(AmenityId, int) {}
AmenityReservationResult AmenitiesSystem::reserveAmenity(GuestId, const AmenityRequest &) { return {}; }
std::vector<AmenityId> AmenitiesSystem::opportunities(GuestId) const { return {}; }
void AmenitiesSystem::tickSecond() {}
void AmenitiesSystem::tickSeconds(std::int64_t) {}
AmenitiesSnapshot AmenitiesSystem::snapshot() const { return {}; }
std::string AmenitiesSystem::save() const { return {}; }
AmenitiesSystem AmenitiesSystem::load(std::string_view) { return {}; }
} // namespace hh::game
