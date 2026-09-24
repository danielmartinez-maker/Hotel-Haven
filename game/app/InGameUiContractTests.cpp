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

        const auto panel = hh::client::computeFinal07PanelLayout(
            resolution[0], resolution[1], scale);
        require(panel.contentWidthPixels > 0,
                "scaled management panel must retain usable width");
        require(panel.contentHeightPixels > 0 &&
                    panel.contentTopPixels < panel.contentBottomPixels,
                "scaled management panel must retain bounded content height");
        require(panel.densityScalePercent <= 115,
                "panel whitespace density must stay capped at large UI scales");
        require(panel.buildColumns == 2 || panel.buildColumns == 3,
                "build catalog must retain a supported responsive column count");
      }
    }
    const auto type100 = hh::client::computeFinal07Typography(100);
    require(type100.normalHeight == -16 && type100.smallHeight == -13 &&
                type100.titleHeight == -23 && type100.numberHeight == -24 &&
                type100.focusInsetPixels == 2,
            "100% scale must preserve baseline typography metrics");
    const auto type150 = hh::client::computeFinal07Typography(150);
    require(type150.normalHeight == -24 && type150.smallHeight == -20 &&
                type150.titleHeight == -35 && type150.numberHeight == -36 &&
                type150.focusInsetPixels == 3,
            "150% scale must enlarge typography and focus affordances");
    for (std::size_t index = 1; index < hh::client::Final07UiScales.size(); ++index) {
      const auto previousType = hh::client::computeFinal07Typography(
          hh::client::Final07UiScales[index - 1]);
      const auto currentType = hh::client::computeFinal07Typography(
          hh::client::Final07UiScales[index]);
      require(-currentType.normalHeight >= -previousType.normalHeight &&
                  -currentType.smallHeight >= -previousType.smallHeight &&
                  -currentType.titleHeight >= -previousType.titleHeight &&
                  -currentType.numberHeight >= -previousType.numberHeight,
              "supported UI scales must never shrink typography as scale increases");
    }

    require(hh::client::final07DensityScalePercent(90) == 90 &&
                hh::client::final07DensityScalePercent(110) == 110 &&
                hh::client::final07DensityScalePercent(125) == 115 &&
                hh::client::final07DensityScalePercent(150) == 115,
            "FINAL-07 density scaling must cap whitespace growth without shrinking typography");
    require(hh::client::final07BuildCatalogColumns(359) == 2 &&
                hh::client::final07BuildCatalogColumns(360) == 3 &&
                hh::client::final07BuildCatalogColumns(474) == 3,
            "FINAL-07 build catalog must use the compact third column when the scaled panel permits it");
    const auto smallestLargeUi =
        hh::client::computeFinal07PanelLayout(1280, 720, 150);
    require(smallestLargeUi.contentHeightPixels >= 250 &&
                smallestLargeUi.buildColumns == 3,
            "1280x720 at 150 percent must preserve a usable compact management workspace");

    require(hh::client::final07PointInManagementPanel(
                1280, 720, 150, 1279, 300),
            "pointer inside scaled sidebar must route wheel input to the panel");
    require(!hh::client::final07PointInManagementPanel(
                1280, 720, 150, 700, 300) &&
                !hh::client::final07PointInManagementPanel(
                    1280, 720, 150, 1279, 100) &&
                !hh::client::final07PointInManagementPanel(
                    1280, 720, 150, 1279, 700),
            "world, header, and footer points must stay outside management-panel wheel routing");

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
