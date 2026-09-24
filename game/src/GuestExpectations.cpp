#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <cstdint>

namespace hh::game {
namespace {
int expectationCategory(int segmentBase, int starClassModifier,
                        int reputationModifier, int pricePositionModifier,
                        int marketingClaimModifier) noexcept {
  const std::int64_t total = static_cast<std::int64_t>(segmentBase) +
                             starClassModifier + reputationModifier +
                             pricePositionModifier + marketingClaimModifier;
  return static_cast<int>(std::clamp<std::int64_t>(total, 0, 100));
}
} // namespace

GuestExpectationState
calculateGuestExpectations(const GuestExpectationInputs &inputs) noexcept {
  GuestExpectationState result;
#define HH_EXPECTATION_FIELD(field)                                              \
  result.field = expectationCategory(                                           \
      inputs.segmentBase.field, inputs.starClassModifier.field,                  \
      inputs.reputationModifier.field, inputs.pricePositionModifier.field,       \
      inputs.marketingClaimModifier.field)
  HH_EXPECTATION_FIELD(room);
  HH_EXPECTATION_FIELD(cleanliness);
  HH_EXPECTATION_FIELD(service);
  HH_EXPECTATION_FIELD(food);
  HH_EXPECTATION_FIELD(amenities);
  HH_EXPECTATION_FIELD(quiet);
  HH_EXPECTATION_FIELD(convenience);
  HH_EXPECTATION_FIELD(value);
#undef HH_EXPECTATION_FIELD
  return result;
}

} // namespace hh::game
