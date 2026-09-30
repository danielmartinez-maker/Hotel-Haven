#include "hh/game/Simulation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
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
    const auto guestSection = data.find("GUEST10 ");
    if (guestSection != std::string::npos)
      data.resize(guestSection);
  }
  return data;
}

std::string asLegacyV9(std::string data) {
  const auto headerEnd = data.find('\n');
  auto header = data.substr(0, headerEnd);
  const auto versionStart = header.find(' ') + 1;
  const auto versionEnd = header.find(' ', versionStart);
  header.replace(versionStart, versionEnd - versionStart, "9");
  data.replace(0, headerEnd, header);

  const auto serviceStart = data.find("FINAL04 ");
  require(serviceStart != std::string::npos,
          "v10 save has no service section for legacy review fixture");
  const auto extensionStart = data.find("GUEST10 ", serviceStart);
  require(extensionStart != std::string::npos,
          "v10 save has no guest section for legacy review fixture");
  auto base = data.substr(0, serviceStart);
  std::vector<std::string> lines;
  std::istringstream lineInput(base);
  for (std::string line; std::getline(lineInput, line);)
    lines.push_back(std::move(line));
  std::size_t lineIndex = 5;
  const auto skipSection = [&](std::size_t &index) {
    require(index < lines.size(), "legacy review fixture ended early");
    const auto count = static_cast<std::size_t>(std::stoull(lines[index]));
    require(count <= lines.size() - index - 1,
            "legacy review fixture section is truncated");
    index += count + 1;
  };
  skipSection(lineIndex); // Rooms.
  skipSection(lineIndex); // People.
  skipSection(lineIndex); // Reservations.
  skipSection(lineIndex); // Tasks.
  require(lineIndex < lines.size(), "legacy review section is missing");
  const auto reviewCount =
      static_cast<std::size_t>(std::stoull(lines[lineIndex]));
  std::size_t reviewDataOffset{};
  for (std::size_t index = 0; index <= lineIndex; ++index)
    reviewDataOffset += lines[index].size() + 1;
  std::istringstream reviewInput(base.substr(reviewDataOffset));
  std::vector<std::string> oldReviews;
  oldReviews.reserve(reviewCount);
  for (std::size_t index = 0; index < reviewCount; ++index) {
    EntityId reservationId{};
    int day{}, score{};
    double rating{}, satisfaction{};
    std::string reviewText;
    reviewInput >> reservationId >> day >> score >> rating >> satisfaction >>
        std::quoted(reviewText);
    if (!reviewInput)
      throw std::runtime_error("v10 review row could not be downgraded");
    std::ostringstream oldReview;
    oldReview << reservationId << ' ' << day << ' ' << score << ' '
              << std::quoted(reviewText);
    oldReviews.push_back(oldReview.str());
  }
  reviewInput >> std::ws;
  const auto remainingOffset = reviewInput.tellg();
  const auto remainder = remainingOffset < 0
                             ? std::string{}
                             : base.substr(reviewDataOffset +
                                           static_cast<std::size_t>(remainingOffset));
  std::ostringstream oldBase;
  for (std::size_t index = 0; index < lineIndex; ++index)
    oldBase << lines[index] << '\n';
  oldBase << reviewCount << '\n';
  for (const auto &review : oldReviews)
    oldBase << review << '\n';
  oldBase << remainder;
  oldBase << data.substr(serviceStart, extensionStart - serviceStart);
  return oldBase.str();
}

void adjustGuestPayloadLength(std::string &data, std::ptrdiff_t delta) {
  const auto marker = data.find("GUEST10 ");
  require(marker != std::string::npos, "v10 guest section was not emitted");
  const auto lineEnd = data.find('\n', marker);
  std::istringstream sectionHeader(data.substr(marker, lineEnd - marker));
  std::string tag;
  int version{};
  std::size_t size{};
  sectionHeader >> tag >> version >> size;
  require(static_cast<bool>(sectionHeader) && tag == "GUEST10" && version == 1,
          "v10 guest section header is invalid");
  const auto replacement = std::string("GUEST10 1 ") +
                           std::to_string(static_cast<std::size_t>(
                               static_cast<std::ptrdiff_t>(size) + delta));
  data.replace(marker, lineEnd - marker, replacement);
}

std::string constructionPayload(const std::string &data) {
  const auto marker = data.find("CONSTRUCTION11 ");
  require(marker != std::string::npos,
          "v11 construction section was not emitted");
  const auto headerEnd = data.find('\n', marker);
  std::istringstream header(data.substr(marker, headerEnd - marker));
  std::string tag;
  int version{};
  std::size_t byteCount{};
  header >> tag >> version >> byteCount;
  require(header && tag == "CONSTRUCTION11" && version == 1,
          "v11 construction section header is invalid");
  return data.substr(headerEnd + 1, byteCount);
}

void replaceConstructionPayload(std::string &data, const std::string &payload) {
  const auto marker = data.find("CONSTRUCTION11 ");
  require(marker != std::string::npos,
          "v11 construction section was not emitted");
  const auto headerEnd = data.find('\n', marker);
  std::istringstream header(data.substr(marker, headerEnd - marker));
  std::string tag;
  int version{};
  std::size_t oldSize{};
  header >> tag >> version >> oldSize;
  require(header && tag == "CONSTRUCTION11" && version == 1,
          "v11 construction section header is invalid");
  data.replace(marker, headerEnd - marker,
               "CONSTRUCTION11 1 " + std::to_string(payload.size()));
  const auto payloadStart = marker +
                            std::string("CONSTRUCTION11 1 ").size() +
                            std::to_string(payload.size()).size() + 1;
  data.replace(payloadStart, oldSize, payload);
}

void replaceConstructionObjectToken(std::string &data, std::size_t row,
                                    std::size_t token,
                                    const std::string &replacement) {
  auto payload = constructionPayload(data);
  const auto marker = payload.find("objects ");
  require(marker != std::string::npos,
          "construction payload is missing its objects table");
  auto lineEnd = payload.find('\n', marker);
  const auto count = static_cast<std::size_t>(
      std::stoull(payload.substr(marker + 8, lineEnd - marker - 8)));
  require(row < count, "construction object row is outside the table");
  std::size_t rowStart = lineEnd + 1;
  for (std::size_t index = 0; index < row; ++index) {
    rowStart = payload.find('\n', rowStart) + 1;
    require(rowStart != 0, "construction object row is truncated");
  }
  lineEnd = payload.find('\n', rowStart);
  std::vector<std::pair<std::size_t, std::size_t>> spans;
  for (std::size_t position = rowStart; position < lineEnd;) {
    while (position < lineEnd && payload[position] == ' ')
      ++position;
    if (position == lineEnd)
      break;
    const auto start = position;
    while (position < lineEnd && payload[position] != ' ')
      ++position;
    spans.emplace_back(start, position - start);
  }
  require(token < spans.size(), "construction object record is truncated");
  const auto [start, length] = spans[token];
  payload.replace(start, length, replacement);
  replaceConstructionPayload(data, payload);
}

void replaceConstructionEdgeToken(std::string &data, const char *sectionName,
                                  std::size_t row, std::size_t token,
                                  const std::string &replacement) {
  auto payload = constructionPayload(data);
  const auto marker = payload.find(std::string(sectionName) + " ");
  require(marker != std::string::npos,
          "construction payload is missing its edge table");
  auto lineEnd = payload.find('\n', marker);
  const auto countStart = marker + std::string(sectionName).size() + 1;
  const auto count = static_cast<std::size_t>(std::stoull(
      payload.substr(countStart, lineEnd - countStart)));
  require(row < count, "construction edge row is outside the table");
  std::size_t rowStart = lineEnd + 1;
  for (std::size_t index = 0; index < row; ++index) {
    rowStart = payload.find('\n', rowStart) + 1;
    require(rowStart != 0, "construction edge row is truncated");
  }
  lineEnd = payload.find('\n', rowStart);
  std::vector<std::pair<std::size_t, std::size_t>> spans;
  for (std::size_t position = rowStart; position < lineEnd;) {
    while (position < lineEnd && payload[position] == ' ')
      ++position;
    if (position == lineEnd)
      break;
    const auto start = position;
    while (position < lineEnd && payload[position] != ' ')
      ++position;
    spans.emplace_back(start, position - start);
  }
  require(token < spans.size(), "construction edge record is truncated");
  const auto [start, length] = spans[token];
  payload.replace(start, length, replacement);
  replaceConstructionPayload(data, payload);
}

void replaceConstructionDeclaredBytes(std::string &data, std::size_t size) {
  const auto marker = data.find("CONSTRUCTION11 ");
  require(marker != std::string::npos,
          "v11 construction section was not emitted");
  const auto headerEnd = data.find('\n', marker);
  data.replace(marker, headerEnd - marker,
               "CONSTRUCTION11 1 " + std::to_string(size));
}

std::string asLegacyV10(std::string data) {
  const auto versionStart = data.find(' ') + 1;
  const auto versionEnd = data.find(' ', versionStart);
  require(data.substr(versionStart, versionEnd - versionStart) == "11",
          "only a v11 save can be downgraded to the v10 fixture");
  data.replace(versionStart, versionEnd - versionStart, "10");
  const auto constructionStart = data.find("CONSTRUCTION11 ");
  require(constructionStart != std::string::npos,
          "v11 fixture has no construction section");
  data.resize(constructionStart);
  return data;
}

void replaceLegacyTile(std::string &data, int width, int height, int floor,
                       int x, int y, int kind) {
  const auto headerEnd = data.find('\n');
  const auto rngEnd = data.find('\n', headerEnd + 1);
  const auto tileLineStart = rngEnd + 1;
  const auto tileLineEnd = data.find('\n', tileLineStart);
  std::size_t position = tileLineStart;
  std::size_t tileCount{};
  while (position < tileLineEnd && data[position] != ' ')
    ++position;
  tileCount = static_cast<std::size_t>(
      std::stoull(data.substr(tileLineStart, position - tileLineStart)));
  const auto index = (static_cast<std::size_t>(floor) * height + y) * width + x;
  require(index < tileCount, "legacy tile index is outside its map");
  for (std::size_t item = 0; item <= index; ++item) {
    while (position < tileLineEnd && data[position] == ' ')
      ++position;
    require(position < tileLineEnd, "legacy tile row is truncated");
    const auto start = position;
    while (position < tileLineEnd && data[position] != ' ')
      ++position;
    if (item == index) {
      data.replace(start, position - start, std::to_string(kind));
      return;
    }
  }
}

Simulation constructionFixture(std::uint64_t seed = 8140) {
  Simulation simulation(seed, 16, 12, 1);
  for (int x = 0; x <= 8; ++x)
    require(simulation.buildTile({0, x, 1},
                                 x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "construction migration corridor failed");
  require(simulation
              .buildFurnishedRoom({"Corner", 0, 2, 2, 6, 6, {0, 2, 2}, 1, 1,
                                   120})
              .ok,
          "construction migration room failed");
  return simulation;
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

void every_legacy_version_migrates_to_v11() {
  for (int version = 2; version <= 9; ++version) {
    const auto migrated = Simulation::load(legacyFixture(version)).save();
    require(migrated.rfind("HHGS 11 ", 0) == 0,
            "legacy save did not migrate to the v11 writer");
  }
  const auto migrated = Simulation::load(asLegacyV10(constructionFixture().save())).save();
  require(migrated.rfind("HHGS 11 ", 0) == 0,
          "v10 save did not migrate to the v11 writer");
}

void v11_roundtrip_preserves_guest_group_memories_and_random_state() {
  auto simulation = guestWithRecordedMemory();
  const auto saved = simulation.save();
  require(saved.rfind("HHGS 11 ", 0) == 0,
          "simulation is not writing save version 11");
  const auto loaded = Simulation::load(saved);
  require(loaded.save() == saved,
          "v11 guest/group/memory state changed during round-trip");
  auto continued = Simulation::load(saved);
  auto uninterrupted = Simulation::load(saved);
  continued.step(1800);
  uninterrupted.step(1800);
  require(continued.save() == uninterrupted.save(),
          "guest random continuation changed after v11 restore");
}

void v10_corner_door_migrates_deterministically() {
  const auto legacy = asLegacyV10(constructionFixture().save());
  const auto loaded = Simulation::load(legacy);
  require(loaded.isReachable({0, 2, 1}, {0, 2, 2}),
          "v10 corner room door was not migrated to the north boundary");
  require(loaded.save() == Simulation::load(legacy).save(),
          "v10 construction migration is not deterministic");
}

void v10_wall_tiles_become_blocking_edges() {
  auto legacy = asLegacyV10(constructionFixture().save());
  replaceLegacyTile(legacy, 16, 12, 0, 4, 1, 2);
  const auto loaded = Simulation::load(legacy);
  require(!loaded.isReachable({0, 1, 1}, {0, 7, 1}),
          "legacy wall tile did not become a blocking construction edge");
}

void v10_door_tiles_become_deterministic_edges() {
  auto legacy = asLegacyV10(constructionFixture().save());
  replaceLegacyTile(legacy, 16, 12, 0, 4, 1, 3);
  const auto migrated = Simulation::load(legacy).save();
  const auto payload = constructionPayload(migrated);
  require(payload.find("0 4 1 1\n") != std::string::npos,
          "freestanding legacy door tile did not migrate to its north edge");
}

void v11_roundtrip_preserves_construction_and_room_ids() {
  auto simulation = constructionFixture();
  require(simulation.setConstructionWall({0, 9, 1, EdgeAxis::Vertical}, true).ok,
          "could not add a hallway wall for save fixture");
  require(simulation.setConstructionDoor({0, 9, 1, EdgeAxis::Vertical}, true).ok,
          "could not add a hallway door for save fixture");
  const auto plant = simulation.placeConstructionObject(
      ConstructionObjectKind::Plant, {0, 4, 4});
  require(plant.ok, "could not place a save-fixture construction object");
  const auto saved = simulation.save();
  require(saved.rfind("HHGS 11 ", 0) == 0,
          "construction save is not using HHGS v11");
  const auto payload = constructionPayload(saved);
  require(payload.find("\nwalls ") != std::string::npos &&
              payload.find("\ndoors ") != std::string::npos &&
              payload.find("\nobjects ") != std::string::npos,
          "construction extension omitted canonical topology or objects");
  const auto loaded = Simulation::load(saved);
  require(loaded.save() == saved,
          "v11 construction state changed during round-trip");
  const auto originalRooms = simulation.view().rooms;
  const auto loadedRooms = loaded.view().rooms;
  require(originalRooms.size() == loadedRooms.size(),
          "v11 round-trip changed the room count");
  for (std::size_t index = 0; index < originalRooms.size(); ++index)
    require(originalRooms[index].id == loadedRooms[index].id,
            "v11 round-trip changed a room identity");
}

void v11_rejects_duplicate_object_ids_and_invalid_footprints() {
  auto simulation = constructionFixture();
  require(simulation.placeConstructionObject(ConstructionObjectKind::Plant,
                                               {0, 4, 4}).ok &&
              simulation.placeConstructionObject(ConstructionObjectKind::Plant,
                                                  {0, 5, 4}).ok,
          "could not place construction objects for corruption fixtures");
  const auto saved = simulation.save();
  const auto payload = constructionPayload(saved);
  const auto objects = payload.find("objects ");
  require(objects != std::string::npos,
          "construction object table was not emitted");
  const auto objectLineEnd = payload.find('\n', objects);
  const auto objectCount = static_cast<std::size_t>(std::stoull(
      payload.substr(objects + 8, objectLineEnd - objects - 8)));
  require(objectCount >= 2, "construction object table is too small");
  const auto firstLine = objectLineEnd + 1;
  const auto idEnd = payload.find(' ', firstLine);
  require(idEnd != std::string::npos,
          "construction object identity was not emitted");
  const auto firstId = payload.substr(firstLine, idEnd - firstLine);

  auto duplicateId = saved;
  replaceConstructionObjectToken(duplicateId, 1, 0, firstId);
  bool duplicateRejected = false;
  try {
    (void)Simulation::load(duplicateId);
  } catch (const std::invalid_argument &) {
    duplicateRejected = true;
  }
  require(duplicateRejected, "duplicate construction object ID was accepted");

  auto invalidKind = saved;
  replaceConstructionObjectToken(invalidKind, 0, 1, "999");
  bool kindRejected = false;
  try {
    (void)Simulation::load(invalidKind);
  } catch (const std::invalid_argument &) {
    kindRejected = true;
  }
  require(kindRejected, "invalid construction object kind was accepted");

  auto invalidOrientation = saved;
  replaceConstructionObjectToken(invalidOrientation, 0, 5, "4");
  bool orientationRejected = false;
  try {
    (void)Simulation::load(invalidOrientation);
  } catch (const std::invalid_argument &) {
    orientationRejected = true;
  }
  require(orientationRejected, "invalid construction object orientation was accepted");

  auto invalidFootprint = saved;
  replaceConstructionObjectToken(invalidFootprint, 0, 4, "9999");
  bool footprintRejected = false;
  try {
    (void)Simulation::load(invalidFootprint);
  } catch (const std::invalid_argument &) {
    footprintRejected = true;
  }
  require(footprintRejected, "out-of-bounds object footprint was accepted");

  auto invalidEdge = saved;
  replaceConstructionEdgeToken(invalidEdge, "walls", 0, 0, "9999");
  bool edgeRejected = false;
  try {
    (void)Simulation::load(invalidEdge);
  } catch (const std::invalid_argument &) {
    edgeRejected = true;
  }
  require(edgeRejected, "out-of-bounds construction edge was accepted");

  auto oversized = saved;
  replaceConstructionDeclaredBytes(oversized, 32 * 1024 * 1024 + 1);
  bool oversizedRejected = false;
  try {
    (void)Simulation::load(oversized);
  } catch (const std::invalid_argument &) {
    oversizedRejected = true;
  }
  require(oversizedRejected, "oversized construction payload was accepted");

  auto trailing = saved + "extra";
  bool trailingRejected = false;
  try {
    (void)Simulation::load(trailing);
  } catch (const std::invalid_argument &) {
    trailingRejected = true;
  }
  require(trailingRejected, "trailing bytes after construction payload were accepted");

  auto truncated = saved;
  truncated.pop_back();
  bool truncatedRejected = false;
  try {
    (void)Simulation::load(truncated);
  } catch (const std::invalid_argument &) {
    truncatedRejected = true;
  }
  require(truncatedRejected, "truncated construction payload was accepted");
}

void migrated_room_references_and_guest_continuation_remain_valid() {
  const auto legacy = asLegacyV10(guestWithRecordedMemory().save());
  const auto oldView = Simulation::load(legacy).view();
  const auto migrated = Simulation::load(legacy);
  const auto newView = migrated.view();
  require(oldView.rooms.size() == newView.rooms.size() &&
              oldView.reservations.size() == newView.reservations.size() &&
              oldView.reviews.size() == newView.reviews.size(),
          "v10 construction migration changed guest or room collection sizes");
  for (std::size_t index = 0; index < oldView.rooms.size(); ++index)
    require(oldView.rooms[index].id == newView.rooms[index].id,
            "v10 construction migration changed a room identity");
  for (std::size_t index = 0; index < oldView.reservations.size(); ++index)
    require(oldView.reservations[index].id == newView.reservations[index].id &&
                oldView.reservations[index].roomId ==
                    newView.reservations[index].roomId,
            "v10 construction migration broke a reservation room reference");
  require(oldView.tasks.size() == newView.tasks.size(),
          "v10 construction migration changed the task count");
  for (std::size_t index = 0; index < oldView.tasks.size(); ++index)
    require(oldView.tasks[index].id == newView.tasks[index].id &&
                oldView.tasks[index].targetId == newView.tasks[index].targetId,
            "v10 construction migration changed a task identity or reference");
  require(oldView.economy.cashCents == newView.economy.cashCents &&
              oldView.economy.revenueCents == newView.economy.revenueCents &&
              oldView.economy.payrollCents == newView.economy.payrollCents &&
              oldView.economy.supplyCostCents ==
                  newView.economy.supplyCostCents &&
              oldView.economy.constructionCostCents ==
                  newView.economy.constructionCostCents &&
              oldView.economy.utilityCostCents ==
                  newView.economy.utilityCostCents,
          "v10 construction migration changed the economic ledgers");
  require(oldView.guests.size() == newView.guests.size(),
          "v10 construction migration changed the guest count");
  for (std::size_t index = 0; index < oldView.guests.size(); ++index)
    require(oldView.guests[index].profile.id == newView.guests[index].profile.id &&
                oldView.guests[index].reservationId ==
                    newView.guests[index].reservationId,
            "v10 construction migration changed guest identities or references");
  auto continued = Simulation::load(migrated.save());
  auto uninterrupted = Simulation::load(migrated.save());
  continued.step(1800);
  uninterrupted.step(1800);
  require(continued.save() == uninterrupted.save(),
          "guest continuation changed after migrated v10 restore");
}

void corrupt_guest_references_are_rejected() {
  auto data = guestWithRecordedMemory().save();
  const auto payload = data.find("GUEST10 ");
  require(payload != std::string::npos,
          "v10 guest section was not emitted");
  const auto payloadStart = data.find('\n', payload) + 1;
  const auto countEnd = data.find('\n', payloadStart) + 1;
  const auto firstGuest = countEnd;
  const auto firstSpace = data.find(' ', firstGuest);
  require(firstSpace != std::string::npos,
          "v10 guest record metadata is missing");
  const auto previousId = data.substr(firstGuest, firstSpace - firstGuest);
  const std::string unknownId = "1";
  data.replace(firstGuest, firstSpace - firstGuest, unknownId);
  adjustGuestPayloadLength(data,
                           static_cast<std::ptrdiff_t>(unknownId.size()) -
                               static_cast<std::ptrdiff_t>(previousId.size()));
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
  const auto previousCount = data.substr(payloadStart, countEnd - payloadStart);
  data.replace(payloadStart, countEnd - payloadStart, "100001");
  adjustGuestPayloadLength(
      data, static_cast<std::ptrdiff_t>(std::string("100001").size()) -
                static_cast<std::ptrdiff_t>(previousCount.size()));
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
    auto legacy = asLegacyV9(simulation.save());
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
    every_legacy_version_migrates_to_v11();
    v10_corner_door_migrates_deterministically();
    v10_wall_tiles_become_blocking_edges();
    v10_door_tiles_become_deterministic_edges();
    v11_roundtrip_preserves_construction_and_room_ids();
    v11_rejects_duplicate_object_ids_and_invalid_footprints();
    migrated_room_references_and_guest_continuation_remain_valid();
    v11_roundtrip_preserves_guest_group_memories_and_random_state();
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

