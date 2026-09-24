#include "hh/game/Departments.h"

#include <array>
#include <stdexcept>

namespace hh::game {

StaffRole departmentRole(DepartmentId department) {
  switch (department) {
  case DepartmentId::FrontOffice:
    return StaffRole::Receptionist;
  case DepartmentId::Housekeeping:
    return StaffRole::Housekeeper;
  case DepartmentId::Engineering:
    return StaffRole::Maintenance;
  }
  throw std::invalid_argument("invalid department");
}

const char *departmentName(DepartmentId department) {
  switch (department) {
  case DepartmentId::FrontOffice:
    return "Front Office";
  case DepartmentId::Housekeeping:
    return "Housekeeping";
  case DepartmentId::Engineering:
    return "Engineering";
  }
  throw std::invalid_argument("invalid department");
}

const std::vector<DepartmentId> &allDepartments() {
  static const std::vector<DepartmentId> departments{
      DepartmentId::FrontOffice, DepartmentId::Housekeeping,
      DepartmentId::Engineering};
  return departments;
}

} // namespace hh::game
