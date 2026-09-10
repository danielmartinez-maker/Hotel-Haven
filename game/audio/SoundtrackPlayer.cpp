#include "SoundtrackPlayer.h"

#include "SoundtrackCatalog.h"

#include <array>
#include <chrono>
#include <cwchar>
#include <mmsystem.h>
#include <string>
#include <string_view>
#include <system_error>
#include <windows.h>

namespace hh::audio {
namespace {
constexpr wchar_t kAlias[] = L"hotel_haven_soundtrack";

std::wstring quoted(const std::filesystem::path &path) {
  return L"\"" + path.wstring() + L"\"";
}

MCIERROR send(const std::wstring &command, wchar_t *result = nullptr,
              unsigned int resultLength = 0) {
  return mciSendStringW(command.c_str(), result, resultLength, nullptr);
}

bool isSmokeTest() {
  const wchar_t *commandLine = GetCommandLineW();
  return commandLine != nullptr &&
         std::wcsstr(commandLine, L"--smoke-test") != nullptr;
}

std::filesystem::path defaultSoundtrackDirectory() {
  std::array<wchar_t, 32768> executablePath{};
  const DWORD length = GetModuleFileNameW(
      nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
  if (length == 0 || length >= executablePath.size())
    return {};
  return std::filesystem::path(executablePath.data()).parent_path() / L"data" /
         L"audio";
}
} // namespace

SoundtrackPlayer::SoundtrackPlayer() noexcept {
  if (!isSmokeTest()) {
    const auto directory = defaultSoundtrackDirectory();
    if (!directory.empty())
      start(directory);
  }
}

SoundtrackPlayer::~SoundtrackPlayer() { stop(); }

bool SoundtrackPlayer::start(
    const std::filesystem::path &soundtrackDirectory) noexcept {
  stop();
  soundtrackDirectory_ = soundtrackDirectory;
  stopRequested_.store(false);
  active_.store(false);
  try {
    worker_ = std::thread(&SoundtrackPlayer::run, this);
  } catch (...) {
    soundtrackDirectory_.clear();
    return false;
  }
  return true;
}

void SoundtrackPlayer::stop() noexcept {
  stopRequested_.store(true);
  if (worker_.joinable())
    worker_.join();
  active_.store(false);
  soundtrackDirectory_.clear();
}

void SoundtrackPlayer::run() noexcept {
  const auto &tracks = defaultSoundtrack();
  if (tracks.empty())
    return;

  std::size_t current = 0;
  std::size_t consecutiveFailures = 0;
  while (!stopRequested_.load() && consecutiveFailures < tracks.size()) {
    const auto path = soundtrackDirectory_ / tracks[current].filename;
    std::error_code fileError;
    if (!std::filesystem::is_regular_file(path, fileError) || fileError) {
      ++consecutiveFailures;
      current = nextTrackIndex(current, tracks.size());
      continue;
    }

    const std::wstring openCommand = L"open " + quoted(path) +
                                     L" type mpegvideo alias " + kAlias;
    if (send(openCommand) != 0) {
      ++consecutiveFailures;
      current = nextTrackIndex(current, tracks.size());
      continue;
    }

    send(std::wstring(L"setaudio ") + kAlias + L" volume to " +
         std::to_wstring(defaultVolumeMci()));
    if (send(std::wstring(L"play ") + kAlias) != 0) {
      send(std::wstring(L"close ") + kAlias);
      ++consecutiveFailures;
      current = nextTrackIndex(current, tracks.size());
      continue;
    }

    consecutiveFailures = 0;
    active_.store(true);
    while (!stopRequested_.load()) {
      std::array<wchar_t, 32> mode{};
      if (send(std::wstring(L"status ") + kAlias + L" mode", mode.data(),
               static_cast<unsigned int>(mode.size())) != 0 ||
          std::wstring_view(mode.data()) != L"playing")
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (stopRequested_.load())
      send(std::wstring(L"stop ") + kAlias);
    send(std::wstring(L"close ") + kAlias);
    active_.store(false);
    current = nextTrackIndex(current, tracks.size());
  }
}

} // namespace hh::audio
