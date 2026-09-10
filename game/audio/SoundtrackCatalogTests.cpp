#include "SoundtrackCatalog.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {
int failures = 0;

void expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    ++failures;
  }
}

void testDefaultPlaylistOrderAndMetadata() {
  const auto &tracks = hh::audio::defaultSoundtrack();
  expect(tracks.size() == 4, "default soundtrack contains four tracks");
  if (tracks.size() != 4)
    return;
  expect(tracks[0].title == "Morning in the Atrium", "track 1 title");
  expect(tracks[0].filename == "Morning_in_the_Atrium.mp3", "track 1 file");
  expect(tracks[1].title == "Sunlight on Marble", "track 2 title");
  expect(tracks[1].filename == "Sunlight_on_Marble.mp3", "track 2 file");
  expect(tracks[2].title == "The Concierge Desk", "track 3 title");
  expect(tracks[2].filename == "The_Concierge_Desk.mp3", "track 3 file");
  expect(tracks[3].title == "First Light on Marble", "track 4 title");
  expect(tracks[3].filename == "First_Light_on_Marble.mp3", "track 4 file");
  expect(hh::audio::defaultVolumeMci() == 350,
         "default music volume is 35 percent");
}

void testAvailableTracksSkipMissingFilesWithoutReordering() {
  const auto root = std::filesystem::temp_directory_path() /
                    "hotel_haven_soundtrack_catalog_test";
  std::error_code error;
  std::filesystem::remove_all(root, error);
  std::filesystem::create_directories(root);
  std::ofstream(root / "Morning_in_the_Atrium.mp3").put('a');
  std::ofstream(root / "The_Concierge_Desk.mp3").put('b');

  const auto files = hh::audio::availableSoundtrackFiles(root);
  expect(files.size() == 2, "only existing tracks are returned");
  if (files.size() == 2) {
    expect(files[0].filename() == "Morning_in_the_Atrium.mp3",
           "first existing track keeps playlist order");
    expect(files[1].filename() == "The_Concierge_Desk.mp3",
           "later existing track keeps playlist order");
  }
  std::filesystem::remove_all(root, error);
}

void testNextTrackWrapsDeterministically() {
  expect(hh::audio::nextTrackIndex(0, 4) == 1, "advance from first track");
  expect(hh::audio::nextTrackIndex(3, 4) == 0, "wrap after final track");
  expect(hh::audio::nextTrackIndex(9, 0) == 0, "empty playlist is safe");
}
} // namespace

int main() {
  testDefaultPlaylistOrderAndMetadata();
  testAvailableTracksSkipMissingFilesWithoutReordering();
  testNextTrackWrapsDeterministically();
  if (failures != 0) {
    std::cerr << failures << " soundtrack catalog assertion(s) failed\n";
    return 1;
  }
  std::cout << "Soundtrack catalog tests passed\n";
  return 0;
}
