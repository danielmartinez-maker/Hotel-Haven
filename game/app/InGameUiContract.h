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

struct Final07Layout {
  int headerPixels{};
  int footerPixels{};
  int sidebarPixels{};
  int viewportX{};
  int viewportY{};
  int viewportWidth{};
  int viewportHeight{};
};

[[nodiscard]] constexpr int final07ScalePixel(int logicalPixels,
                                               int scalePercent) noexcept {
  return (logicalPixels * scalePercent + 50) / 100;
}

[[nodiscard]] constexpr Final07Layout
computeFinal07Layout(int clientWidth, int clientHeight,
                     int scalePercent) noexcept {
  Final07Layout result;
  result.headerPixels = final07ScalePixel(88, scalePercent);
  result.footerPixels = final07ScalePixel(58, scalePercent);
  result.sidebarPixels = final07ScalePixel(356, scalePercent);
  result.viewportX = 0;
  result.viewportY = result.headerPixels;
  result.viewportWidth = clientWidth - result.sidebarPixels;
  result.viewportHeight =
      clientHeight - result.headerPixels - result.footerPixels;
  return result;
}

static_assert(Final07PageLabels.size() == 12);
static_assert(Final07SpeedButtons[0] == 0 && Final07SpeedButtons[4] == 8);
static_assert(computeFinal07Layout(1500, 960, 100).sidebarPixels == 356);

} // namespace hh::client
