#include "SoundtrackPlayer.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

namespace {

bool require(bool condition, const char *message) {
  if (condition)
    return true;
  std::cerr << "SoundtrackPlayer stress failure: " << message << '\n';
  return false;
}

} // namespace

int main() {
  using namespace std::chrono_literals;

  const auto missingDirectory =
      std::filesystem::temp_directory_path() /
      "hotel-haven-soundtrack-player-stress-missing";
  std::error_code error;
  std::filesystem::remove_all(missingDirectory, error);

  hh::audio::SoundtrackPlayer player;
  player.stop();
  if (!require(!player.active(), "fresh player must be inactive after stop"))
    return 1;

  constexpr int cycles = 512;
  for (int cycle = 0; cycle < cycles; ++cycle) {
    if (!require(player.start(missingDirectory),
                 "start must accept a valid filesystem path"))
      return 1;

    if ((cycle & 15) == 0)
      std::this_thread::yield();

    player.stop();
    if (!require(!player.active(), "stop must leave player inactive"))
      return 1;

    // stop() is part of the shutdown path and must remain idempotent.
    player.stop();
    if (!require(!player.active(), "repeated stop must remain inactive"))
      return 1;
  }

  // Exercise replacement of an existing worker without an explicit stop.
  if (!require(player.start(missingDirectory), "first restart failed") ||
      !require(player.start(missingDirectory), "second restart failed"))
    return 1;
  player.stop();
  if (!require(!player.active(), "restart sequence leaked active state"))
    return 1;

  std::cout << "StressSoundtrackPlayer PASS cycles=" << cycles << '\n';
  return 0;
}
