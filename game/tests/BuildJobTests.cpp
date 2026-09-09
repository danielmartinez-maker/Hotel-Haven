#include "hh/game/BuildJobs.h"
#include "hh/game/Construction.h"
#include "hh/game/Simulation.h"
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void supportFloor(Simulation &sim) {
  for (int y = 0; y < 8; ++y)
    for (int x = 0; x < 12; ++x)
      require(sim.buildTile({0, x, y}, x == 0 && y == 0
                                           ? TileKind::Entrance
                                           : TileKind::Floor)
                  .ok,
              "build-job fixture floor failed");
}

BuildPlan deskPlan(Position position = {0, 4, 4}) {
  BuildPlan plan;
  plan.construction.placements.push_back({"desk", position, 0});
  return plan;
}

void construction_plan_reserves_cash_and_waits_for_materials() {
  Simulation sim(2001, 12, 8, 1);
  supportFloor(sim);
  const auto cashBefore = sim.view().economy.cashCents;
  const auto queued = sim.queueBuild(deskPlan());
  require(queued.ok && queued.jobId != 0, "valid build plan did not queue");
  auto snapshot = sim.constructionSnapshot();
  require(snapshot.buildJobs.size() == 1 &&
              snapshot.buildJobs.front().state ==
                  BuildJobState::WaitingForMaterials,
          "build job did not wait for unavailable materials");
  require(sim.view().economy.cashCents < cashBefore,
          "queued construction did not reserve cash");

  require(sim.addConstructionMaterials({4, 4, 4, 4, 4}).ok,
          "construction materials could not be stocked");
  sim.step(1);
  snapshot = sim.constructionSnapshot();
  require(snapshot.buildJobs.front().state !=
              BuildJobState::WaitingForMaterials,
          "material arrival did not unblock construction");
}

void cancelling_unstarted_work_refunds_cash_and_reserved_materials() {
  Simulation sim(2002, 12, 8, 1);
  supportFloor(sim);
  require(sim.addConstructionMaterials({4, 4, 4, 4, 4}).ok,
          "construction materials could not be stocked");
  const auto cashBefore = sim.view().economy.cashCents;
  const auto materialBefore = sim.constructionSnapshot().availableMaterials;
  const auto queued = sim.queueBuild(deskPlan());
  require(queued.ok, "build plan did not queue for cancellation test");
  require(sim.cancelBuild(queued.jobId).ok,
          "unstarted build job could not be cancelled");
  const auto snapshot = sim.constructionSnapshot();
  require(sim.view().economy.cashCents == cashBefore,
          "unstarted cancellation did not refund reserved cash");
  require(snapshot.availableMaterials == materialBefore,
          "unstarted cancellation did not restore reserved materials");
  require(snapshot.buildJobs.front().state == BuildJobState::Cancelled,
          "cancelled build job did not retain diagnostic state");
}
} // namespace

int main() {
  try {
    construction_plan_reserves_cash_and_waits_for_materials();
    cancelling_unstarted_work_refunds_cash_and_reserved_materials();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All build-job tests passed\n";
  return 0;
}
