#include "hh/game/Simulation.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static const DepartmentView &findDepartment(
    const std::vector<DepartmentView> &departments, DepartmentId id) {
  const auto it = std::find_if(
      departments.begin(), departments.end(),
      [&](const auto &department) { return department.id == id; });
  if (it == departments.end())
    throw std::runtime_error("department missing");
  return *it;
}

int main() {
  try {
    Simulation sim(611, 8, 8, 1);
    require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
            "department entrance failed");

    const auto front =
        sim.hireStaff({"Front", PersonKind::Receptionist, 0, 0, 20});
    const auto roomsA =
        sim.hireStaff({"Rooms A", PersonKind::Housekeeper, 0, 0, 18});
    const auto roomsB =
        sim.hireStaff({"Rooms B", PersonKind::Housekeeper, 8, 16, 18});
    const auto engineering =
        sim.hireStaff({"Engineer", PersonKind::Maintenance, 0, 0, 24});
    require(front.ok && roomsA.ok && roomsB.ok && engineering.ok,
            "department staff hire failed");

    const auto before = sim.save();
    const auto departments = sim.departments();
    require(sim.save() == before, "department projection mutated simulation");
    require(departments.size() == 3, "department catalog size changed");

    const auto &frontOffice =
        findDepartment(departments, DepartmentId::FrontOffice);
    const auto &housekeeping =
        findDepartment(departments, DepartmentId::Housekeeping);
    const auto &engineeringDepartment =
        findDepartment(departments, DepartmentId::Engineering);

    require(frontOffice.name == "Front Office" &&
                frontOffice.role == StaffRole::Receptionist,
            "front office metadata mismatch");
    require(frontOffice.managerId == 0,
            "read-only roster invented a manager");
    require(frontOffice.directReports == std::vector<std::uint64_t>{front.id},
            "front office roster mismatch");

    const std::vector<std::uint64_t> expectedRooms{roomsA.id, roomsB.id};
    require(housekeeping.directReports == expectedRooms,
            "housekeeping roster mismatch");
    require(engineeringDepartment.directReports ==
                std::vector<std::uint64_t>{engineering.id},
            "engineering roster mismatch");

    require(sim.fireStaff(roomsA.id).ok, "department firing failed");
    const auto refreshed = sim.departments();
    require(findDepartment(refreshed, DepartmentId::Housekeeping)
                .directReports ==
                std::vector<std::uint64_t>{roomsB.id},
            "department roster did not reflect authoritative firing");
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "Department roster tests passed\n";
  return 0;
}
