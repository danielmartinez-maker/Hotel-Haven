#include "hh/game/RevenueInventory.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  RevenueInventory inventory(818);
  inventory.setPhysicalCapacity("standard", 3);
  inventory.setOverbookingAllowance("standard", 1);

  InventoryBlock owner;
  owner.id = 1;
  owner.roomCategory = "standard";
  owner.startDay = 10;
  owner.endDay = 12;
  owner.units = 1;
  owner.kind = InventoryBlockKind::Owner;
  require(inventory.setInventoryBlock(owner).ok, "valid owner block rejected");

  InventoryBlock scenario;
  scenario.id = 2;
  scenario.roomCategory = "standard";
  scenario.startDay = 11;
  scenario.endDay = 11;
  scenario.units = 2;
  scenario.kind = InventoryBlockKind::Scenario;
  require(inventory.setInventoryBlock(scenario).ok, "valid scenario block rejected");

  require(inventory.sellableUnits(9, "standard") == 4,
          "inventory block leaked before its date window");
  require(inventory.sellableUnits(10, "standard") == 3,
          "owner block did not reduce sellable inventory before overbooking");
  require(inventory.sellableUnits(11, "standard") == 1,
          "overlapping owner/scenario blocks did not conserve physical inventory");
  require(inventory.sellableUnits(12, "standard") == 3,
          "owner block did not remain active through inclusive end day");
  require(inventory.sellableUnits(13, "standard") == 4,
          "inventory block leaked after its date window");

  require(!inventory.setInventoryBlock({3, "unknown", 10, 12, 1,
                                         InventoryBlockKind::Scenario}).ok,
          "block for unknown category was accepted");
  require(!inventory.setInventoryBlock({4, "standard", 12, 10, 1,
                                         InventoryBlockKind::Owner}).ok,
          "invalid block date range was accepted");

  const auto snapshot = inventory.snapshot();
  require(snapshot.inventoryBlocks.size() == 2,
          "inventory snapshot omitted active owner/scenario blocks");
  const auto saved = inventory.save();
  auto restored = RevenueInventory::load(saved);
  require(restored.save() == saved, "inventory blocks did not round-trip");
  require(restored.snapshot() == snapshot,
          "inventory block state changed across save/load");

  require(restored.removeInventoryBlock(2).ok,
          "removing existing scenario block failed");
  require(restored.sellableUnits(11, "standard") == 3,
          "removing scenario block did not restore inventory");
  require(!restored.removeInventoryBlock(2).ok,
          "removing missing block did not return rejection");
}
