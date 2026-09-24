#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace hh::audio {

struct SoundtrackTrack {
  std::string_view id;
  std::string_view title;
  std::filesystem::path filename;
  double durationSeconds{};
  std::uintmax_t expectedBytes{};
  std::string_view sha256;
};

const std::array<SoundtrackTrack, 4> &defaultSoundtrack() noexcept;
int defaultVolumeMci() noexcept;
std::size_t nextTrackIndex(std::size_t current, std::size_t trackCount) noexcept;
std::vector<std::filesystem::path>
availableSoundtrackFiles(const std::filesystem::path &root);

} // namespace hh::audio
