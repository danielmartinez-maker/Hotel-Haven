#include "SoundtrackCatalog.h"

#include <system_error>

namespace hh::audio {
namespace {
const std::array<SoundtrackTrack, 4> kTracks{{
    {"morning_in_the_atrium", "Morning in the Atrium",
     "Morning_in_the_Atrium.mp3", 179.905250, 4323801,
     "af78af388491faf33449efad632dfc6d09f53406d3ccf05cbad91dace933e69d"},
    {"sunlight_on_marble", "Sunlight on Marble", "Sunlight_on_Marble.mp3",
     180.192583, 4330697,
     "cc0018a839e3fa9b0f0775b128a9774d8cb5d13802682bda4317b80347199460"},
    {"the_concierge_desk", "The Concierge Desk",
     "The_Concierge_Desk.mp3", 180.688917, 4342609,
     "89b2308da2b26d58f7186516c592810e83660ec86944707f0885aa94057939b7"},
    {"first_light_on_marble", "First Light on Marble",
     "First_Light_on_Marble.mp3", 182.125667, 4377091,
     "06031b117eb1b1af6bd6c7f0cf72d955d07c176f38c06df97e32f577a3e51823"},
}};
}

const std::array<SoundtrackTrack, 4> &defaultSoundtrack() noexcept {
  return kTracks;
}

int defaultVolumeMci() noexcept { return 350; }

std::size_t nextTrackIndex(std::size_t current,
                           std::size_t trackCount) noexcept {
  return trackCount == 0 ? 0 : (current + 1) % trackCount;
}

std::vector<std::filesystem::path>
availableSoundtrackFiles(const std::filesystem::path &root) {
  std::vector<std::filesystem::path> result;
  result.reserve(kTracks.size());
  for (const auto &track : kTracks) {
    const auto candidate = root / track.filename;
    std::error_code error;
    if (std::filesystem::is_regular_file(candidate, error) && !error)
      result.push_back(candidate);
  }
  return result;
}

} // namespace hh::audio
