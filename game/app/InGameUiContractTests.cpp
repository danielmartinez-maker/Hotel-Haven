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
    require(hh::client::Final07PageLabels.size() == 12,
            "FINAL-07 client page count changed unexpectedly");

    std::cout << "FINAL-07 in-game client contract passed\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
