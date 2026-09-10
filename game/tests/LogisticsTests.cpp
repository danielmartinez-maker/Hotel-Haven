#include "hh/game/Logistics.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

int main() {
  {
    auto logistics = LogisticsSystem::standardHotel();
    const auto storage = logistics.firstStorage(StorageKind::CentralStorage);
    const auto receiving = logistics.firstStorage(StorageKind::Receiving);
    auto order = logistics.placePurchaseOrder("amenity_kit", 20, storage, 60);
    require(order.ok(), "purchase order rejected");
    logistics.tickSeconds(60);
    require(logistics.inventoryAt(receiving, "amenity_kit") == 20, "delivery did not physically enter receiving");
    require(logistics.inventoryUsable("amenity_kit") == 0, "receiving stock was usable before stock move");
    logistics.tickSeconds(180);
    require(logistics.inventoryAt(receiving, "amenity_kit") == 0, "stock remained in receiving after move");
    require(logistics.inventoryUsable("amenity_kit") == 20, "stock move did not make inventory usable");
  }
  {
    LogisticsSystem logistics;
    logistics.addStorage({StorageKind::Receiving, 2000, true});
    const auto tiny = logistics.addStorage({StorageKind::CentralStorage, 5, true});
    auto order = logistics.placePurchaseOrder("amenity_kit", 999, tiny, 60);
    require(order.error == OrderError::InsufficientStorageCapacity, "storage capacity allowed silent overflow");
  }
  {
    auto logistics = LogisticsSystem::standardHotel();
    logistics.produceWaste(7);
    logistics.tickSeconds(300);
    require(logistics.snapshot().wasteInBackOfHouse == 7, "waste did not physically collect to back of house");
    logistics.requestWastePickup();
    logistics.tickSeconds(120);
    require(logistics.snapshot().wasteInBackOfHouse == 0, "external waste pickup did not remove stored waste");
  }
}
