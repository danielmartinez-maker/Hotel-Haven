#include "hh/game/SimulationEconomyBridge.h"
#include <algorithm>
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

static int expectedSellable(const SimulationView &view) {
  int count = 0;
  for (const auto &room : view.rooms)
    if (!room.closed && room.status != RoomStatus::Incomplete &&
        room.status != RoomStatus::OutOfOrder)
      ++count;
  return count;
}

int main() {
  auto bridge = SimulationEconomyBridge::tutorial(5150);
  const auto initialView = bridge.view();
  const auto initialInventory = bridge.revenueManagementSnapshot().inventory;
  const auto initialCapacity = initialInventory.physicalCapacity.find("standard");
  require(initialCapacity != initialInventory.physicalCapacity.end(),
          "bridge inventory snapshot omitted standard physical capacity");
  require(initialCapacity->second == expectedSellable(initialView),
          "bridge counted non-sellable rooms as physical inventory");

  const auto ready = std::find_if(initialView.rooms.begin(), initialView.rooms.end(),
                                  [](const auto &room) {
                                    return !room.closed &&
                                           room.status == RoomStatus::VacantReady;
                                  });
  require(ready != initialView.rooms.end(),
          "tutorial fixture has no vacant-ready room");
  const auto roomId = ready->id;
  const int beforeClose = initialCapacity->second;

  require(bridge.closeRoom(roomId, true).ok,
          "closing vacant room failed in capacity fixture");
  const auto closedInventory = bridge.revenueManagementSnapshot().inventory;
  require(closedInventory.physicalCapacity.at("standard") == beforeClose - 1,
          "closing room did not immediately remove sellable capacity");
  require(closedInventory.physicalCapacity.at("standard") ==
              expectedSellable(bridge.view()),
          "closed-room capacity diverged from physical simulation state");

  require(bridge.closeRoom(roomId, false).ok,
          "reopening healthy room failed in capacity fixture");
  require(bridge.revenueManagementSnapshot().inventory.physicalCapacity.at("standard") ==
              beforeClose,
          "reopening room did not immediately restore sellable capacity");

  const auto repairTargetView = bridge.view();
  const auto repairTarget = std::find_if(
      repairTargetView.rooms.begin(), repairTargetView.rooms.end(),
      [](const auto &room) {
        return !room.closed && room.status == RoomStatus::VacantReady;
      });
  require(repairTarget != repairTargetView.rooms.end(),
          "tutorial fixture has no repairable vacant room");
  require(bridge.requestRepair(repairTarget->id).ok,
          "requesting room repair failed in capacity fixture");
  const auto repairInventory = bridge.revenueManagementSnapshot().inventory;
  require(repairInventory.physicalCapacity.at("standard") == beforeClose - 1,
          "out-of-order repair room remained in sellable capacity");
  require(repairInventory.physicalCapacity.at("standard") ==
              expectedSellable(bridge.view()),
          "repair capacity diverged from physical simulation state");
}
