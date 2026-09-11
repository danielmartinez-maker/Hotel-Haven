#include "InGameUiContract.h"
#include "hh/frontend/GameUiTypes.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
} // namespace

int main() {
  try {
    constexpr std::array<int, 5> expectedSpeeds{0, 1, 2, 4, 8};
    require(hh::client::Final07SpeedButtons == expectedSpeeds,
            "FINAL-07 speed controls must be exactly 0x/1x/2x/4x/8x");

    constexpr std::array<std::string_view, 12> expectedPages{
        "Build",      "Rooms",      "Staff",      "Guests",
        "Supplies",   "Operations", "Finance",    "Alerts",
        "Objectives", "Overlays",   "Settings",   "Guide"};
    require(hh::client::Final07PageLabels == expectedPages,
            "FINAL-07 client shell is missing a required management surface");

    constexpr std::array<std::string_view, 5> expectedFinanceViews{
        "Overview", "Revenue", "Market", "Controls", "Risk"};
    require(hh::client::Final07FinanceViewLabels == expectedFinanceViews,
            "FINAL-07 finance data must be split into reachable bounded views");

    require(hh::client::shouldRefreshClientSnapshot(true, 0, 0.0),
            "first frame must always build the UI snapshot");
    require(!hh::client::shouldRefreshClientSnapshot(false, 0, 10.0),
            "paused stable simulation must not rebuild snapshots periodically");
    require(!hh::client::shouldRefreshClientSnapshot(false, 1, 0.099),
            "running simulation must respect the 100 ms UI refresh budget");
    require(hh::client::shouldRefreshClientSnapshot(false, 1, 0.100),
            "running simulation must refresh at the 100 ms boundary");
    require(hh::client::shouldRefreshClientSnapshot(false, 8, 0.250),
            "accelerated simulation must continue publishing snapshots");

    using hh::client::ClientUiIntent;
    using hh::client::clientUiIntent;
    using hh::frontend::UiAction;
    require(clientUiIntent(UiAction::NavigatePrevious) == ClientUiIntent::FocusPrevious,
            "remapped previous action must route to previous focus");
    require(clientUiIntent(UiAction::NavigateNext) == ClientUiIntent::FocusNext,
            "remapped next action must route to next focus");
    require(clientUiIntent(UiAction::Activate) == ClientUiIntent::Activate,
            "remapped activate action must route to activation");
    require(clientUiIntent(UiAction::Cancel) == ClientUiIntent::Cancel,
            "remapped cancel action must route to cancellation");
    require(clientUiIntent(UiAction::SpeedDown) == ClientUiIntent::HudCommand &&
                clientUiIntent(UiAction::SpeedUp) == ClientUiIntent::HudCommand &&
                clientUiIntent(UiAction::PauseToggle) == ClientUiIntent::HudCommand,
            "remapped speed/pause actions must route through HUD commands");

    constexpr std::array<std::array<int, 2>, 5> resolutions{{
        {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {2560, 1080}}};
    for (const int scale : hh::client::Final07UiScales) {
      for (const auto &resolution : resolutions) {
        const auto layout = hh::client::computeFinal07Layout(
            resolution[0], resolution[1], scale);
        require(layout.headerPixels > 0 && layout.footerPixels > 0 &&
                    layout.sidebarPixels > 0,
                "scaled chrome dimensions must stay positive");
        require(layout.viewportWidth > 0 && layout.viewportHeight > 0,
                "supported resolution/scale matrix must preserve a world viewport");
        require(layout.viewportX == 0 &&
                    layout.viewportY == layout.headerPixels,
                "viewport origin must follow scaled header geometry");
        require(layout.viewportWidth + layout.sidebarPixels == resolution[0],
                "scaled sidebar must reconcile with viewport width");
        require(layout.viewportHeight + layout.headerPixels +
                        layout.footerPixels ==
                    resolution[1],
                "scaled vertical chrome must reconcile with viewport height");
      }
    }
    const auto normal = hh::client::computeFinal07Layout(1500, 960, 100);
    require(normal.headerPixels == 88 && normal.footerPixels == 58 &&
                normal.sidebarPixels == 356,
            "100% scale must preserve baseline Hotel Haven geometry");
    const auto large = hh::client::computeFinal07Layout(1920, 1080, 150);
    require(large.headerPixels == 132 && large.footerPixels == 87 &&
                large.sidebarPixels == 534,
            "150% scale must scale chrome consistently");

    std::cout << "FINAL-07 in-game client contract passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
