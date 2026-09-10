#pragma once

#include <array>
#include <string_view>

namespace hh::client {

enum class Page {
  Build,
  Rooms,
  Staff,
  Guests,
  Supplies,
  Operations,
  Finance,
  Alerts,
  Objectives,
  Overlays,
  Settings,
  Guide
};

inline constexpr std::array<int, 5> Final07SpeedButtons{0, 1, 2, 4, 8};
inline constexpr std::array<std::string_view, 12> Final07PageLabels{
    "Build",      "Rooms",      "Staff",      "Guests",
    "Supplies",   "Operations", "Finance",    "Alerts",
    "Objectives", "Overlays",   "Settings",   "Guide"};
inline constexpr std::array<int, 5> Final07UiScales{90, 100, 110, 125, 150};

static_assert(Final07PageLabels.size() == 12);
static_assert(Final07SpeedButtons[0] == 0 && Final07SpeedButtons[4] == 8);

} // namespace hh::client
