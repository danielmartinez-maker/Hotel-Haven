#include "hh/game/Departments.h"
#include "hh/game/Simulation.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static const PersonView &employee(const SimulationView &view, EntityId id) {
  for (const auto &person : view.people)
    if (person.id == id)
      return person;
  throw std::runtime_error("employee missing");
}

static void department_forecast_reports_uncovered_labor() {
  auto sim = Simulation::tutorial(301);
  require(sim.loadDefinitions(R"({"baseDemand":100})").ok,
          "forecast demand definitions rejected");
  sim.step(3601);

  int forecastDay = -1;
  for (const auto &reservation : sim.view().reservations)
    if (!reservation.completed && reservation.departureDay >= sim.view().day)
      forecastDay = forecastDay < 0 ? reservation.departureDay
                                    : std::min(forecastDay,
                                               reservation.departureDay);
  require(forecastDay >= 0, "forecast scenario produced no known reservation");

  for (const auto &person : sim.view().people)
    if (person.kind == PersonKind::Housekeeper)
      require(sim.fireStaff(person.id).ok, "failed to remove housekeeper");

  const auto forecast =
      sim.departmentForecast(DepartmentId::Housekeeping, forecastDay);
  require(forecast.requiredMinutes > forecast.scheduledMinutes,
          "forecast did not expose uncovered housekeeping labor");
  require(forecast.uncoveredMinutes ==
              forecast.requiredMinutes - forecast.scheduledMinutes,
          "forecast uncovered total is inconsistent");
  require(forecast.buckets.size() == 4,
          "forecast did not expose fixed six-hour planning buckets");
}

static void manager_span_excludes_other_departments() {
  auto sim = Simulation::tutorial(302);
  const auto extra =
      sim.hireStaff({"Rooms Two", PersonKind::Housekeeper, 8, 16, 18});
  require(extra.ok, "second housekeeper hire failed");

  EntityId manager{};
  EntityId receptionist{};
  for (const auto &person : sim.view().people) {
    if (!manager && person.kind == PersonKind::Housekeeper)
      manager = person.id;
    if (person.kind == PersonKind::Receptionist)
      receptionist = person.id;
  }
  require(manager != 0 && receptionist != 0, "manager scenario staff missing");
  require(sim.assignDepartmentManager(DepartmentId::Housekeeping, manager).ok,
          "valid housekeeping manager assignment rejected");
  require(!sim.assignDepartmentManager(DepartmentId::Housekeeping,
                                       receptionist),
          "cross-department manager assignment was accepted");

  const auto departments = sim.departments();
  const auto it = std::find_if(
      departments.begin(), departments.end(), [](const DepartmentView &view) {
        return view.id == DepartmentId::Housekeeping;
      });
  require(it != departments.end(), "housekeeping department missing");
  require(it->managerId == manager, "manager assignment not visible");
  require(std::find(it->directReports.begin(), it->directReports.end(),
                    receptionist) == it->directReports.end(),
          "manager span included employee from another department");
  for (const auto reportId : it->directReports)
    require(employee(sim.view(), reportId).kind == PersonKind::Housekeeper,
            "manager span contains a non-housekeeping report");
}

static void organization_and_forecast_round_trip() {
  auto sim = Simulation::tutorial(303);
  EntityId manager{};
  for (const auto &person : sim.view().people)
    if (person.kind == PersonKind::Housekeeper) {
      manager = person.id;
      break;
    }
  require(manager != 0, "round-trip manager missing");
  require(sim.assignDepartmentManager(DepartmentId::Housekeeping, manager).ok,
          "round-trip manager assignment failed");
  const auto beforeDepartments = sim.departments();
  const auto beforeForecast =
      sim.departmentForecast(DepartmentId::Housekeeping, sim.view().day);

  auto loaded = Simulation::load(sim.save());
  require(loaded.departments() == beforeDepartments,
          "department organization did not survive save/load");
  require(loaded.departmentForecast(DepartmentId::Housekeeping,
                                    loaded.view().day) == beforeForecast,
          "department forecast changed across save/load");
}

int main() {
  try {
    department_forecast_reports_uncovered_labor();
    manager_span_excludes_other_departments();
    organization_and_forecast_round_trip();
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "Department tests passed\n";
  return 0;
}
