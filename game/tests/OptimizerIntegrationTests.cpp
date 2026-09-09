#include "hh/game/Simulation.h"

#include <iostream>
#include <stdexcept>

using namespace hh::game;

namespace {

void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

void guest_checkout_preempts_older_checkin_backlog() {
  auto simulation = Simulation::tutorial(410);
  require(simulation.loadDefinitions(R"({"baseDemand":100})").ok,
          "optimizer integration definitions rejected");

  EntityId originalReceptionist = 0;
  for (const auto &person : simulation.view().people)
    if (person.kind == PersonKind::Receptionist)
      originalReceptionist = person.id;
  require(originalReceptionist != 0, "tutorial receptionist missing");

  // Allow the arrival wave to reach reception and complete one check-in, then
  // remove reception coverage so older check-in work remains queued.
  simulation.step(3600 + 360);
  require(simulation.fireStaff(originalReceptionist).ok,
          "could not remove original receptionist");

  // Four days guarantees the already-checked-in guest reaches its departure
  // day while the remaining original check-in tasks are still waiting.
  simulation.step(4 * 86400);
  const auto queued = simulation.view();
  bool hasCheckIn = false;
  bool hasCheckOut = false;
  for (const auto &task : queued.tasks) {
    if (task.status == TaskStatus::Completed)
      continue;
    hasCheckIn |= task.kind == TaskKind::CheckIn;
    hasCheckOut |= task.kind == TaskKind::CheckOut;
  }
  require(hasCheckIn, "test did not produce an older check-in backlog");
  require(hasCheckOut, "test did not produce a checkout task");

  const auto relief = simulation.hireStaff(
      {"Relief", PersonKind::Receptionist, 0, 0, 20});
  require(relief.ok, "could not hire relief receptionist");
  simulation.step(1);

  TaskKind assignedKind = TaskKind::Turnover;
  bool foundAssignment = false;
  for (const auto &task : simulation.view().tasks)
    if (task.employeeId == relief.id && task.status != TaskStatus::Completed) {
      assignedKind = task.kind;
      foundAssignment = true;
      break;
    }

  require(foundAssignment, "relief receptionist received no queued work");
  require(assignedKind == TaskKind::CheckOut,
          "higher-priority checkout did not preempt older check-in backlog");
}

} // namespace

int main() {
  try {
    guest_checkout_preempts_older_checkin_backlog();
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
  std::cout << "Optimizer integration tests passed\n";
  return 0;
}
