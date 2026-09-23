#include "hh/game/ServiceLogistics.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

int main() {
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
}
