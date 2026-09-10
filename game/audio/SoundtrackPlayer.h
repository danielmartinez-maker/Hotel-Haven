#pragma once

#include <atomic>
#include <filesystem>
#include <thread>

namespace hh::audio {

class SoundtrackPlayer {
public:
  SoundtrackPlayer() noexcept;
  ~SoundtrackPlayer();

  SoundtrackPlayer(const SoundtrackPlayer &) = delete;
  SoundtrackPlayer &operator=(const SoundtrackPlayer &) = delete;

  bool start(const std::filesystem::path &soundtrackDirectory) noexcept;
  void stop() noexcept;
  bool active() const noexcept { return active_.load(); }

private:
  void run() noexcept;

  std::filesystem::path soundtrackDirectory_;
  std::thread worker_;
  std::atomic_bool stopRequested_{false};
  std::atomic_bool active_{false};
};

} // namespace hh::audio
