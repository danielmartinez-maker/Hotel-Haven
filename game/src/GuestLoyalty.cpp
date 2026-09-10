#include "hh/game/GuestPsychology.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace hh::game {

int calculateRepeatIntent(const GuestPsychologySnapshot &psychology,
                          std::int64_t nowSeconds) noexcept {
  const std::array<int, 8> expectations{
      psychology.expectations.room,        psychology.expectations.cleanliness,
      psychology.expectations.service,     psychology.expectations.food,
      psychology.expectations.amenities,   psychology.expectations.quiet,
      psychology.expectations.convenience, psychology.expectations.value};
  int expectationTotal = 0;
  for (const int expectation : expectations)
    expectationTotal += std::clamp(expectation, 0, 100);
  const int expectationMean = (expectationTotal + 4) / 8;

  const int overall = std::clamp(psychology.satisfaction.overall, 0, 100);
  const int expectationAdjusted = std::clamp(
      overall + (overall - expectationMean) / 2, 0, 100);
  const int value =
      std::clamp(psychology.operational.valuePerception, 0, 100);
  const int expectationFit =
      std::clamp(psychology.satisfaction.expectationFit, 0, 100);

  double memoryContribution = 0.0;
  for (const auto &memory : psychology.memories)
    memoryContribution +=
        GuestPsychology::memoryContribution(memory, nowSeconds);
  const int memoryScore = static_cast<int>(std::clamp(
      std::lround(50.0 + memoryContribution), 0L, 100L));

  const int weightedPercent =
      (expectationAdjusted * 50 + value * 20 + expectationFit * 15 +
       memoryScore * 15 + 50) /
      100;
  return std::clamp(weightedPercent, 0, 100) * 100;
}

} // namespace hh::game
