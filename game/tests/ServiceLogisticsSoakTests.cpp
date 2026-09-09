#include "hh/game/ServiceLogistics.h"
#include <cstdint>
#include <stdexcept>
#include <unordered_set>
#include <utility>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static void validateInvariants(const ServiceLogisticsRuntime &runtime,
                               const std::unordered_set<RoomId> &rooms,
                               int expectedLinen) {
  const auto elapsed = runtime.elapsedSeconds();
  const auto logistics = runtime.logisticsSnapshot();
  const auto housekeeping = runtime.housekeeping().snapshot();
  const auto laundry = runtime.laundry().snapshot();
  const auto engineering = runtime.engineering().snapshot();
  const auto roomService = runtime.roomService().snapshot();

  require(logistics.elapsedSeconds == elapsed, "logistics clock diverged");
  require(housekeeping.elapsedSeconds == elapsed, "housekeeping clock diverged");
  require(laundry.elapsedSeconds == elapsed, "laundry clock diverged");
  require(engineering.elapsedSeconds == elapsed, "engineering clock diverged");
  require(roomService.elapsedSeconds == elapsed, "room-service clock diverged");
  require(runtime.laundry().totalLinenUnits() == expectedLinen,
          "linen conservation invariant failed");
  require(logistics.wasteAtSources >= 0 && logistics.wasteInBackOfHouse >= 0 &&
              logistics.wasteOverflowUnits == 0,
          "waste invariant failed");

  std::unordered_set<StorageNodeId> storageIds;
  for (const auto &node : logistics.storage) {
    require(node.id != 0 && storageIds.insert(node.id).second,
            "duplicate or zero storage id");
    require(node.capacityUnits > 0 && node.usedUnits >= 0 &&
                node.reservedUnits >= 0 &&
                node.usedUnits + node.reservedUnits <= node.capacityUnits,
            "storage capacity invariant failed");
  }
  for (const auto &stack : logistics.inventory) {
    require(storageIds.contains(stack.storage),
            "inventory references missing storage");
    require(stack.quantity >= 0 && stack.reservedQuantity >= 0 &&
                stack.reservedQuantity <= stack.quantity,
            "inventory quantity invariant failed");
  }

  std::unordered_set<PurchaseOrderId> purchaseOrderIds;
  for (const auto &order : logistics.purchaseOrders) {
    require(order.id != 0 && purchaseOrderIds.insert(order.id).second,
            "duplicate or zero purchase-order id");
    require(storageIds.contains(order.destination),
            "purchase order references missing destination");
    require(order.quantity > 0 && order.remainingSeconds >= 0,
            "purchase-order quantity invariant failed");
  }
  std::unordered_set<ServiceId> stockMoveIds;
  for (const auto &move : logistics.stockMoves) {
    require(move.id != 0 && stockMoveIds.insert(move.id).second,
            "duplicate or zero stock-move id");
    require(purchaseOrderIds.contains(move.orderId),
            "stock move references missing purchase order");
    require(storageIds.contains(move.from) && storageIds.contains(move.to),
            "stock move references missing storage");
    require(move.quantity > 0 && move.remainingSeconds >= 0,
            "stock-move quantity invariant failed");
  }

  std::unordered_set<TaskId> housekeepingIds;
  for (const auto &job : housekeeping.jobs) {
    require(job.id != 0 && housekeepingIds.insert(job.id).second,
            "duplicate or zero housekeeping job id");
    require(rooms.contains(job.roomId),
            "housekeeping job references missing room");
    require(job.remainingSeconds >= 0,
            "housekeeping remaining time became negative");
  }

  std::unordered_set<LaundryBatchId> laundryIds;
  for (const auto &batch : laundry.batches) {
    require(batch.id != 0 && laundryIds.insert(batch.id).second,
            "duplicate or zero laundry batch id");
    require(batch.quantity > 0 && batch.remainingSeconds >= 0,
            "laundry batch invariant failed");
  }

  std::unordered_set<AssetId> assetIds;
  for (const auto &asset : engineering.assets) {
    require(asset.id != 0 && assetIds.insert(asset.id).second,
            "duplicate or zero engineering asset id");
    require(asset.condition >= 0 && asset.condition <= 10000 &&
                asset.failurePressure >= 0 && asset.failurePressure <= 9500,
            "engineering asset invariant failed");
  }
  std::unordered_set<WorkOrderId> workOrderIds;
  for (const auto &order : engineering.workOrders) {
    require(order.id != 0 && workOrderIds.insert(order.id).second,
            "duplicate or zero work-order id");
    require(assetIds.contains(order.assetId),
            "work order references missing asset");
    require(order.remainingSeconds >= 0,
            "work-order remaining time became negative");
  }

  std::unordered_set<RoomServiceOrderId> roomServiceIds;
  for (const auto &order : roomService.orders) {
    require(order.id != 0 && roomServiceIds.insert(order.id).second,
            "duplicate or zero room-service order id");
    require(order.guestId != 0 && order.remainingSeconds >= 0 &&
                order.ageSeconds >= 0 && order.promisedSeconds > 0,
            "room-service order invariant failed");
  }
}

int main() {
  ServiceLogisticsRuntime runtime(0x504C534FULL, {2, 2, 2});
  auto &logistics = runtime.logistics();
  const auto cleanLinen = logistics.firstStorage(StorageKind::CleanLinen);
  const auto floorCloset = logistics.firstStorage(StorageKind::FloorCloset);
  const auto central = logistics.firstStorage(StorageKind::CentralStorage);
  require(cleanLinen != 0 && floorCloset != 0 && central != 0,
          "standard hotel storage is incomplete");
  require(logistics.addInventory(cleanLinen, "clean_linen_set", 30),
          "failed to seed clean linen");
  require(logistics.addInventory(floorCloset, "towel_unit", 100),
          "failed to seed towels");
  require(logistics.addInventory(floorCloset, "amenity_kit", 50),
          "failed to seed amenities");
  require(logistics.addInventory(floorCloset, "cleaning_chemical", 50),
          "failed to seed cleaning chemicals");
  require(logistics.addInventory(central, "maintenance_part", 50),
          "failed to seed maintenance parts");

  std::unordered_set<RoomId> rooms;
  for (RoomId room = 1; room <= 12; ++room) {
    rooms.insert(room);
    runtime.registerRoom(room);
    runtime.registerAsset(room, 10000);
  }
  const int expectedLinen = runtime.laundry().totalLinenUnits();
  require(expectedLinen == 30, "unexpected initial linen total");

  constexpr std::int64_t daySeconds = 24 * 60 * 60;
  constexpr std::int64_t activeWindow = 1600 + 5500;
  for (int day = 0; day < 100; ++day) {
    if (day % 10 == 0) {
      const RoomId firstRoom = 1 + static_cast<RoomId>((day / 10 * 2) % 12);
      const RoomId secondRoom = 1 + (firstRoom % 12);
      require(runtime.requestRoomTurn(firstRoom) != 0,
              "failed to create first room turn");
      require(runtime.requestRoomTurn(secondRoom) != 0,
              "failed to create second room turn");
      require(runtime.createWorkOrder(firstRoom, WorkOrderType::Preventive) != 0,
              "failed to create preventive work order");

      RoomServiceOrder roomServiceSpec;
      roomServiceSpec.itemCount = 2;
      roomServiceSpec.promisedSeconds = 35 * 60;
      const auto roomServiceOrder = runtime.placeRoomServiceOrder(
          5000 + static_cast<GuestId>(day), roomServiceSpec);
      require(roomServiceOrder != 0,
              "failed to create room-service order");
      require(runtime.markRoomServiceProductionReady(roomServiceOrder),
              "failed production-ready handoff");

      if (day % 20 == 0) {
        const auto purchase =
            logistics.placePurchaseOrder("towel_unit", 5, central, 1800);
        require(purchase.ok(), "failed to create receiving purchase order");
      }

      runtime.tickSeconds(1600);
      require(runtime.housekeeping().roomStatus(firstRoom) ==
                  ServiceRoomStatus::Ready &&
                  runtime.housekeeping().roomStatus(secondRoom) ==
                      ServiceRoomStatus::Ready,
              "room turn did not reach ready state");
      require(runtime.roomService().stage(roomServiceOrder) ==
                  RoomServiceStage::AwaitingTrayPickup,
              "room service did not reach tray-pickup state");
      require(runtime.requestRoomServiceTrayPickup(roomServiceOrder),
              "tray pickup request failed");

      const auto laundryBatch = runtime.requestLaundryBatch(2);
      require(laundryBatch != 0, "failed to create laundry batch");
      logistics.requestWastePickup();
      runtime.tickSeconds(5500);
      require(runtime.roomService().stage(roomServiceOrder) ==
                  RoomServiceStage::Completed,
              "room-service tray did not return");
      bool laundryCompleted = false;
      for (const auto &batch : runtime.laundry().snapshot().batches)
        if (batch.id == laundryBatch)
          laundryCompleted = batch.stage == LaundryStage::Completed;
      require(laundryCompleted, "laundry batch did not complete");
      runtime.tickSeconds(daySeconds - activeWindow);
    } else {
      runtime.tickSeconds(daySeconds);
    }

    validateInvariants(runtime, rooms, expectedLinen);

    if (day == 49) {
      const auto encoded = runtime.save();
      auto restored = ServiceLogisticsRuntime::load(encoded);
      require(restored.save() == encoded,
              "mid-soak save/load did not round-trip exactly");
      runtime = std::move(restored);
      validateInvariants(runtime, rooms, expectedLinen);
    }
  }

  require(runtime.elapsedSeconds() == 100 * daySeconds,
          "100-day soak did not advance exactly 100 days");
  const auto finalSave = runtime.save();
  const auto finalReload = ServiceLogisticsRuntime::load(finalSave);
  require(finalReload.save() == finalSave,
          "final soak state did not round-trip exactly");
  validateInvariants(finalReload, rooms, expectedLinen);
}
