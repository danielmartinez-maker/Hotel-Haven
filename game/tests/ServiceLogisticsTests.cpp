#include "hh/game/ServiceLogistics.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

static std::vector<std::string> splitLines(const std::string &encoded) {
  std::vector<std::string> lines;
  std::istringstream input(encoded);
  for (std::string line; std::getline(input, line);)
    lines.push_back(std::move(line));
  return lines;
}

static std::string joinLines(const std::vector<std::string> &lines) {
  std::ostringstream output;
  for (const auto &line : lines)
    output << line << '\n';
  return output.str();
}

static std::size_t findSection(const std::vector<std::string> &lines,
                               const char *prefix) {
  for (std::size_t index = 0; index < lines.size(); ++index)
    if (lines[index].rfind(prefix, 0) == 0)
      return index;
  throw std::runtime_error("service test section missing");
}

static void requireLoadRejected(const std::string &encoded,
                                const char *message) {
  bool rejected = false;
  try {
    (void)ServiceLogisticsRuntime::load(encoded);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, message);
}

int main() {
  {
    bool rejected = false;
    try {
      ServiceLogisticsRuntime bounded(98);
      bounded.tickSeconds(32LL * 86400LL);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected,
            "FINAL-04 accepted a single time advance beyond the execution budget");
  }

  ServiceLogisticsRuntime original(99);
  auto &logistics = original.logistics();
  const auto clean = logistics.firstStorage(StorageKind::CleanLinen);
  const auto dirty = logistics.firstStorage(StorageKind::DirtyLinen);
  const auto closet = logistics.firstStorage(StorageKind::FloorCloset);
  const auto central = logistics.firstStorage(StorageKind::CentralStorage);
  require(logistics.addInventory(clean, "clean_linen_set", 5), "seed clean linen");
  require(logistics.addInventory(dirty, "dirty_linen_set", 8), "seed dirty linen");
  require(logistics.addInventory(closet, "towel_unit", 12), "seed towels");
  require(logistics.addInventory(closet, "amenity_kit", 6), "seed amenities");
  require(logistics.addInventory(closet, "cleaning_chemical", 6), "seed chemicals");
  require(logistics.addInventory(central, "maintenance_part", 4), "seed parts");

  constexpr RoomId room = 101;
  constexpr AssetId asset = 9001;
  original.registerRoom(room);
  original.registerAsset(asset, 4500);
  require(original.requestRoomTurn(room) != 0, "room turn not created");
  require(original.requestLaundryBatch(5) != 0, "laundry batch not created");
  require(original.createWorkOrder(asset, WorkOrderType::Preventive) != 0,
          "work order not created");
  RoomServiceOrder roomServiceOrder;
  const auto service = original.placeRoomServiceOrder(7001, roomServiceOrder);
  require(service != 0 && original.markRoomServiceProductionReady(service),
          "room service production handoff failed");

  original.tickSeconds(600);
  require(original.requestRoomServiceTrayPickup(service), "tray pickup not requested");
  original.tickSeconds(60);

  const auto encoded = original.save();
  auto restored = ServiceLogisticsRuntime::load(encoded);
  require(restored.save() == encoded, "service save did not round-trip exactly");

  original.tickSeconds(5000);
  restored.tickSeconds(5000);
  require(original.save() == restored.save(), "loaded service continuation diverged");
  require(original.laundry().totalLinenUnits() == 13,
          "active service chains minted or lost linen across save/load");

  ServiceLogisticsRuntime retirement(100);
  auto &retirementLogistics = retirement.logistics();
  require(retirementLogistics.addInventory(
              retirementLogistics.firstStorage(StorageKind::CleanLinen),
              "clean_linen_set", 2),
          "seed retirement clean linen");
  require(retirementLogistics.addInventory(
              retirementLogistics.firstStorage(StorageKind::FloorCloset),
              "towel_unit", 4),
          "seed retirement towels");
  require(retirementLogistics.addInventory(
              retirementLogistics.firstStorage(StorageKind::FloorCloset),
              "amenity_kit", 2),
          "seed retirement amenities");
  require(retirementLogistics.addInventory(
              retirementLogistics.firstStorage(StorageKind::FloorCloset),
              "cleaning_chemical", 2),
          "seed retirement chemicals");
  require(retirementLogistics.addInventory(
              retirementLogistics.firstStorage(StorageKind::CentralStorage),
              "maintenance_part", 2),
          "seed retirement parts");

  constexpr RoomId retiringRoom = 202;
  retirement.registerRoom(retiringRoom);
  retirement.registerAsset(retiringRoom, 8000);
  require(retirement.requestRoomTurn(retiringRoom) != 0,
          "retirement room turn not created");
  require(retirement.createWorkOrder(retiringRoom, WorkOrderType::Preventive) != 0,
          "retirement work order not created");
  require(!retirement.retireRoomAndAsset(retiringRoom),
          "room retired while FINAL-04 work was active");

  retirement.tickSeconds(2400);
  require(retirement.retireRoomAndAsset(retiringRoom),
          "completed FINAL-04 room state could not retire");
  require(retirement.requestRoomTurn(retiringRoom) == 0,
          "retired room remained registered with housekeeping");
  require(retirement.createWorkOrder(retiringRoom, WorkOrderType::Preventive) == 0,
          "retired room remained registered as an engineering asset");

  const auto retiredState = retirement.save();
  require(ServiceLogisticsRuntime::load(retiredState).save() == retiredState,
          "retired FINAL-04 state did not round-trip");

  ServiceLogisticsRuntime integrity(101);
  integrity.registerRoom(303);
  integrity.registerAsset(303, 9000);
  require(integrity.requestRoomTurn(303) != 0,
          "integrity housekeeping job not created");
  require(integrity.createWorkOrder(303, WorkOrderType::Preventive) != 0,
          "integrity engineering work order not created");
  integrity.tickSeconds(7);
  const auto integrityState = integrity.save();

  {
    auto lines = splitLines(integrityState);
    const auto h = findSection(lines, "H ");
    std::istringstream header(lines[h]);
    std::string tag;
    std::uint64_t nextId{};
    std::int64_t elapsed{};
    header >> tag >> nextId >> elapsed;
    require(static_cast<bool>(header), "could not parse housekeeping header");
    std::ostringstream changed;
    changed << tag << ' ' << nextId << ' ' << elapsed + 1;
    lines[h] = changed.str();
    requireLoadRejected(joinLines(lines),
                        "FINAL-04 accepted a drifting subsystem clock");
  }

  {
    auto lines = splitLines(integrityState);
    const auto h = findSection(lines, "H ");
    const auto roomCount = static_cast<std::size_t>(
        std::stoull(lines.at(h + 1)));
    const auto jobCountLine = h + 2 + roomCount;
    require(std::stoull(lines.at(jobCountLine)) > 0,
            "integrity housekeeping job record missing");
    const auto jobLine = jobCountLine + 1;
    std::istringstream record(lines.at(jobLine));
    std::uint64_t jobId{}, roomId{};
    int stage{}, remaining{}, block{}, started{};
    record >> jobId >> roomId >> stage >> remaining >> block >> started;
    require(static_cast<bool>(record), "could not parse housekeeping job");
    std::ostringstream changed;
    changed << jobId << " 999999 " << stage << ' ' << remaining << ' '
            << block << ' ' << started;
    lines[jobLine] = changed.str();
    requireLoadRejected(joinLines(lines),
                        "FINAL-04 accepted an orphan housekeeping job");
  }

  {
    auto lines = splitLines(integrityState);
    const auto e = findSection(lines, "E ");
    const auto assetCount = static_cast<std::size_t>(
        std::stoull(lines.at(e + 2)));
    const auto workCountLine = e + 3 + assetCount;
    require(std::stoull(lines.at(workCountLine)) > 0,
            "integrity engineering work-order record missing");
    const auto orderLine = workCountLine + 1;
    std::istringstream record(lines.at(orderLine));
    std::uint64_t orderId{}, assetId{};
    int type{}, stage{}, remaining{}, block{}, partClaimed{};
    record >> orderId >> assetId >> type >> stage >> remaining >> block >>
        partClaimed;
    require(static_cast<bool>(record), "could not parse engineering work order");
    std::ostringstream changed;
    changed << orderId << " 999999 " << type << ' ' << stage << ' '
            << remaining << ' ' << block << ' ' << partClaimed;
    lines[orderLine] = changed.str();
    requireLoadRejected(joinLines(lines),
                        "FINAL-04 accepted an orphan engineering work order");
  }

  {
    auto lines = splitLines(integrityState);
    const auto h = findSection(lines, "H ");
    std::istringstream header(lines[h]);
    std::string tag;
    std::uint64_t nextId{};
    std::int64_t elapsed{};
    header >> tag >> nextId >> elapsed;
    const auto roomCount = static_cast<std::size_t>(
        std::stoull(lines.at(h + 1)));
    const auto jobCountLine = h + 2 + roomCount;
    const auto jobLine = jobCountLine + 1;
    std::istringstream record(lines.at(jobLine));
    std::uint64_t jobId{}, roomId{};
    int stage{}, remaining{}, block{}, started{};
    record >> jobId >> roomId >> stage >> remaining >> block >> started;
    require(static_cast<bool>(header) && static_cast<bool>(record),
            "could not parse allocator collision fixture");
    std::ostringstream changed;
    changed << nextId << ' ' << roomId << ' ' << stage << ' ' << remaining
            << ' ' << block << ' ' << started;
    lines[jobLine] = changed.str();
    requireLoadRejected(joinLines(lines),
                        "FINAL-04 accepted an ID at the allocator frontier");
  }
}
