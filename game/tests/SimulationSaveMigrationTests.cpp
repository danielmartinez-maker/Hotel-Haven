#include "hh/game/Simulation.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

// Build a small, valid old save from the canonical current empty save. The
// downgrade removes only sections absent from that historical version. This
// keeps fixture provenance deterministic while exercising the real base
// serializer for map, economy, RNG, and service state.
std::string legacyFixture(int version) {
  if (version < 2 || version > 9)
    throw std::invalid_argument("legacy fixture version out of range");
  auto data = Simulation(8102, 8, 8, 1).save();

  const auto headerEnd = data.find('\n');
  std::istringstream header(data.substr(0, headerEnd));
  std::vector<std::string> fields;
  for (std::string field; header >> field;)
    fields.push_back(field);
  require(fields.size() == 17, "unexpected current save header shape");
  fields[1] = std::to_string(version);
  if (version < 5)
    fields.pop_back();
  std::ostringstream oldHeader;
  for (std::size_t index = 0; index < fields.size(); ++index)
    oldHeader << (index == 0 ? "" : " ") << fields[index];
  data.replace(0, headerEnd, oldHeader.str());

  if (version < 9) {
    const auto finalSection = data.find("FINAL04 ");
    require(finalSection != std::string::npos,
            "canonical save has no service section");
    if (version < 8) {
      const auto previousLineEnd = data.rfind('\n', finalSection - 2);
      const auto workforceLineStart = previousLineEnd + 1;
      data.erase(workforceLineStart, finalSection - workforceLineStart);
      // Preserve the service marker location after removing the workforce row.
      const auto newFinalSection = data.find("FINAL04 ");
      data.resize(newFinalSection);
    } else {
      data.resize(finalSection);
    }
  } else {
    const auto guestSection = data.find("GUEST9 ");
    if (guestSection != std::string::npos)
      data.resize(guestSection);
  }
  return data;
}

Simulation groupHotel(std::uint64_t seed) {
  auto simulation = Simulation::tutorial(seed);
  for (const auto &room : simulation.view().rooms)
    require(simulation.closeRoom(room.id, true).ok,
            "could not close the tutorial rooms");
  require(simulation.loadDefinitions(
              R"({"baseDemand":100,"checkInWorkSeconds":1,"turnoverWorkSeconds":1,"repairWorkSeconds":1,"roomConditionLossPerDay":0,"initialLinen":400,"initialTowels":500,"initialAmenities":200,"initialChemicals":200})")
              .ok,
          "guest-save test definitions rejected");
  for (int x : {4, 12, 20})
    require(simulation.buildFurnishedRoom(
                         {"Save group " + std::to_string(x), 2, x, 9, 6, 6,
                          {2, x, 9}, 2, 1, 140})
                .ok,
            "could not build a guest-save test room");
  simulation.step(3600);
  return simulation;
}

Simulation guestWithRecordedMemory() {
  for (std::uint64_t seed = 1; seed <= 16; ++seed) {
    auto simulation = groupHotel(seed);
    auto view = simulation.view();
    const auto guest = std::find_if(view.guests.begin(), view.guests.end(),
                                    [](const GuestView &candidate) {
                                      return candidate.groupId != 0;
                                    });
    if (guest == view.guests.end())
      continue;
    GuestExperienceEvent event;
    event.guestId = guest->profile.id;
    event.type = GuestExperienceEventType::DirtyBathroom;
    event.locationId = guest->reservationId;
    for (const auto &reservation : view.reservations)
      if (reservation.id == guest->reservationId)
        event.locationId = reservation.roomId;
    event.category = GuestCategory::Cleanliness;
    event.observedValue = 5;
    event.expectedValue = 75;
    event.rawImpact = -48;
    event.salience = 0.95;
    event.complaintEligible = true;
    event.reviewStatement = "The bathroom was not clean.";
    require(simulation.reportGuestExperience(event).ok,
            "failed to record the test guest memory");
    return simulation;
  }
  throw std::runtime_error("no group guest was generated for save test");
}

void every_legacy_version_migrates_to_v10() {
  for (int version = 2; version <= 9; ++version) {
    const auto migrated = Simulation::load(legacyFixture(version)).save();
    require(migrated.rfind("HHGS 10 ", 0) == 0,
            "legacy save did not migrate to the v10 writer");
  }
}

void v10_roundtrip_preserves_guest_group_memories_and_random_state() {
  auto simulation = guestWithRecordedMemory();
  const auto saved = simulation.save();
  require(saved.rfind("HHGS 10 ", 0) == 0,
          "simulation is not writing save version 10");
  const auto loaded = Simulation::load(saved);
  require(loaded.save() == saved,
          "v10 guest/group/memory state changed during round-trip");
  auto continued = Simulation::load(saved);
  auto uninterrupted = Simulation::load(saved);
  continued.step(1800);
  uninterrupted.step(1800);
  require(continued.save() == uninterrupted.save(),
          "guest random continuation changed after v10 restore");
}

void corrupt_guest_references_are_rejected() {
  auto data = guestWithRecordedMemory().save();
  const auto payload = data.find("GUEST10 ");
  require(payload != std::string::npos,
          "v10 guest section was not emitted");
  const auto payloadStart = data.find('\n', payload) + 1;
  const auto countEnd = data.find('\n', payloadStart) + 1;
  const auto firstGuest = data.find('\n', countEnd) + 1;
  const auto firstSpace = data.find(' ', firstGuest);
  require(firstSpace != std::string::npos,
          "v10 guest record metadata is missing");
  data.replace(firstGuest, firstSpace - firstGuest, "18446744073709551614");
  bool rejected = false;
  try {
    (void)Simulation::load(data);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "unknown saved guest identity was accepted");
}

void guest_collection_limits_are_enforced() {
  auto data = guestWithRecordedMemory().save();
  const auto payload = data.find("GUEST10 ");
  require(payload != std::string::npos,
          "v10 guest section was not emitted");
  const auto payloadStart = data.find('\n', payload) + 1;
  const auto countEnd = data.find('\n', payloadStart);
  data.replace(payloadStart, countEnd - payloadStart, "100001");
  bool rejected = false;
  try {
    (void)Simulation::load(data);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, "oversized guest collection was accepted");
}

void legacy_review_scores_migrate_to_hmg_rating() {
  bool checked = false;
  for (std::uint64_t seed = 1; seed <= 16 && !checked; ++seed) {
    auto simulation = groupHotel(seed);
    simulation.step(5 * 86400);
    auto view = simulation.view();
    if (view.reviews.empty())
      continue;
    auto legacy = simulation.save();
    const auto guestSection = legacy.find("GUEST10 ");
    require(guestSection != std::string::npos,
            "v10 save has no length-bounded guest section");
    legacy.resize(guestSection);
    const auto loaded = Simulation::load(legacy);
    const auto migrated = loaded.view();
    require(!migrated.reviews.empty(), "legacy review was lost");
    require(std::abs(migrated.reviews.front().rating -
                     migrated.reviews.front().score / 10.0) < 0.001,
            "legacy 0-100 review score was not converted to 1-10 rating");
    checked = true;
  }
  require(checked, "no review was generated for legacy rating migration");
}

void legacy_migration_does_not_consume_shared_rng() {
  const auto legacy = legacyFixture(9);
  const auto expectedRngStart = legacy.find('\n') + 1;
  const auto expectedRngEnd = legacy.find('\n', expectedRngStart);
  const auto expectedRng =
      legacy.substr(expectedRngStart, expectedRngEnd - expectedRngStart);
  const auto migrated = Simulation::load(legacy).save();
  const auto actualRngStart = migrated.find('\n') + 1;
  const auto actualRngEnd = migrated.find('\n', actualRngStart);
  const auto actualRng =
      migrated.substr(actualRngStart, actualRngEnd - actualRngStart);
  require(actualRng == expectedRng,
          "legacy guest migration advanced the shared simulation RNG");
}
} // namespace

int main() {
  try {
    every_legacy_version_migrates_to_v10();
    v10_roundtrip_preserves_guest_group_memories_and_random_state();
    corrupt_guest_references_are_rejected();
    guest_collection_limits_are_enforced();
    legacy_review_scores_migrate_to_hmg_rating();
    legacy_migration_does_not_consume_shared_rng();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All guest save migration tests passed\n";
}

