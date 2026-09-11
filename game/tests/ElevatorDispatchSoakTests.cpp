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
    deterministic_burst_soak_preserves_bank_and_capacity_invariants();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "All elevator dispatch soak tests passed\n";
  return 0;
}
