#include "StressHarness.h"
#include "hh/game/Simulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {
using namespace hh::game;

struct ScenarioResult {
  std::string save;
  std::string hash;
  std::uint64_t finalTick{};
};

std::size_t operationBudget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return 25'000;
  case hh::stress::Scale::Extended:
    return 250'000;
  case hh::stress::Scale::Exhaustive:
    return 2'000'000;
  }
  std::abort();
}

std::string fnv1a64(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const unsigned char byte : text) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << hash;
  return out.str();
}

template <class Range, class IdFn>
bool uniqueIds(const Range &range, IdFn idFn) {
  std::unordered_set<std::uint64_t> seen;
  seen.reserve(range.size() * 2 + 1);
  for (const auto &item : range)
    if (!seen.insert(static_cast<std::uint64_t>(idFn(item))).second)
      return false;
  return true;
}

void checkSnapshot(const SimulationView &view, std::int64_t previousElapsed,
                   std::uint64_t operation, hh::stress::RunContext &ctx) {
  if (view.elapsedSeconds < previousElapsed)
    ctx.fail("simulation time moved backwards", operation);
  if (!std::isfinite(view.economy.occupancy) ||
      !std::isfinite(view.economy.reputation))
    ctx.fail("non-finite economy snapshot", operation);
  if (view.inventory.linen < 0 || view.inventory.towels < 0 ||
      view.inventory.amenities < 0 || view.inventory.chemicals < 0 ||
      view.inventory.parts < 0)
    ctx.fail("negative simulation inventory", operation);

  for (const auto &room : view.rooms)
    if (!std::isfinite(room.cleanliness) || !std::isfinite(room.condition) ||
        !std::isfinite(room.nightlyRate))
      ctx.fail("non-finite room state", operation);
  for (const auto &person : view.people)
    if (!std::isfinite(person.fatigue) ||
        !std::isfinite(person.satisfaction) || !std::isfinite(person.hunger) ||
        !std::isfinite(person.rest) || !std::isfinite(person.patience) ||
        !std::isfinite(person.skill))
      ctx.fail("non-finite person state", operation);

  if (!uniqueIds(view.rooms, [](const auto &v) { return v.id; }))
    ctx.fail("duplicate room id", operation);
  if (!uniqueIds(view.people, [](const auto &v) { return v.id; }))
    ctx.fail("duplicate person id", operation);
  if (!uniqueIds(view.reservations, [](const auto &v) { return v.id; }))
    ctx.fail("duplicate reservation id", operation);
  if (!uniqueIds(view.tasks, [](const auto &v) { return v.id; }))
    ctx.fail("duplicate task id", operation);
  if (!uniqueIds(view.supplyOrders, [](const auto &v) { return v.id; }))
    ctx.fail("duplicate supply order id", operation);

  if (view.tasks.size() > operation + 10'000)
    ctx.fail("task collection grew beyond issued work", operation);
}

EntityId chooseRoom(const SimulationView &view, hh::stress::Rng &rng) {
  if (view.rooms.empty())
    return 0;
  return view.rooms[static_cast<std::size_t>(rng.bounded(view.rooms.size()))].id;
}

void applyCommand(Simulation &sim, hh::stress::Rng &rng, std::size_t operation,
                  std::string_view scenario, hh::stress::RunContext &ctx) {
  const auto before = sim.view();
  const auto roomId = chooseRoom(before, rng);
  const auto action = rng.bounded(scenario == "long_run" ? 5 : 8);
  switch (action) {
  case 0: {
    const double seconds = scenario == "long_run"
                               ? static_cast<double>(60 + rng.bounded(181))
                               : static_cast<double>(1 + rng.bounded(30));
    sim.step(seconds);
    ctx.trace.push("step=" + std::to_string(static_cast<int>(seconds)));
    break;
  }
  case 1:
    if (roomId != 0) {
      const auto rate = 60.0 + static_cast<double>(rng.bounded(441));
      const auto result = sim.setRoomRate(roomId, rate);
      ctx.trace.push("rate room=" + std::to_string(roomId) +
                     " ok=" + std::to_string(result.ok));
    }
    break;
  case 2:
    if (roomId != 0) {
      const auto result = sim.requestClean(roomId);
      ctx.trace.push("clean room=" + std::to_string(roomId) +
                     " ok=" + std::to_string(result.ok));
    }
    break;
  case 3:
    if (roomId != 0) {
      const auto result = sim.requestRepair(roomId);
      ctx.trace.push("repair room=" + std::to_string(roomId) +
                     " ok=" + std::to_string(result.ok));
    }
    break;
  case 4:
    sim.step(scenario == "long_run" ? 300.0 : 15.0);
    ctx.trace.push("progress");
    break;
  case 5:
    if (roomId != 0) {
      const bool closed = (rng.next() & 1ULL) != 0;
      const auto result = sim.closeRoom(roomId, closed);
      ctx.trace.push("close room=" + std::to_string(roomId) +
                     " value=" + std::to_string(closed) +
                     " ok=" + std::to_string(result.ok));
    }
    break;
  case 6: {
    SupplyOrder order;
    order.linen = static_cast<int>(rng.bounded(4));
    order.towels = static_cast<int>(rng.bounded(7));
    order.amenities = static_cast<int>(rng.bounded(4));
    order.chemicals = static_cast<int>(rng.bounded(4));
    order.parts = static_cast<int>(rng.bounded(2));
    const auto result = sim.orderSupplies(order);
    ctx.trace.push("supplies ok=" + std::to_string(result.ok));
    break;
  }
  case 7:
    sim.step(static_cast<double>(1 + rng.bounded(120)));
    ctx.trace.push("burst-progress");
    break;
  default:
    ctx.fail("invalid generated action", operation);
  }
}

ScenarioResult runScenario(std::uint64_t seed, std::size_t operations,
                           std::string_view scenario,
                           const hh::stress::Config &config) {
  auto sim = Simulation::tutorial(seed);
  if (!sim.loadDefinitions(
          R"({"baseDemand":100,"initialLinen":400,"initialTowels":400,"initialAmenities":200,"initialChemicals":200,"initialParts":100})"))
    throw std::runtime_error("stress definitions rejected");

  hh::stress::RunContext ctx{"core-simulation", config};
  ctx.phase = std::string(scenario);
  hh::stress::Rng rng{seed ^ 0xD1B54A32D192ED03ULL};
  std::int64_t previousElapsed = sim.view().elapsedSeconds;

  for (std::size_t operation = 0; operation < operations; ++operation) {
    applyCommand(sim, rng, operation, scenario, ctx);
    if ((operation & 255U) == 0U) {
      const auto view = sim.view();
      checkSnapshot(view, previousElapsed, operation, ctx);
      previousElapsed = view.elapsedSeconds;
    }
    if (operation != 0 && (operation % 4096U) == 0U) {
      const auto serialized = sim.save();
      try {
        const auto restored = Simulation::load(serialized);
        if (restored.save() != serialized)
          ctx.fail("save/load round-trip changed authoritative state", operation,
                   fnv1a64(serialized));
      } catch (const std::invalid_argument &error) {
        ctx.fail(std::string("save/load rejected authoritative checkpoint: ") +
                     error.what(),
                 operation, fnv1a64(serialized));
      }
      ctx.trace.push("roundtrip");
    }
  }

  const auto finalView = sim.view();
  checkSnapshot(finalView, previousElapsed, operations, ctx);
  auto serialized = sim.save();
  return {serialized, fnv1a64(serialized),
          static_cast<std::uint64_t>(finalView.elapsedSeconds)};
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "long_run" ||
      scenario == "burst_commands" || scenario == "repeat_hash")
    return;
  throw std::invalid_argument(
      "HH_STRESS_SCENARIO must be long_run, burst_commands, repeat_hash, or empty");
}
} // namespace

int main() {
  try {
    const auto config =
        hh::stress::configFromEnvironment(0xC0FFEE1234ABCDEFULL);
    validateScenario(config.scenario);
    const auto operations = operationBudget(config.scale);

    if (config.scenario.empty() || config.scenario == "long_run")
      (void)runScenario(config.seed, operations, "long_run", config);
    if (config.scenario.empty() || config.scenario == "burst_commands")
      (void)runScenario(config.seed ^ 0xA5A5A5A5A5A5A5A5ULL, operations,
                        "burst_commands", config);
    if (config.scenario.empty() || config.scenario == "repeat_hash") {
      const auto first =
          runScenario(config.seed, operations, "repeat_hash", config);
      const auto second =
          runScenario(config.seed, operations, "repeat_hash", config);
      if (first.save != second.save) {
        hh::stress::RunContext ctx{"core-simulation", config};
        ctx.phase = "repeat_hash";
        ctx.fail("identical seed and command stream diverged", first.finalTick,
                 first.hash + "/" + second.hash);
      }
      std::cout << "StressCoreSimulation seed=0x" << std::hex << config.seed
                << std::dec << " operations=" << operations
                << " final_hash=" << first.hash << '\n';
    }
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
