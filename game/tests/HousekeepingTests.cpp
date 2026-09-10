#include "hh/game/Housekeeping.h"
#include "hh/game/Logistics.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

int main() {
  {
    auto logistics = LogisticsSystem::standardHotel();
    require(logistics.addInventory(logistics.firstStorage(StorageKind::CleanLinen),
                                   "clean_linen_set", 1),
            "seed clean linen");
    require(logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                                   "towel_unit", 2),
            "seed towels");
    require(logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                                   "amenity_kit", 1),
            "seed amenities");
    require(logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                                   "cleaning_chemical", 1),
            "seed chemicals");
    HousekeepingSystem housekeeping(logistics);
    constexpr RoomId room = 101;
    housekeeping.registerRoom(room);
    const auto task = housekeeping.requestRoomTurn(room);
    require(task != 0, "room turn not created");
    require(housekeeping.roomStatus(room) == ServiceRoomStatus::Dirty,
            "room not dirtied");

    housekeeping.tickSeconds(2400);
    require(housekeeping.roomStatus(room) != ServiceRoomStatus::Ready,
            "clock time completed a room turn without staff work");

    bool completed = false;
    for (int second = 0; second < 2400 && !completed; ++second) {
      const auto result = housekeeping.workSecond(task);
      require(result.valid && result.blockedReason == BlockReason::None,
              "staff work unexpectedly blocked");
      completed = result.completed;
      housekeeping.tickSecond();
    }
    require(completed && housekeeping.roomStatus(room) == ServiceRoomStatus::Ready,
            "staff work did not complete room turn");
    require(housekeeping.snapshot().jobs.front().stage ==
                HousekeepingStage::Completed,
            "turn stages did not complete");
  }
  {
    auto logistics = LogisticsSystem::standardHotel();
    require(logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                                   "towel_unit", 2),
            "seed towels");
    require(logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                                   "amenity_kit", 1),
            "seed amenities");
    require(logistics.addInventory(logistics.firstStorage(StorageKind::FloorCloset),
                                   "cleaning_chemical", 1),
            "seed chemicals");
    HousekeepingSystem housekeeping(logistics);
    constexpr RoomId room = 102;
    housekeeping.registerRoom(room);
    const auto task = housekeeping.requestRoomTurn(room);
    require(task != 0, "blocked room turn not created");
    ServiceWorkResult work;
    for (int second = 0; second < 1800; ++second) {
      work = housekeeping.workSecond(task);
      housekeeping.tickSecond();
      if (work.blockedReason != BlockReason::None)
        break;
    }
    const auto snapshot = housekeeping.snapshot();
    require(!snapshot.jobs.empty(), "missing housekeeping job");
    require(work.blockedReason == BlockReason::MissingCleanLinen &&
                snapshot.jobs.front().blockedReason ==
                    BlockReason::MissingCleanLinen,
            "missing linen did not explicitly block turn");
    require(housekeeping.roomStatus(room) != ServiceRoomStatus::Ready,
            "blocked room became sellable");
  }
}
