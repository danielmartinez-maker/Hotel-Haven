#pragma once

#include "hh/game/Simulation.h"
#include <cstdint>
#include <random>

namespace hh::game {

using GuestId = EntityId;

class GuestPsychology {
public:
  explicit GuestPsychology(std::uint64_t campaignSeed) noexcept;

  [[nodiscard]] GuestProfileView generateGuestProfile(GuestId guestId) const;
  [[nodiscard]] static bool validProfile(const GuestProfileView &profile) noexcept;

private:
  std::uint64_t campaignSeed_{};
};

namespace detail {
[[nodiscard]] GuestProfileView
 generateGuestProfileFromRandom(std::mt19937_64 &random);
[[nodiscard]] bool validGuestProfile(const GuestProfileView &profile) noexcept;
[[nodiscard]] int queueToleranceFor(const GuestProfileView &profile,
                                    double baseSeconds = 480) noexcept;
} // namespace detail

} // namespace hh::game
