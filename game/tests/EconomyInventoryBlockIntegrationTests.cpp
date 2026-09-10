#include "hh/game/EconomyRuntime.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

int main() {
  EconomyRuntime runtime(222, 2'000'000);
  runtime.setPhysicalRoomCapacity("standard", 3);

  InventoryBlock owner{1, "standard", 10, 10, 1, InventoryBlockKind::Owner};
  require(runtime.setInventoryBlock(owner).ok,
          "economy runtime rejected valid owner block");
  require(runtime.revenueManagementSnapshot().inventory.inventoryBlocks.size() == 1,
          "economy runtime did not expose owner block in immutable snapshot");

  CommercialContract contract;
  contract.id = 99;
  contract.startDay = 10;
  contract.endDay = 10;
  contract.minimumRoomNights = 3;
  contract.maximumRoomNights = 3;
  contract.negotiatedRateCents = 12'000;
  ContractFeasibility optimistic{3, 0, 0};
  const auto rejected = runtime.acceptCommercialContract(contract, optimistic, false);
  require(!rejected.ok,
          "contract feasibility ignored authoritative blocked inventory");

  const auto saved = runtime.save();
  const auto restored = EconomyRuntime::load(saved);
  require(restored.save() == saved,
          "economy inventory block did not survive runtime save/load");
  require(restored.revenueManagementSnapshot().inventory.inventoryBlocks.size() == 1,
          "restored economy runtime lost inventory block");

  auto mutableRestored = restored;
  require(mutableRestored.removeInventoryBlock(owner.id).ok,
          "economy runtime failed to remove existing owner block");
  require(mutableRestored.revenueManagementSnapshot().inventory.inventoryBlocks.empty(),
          "removed owner block remained in economy snapshot");
  require(!mutableRestored.removeInventoryBlock(owner.id).ok,
          "removing missing owner block did not return reason-coded rejection");
}
