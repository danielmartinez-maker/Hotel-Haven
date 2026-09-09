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
}
