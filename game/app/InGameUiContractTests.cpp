#include "InGameUiContract.h"

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
