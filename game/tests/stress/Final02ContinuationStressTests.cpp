#include "StressHarness.h"
#include "hh/game/BuildingSystems.h"
#include "hh/game/Construction.h"
#include "hh/game/GuestPsychology.h"
#include "hh/game/Simulation.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace hh::game;

std::size_t operationBudget(hh::stress::Scale scale) {
  switch (scale) {
  case hh::stress::Scale::Pr:
    return 5'000;
  case hh::stress::Scale::Extended:
    return 50'000;
  case hh::stress::Scale::Exhaustive:
    return 250'000;
  }
  std::abort();
}

std::string hashSave(std::string_view text) {
  std::uint64_t hash = 1469598103934665603ULL;
  for (const unsigned char byte : text) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << hash;
  return out.str();
}

std::vector<EntityId> liveGuestIds(const Simulation &sim) {
  std::vector<EntityId> result;
  for (const auto &person : sim.view().people)
    if (person.kind == PersonKind::Guest && person.reservationId != 0)
      result.push_back(person.reservationId);
  std::sort(result.begin(), result.end());
  return result;
}

std::vector<EntityId> roomIds(const Simulation &sim) {
  std::vector<EntityId> result;
  for (const auto &room : sim.view().rooms)
    result.push_back(room.id);
  std::sort(result.begin(), result.end());
  return result;
}

void applyTailOperation(Simulation &sim, hh::stress::Rng &rng,
                        std::size_t operation) {
  const auto rooms = roomIds(sim);
  const auto guests = liveGuestIds(sim);
  switch (rng.bounded(5)) {
  case 0:
    sim.step(static_cast<double>(1 + rng.bounded(120)));
    break;
  case 1:
    if (!rooms.empty())
      (void)sim.setRoomUtility(
          rooms[static_cast<std::size_t>(rng.bounded(rooms.size()))],
          (rng.next() & 1ULL) ? UtilityKind::Power : UtilityKind::Water,
          (rng.next() & 1ULL) != 0);
    break;
  case 2:
    if (!rooms.empty())
      (void)sim.setRoomInfrastructure(
          rooms[static_cast<std::size_t>(rng.bounded(rooms.size()))],
          (rng.next() & 1ULL) ? InfrastructureKind::Egress
                              : InfrastructureKind::Accessibility,
          (rng.next() & 1ULL) != 0);
    break;
  case 3:
    if (!guests.empty()) {
      ExperienceEvent event;
      event.type = (operation & 1U) ? ExperienceEventType::GreatMeal
                                    : ExperienceEventType::ElevatorDelay;
      event.timestampSeconds = sim.view().elapsedSeconds;
      event.category = (operation & 1U) ? ExperienceCategory::Food
                                        : ExperienceCategory::Convenience;
      event.rawImpact = (operation & 1U) ? 15 : -20;
      event.memorySalience = 8'000;
      event.memoryHalfLifeHours = 12;
      event.complaintEligible = event.rawImpact < 0;
      (void)sim.recordGuestExperience(
          guests[static_cast<std::size_t>(rng.bounded(guests.size()))], event);
    }
    break;
  case 4: {
    ConstructionCommand command;
    command.placements.push_back(
        {"chair",
         {0, static_cast<int>(rng.bounded(32)),
          static_cast<int>(rng.bounded(20))},
         0});
    (void)sim.executeConstruction(command);
    break;
  }
  default:
    break;
  }
}

void assertIntegratedState(const Simulation &sim, hh::stress::RunContext &ctx,
                           std::uint64_t checkpoint) {
  const auto construction = sim.constructionSnapshot();
  if (construction.availableMaterials.lumber < 0 ||
      construction.availableMaterials.drywall < 0 ||
      construction.availableMaterials.electrical < 0 ||
      construction.availableMaterials.plumbing < 0 ||
      construction.availableMaterials.hardware < 0 ||
      construction.reservedMaterials.lumber < 0 ||
      construction.reservedMaterials.drywall < 0 ||
      construction.reservedMaterials.electrical < 0 ||
      construction.reservedMaterials.plumbing < 0 ||
      construction.reservedMaterials.hardware < 0)
    ctx.fail("combined construction materials became negative", checkpoint);

  for (const auto guestId : liveGuestIds(sim)) {
    const auto psychology = sim.guestPsychology(guestId);
    if (psychology.guestId != guestId || psychology.needs.energy < 0 ||
        psychology.needs.energy > 100 || psychology.needs.hunger < 0 ||
        psychology.needs.hunger > 100 || psychology.satisfaction.overall < 0 ||
        psychology.satisfaction.overall > 100)
      ctx.fail("combined guest state out of bounds", checkpoint);
  }

  const auto systems = sim.buildingSystemsSnapshot();
  for (const auto &edge : systems.utilityEdges) {
    const auto from = std::find_if(systems.utilityNodes.begin(), systems.utilityNodes.end(),
                                   [&](const auto &node) { return node.id == edge.from; });
    const auto to = std::find_if(systems.utilityNodes.begin(), systems.utilityNodes.end(),
                                 [&](const auto &node) { return node.id == edge.to; });
    if (from == systems.utilityNodes.end() || to == systems.utilityNodes.end())
      ctx.fail("combined utility graph has dangling endpoint", checkpoint);
  }
}

std::string runScenario(const hh::stress::Config &config,
                        std::string_view scenario) {
  auto sim = Simulation::tutorial(config.seed);
  if (!sim.loadDefinitions(R"({"baseDemand":100})").ok)
    throw std::runtime_error("FINAL-02 continuation demand fixture rejected");
  sim.step(3600);
  if (liveGuestIds(sim).empty())
    throw std::runtime_error("FINAL-02 continuation fixture produced no guests");
  (void)sim.addConstructionMaterials({10'000, 10'000, 10'000, 10'000, 10'000});

  hh::stress::RunContext ctx{"final02-continuation", config};
  ctx.phase = std::string(scenario);
  hh::stress::Rng rng{config.seed ^ 0x94D049BB133111EBULL};
  const auto operations = operationBudget(config.scale);

  for (std::size_t operation = 0; operation < operations; ++operation) {
    applyTailOperation(sim, rng, operation);
    if ((operation & 255U) == 0U)
      assertIntegratedState(sim, ctx, operation);

    if (operation != 0 && operation % 1024U == 0U) {
      const auto checkpoint = sim.save();
      auto originalTail = Simulation::load(checkpoint);
      auto restoredTail = Simulation::load(checkpoint);
      hh::stress::Rng tailA{config.seed ^ operation};
      hh::stress::Rng tailB{config.seed ^ operation};
      for (std::size_t tail = 0; tail < 128; ++tail) {
        applyTailOperation(originalTail, tailA, tail);
        applyTailOperation(restoredTail, tailB, tail);
      }
      if (originalTail.save() != restoredTail.save())
        ctx.fail("save-point deterministic continuation diverged", operation,
                 hashSave(checkpoint));
      if (Simulation::load(checkpoint).save() != checkpoint)
        ctx.fail("combined save was not byte-stable", operation,
                 hashSave(checkpoint));
    }
  }
  assertIntegratedState(sim, ctx, operations);
  return sim.save();
}

void validateScenario(std::string_view scenario) {
  if (scenario.empty() || scenario == "topology_guest_crisis" ||
      scenario == "save_in_crisis")
    return;
  throw std::invalid_argument(
      "HH_STRESS_SCENARIO must be topology_guest_crisis, save_in_crisis, or empty");
}
} // namespace

int main() {
  try {
    const auto config = hh::stress::configFromEnvironment(0x5EED5EED5EED5EEDULL);
    validateScenario(config.scenario);
    const auto scenario =
        config.scenario.empty() ? std::string_view{"topology_guest_crisis"}
                                : std::string_view{config.scenario};
    const auto first = runScenario(config, scenario);
    const auto second = runScenario(config, scenario);
    if (first != second) {
      hh::stress::RunContext ctx{"final02-continuation", config};
      ctx.phase = std::string(scenario);
      ctx.fail("full FINAL-02 replay diverged", operationBudget(config.scale),
               hashSave(first) + "/" + hashSave(second));
    }
    std::cout << "StressFinal02Continuation PASS hash=" << hashSave(first)
              << " operations=" << operationBudget(config.scale) << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
