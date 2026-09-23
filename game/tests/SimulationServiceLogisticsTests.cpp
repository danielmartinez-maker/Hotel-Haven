#include "hh/game/Simulation.h"
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace hh::game;
static void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }

static std::string transformFinal04(
    const std::string &simulationState,
    const std::function<std::string(const std::string &)> &transform) {
  const auto headerStart = simulationState.find("FINAL04 ");
  if (headerStart == std::string::npos)
    throw std::runtime_error("FINAL-04 section missing from simulation save");
  const auto headerEnd = simulationState.find('\n', headerStart);
  if (headerEnd == std::string::npos)
    throw std::runtime_error("FINAL-04 section header is truncated");

  std::istringstream header(
      simulationState.substr(headerStart, headerEnd - headerStart));
  std::string tag;
  std::size_t bytes{};
  header >> tag >> bytes;
  if (!header || tag != "FINAL04")
    throw std::runtime_error("FINAL-04 section header could not be parsed");

  const auto stateStart = headerEnd + 1;
  if (bytes > simulationState.size() - stateStart)
    throw std::runtime_error("FINAL-04 section payload is truncated");
  const auto stateEnd = stateStart + bytes;
  if (stateEnd >= simulationState.size() || simulationState[stateEnd] != '\n')
    throw std::runtime_error("FINAL-04 section terminator is missing");

  const auto replacement =
      transform(simulationState.substr(stateStart, bytes));
  std::ostringstream rebuilt;
  rebuilt << simulationState.substr(0, headerStart) << "FINAL04 "
          << replacement.size() << '\n' << replacement << '\n'
          << simulationState.substr(stateEnd + 1);
  return rebuilt.str();
}

static void requireSimulationLoadRejected(const std::string &encoded,
                                          const char *message) {
  bool rejected = false;
  try {
    (void)Simulation::load(encoded);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  require(rejected, message);
}

int main() {
  auto sim = Simulation::tutorial(321);
  const auto view = sim.view();
  require(!view.rooms.empty(), "tutorial requires a serviceable room");
  require(sim.logisticsSnapshot().elapsedSeconds == view.elapsedSeconds,
          "tutorial FINAL-04 clock did not start on the simulation clock");
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
  require(demolition.removeRoom(built.id).ok,
          "room could not be demolished after FINAL-04 work completed");
  require(demolition.createWorkOrder(built.id, WorkOrderType::Preventive) == 0,
          "demolished room left a ghost engineering asset");
  require(demolition.requestRoomTurn(built.id) == 0,
          "demolished room left a ghost housekeeping registration");

  const auto demolishedState = demolition.save();
  require(Simulation::load(demolishedState).save() == demolishedState,
          "demolition with retired FINAL-04 state did not round-trip");

  auto parity = Simulation::tutorial(323);
  parity.step(10);
  const auto parityState = parity.save();

  const auto clockDrift = transformFinal04(
      parityState, [](const std::string &serviceState) {
        auto service = ServiceLogisticsRuntime::load(serviceState);
        service.tickSecond();
        return service.save();
      });
  requireSimulationLoadRejected(
      clockDrift,
      "Simulation accepted FINAL-04 state on a different authoritative clock");

  const auto ghostRegistration = transformFinal04(
      parityState, [](const std::string &serviceState) {
        auto service = ServiceLogisticsRuntime::load(serviceState);
        service.registerRoom(999999);
        service.registerAsset(999999, 9000);
        return service.save();
      });
  requireSimulationLoadRejected(
      ghostRegistration,
      "Simulation accepted ghost FINAL-04 room/asset registrations");

  const auto physicalRoom = parity.view().rooms.front().id;
  const auto missingRegistration = transformFinal04(
      parityState, [physicalRoom](const std::string &serviceState) {
        auto service = ServiceLogisticsRuntime::load(serviceState);
        require(service.retireRoomAndAsset(physicalRoom),
                "could not build missing-registration corruption fixture");
        return service.save();
      });
  requireSimulationLoadRejected(
      missingRegistration,
      "Simulation accepted missing FINAL-04 room/asset registrations");
}
