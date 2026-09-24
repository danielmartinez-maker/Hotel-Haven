#include "hh/game/Simulation.h"

#include <algorithm>
#include <stdexcept>

namespace hh::game {
namespace {

StaffRole roleForPerson(PersonKind kind) {
  switch (kind) {
  case PersonKind::Receptionist:
    return StaffRole::Receptionist;
  case PersonKind::Housekeeper:
    return StaffRole::Housekeeper;
  case PersonKind::Maintenance:
    return StaffRole::Maintenance;
  case PersonKind::Guest:
    break;
  }
  throw std::invalid_argument("guest has no department role");
}

} // namespace

std::vector<DepartmentView> Simulation::departments() const {
  const auto current = view();
  std::vector<DepartmentView> result;
  result.reserve(allDepartments().size());

  for (const auto department : allDepartments()) {
    DepartmentView departmentView;
    departmentView.id = department;
    departmentView.name = departmentName(department);
    departmentView.role = departmentRole(department);

    for (const auto &person : current.people) {
      if (person.kind == PersonKind::Guest)
        continue;
      if (roleForPerson(person.kind) == departmentView.role)
        departmentView.directReports.push_back(person.id);
    }
    std::sort(departmentView.directReports.begin(),
              departmentView.directReports.end());
    result.push_back(std::move(departmentView));
  }
  return result;
}

} // namespace hh::game
