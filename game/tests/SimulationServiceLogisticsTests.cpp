#include "hh/game/Simulation.h"
#include <stdexcept>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

int main() {
  auto sim = Simulation::tutorial(321);
  const auto view = sim.view();
  require(!view.rooms.empty(), "tutorial requires a serviceable room");
  const auto room = view.rooms.front().id;

  const auto turn = sim.requestRoomTurn(room);
  require(turn != 0, "Simulation did not expose room-turn command");

  const auto work = sim.createWorkOrder(room, WorkOrderType::Preventive);
  require(work != 0, "Simulation did not expose engineering command");

  RoomServiceOrder order;
  const auto roomService = sim.placeRoomServiceOrder(7001, order);
  require(roomService != 0, "Simulation did not expose room-service command");

  const auto before = sim.logisticsSnapshot().elapsedSeconds;
  sim.step(10.0);
  const auto after = sim.logisticsSnapshot().elapsedSeconds;
  require(after - before == 10, "service/logistics clock is not driven by Simulation");

  const auto encoded = sim.save();
  auto restored = Simulation::load(encoded);
  require(restored.logisticsSnapshot().elapsedSeconds == after,
          "Simulation save/load dropped FINAL-04 state");

  Simulation demolition(322, 16, 10, 1);
  require(demolition.loadDefinitions(R"({"baseDemand":0})").ok,
          "demolition test definitions rejected");
  for (int x = 0; x < 10; ++x)
    require(demolition
                .buildTile({0, x, 1},
                           x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "demolition test corridor build failed");
  const auto built = demolition.buildFurnishedRoom(
      {"201", 0, 4, 2, 4, 4, {0, 4, 2}, 1, 1, 120});
  require(built.ok, "demolition test room build failed");

  require(demolition.requestRoomTurn(built.id) != 0,
          "demolition test room turn not created");
  require(!demolition.removeRoom(built.id),
          "room demolished while FINAL-04 housekeeping work was active");
  demolition.step(2400);
  require(demolition.removeRoom(built.id),
          "room could not be demolished after FINAL-04 work completed");
  require(demolition.createWorkOrder(built.id, WorkOrderType::Preventive) == 0,
          "demolished room left a ghost engineering asset");
  require(demolition.requestRoomTurn(built.id) == 0,
          "demolished room left a ghost housekeeping registration");

  const auto demolishedState = demolition.save();
  require(Simulation::load(demolishedState).save() == demolishedState,
          "demolition with retired FINAL-04 state did not round-trip");
}
