#pragma once

#include "hh/frontend/GameUiTypes.h"
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

enum class FinanceView {
  Overview,
  Revenue,
  Market,
  Controls,
  Risk
};

enum class ClientUiIntent {
  None,
  FocusPrevious,
  FocusNext,
  Activate,
  Cancel,
  HudCommand
};

[[nodiscard]] constexpr ClientUiIntent
clientUiIntent(hh::frontend::UiAction action) noexcept {
  using hh::frontend::UiAction;
  switch (action) {
  case UiAction::NavigatePrevious: return ClientUiIntent::FocusPrevious;
  case UiAction::NavigateNext: return ClientUiIntent::FocusNext;
  case UiAction::Activate: return ClientUiIntent::Activate;
  case UiAction::Cancel: return ClientUiIntent::Cancel;
  case UiAction::SpeedDown:
  case UiAction::SpeedUp:
  case UiAction::PauseToggle: return ClientUiIntent::HudCommand;
  }
  return ClientUiIntent::None;
}

inline constexpr std::array<int, 5> Final07SpeedButtons{0, 1, 2, 4, 8};
inline constexpr std::array<std::string_view, 12> Final07PageLabels{
    "Build",      "Rooms",      "Staff",      "Guests",
    "Supplies",   "Operations", "Finance",    "Alerts",
    "Objectives", "Overlays",   "Settings",   "Guide"};
inline constexpr std::array<std::string_view, 5> Final07FinanceViewLabels{
    "Overview", "Revenue", "Market", "Controls", "Risk"};
inline constexpr std::array<int, 5> Final07UiScales{90, 100, 110, 125, 150};

[[nodiscard]] constexpr bool
shouldRefreshClientSnapshot(bool firstFrame, int simulationSpeed,
                            double elapsedSinceRefreshSeconds) noexcept {
  return firstFrame ||
         (simulationSpeed > 0 && elapsedSinceRefreshSeconds >= 0.100);
}

struct Final07Layout {
  int headerPixels{};
  int footerPixels{};
  int sidebarPixels{};
  int viewportX{};
  int viewportY{};
  int viewportWidth{};
  int viewportHeight{};
};

struct Final07Typography {
  int normalHeight{};
  int smallHeight{};
  int titleHeight{};
  int numberHeight{};
  int focusInsetPixels{};
};

struct Final07PanelLayout {
  int densityScalePercent{};
  int contentWidthPixels{};
  int contentTopPixels{};
  int contentBottomPixels{};
  int contentHeightPixels{};
  int buildColumns{};
};

[[nodiscard]] constexpr int final07ScalePixel(int logicalPixels,
                                               int scalePercent) noexcept {
  return (logicalPixels * scalePercent + 50) / 100;
}

[[nodiscard]] constexpr int
final07DensityScalePercent(int scalePercent) noexcept {
  return scalePercent > 115 ? 115 : scalePercent;
}

[[nodiscard]] constexpr int
final07BuildCatalogColumns(int panelWidthPixels) noexcept {
  return panelWidthPixels >= 360 ? 3 : 2;
}

[[nodiscard]] constexpr Final07Typography
computeFinal07Typography(int scalePercent) noexcept {
  Final07Typography result;
  result.normalHeight = -final07ScalePixel(16, scalePercent);
  result.smallHeight = -final07ScalePixel(13, scalePercent);
  result.titleHeight = -final07ScalePixel(23, scalePercent);
  result.numberHeight = -final07ScalePixel(24, scalePercent);
  result.focusInsetPixels = final07ScalePixel(2, scalePercent);
  return result;
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

[[nodiscard]] constexpr Final07PanelLayout
computeFinal07PanelLayout(int clientWidth, int clientHeight,
                          int scalePercent) noexcept {
  const auto shell = computeFinal07Layout(clientWidth, clientHeight, scalePercent);
  Final07PanelLayout result;
  result.densityScalePercent = final07DensityScalePercent(scalePercent);
  result.contentWidthPixels =
      shell.sidebarPixels - final07ScalePixel(40, scalePercent);
  result.contentTopPixels =
      shell.headerPixels + final07ScalePixel(126, result.densityScalePercent);
  result.contentBottomPixels =
      clientHeight - shell.footerPixels -
      final07ScalePixel(52, result.densityScalePercent);
  result.contentHeightPixels =
      result.contentBottomPixels > result.contentTopPixels
          ? result.contentBottomPixels - result.contentTopPixels
          : 0;
  result.buildColumns = final07BuildCatalogColumns(result.contentWidthPixels);
  return result;
}

[[nodiscard]] constexpr bool
final07PointInManagementPanel(int clientWidth, int clientHeight,
                              int scalePercent, int x, int y) noexcept {
  const auto shell = computeFinal07Layout(clientWidth, clientHeight, scalePercent);
  return x >= clientWidth - shell.sidebarPixels && x < clientWidth &&
         y >= shell.headerPixels &&
         y < clientHeight - shell.footerPixels;
}

static_assert(Final07PageLabels.size() == 12);
static_assert(Final07FinanceViewLabels.size() == 5);
static_assert(Final07SpeedButtons[0] == 0 && Final07SpeedButtons[4] == 8);
static_assert(computeFinal07Layout(1500, 960, 100).sidebarPixels == 356);

} // namespace hh::client
