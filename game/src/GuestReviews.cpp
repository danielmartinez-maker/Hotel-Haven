#include "hh/game/GuestReviews.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

namespace hh::game {
namespace {
std::uint64_t mix(std::uint64_t value) noexcept {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31);
}

std::string_view phraseFor(const GuestMemory &memory) noexcept {
  switch (memory.type) {
  case ExperienceEventType::FastCheckIn:
    return memory.valence >= 0 ? "Check-in was impressively quick."
                               : "Check-in did not go as smoothly as expected.";
  case ExperienceEventType::LongCheckInQueue:
    return "The check-in wait was frustrating.";
  case ExperienceEventType::RoomNotReady:
    return "The room was not ready when expected.";
  case ExperienceEventType::FreeUpgrade:
    return "The room upgrade was a welcome surprise.";
  case ExperienceEventType::DirtyBathroom:
    return "The bathroom cleanliness was disappointing.";
  case ExperienceEventType::ExcellentRoomCleanliness:
    return "The room cleanliness stood out positively.";
  case ExperienceEventType::BrokenAC:
    return "The broken air conditioning hurt the stay.";
  case ExperienceEventType::QuickMaintenanceRecovery:
    return "Maintenance resolved the problem quickly.";
  case ExperienceEventType::GreatMeal:
    return "The meal was excellent.";
  case ExperienceEventType::SlowRoomService:
    return "Room service was too slow.";
  case ExperienceEventType::ElevatorDelay:
    return "The elevator delay was inconvenient.";
  case ExperienceEventType::NoiseDisturbance:
    return "Noise made the stay less comfortable.";
  case ExperienceEventType::StaffRudeness:
    return "A rude staff interaction hurt the experience.";
  case ExperienceEventType::StaffExceptionalService:
    return "The staff service was exceptional.";
  }
  return {};
}

struct RankedMemory {
  const GuestMemory *memory{};
  double strength{};
};
} // namespace

GuestReviewDraft buildGuestReview(const GuestPsychologySnapshot &psychology,
                                  std::uint64_t campaignSeed,
                                  std::int64_t nowSeconds) noexcept {
  GuestReviewDraft review;

  const double normalized =
      std::clamp(static_cast<double>(psychology.satisfaction.overall), 0.0, 100.0);
  const std::uint64_t reviewSeed =
      mix(campaignSeed ^ mix(psychology.guestId) ^ 0x524556494557ULL);
  const double unit = static_cast<double>(reviewSeed >> 11) *
                      (1.0 / static_cast<double>(std::uint64_t{1} << 53));
  const double jitter = (unit * 2.0 - 1.0) * 0.15;
  review.score = std::clamp(1.0 + normalized * 0.09 + jitter, 1.0, 10.0);

  std::vector<RankedMemory> ranked;
  ranked.reserve(psychology.memories.size());
  for (const auto &memory : psychology.memories) {
    const double strength =
        std::abs(GuestPsychology::memoryContribution(memory, nowSeconds));
    if (strength >= 10.0)
      ranked.push_back({&memory, strength});
  }
  std::stable_sort(ranked.begin(), ranked.end(),
                   [](const RankedMemory &a, const RankedMemory &b) {
                     if (a.strength != b.strength)
                       return a.strength > b.strength;
                     if (a.memory->timestampSeconds != b.memory->timestampSeconds)
                       return a.memory->timestampSeconds < b.memory->timestampSeconds;
                     return static_cast<int>(a.memory->type) <
                            static_cast<int>(b.memory->type);
                   });
  if (ranked.size() > 3)
    ranked.resize(3);

  if (ranked.empty()) {
    review.text = "The stay matched my overall experience.";
    return review;
  }

  for (const auto &entry : ranked) {
    review.evidence.push_back(entry.memory->type);
    const auto phrase = phraseFor(*entry.memory);
    if (phrase.empty())
      continue;
    if (!review.text.empty())
      review.text.push_back(' ');
    review.text.append(phrase);
  }
  if (review.text.empty())
    review.text = "The stay matched my overall experience.";
  return review;
}

} // namespace hh::game
