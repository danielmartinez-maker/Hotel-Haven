#include "SoundtrackCatalog.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
constexpr std::array<std::string_view, 4> kExpectedNames{
    "Morning_in_the_Atrium.mp3",
    "Sunlight_on_Marble.mp3",
    "The_Concierge_Desk.mp3",
    "First_Light_on_Marble.mp3"};

struct Config {
  std::string scale{"pr"};
  std::uint64_t seed{0x5EED5EED5EED5EEDULL};
  std::string scenario;
};

std::uint64_t parseSeed(const char *text) {
  if (!text || !*text || *text == '-')
    throw std::invalid_argument("invalid HH_STRESS_SEED");
  std::size_t used = 0;
  const auto value = std::stoull(text, &used, 0);
  if (text[used] != '\0')
    throw std::invalid_argument("invalid HH_STRESS_SEED");
  return value;
}

Config config() {
  Config c;
  if (const char *value = std::getenv("HH_STRESS_SCALE"); value && *value)
    c.scale = value;
  if (c.scale != "pr" && c.scale != "extended" && c.scale != "exhaustive")
    throw std::invalid_argument("HH_STRESS_SCALE must be pr, extended, or exhaustive");
  if (const char *value = std::getenv("HH_STRESS_SEED"); value && *value)
    c.seed = parseSeed(value);
  if (const char *value = std::getenv("HH_STRESS_SCENARIO"); value && *value)
    c.scenario = value;
  if (!c.scenario.empty() && c.scenario != "catalog_order" &&
      c.scenario != "availability" && c.scenario != "sequencing")
    throw std::invalid_argument(
        "HH_STRESS_SCENARIO must be catalog_order, availability, sequencing, or empty");
  return c;
}

std::uint64_t splitmix(std::uint64_t &state) {
  state += 0x9E3779B97F4A7C15ULL;
  auto z = state;
  z = (z ^ (z >> 30U)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27U)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31U);
}

std::size_t budget(const Config &c, std::size_t pr, std::size_t extended,
                   std::size_t exhaustive) {
  return c.scale == "pr" ? pr : c.scale == "extended" ? extended : exhaustive;
}

[[noreturn]] void fail(const Config &c, std::string_view phase,
                       std::size_t iteration, std::string_view message) {
  std::cerr << "soundtrack stress failure seed=0x" << std::hex << c.seed
            << std::dec << " scale=" << c.scale << " phase=" << phase
            << " iteration=" << iteration << " invariant=" << message << '\n';
  throw std::runtime_error(std::string(message));
}

void stressCatalogOrder(const Config &c) {
  const auto iterations = budget(c, 10'000, 100'000, 500'000);
  for (std::size_t i = 0; i < iterations; ++i) {
    const auto &tracks = hh::audio::defaultSoundtrack();
    if (tracks.size() != kExpectedNames.size())
      fail(c, "catalog_order", i, "catalog size changed");
    if (hh::audio::defaultVolumeMci() != 350)
      fail(c, "catalog_order", i, "default volume changed");
    for (std::size_t index = 0; index < tracks.size(); ++index)
      if (tracks[index].filename.generic_string() != kExpectedNames[index])
        fail(c, "catalog_order", i, "track order changed");
  }
}

void stressAvailability(const Config &c) {
  const auto root = std::filesystem::temp_directory_path() /
                    ("hotel_haven_soundtrack_stress_" + std::to_string(c.seed));
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root);
  std::uint64_t state = c.seed;
  const auto iterations = budget(c, 10'000, 100'000, 500'000);

  for (std::size_t i = 0; i < iterations; ++i) {
    for (const auto name : kExpectedNames)
      std::filesystem::remove_all(root / name, error);

    std::array<bool, 4> present{};
    for (std::size_t index = 0; index < present.size(); ++index) {
      present[index] = (splitmix(state) & 1ULL) != 0;
      if (present[index]) {
        std::ofstream file(root / kExpectedNames[index]);
        file << 'x';
      } else if ((splitmix(state) & 3ULL) == 0) {
        std::filesystem::create_directory(root / kExpectedNames[index]);
      }
    }

    const auto available = hh::audio::availableSoundtrackFiles(root);
    std::size_t expectedIndex = 0;
    for (std::size_t index = 0; index < present.size(); ++index) {
      if (!present[index])
        continue;
      if (expectedIndex >= available.size() ||
          available[expectedIndex].filename().generic_string() !=
              kExpectedNames[index])
        fail(c, "availability", i,
             "available track order/fallback incorrect");
      ++expectedIndex;
    }
    if (available.size() != expectedIndex)
      fail(c, "availability", i, "unavailable path was accepted");
  }

  std::filesystem::remove_all(root, error);
}

void stressSequencing(const Config &c) {
  const auto transitions = budget(c, 100'000, 1'000'000, 5'000'000);
  std::size_t index = 0;
  for (std::size_t i = 0; i < transitions; ++i) {
    index = hh::audio::nextTrackIndex(index, kExpectedNames.size());
    if (index >= kExpectedNames.size())
      fail(c, "sequencing", i, "track index escaped catalog");
    if (index != (i + 1) % kExpectedNames.size())
      fail(c, "sequencing", i, "loop sequence diverged");
  }
  if (hh::audio::nextTrackIndex(99, 0) != 0)
    fail(c, "sequencing", transitions, "empty catalog next index must be zero");
}
} // namespace

int main() {
  try {
    const auto c = config();
    if (c.scenario.empty() || c.scenario == "catalog_order")
      stressCatalogOrder(c);
    if (c.scenario.empty() || c.scenario == "availability")
      stressAvailability(c);
    if (c.scenario.empty() || c.scenario == "sequencing")
      stressSequencing(c);
    std::cout << "soundtrack stress PASS seed=0x" << std::hex << c.seed
              << std::dec << " scale=" << c.scale << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
