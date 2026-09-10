#include "hh/game/Simulation.h"
#include <algorithm>
#include <cstdlib>
#include "hh/game/Workforce.h"

#include <iostream>
#include <stdexcept>

using namespace hh::game;

static void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

static PersonView employee(const SimulationView &view, EntityId id) {
  for (const auto &person : view.people)
    if (person.id == id)
      return person;
  throw std::runtime_error("employee missing");
}

static void applicant_pool_is_deterministic_for_campaign_seed_and_day() {
  auto a = Simulation::tutorial(99);
  auto b = Simulation::tutorial(99);

  const auto applicantsA = a.applicants();
  const auto applicantsB = b.applicants();
  require(!applicantsA.empty(), "applicant pool was empty");
  require(applicantsA == applicantsB,
          "same campaign seed/day produced different applicants");
}

static void hiring_deducts_configured_onboarding_cost_and_creates_contract() {
  auto sim = Simulation::tutorial(101);
  require(sim.loadDefinitions(R"({"onboardingCostCents":12345})").ok,
          "onboarding configuration rejected");

  const auto applicants = sim.applicants();
  require(!applicants.empty(), "no applicant available to hire");
  const auto beforeCash = sim.view().economy.cashCents;

  const auto result = sim.hireApplicant(applicants.front().id);
  require(result.ok, "valid applicant hire was rejected");

  const auto view = sim.view();
  const auto &hired = employee(view, result.employeeId);
  require(hired.contract.hourlyWageCents > 0,
          "hired employee contract has no hourly wage");
  require(hired.contract.onboardingCostCents == 12345,
          "configured onboarding cost was not persisted on contract");
  require(view.economy.cashCents == beforeCash - 12345,
          "configured onboarding cost was not deducted exactly");
}

static void workforce_state_round_trips_through_save() {
  auto sim = Simulation::tutorial(103);
  require(sim.loadDefinitions(R"({"onboardingCostCents":7777})").ok,
          "round-trip onboarding configuration rejected");
  const auto applicant = sim.applicants().front();
  const auto hired = sim.hireApplicant(applicant.id);
  require(hired.ok, "round-trip applicant hire failed");
  const auto beforeEmployee = employee(sim.view(), hired.employeeId);
  const auto beforeApplicants = sim.applicants();

  auto loaded = Simulation::load(sim.save());
  const auto afterEmployee = employee(loaded.view(), hired.employeeId);
  require(afterEmployee.contract == beforeEmployee.contract,
          "employee contract did not survive save/load");
  require(afterEmployee.reliability == beforeEmployee.reliability,
          "employee reliability did not survive save/load");
  require(loaded.applicants() == beforeApplicants,
          "consumed applicant state did not survive save/load");
}

static Simulation make_staff_scenario(std::uint64_t seed, EntityId &employeeId,
                                      EntityId &roomId) {
  Simulation sim(seed, 16, 12, 1);
  for (int x = 0; x <= 8; ++x)
    require(sim.buildTile({0, x, 0},
                          x == 0 ? TileKind::Entrance : TileKind::Floor)
                .ok,
            "staff scenario corridor failed");
  require(sim.buildTile({0, 2, 0}, TileKind::SupplyCloset).ok,
          "staff scenario supply closet failed");
  const auto room = sim.buildFurnishedRoom(
      {"101", 0, 4, 1, 4, 4, {0, 4, 1}, 1, 1, 120});
  require(room.ok, "staff scenario room failed");
  const auto hire =
      sim.hireStaff({"Cleaner", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "staff scenario hire failed");
  employeeId = hire.id;
  roomId = room.id;
  return sim;
}

static void missed_breaks_increase_fatigue_and_reduce_morale() {
  EntityId employeeId{}, roomId{};
  auto sim = make_staff_scenario(201, employeeId, roomId);
  require(sim.loadDefinitions(
                 R"({"turnoverWorkSeconds":7200,"staffBreakAfterMinutes":1,"staffBreakDurationMinutes":1,"missedBreakFatiguePerHour":60,"missedBreakMoralePerHour":30})")
              .ok,
          "break policy configuration rejected");
  require(sim.requestClean(roomId).ok, "long cleaning task rejected");
  sim.step(150);
  const auto &worker = employee(sim.view(), employeeId);
  require(worker.fatigue > 0, "missed break did not increase fatigue");
  require(worker.morale < 100, "missed break did not reduce morale");
  require(worker.breakMinutesTakenToday == 0,
          "busy employee somehow completed a break");
}

static void idle_employee_completes_policy_break() {
  Simulation sim(202, 8, 8, 1);
  require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "break scenario entrance failed");
  const auto hire =
      sim.hireStaff({"Idle", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "idle employee hire failed");
  require(sim.loadDefinitions(
                 R"({"staffBreakAfterMinutes":1,"staffBreakDurationMinutes":1})")
              .ok,
          "break duration configuration rejected");
  sim.step(150);
  const auto &worker = employee(sim.view(), hire.id);
  require(worker.breakMinutesTakenToday >= 1,
          "eligible idle employee never completed a break");
}

static void training_never_overlaps_work_and_skill_changes_on_completion() {
  EntityId busyId{}, roomId{};
  auto busy = make_staff_scenario(203, busyId, roomId);
  require(busy.loadDefinitions(R"({"turnoverWorkSeconds":7200})").ok,
          "busy training definitions rejected");
  require(busy.requestClean(roomId).ok, "busy training clean rejected");
  busy.step(2);
  require(!busy.scheduleTraining(busyId, busy.view().elapsedSeconds + 60, 1),
          "training overlapped an active work assignment");

  Simulation idle(204, 8, 8, 1);
  require(idle.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "training scenario entrance failed");
  const auto hire =
      idle.hireStaff({"Trainee", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "trainee hire failed");
  require(idle.loadDefinitions(R"({"trainingSkillGain":10})").ok,
          "training skill configuration rejected");
  const auto initialSkill = employee(idle.view(), hire.id).skill;
  require(idle.scheduleTraining(hire.id, idle.view().elapsedSeconds + 1, 1).ok,
          "valid training reservation rejected");
  idle.step(30);
  require(employee(idle.view(), hire.id).skill == initialSkill,
          "training changed skill before completion");
  idle.step(40);
  require(employee(idle.view(), hire.id).skill == initialSkill + 10,
          "completed training did not apply configured skill gain");
}

static void due_break_has_priority_over_ready_training() {
  Simulation sim(210, 8, 8, 1);
  require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "break-priority scenario entrance failed");
  const auto hire =
      sim.hireStaff({"Priority", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "break-priority employee hire failed");
  require(sim.loadDefinitions(
                 R"({"staffBreakAfterMinutes":1,"staffBreakDurationMinutes":1})")
              .ok,
          "break-priority definitions rejected");
  require(sim.scheduleTraining(hire.id, 60, 1).ok,
          "break-priority training scheduling failed");

  sim.step(61);
  bool breakActive = false;
  bool trainingActive = false;
  for (const auto &task : sim.view().tasks) {
    if (task.employeeId != hire.id ||
        (task.status != TaskStatus::Traveling && task.status != TaskStatus::Working))
      continue;
    breakActive |= task.kind == TaskKind::Break;
    trainingActive |= task.kind == TaskKind::Training;
  }
  require(breakActive, "due break did not preempt ready training");
  require(!trainingActive, "training ran before policy break");
}

static void absence_is_deterministic_at_shift_boundaries() {
  auto make = []() {
    Simulation sim(205, 8, 8, 1);
    require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
            "absence scenario entrance failed");
    const auto pool = sim.applicants();
    const Applicant *candidate = &pool.front();
    for (const auto &applicant : pool)
      if (applicant.reliability < candidate->reliability)
        candidate = &applicant;
    const auto hired = sim.hireApplicant(candidate->id);
    require(hired.ok, "absence applicant hire failed");
    require(sim.setStaffShift(hired.employeeId, 1, 2).ok,
            "absence test shift change failed");
    return std::pair<Simulation, EntityId>{std::move(sim), hired.employeeId};
  };

  auto [a, employeeA] = make();
  auto [b, employeeB] = make();
  bool sawAbsence = false;
  for (int day = 0; day < 20; ++day) {
    a.step(day == 0 ? 3601 : 23 * 3600 + 1);
    b.step(day == 0 ? 3601 : 23 * 3600 + 1);
    const auto &ea = employee(a.view(), employeeA);
    const auto &eb = employee(b.view(), employeeB);
    require(ea.absent == eb.absent,
            "same seed/shift produced different absence decisions");
    if (ea.absent) {
      sawAbsence = true;
      require(!ea.onShift, "absent employee remained active labor");
    }
    a.step(3600);
    b.step(3600);
  }
  require(sawAbsence, "deterministic absence scenario never produced absence");
}

static void firing_employee_cancels_personal_workforce_tasks() {
  Simulation sim(207, 8, 8, 1);
  require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "fire-task scenario entrance failed");
  const auto hire =
      sim.hireStaff({"Departing", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "fire-task scenario hire failed");
  require(sim.scheduleTraining(hire.id, sim.view().elapsedSeconds + 600, 30).ok,
          "future training scheduling failed");
  require(sim.fireStaff(hire.id).ok, "employee firing failed");

  for (const auto &task : sim.view().tasks)
    require(!((task.kind == TaskKind::Break || task.kind == TaskKind::Training) &&
              task.targetId == hire.id && task.status != TaskStatus::Completed),
            "fired employee retained personal workforce task");

  auto loaded = Simulation::load(sim.save());
  require(loaded.save() == sim.save(),
          "fired employee workforce cleanup did not round-trip");
}

static void require_assignment_invariants(const SimulationView &view) {
  std::vector<EntityId> activeEmployees;
  for (const auto &task : view.tasks) {
    if (task.status != TaskStatus::Traveling && task.status != TaskStatus::Working)
      continue;
    require(task.employeeId != 0, "active task has no employee");
    require(std::find(activeEmployees.begin(), activeEmployees.end(), task.employeeId) ==
                activeEmployees.end(),
            "employee assigned to overlapping active tasks");
    activeEmployees.push_back(task.employeeId);

    bool foundEmployee = false;
    for (const auto &person : view.people)
      if (person.id == task.employeeId) {
        foundEmployee = true;
        require(person.kind != PersonKind::Guest,
                "guest assigned to staff task");
        require(!person.absent, "absent employee assigned active task");
      }
    require(foundEmployee, "active task references missing employee");
  }
}

static void three_hundred_employee_snapshot_and_scheduler_remain_valid() {
  Simulation sim(208, 16, 12, 1);
  require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "scale scenario entrance failed");
  for (int index = 0; index < 300; ++index) {
    const PersonKind role = index % 3 == 0
                                ? PersonKind::Receptionist
                                : index % 3 == 1 ? PersonKind::Housekeeper
                                                 : PersonKind::Maintenance;
    const auto hired = sim.hireStaff(
        {"Scale " + std::to_string(index), role, 0, 0, 18.0 + (index % 5)});
    require(hired.ok, "300-employee scale hire failed");
  }
  require(sim.loadDefinitions(
                 R"({"staffBreakAfterMinutes":60,"staffBreakDurationMinutes":15})")
              .ok,
          "scale workforce definitions rejected");

  const auto snapshot = sim.buildOptimizerSnapshot();
  require(snapshot.employees.size() == 300,
          "optimizer snapshot omitted employees at target scale");
  sim.step(6 * 3600);
  const auto view = sim.view();
  int staffCount = 0;
  for (const auto &person : view.people)
    staffCount += person.kind != PersonKind::Guest;
  require(staffCount == 300, "target-scale workforce changed unexpectedly");
  require_assignment_invariants(view);
}

static void thirty_day_workforce_soak_preserves_assignment_invariants() {
  auto sim = Simulation::tutorial(209);
  require(sim.loadDefinitions(
                 R"({"baseDemand":100,"initialLinen":1000,"initialTowels":2000,"initialAmenities":1000,"initialChemicals":1000,"initialParts":500,"staffBreakAfterMinutes":240,"staffBreakDurationMinutes":15})")
              .ok,
          "soak definitions rejected");
  for (int index = 0; index < 18; ++index) {
    const PersonKind role = index % 3 == 0
                                ? PersonKind::Receptionist
                                : index % 3 == 1 ? PersonKind::Housekeeper
                                                 : PersonKind::Maintenance;
    require(sim.hireStaff({"Soak " + std::to_string(index), role, 0, 0, 20}).ok,
            "soak employee hire failed");
  }

  for (int interval = 0; interval < 30 * 4; ++interval) {
    sim.step(6 * 3600);
    require_assignment_invariants(sim.view());
  }

  auto restored = Simulation::load(sim.save());
  require(restored.save() == sim.save(),
          "30-day workforce soak state did not round-trip");
}

static void workforce_dynamics_round_trip_without_divergence() {
  Simulation sim(206, 8, 8, 1);
  require(sim.buildTile({0, 0, 0}, TileKind::Entrance).ok,
          "dynamic save scenario entrance failed");
  const auto hire =
      sim.hireStaff({"Persistent", PersonKind::Housekeeper, 0, 0, 18});
  require(hire.ok, "dynamic save scenario hire failed");
  require(sim.loadDefinitions(
                 R"({"staffBreakAfterMinutes":2,"staffBreakDurationMinutes":1,"trainingSkillGain":7})")
              .ok,
          "dynamic save definitions rejected");
  require(sim.scheduleTraining(hire.id, 5 * 60, 1).ok,
          "future training scheduling failed");
  sim.step(90);

  auto loaded = Simulation::load(sim.save());
  sim.step(300);
  loaded.step(300);
  require(sim.save() == loaded.save(),
          "workforce dynamics diverged after save/load continuation");
}

int main() {
  try {
    applicant_pool_is_deterministic_for_campaign_seed_and_day();
    hiring_deducts_configured_onboarding_cost_and_creates_contract();
    workforce_state_round_trips_through_save();
    missed_breaks_increase_fatigue_and_reduce_morale();
    idle_employee_completes_policy_break();
    training_never_overlaps_work_and_skill_changes_on_completion();
    due_break_has_priority_over_ready_training();
    absence_is_deterministic_at_shift_boundaries();
    workforce_dynamics_round_trip_without_divergence();
    firing_employee_cancels_personal_workforce_tasks();
    if (std::getenv("HH_SKIP_LONG_WORKFORCE_GATES") == nullptr) {
      three_hundred_employee_snapshot_and_scheduler_remain_valid();
      thirty_day_workforce_soak_preserves_assignment_invariants();
    }
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
  std::cout << "Workforce tests passed\n";
  return 0;
}
