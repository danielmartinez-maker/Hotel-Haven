#include "hh/game/Laundry.h"
#include "hh/game/Logistics.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

int main() {
  {
    auto logistics = LogisticsSystem::standardHotel();
    const auto dirty = logistics.firstStorage(StorageKind::DirtyLinen);
    require(logistics.addInventory(dirty, "dirty_linen_set", 10), "seed dirty linen");
    LaundrySystem laundry(logistics, {1, 1, 1});
    const int before = laundry.totalLinenUnits();
    require(laundry.requestBatch(10) != 0, "batch not created");
    laundry.tickSeconds(3 * 3600);
    require(logistics.inventoryAt(dirty, "dirty_linen_set") == 0, "dirty linen remained after completed batch");
    require(logistics.inventoryUsable("clean_linen_set") == 10, "clean stock not produced");
    require(laundry.totalLinenUnits() == before, "laundry minted or lost linen");
    require(laundry.snapshot().batches.front().stage == LaundryStage::Completed, "batch did not complete wash/dry/fold");
  }
  {
    auto logistics = LogisticsSystem::standardHotel();
    const auto dirty = logistics.firstStorage(StorageKind::DirtyLinen);
    require(logistics.addInventory(dirty, "dirty_linen_set", 5), "seed dirty linen");
    LaundrySystem laundry(logistics, {1, 0, 1});
    require(laundry.requestBatch(5) != 0, "batch not created");
    const int before = laundry.totalLinenUnits();
    laundry.tickSeconds(6 * 3600);
    const auto snapshot = laundry.snapshot();
    require(laundry.totalLinenUnits() == before, "blocked batch duplicated/lost linen");
    require(snapshot.batches.front().blockedReason == BlockReason::MissingDryer, "missing dryer not diagnosed");
  }
}
