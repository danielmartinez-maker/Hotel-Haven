#pragma once

#include "hh/game/GuestPsychology.h"
#include <cstdint>
#include <string>
#include <vector>

namespace hh::game {

struct GuestReviewDraft {
  double score{};
  std::string text;
  std::vector<ExperienceEventType> evidence;
  bool operator==(const GuestReviewDraft &) const = default;
};

[[nodiscard]] GuestReviewDraft
buildGuestReview(const GuestPsychologySnapshot &psychology,
                 std::uint64_t campaignSeed,
                 std::int64_t nowSeconds) noexcept;

} // namespace hh::game
