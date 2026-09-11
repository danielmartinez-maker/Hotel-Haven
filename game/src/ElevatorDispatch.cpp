#include "hh/game/BuildingSystems.h"
#include "hh/game/Simulation.h"

namespace hh::game {
CommandResult Simulation::requestElevator(ElevatorKind kind, int pickupFloor,
                                          int destinationFloor) {
  if (kind != ElevatorKind::Passenger && kind != ElevatorKind::Service)
    return {false, "Elevator bank kind is invalid"};
  if (pickupFloor == destinationFloor)
    return {false, "Elevator request requires a different destination floor"};
  const auto elevatorId = detail::selectElevatorForRequest(
      buildingSystemsSnapshot(), kind, pickupFloor, destinationFloor);
  if (elevatorId == 0)
    return {false, "No compatible elevator serves the requested floors"};
  return requestElevator(elevatorId, pickupFloor, destinationFloor);
}
} // namespace hh::game
