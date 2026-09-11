#include "hh/game/BuildingSystems.h"
#include "hh/game/Simulation.h"
#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

void installBank(Simulation &sim) {
  ElevatorSpec spec;
  spec.minFloor = 0;
  spec.maxFloor = 15;
  spec.capacity = 6;
  spec.travelSecondsPerFloor = 2;
  spec.doorSeconds = 1;
  spec.kind = ElevatorKind::Passenger;
  spec.startFloor = 0;
  require(sim.installElevator(spec).ok, "passenger car 1 install failed");
  spec.startFloor = 15;
  require(sim.installElevator(spec).ok, "passenger car 2 install failed");
  spec.kind = ElevatorKind::Service;
  spec.capacity = 4;
  spec.startFloor = 3;
  require(sim.installElevator(spec).ok, "service car 1 install failed");
  spec.startFloor = 12;
  require(sim.installElevator(spec).ok, "service car 2 install failed");
}

void assertCapacity(const Simulation &sim) {
  for (const auto &car : sim.buildingSystemsSnapshot().elevators) {
    int boarded = 0;
    for (const auto &request : car.requests)
      boarded += request.boarded ? 1 : 0;
    require(boarded <= car.capacity, "elevator exceeded hard capacity");
  }
}

void selector_accounts_for_committed_onboard_sweep() {
  Simulation sim(91000, 8, 8, 16);
  ElevatorSpec spec;
  spec.kind = ElevatorKind::Passenger;
  spec.minFloor = 0;
  spec.maxFloor = 15;
  spec.capacity = 2;
  spec.travelSecondsPerFloor = 2;
  spec.doorSeconds = 1;
  spec.startFloor = 0;
  const auto busy = sim.installElevator(spec);
  spec.startFloor = 10;
  const auto idle = sim.installElevator(spec);
  require(busy.ok && idle.ok, "selector ETA fixture install failed");

  require(sim.requestElevator(busy.id, 0, 1).ok &&
              sim.requestElevator(busy.id, 0, 15).ok,
          "selector ETA fixture direct requests failed");
  sim.step(2);
  const auto before = sim.buildingSystemsSnapshot();
  require(before.elevators[0].state == ElevatorState::MovingToDestination,
          "busy car did not enter committed onboard sweep");

  const auto request = sim.requestElevator(ElevatorKind::Passenger, 2, 3);
  require(request.ok, "selector ETA bank request failed");
  const auto after = sim.buildingSystemsSnapshot();
  require(after.elevators[0].id == busy.id && after.elevators[1].id == idle.id,
          "selector ETA fixture car ordering changed");
  require(after.elevators[0].requests.size() == 2 &&
              after.elevators[1].requests.size() == 1 &&
              after.elevators[1].requests.front().id == request.id,
          "dispatcher underestimated a committed onboard sweep and selected the busy car");
}

void deterministic_burst_soak_preserves_bank_and_capacity_invariants() {
  Simulation a(91001, 8, 8, 16);
  Simulation b(91001, 8, 8, 16);
  installBank(a);
  installBank(b);

  for (int index = 0; index < 400; ++index) {
    const auto kind = index % 5 == 0 ? ElevatorKind::Service
                                     : ElevatorKind::Passenger;
    const int pickup = (index * 7 + 3) % 16;
    int destination = (index * 11 + 9) % 16;
    if (destination == pickup)
      destination = (destination + 5) % 16;
    const auto ra = a.requestElevator(kind, pickup, destination);
    const auto rb = b.requestElevator(kind, pickup, destination);
    require(ra.ok && rb.ok && ra.id == rb.id,
            "same-seed burst request diverged");
    a.step(1);
    b.step(1);
    assertCapacity(a);
    assertCapacity(b);
    require(a.buildingSystemsSnapshot() == b.buildingSystemsSnapshot(),
            "same-seed bank state diverged during burst");
  }

  for (int second = 0; second < 20000; ++second) {
    a.step(1);
    b.step(1);
    assertCapacity(a);
    assertCapacity(b);
  }
  const auto sa = a.buildingSystemsSnapshot();
  const auto sb = b.buildingSystemsSnapshot();
  require(sa == sb, "same-seed final elevator snapshots diverged");
  require(a.save() == b.save(), "same-seed final save bytes diverged");
  for (const auto &car : sa.elevators)
    require(car.requests.empty(),
            "burst soak left stranded elevator requests");
}
} // namespace

int main() {
  try {
    selector_accounts_for_committed_onboard_sweep();
    deterministic_burst_soak_preserves_bank_and_capacity_invariants();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All elevator dispatch soak tests passed\n";
  return 0;
}
