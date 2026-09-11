#include "StressHarness.h"
#include "hh/game/BuildJobs.h"
#include "hh/game/BuildingSystems.h"
#include "hh/game/Construction.h"
#include "hh/game/Simulation.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
using namespace hh::game;

std::size_t operationBudget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return 25'000;
  case hh::stress::Scale::Extended:
    return 250'000;
  case hh::stress::Scale::Exhaustive:
    return 1'000'000;
  }
  std::abort();
}

ConstructionCommand placement(std::string type, Position position, int rotation = 0) {
  ConstructionCommand command;
  command.placements.push_back({std::move(type), position, rotation});
  return command;
}

struct Fixture {
  Simulation sim;
  EntityId roomId{};
};

Fixture makeFixture(std::uint64_t seed, hh::stress::RunContext &ctx) {
  Simulation sim(seed, 40, 24, 3);
  for (int x = 0; x < 40; ++x)
    if (!sim.buildTile({0, x, 0}, x == 0 ? TileKind::Entrance : TileKind::Floor).ok)
      ctx.fail("failed to build corridor", x);
  for (int y = 8; y < 24; ++y)
    for (int x = 0; x < 40; ++x)
      if (!sim.buildTile({0, x, y}, TileKind::Floor).ok)
        ctx.fail("failed to build construction support floor", y * 40 + x);

  const auto room = sim.buildFurnishedRoom(
      {"stress-101", 0, 3, 1, 4, 4, {0, 3, 1}, 1, 1, 120});
  if (!room.ok)
    ctx.fail("failed to create building-system room", 0);
  if (!sim.setRoomUtility(room.id, UtilityKind::Power, true).ok ||
      !sim.setRoomUtility(room.id, UtilityKind::Water, true).ok ||
      !sim.setRoomInfrastructure(room.id, InfrastructureKind::Egress, true).ok ||
      !sim.setRoomInfrastructure(room.id, InfrastructureKind::Accessibility, true).ok)
    ctx.fail("failed to initialize room systems", 0);

  ConstructionMaterials materials{250'000, 250'000, 250'000, 250'000, 250'000};
  if (!sim.addConstructionMaterials(materials).ok)
    ctx.fail("failed to seed construction materials", 0);

  ElevatorSpec elevator;
  elevator.kind = ElevatorKind::Passenger;
  elevator.minFloor = 0;
  elevator.maxFloor = 2;
  elevator.startFloor = 0;
  (void)sim.installElevator(elevator);
  return {std::move(sim), room.id};
}

bool nonnegative(const ConstructionMaterials &m) {
  return m.lumber >= 0 && m.drywall >= 0 && m.electrical >= 0 &&
         m.plumbing >= 0 && m.hardware >= 0;
}

void assertConstruction(const Simulation &sim, hh::stress::RunContext &ctx,
                        std::uint64_t checkpoint) {
  const auto snapshot = sim.constructionSnapshot();
  if (!nonnegative(snapshot.availableMaterials) ||
      !nonnegative(snapshot.reservedMaterials))
    ctx.fail("negative construction material count", checkpoint);

  std::unordered_set<EntityId> objectIds;
  for (const auto &object : snapshot.objects) {
    if (object.id == 0 || object.width <= 0 || object.height <= 0 ||
        !detail::constructionDefinition(object.typeId) ||
        !objectIds.insert(object.id).second)
      ctx.fail("invalid completed construction object", checkpoint);
  }

  std::unordered_set<EntityId> jobIds;
  std::unordered_set<std::string> activeTargets;
  for (const auto &job : snapshot.buildJobs) {
    if (job.id == 0 || !jobIds.insert(job.id).second || job.reservedCashCents < 0 ||
        !nonnegative(job.reservedMaterials))
      ctx.fail("invalid build job state", checkpoint);
    if (job.state == BuildJobState::Completed || job.state == BuildJobState::Cancelled)
      continue;
    for (const auto &entry : job.construction.placements) {
      if (!detail::constructionDefinition(entry.typeId))
        ctx.fail("build job references unknown construction type", checkpoint);
      const auto key = std::to_string(entry.origin.floor) + ':' +
                       std::to_string(entry.origin.x) + ':' +
                       std::to_string(entry.origin.y) + ':' + entry.typeId;
      if (!activeTargets.insert(key).second)
        ctx.fail("two active build jobs claim the same target", checkpoint);
    }
  }
}

void assertBuildingSystems(const Simulation &sim, EntityId roomId,
                           hh::stress::RunContext &ctx,
                           std::uint64_t checkpoint) {
  const auto systems = sim.buildingSystemsSnapshot();
  std::unordered_set<EntityId> nodeIds;
  for (const auto &node : systems.utilityNodes) {
    if (node.id == 0 || node.capacity < 0 || node.load < 0 ||
        !nodeIds.insert(node.id).second)
      ctx.fail("invalid utility node", checkpoint);
  }
  for (const auto &edge : systems.utilityEdges)
    if (!nodeIds.contains(edge.from) || !nodeIds.contains(edge.to))
      ctx.fail("utility edge contains dangling endpoint", checkpoint);

  const auto room = std::find_if(
      systems.rooms.begin(), systems.rooms.end(),
      [roomId](const RoomSystemSnapshot &entry) { return entry.roomId == roomId; });
  if (room == systems.rooms.end())
    ctx.fail("building-system snapshot lost room", checkpoint);
  const bool requiredSystems = room->powerConnected && room->waterConnected &&
                               room->egress && room->accessible;
  if (sim.validateRoomForSale(roomId).sellable != requiredSystems)
    ctx.fail("room validation disagrees with current utility/topology state", checkpoint);

  for (const auto &elevator : systems.elevators) {
    if (elevator.id == 0 || elevator.minFloor > elevator.maxFloor ||
        elevator.currentFloor < elevator.minFloor ||
        elevator.currentFloor > elevator.maxFloor ||
        elevator.targetFloor < elevator.minFloor ||
        elevator.targetFloor > elevator.maxFloor ||
        elevator.capacity <= 0 || elevator.phaseSecondsRemaining < 0)
      ctx.fail("elevator state out of bounds", checkpoint);
    for (const auto &request : elevator.requests)
      if (request.id == 0 || request.pickupFloor < elevator.minFloor ||
          request.pickupFloor > elevator.maxFloor ||
          request.destinationFloor < elevator.minFloor ||
          request.destinationFloor > elevator.maxFloor)
        ctx.fail("elevator request out of range", checkpoint);
  }
}

void applyOperation(Fixture &fixture, hh::stress::Rng &rng,
                    std::string_view scenario, std::size_t operation,
                    hh::stress::RunContext &ctx) {
  auto &sim = fixture.sim;
  auto action = rng.bounded(9);
  if (scenario == "utility_churn" || scenario == "validation_churn")
    action = 6 + rng.bounded(2);
  if (scenario == "job_queue_saturation")
    action = 3 + rng.bounded(2);

  const Position position{0, static_cast<int>(rng.bounded(40)),
                          8 + static_cast<int>(rng.bounded(16))};
  switch (action) {
  case 0: {
    const auto result = sim.executeConstruction(placement("chair", position));
    ctx.trace.push("place chair ok=" + std::to_string(result.ok));
    break;
  }
  case 1: {
    const auto objects = sim.constructionSnapshot().objects;
    if (!objects.empty()) {
      ConstructionCommand command;
      command.removeObjectIds.push_back(
          objects[static_cast<std::size_t>(rng.bounded(objects.size()))].id);
      const auto result = sim.executeConstruction(command);
      ctx.trace.push("remove ok=" + std::to_string(result.ok));
    }
    break;
  }
  case 2: {
    const auto invalid = sim.previewConstruction(placement("wall_sconce", position));
    if (invalid.valid)
      ctx.fail("wall-only object unexpectedly valid on floor", operation);
    ctx.trace.push("invalid-preview");
    break;
  }
  case 3: {
    BuildPlan plan;
    plan.construction = placement("chair", position);
    plan.workSeconds = 1 + static_cast<int>(rng.bounded(60));
    const auto result = sim.queueBuild(plan);
    ctx.trace.push("queue-build ok=" + std::to_string(result.ok));
    break;
  }
  case 4: {
    const auto jobs = sim.constructionSnapshot().buildJobs;
    if (!jobs.empty()) {
      const auto &job = jobs[static_cast<std::size_t>(rng.bounded(jobs.size()))];
      const auto result = sim.cancelBuild(job.id);
      ctx.trace.push("cancel-build ok=" + std::to_string(result.ok));
    }
    break;
  }
  case 5:
    sim.step(static_cast<double>(1 + rng.bounded(60)));
    ctx.trace.push("construction-step");
    break;
  case 6: {
    const bool connected = (rng.next() & 1ULL) != 0;
    const auto kind = (rng.next() & 1ULL) ? UtilityKind::Power : UtilityKind::Water;
    const auto result = sim.setRoomUtility(fixture.roomId, kind, connected);
    if (!result.ok)
      ctx.fail("valid utility churn rejected", operation);
    ctx.trace.push("utility-churn");
    break;
  }
  case 7: {
    const bool installed = (rng.next() & 1ULL) != 0;
    const auto kind = (rng.next() & 1ULL) ? InfrastructureKind::Egress
                                         : InfrastructureKind::Accessibility;
    const auto result = sim.setRoomInfrastructure(fixture.roomId, kind, installed);
    if (!result.ok)
      ctx.fail("valid infrastructure churn rejected", operation);
    (void)sim.validateRoomForSale(fixture.roomId);
    ctx.trace.push("validation-churn");
    break;
  }
  case 8: {
    const auto systems = sim.buildingSystemsSnapshot();
    if (!systems.elevators.empty()) {
      const auto &elevator = systems.elevators.front();
      const int pickup = static_cast<int>(rng.bounded(3));
      const int destination = static_cast<int>(rng.bounded(3));
      (void)sim.requestElevator(elevator.id, pickup, destination);
      sim.step(static_cast<double>(1 + rng.bounded(20)));
    }
    ctx.trace.push("elevator-churn");
    break;
  }
  default:
    ctx.fail("invalid construction stress action", operation);
  }
}

void runStress(const hh::stress::Config &config, std::string_view scenario) {
  hh::stress::RunContext ctx{"final02-construction", config};
  ctx.phase = std::string(scenario);
  auto fixture = makeFixture(config.seed, ctx);
  hh::stress::Rng rng{config.seed ^ 0x8EBC6AF09C88C6E3ULL};
  const auto operations = operationBudget(config.scale);

  for (std::size_t operation = 0; operation < operations; ++operation) {
    applyOperation(fixture, rng, scenario, operation, ctx);
    if ((operation & 255U) == 0U) {
      assertConstruction(fixture.sim, ctx, operation);
      assertBuildingSystems(fixture.sim, fixture.roomId, ctx, operation);
    }
    if ((scenario == "partial_build_save" || scenario == "dense_place_cancel") &&
        operation != 0 && operation % 4096U == 0U) {
      const auto encoded = fixture.sim.save();
      try {
        auto restored = Simulation::load(encoded);
        if (restored.save() != encoded ||
            restored.constructionSnapshot() != fixture.sim.constructionSnapshot() ||
            restored.buildingSystemsSnapshot() != fixture.sim.buildingSystemsSnapshot())
          ctx.fail("partial construction save did not round-trip", operation);
        restored.step(120);
        auto replay = Simulation::load(encoded);
        replay.step(120);
        if (restored.save() != replay.save())
          ctx.fail("partial construction continuation diverged", operation);
      } catch (const std::invalid_argument &error) {
        ctx.fail(std::string("construction checkpoint rejected: ") + error.what(),
                 operation);
      }
    }
  }
  assertConstruction(fixture.sim, ctx, operations);
  assertBuildingSystems(fixture.sim, fixture.roomId, ctx, operations);
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "dense_place_cancel" ||
      scenario == "topology_churn" || scenario == "utility_churn" ||
      scenario == "validation_churn" || scenario == "job_queue_saturation" ||
      scenario == "partial_build_save")
    return;
  throw std::invalid_argument("unknown construction/building stress scenario");
}
} // namespace

int main() {
  try {
    const auto config = hh::stress::configFromEnvironment(0xC0FFEE1234ABCDEFULL);
    validateScenario(config.scenario);
    runStress(config, config.scenario.empty() ? "dense_place_cancel" : config.scenario);
    std::cout << "StressConstructionBuilding PASS operations="
              << operationBudget(config.scale) << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
