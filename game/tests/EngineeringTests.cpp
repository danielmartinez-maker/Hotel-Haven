#include "hh/game/Engineering.h"
#include "hh/game/Logistics.h"
#include "hh/game/RoomService.h"
#include <stdexcept>
#include <vector>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

int main() {
  {
    auto maintainedLogistics = LogisticsSystem::standardHotel();
    auto ignoredLogistics = LogisticsSystem::standardHotel();
    require(maintainedLogistics.addInventory(
                maintainedLogistics.firstStorage(StorageKind::CentralStorage),
                "maintenance_part", 20),
            "seed maintenance parts");
    require(ignoredLogistics.addInventory(
                ignoredLogistics.firstStorage(StorageKind::CentralStorage),
                "maintenance_part", 20),
            "seed maintenance parts");
    EngineeringSystem maintained(maintainedLogistics, 77);
    EngineeringSystem ignored(ignoredLogistics, 77);
    constexpr AssetId asset = 5001;
    maintained.registerAsset(asset, 5000);
    ignored.registerAsset(asset, 5000);
    const auto preventive =
        maintained.createWorkOrder(asset, WorkOrderType::Preventive);
    require(preventive != 0, "preventive work order not created");

    maintained.tickSeconds(1800);
    require(maintained.snapshot().workOrders.front().stage ==
                WorkOrderStage::Queued,
            "clock time completed maintenance without staff work");
    for (int second = 0; second < 1800; ++second) {
      const auto result = maintained.workSecond(preventive);
      require(result.valid && result.blockedReason == BlockReason::None,
              "preventive work unexpectedly blocked");
      maintained.tickSecond();
      if (result.completed)
        break;
    }
    require(maintained.snapshot().workOrders.front().stage ==
                WorkOrderStage::Completed,
            "staff work did not complete preventive maintenance");

    maintained.tickSeconds(30 * 86400);
    ignored.tickSeconds(30 * 86400);
    require(maintained.snapshot().failures <= ignored.snapshot().failures,
            "preventive maintenance increased deterministic failure pressure");
  }
  {
    auto logistics = LogisticsSystem::standardHotel();
    EngineeringSystem engineering(logistics, 9);
    constexpr AssetId asset = 5002;
    engineering.registerAsset(asset, 0);
    for (int day = 0; day < 60 && engineering.snapshot().failures == 0; ++day)
      engineering.tickSeconds(86400);
    const auto failed = engineering.snapshot().failures;
    require(failed == 1, "deterministic asset did not enter failed state once");
    engineering.tickSeconds(30 * 86400);
    require(engineering.snapshot().failures == failed,
            "already-failed asset was counted as repeated new failures");
  }
  {
    RoomServiceSystem roomService;
    RoomServiceOrder order;
    order.itemCount = 2;
    order.promisedSeconds = 35 * 60;
    const auto id = roomService.placeRoomServiceOrder(7001, order);
    require(id != 0, "room service order not created");
    require(roomService.stage(id) == RoomServiceStage::AwaitingProduction,
            "room service skipped production handoff");
    require(roomService.markProductionReady(id), "production handoff rejected");
    roomService.tickSeconds(1200);
    require(roomService.stage(id) == RoomServiceStage::AwaitingTrayPickup,
            "order did not reach guest handoff");
    require(roomService.requestTrayPickup(id), "tray pickup not created");
    roomService.tickSeconds(900);
    require(roomService.stage(id) == RoomServiceStage::Completed,
            "tray return did not close order");
    const std::vector<RoomServiceStage> expected{
        RoomServiceStage::AwaitingProduction,
        RoomServiceStage::RunnerPickup,
        RoomServiceStage::InTransit,
        RoomServiceStage::GuestHandoff,
        RoomServiceStage::AwaitingTrayPickup,
        RoomServiceStage::TrayReturn,
        RoomServiceStage::Completed};
    require(roomService.history(id) == expected,
            "room service stage sequence changed");
  }
}
