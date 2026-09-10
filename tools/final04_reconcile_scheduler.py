from pathlib import Path
import subprocess


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label} anchor mismatch: {count}")
    return text.replace(old, new, 1)


cmake = Path("game/CMakeLists.txt")
text = cmake.read_text(encoding="utf-8")
missing_sources = [
    src
    for src in ("src/Workforce.cpp", "src/Departments.cpp", "src/StaffOptimization.cpp")
    if src not in text
]
if missing_sources:
    insertion = "".join(f"  {src}\n" for src in missing_sources)
    text = text.replace(
        "  src/ServiceLogistics.cpp\n)",
        "  src/ServiceLogistics.cpp\n" + insertion + ")",
    )
if "hh_workforce_tests" not in text:
    text = text.replace(
        "endif()\n",
        """  add_executable(hh_workforce_tests tests/WorkforceTests.cpp)
  target_link_libraries(hh_workforce_tests PRIVATE hh_game)
  add_test(NAME Workforce COMMAND hh_workforce_tests)

  add_executable(hh_department_tests tests/DepartmentTests.cpp)
  target_link_libraries(hh_department_tests PRIVATE hh_game)
  add_test(NAME Department COMMAND hh_department_tests)

  add_executable(hh_optimizer_tests tests/OptimizerIntegrationTests.cpp)
  target_link_libraries(hh_optimizer_tests PRIVATE hh_game)
  add_test(NAME Optimizer COMMAND hh_optimizer_tests)
endif()
""",
        1,
    )
cmake.write_text(text, encoding="utf-8")

header = Path("game/include/hh/game/Simulation.h")
text = header.read_text(encoding="utf-8")
anchor = '#include "hh/game/ServiceLogistics.h"\n'
includes = "".join(
    f'#include "hh/game/{name}.h"\n'
    for name in ("Departments", "StaffOptimization", "Workforce")
)
if '#include "hh/game/Workforce.h"' not in text:
    text = text.replace(anchor, anchor + includes)
header.write_text(text, encoding="utf-8")

path = Path("game/src/Simulation.cpp")
merged = path.read_text(encoding="utf-8")
main = subprocess.check_output(
    ["git", "show", "origin/main:game/src/Simulation.cpp"], text=True
)
start = main.index("  void staffAndTasks() {")
end = main.index("  void guests() {", start)
fn = main[start:end]

fn = replace_once(
    fn,
    """        bool resources = true;
        if (task.kind == TaskKind::Turnover)
          resources = task.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) && inventory.linen >= 1 &&
                       inventory.towels >= 2 && inventory.amenities >= 1 &&
                       inventory.chemicals >= 1);
        if (task.kind == TaskKind::Repair)
          resources = task.resourcesClaimed || inventory.parts >= 1;
""",
    """        bool resources = true;
        if (task.kind == TaskKind::Turnover) {
          const auto &logistics = services.logistics();
          resources = task.resourcesClaimed ||
                      (has(TileKind::SupplyCloset) &&
                       logistics.inventoryUsable("clean_linen_set") >= 1 &&
                       logistics.inventoryUsable("towel_unit") >= 2 &&
                       logistics.inventoryUsable("amenity_kit") >= 1 &&
                       logistics.inventoryUsable("cleaning_chemical") >= 1);
        }
        if (task.kind == TaskKind::Repair)
          resources = task.resourcesClaimed ||
                      services.logistics().inventoryUsable("maintenance_part") >= 1;
""",
    "physical-resource-check",
)

fn = replace_once(
    fn,
    """          if (p.onShift && !p.absent && p.task == 0 && eligible(p, task)) {
""",
    """          if (p.onShift && !p.absent && p.task == 0 &&
              p.goal != "Preventive maintenance" && eligible(p, task)) {
""",
    "preventive-reservation",
)

fn = replace_once(
    fn,
    """        if (task.kind == TaskKind::Repair && !task.resourcesClaimed) {
          inventory.parts--;
          task.resourcesClaimed = true;
        }
""",
    """        if (task.kind == TaskKind::Repair && !task.resourcesClaimed)
          task.resourcesClaimed = true;
""",
    "repair-resource-authority",
)

fn = replace_once(
    fn,
    """          if (task.kind == TaskKind::Turnover &&
              same(p->destination, supply()) && !task.resourcesClaimed) {
            if (inventory.linen < 1 || inventory.towels < 2 ||
                inventory.amenities < 1 || inventory.chemicals < 1) {
              task.status = TaskStatus::Blocked;
              task.blockedReason = "Required local supplies unavailable";
              task.employeeId = 0;
              p->task = 0;
              p->state = PersonState::Idle;
              continue;
            }
            inventory.linen--;
            inventory.towels -= 2;
            inventory.amenities--;
            inventory.chemicals--;
            task.resourcesClaimed = true;
""",
    """          if (task.kind == TaskKind::Turnover &&
              same(p->destination, supply()) && !task.resourcesClaimed) {
            const auto &logistics = services.logistics();
            if (logistics.inventoryUsable("clean_linen_set") < 1 ||
                logistics.inventoryUsable("towel_unit") < 2 ||
                logistics.inventoryUsable("amenity_kit") < 1 ||
                logistics.inventoryUsable("cleaning_chemical") < 1) {
              task.status = TaskStatus::Blocked;
              task.blockedReason = "Required physical supplies unavailable";
              task.employeeId = 0;
              p->task = 0;
              p->state = PersonState::Idle;
              continue;
            }
            task.resourcesClaimed = true;
""",
    "turnover-resource-authority",
)

fn = replace_once(
    fn,
    """      } else {
        p->onBreak = false;
        p->inTraining = false;
        const double fatiguePerHour =
""",
    """      } else {
        p->onBreak = false;
        p->inTraining = false;
        if (task.kind == TaskKind::Turnover) {
          const auto serviceWork = services.workRoomTurnSecond(task.targetId);
          if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
            task.status = TaskStatus::Blocked;
            task.blockedReason = !serviceWork.valid
                                     ? "FINAL-04 room-turn job missing"
                                     : "FINAL-04 room-turn resources blocked";
            task.employeeId = 0;
            p->task = 0;
            p->state = PersonState::Idle;
            continue;
          }
        } else if (task.kind == TaskKind::Repair) {
          const auto serviceWork = services.workEngineeringSecond(
              task.targetId, WorkOrderType::Corrective);
          if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
            task.status = TaskStatus::Blocked;
            task.blockedReason = !serviceWork.valid
                                     ? "FINAL-04 engineering work order missing"
                                     : "FINAL-04 engineering resources blocked";
            task.employeeId = 0;
            p->task = 0;
            p->state = PersonState::Idle;
            continue;
          }
        }
        const double fatiguePerHour =
""",
    "service-work-hook",
)

fn = replace_once(
    fn,
    """      if (task.workRemainingSeconds > 0)
        continue;

      task.status = TaskStatus::Completed;
""",
    """      const bool serviceTurnComplete =
          task.kind != TaskKind::Turnover ||
          services.housekeeping().roomStatus(task.targetId) ==
              ServiceRoomStatus::Ready;
      bool serviceRepairComplete = task.kind != TaskKind::Repair;
      if (task.kind == TaskKind::Repair) {
        const auto engineering = services.engineering().snapshot();
        for (const auto &order : engineering.workOrders)
          if (order.assetId == task.targetId &&
              order.type == WorkOrderType::Corrective &&
              order.stage == WorkOrderStage::Completed)
            serviceRepairComplete = true;
      }
      if (task.workRemainingSeconds > 0 || !serviceTurnComplete ||
          !serviceRepairComplete)
        continue;

      task.status = TaskStatus::Completed;
""",
    "service-completion-gate",
)

fn = replace_once(
    fn,
    """        if (task.kind == TaskKind::Repair) {
          room->condition = 100;
""",
    """        if (task.kind == TaskKind::Repair) {
          const auto engineering = services.engineering().snapshot();
          for (const auto &asset : engineering.assets)
            if (asset.id == room->id)
              room->condition = asset.condition / 100.0;
""",
    "repair-condition-authority",
)

fn = replace_once(
    fn,
    """    for (EntityId roomId : failedRooms)
      if (auto *room = getRoom(roomId))
        createTask(TaskKind::Repair, roomId, room->door, repairWork);
  }
""",
    """    for (EntityId roomId : failedRooms)
      if (auto *room = getRoom(roomId))
        createTask(TaskKind::Repair, roomId, room->door, repairWork);

    // FINAL-03 scheduler supplies labor to authoritative FINAL-04 preventive work.
    std::unordered_set<EntityId> preventiveWorkers;
    const auto engineering = services.engineering().snapshot();
    for (const auto &order : engineering.workOrders) {
      if (order.type != WorkOrderType::Preventive ||
          order.stage == WorkOrderStage::Completed)
        continue;
      auto *target = getRoom(order.assetId);
      if (!target)
        continue;
      Person *best = nullptr;
      int bestDistance = std::numeric_limits<int>::max();
      for (auto &person : people) {
        if (person.kind != PersonKind::Maintenance || !person.onShift ||
            person.absent || person.task != 0 ||
            preventiveWorkers.contains(person.id))
          continue;
        const int distance = manhattan(person.position, target->door);
        if (distance < bestDistance ||
            (distance == bestDistance && (!best || person.id < best->id))) {
          bestDistance = distance;
          best = &person;
        }
      }
      if (!best)
        continue;
      preventiveWorkers.insert(best->id);
      best->destination = target->door;
      best->goal = "Preventive maintenance";
      if (!same(best->position, best->destination)) {
        auto route = path(best->position, best->destination);
        if (route.empty()) {
          best->state = PersonState::Idle;
          best->goal.clear();
          continue;
        }
        best->state = PersonState::Traveling;
        best->position = route.front();
        ++best->travelSeconds;
        best->fatigue = std::min(100.0, best->fatigue + 4.0 / 3600.0);
        continue;
      }
      const auto serviceWork = services.workEngineeringSecond(
          order.assetId, WorkOrderType::Preventive);
      if (!serviceWork.valid || serviceWork.blockedReason != BlockReason::None) {
        best->state = PersonState::Idle;
        best->goal.clear();
        continue;
      }
      best->state = PersonState::Working;
      best->fatigue = std::min(100.0, best->fatigue + 6.0 / 3600.0);
      if (serviceWork.completed) {
        best->state = PersonState::Idle;
        best->goal.clear();
      }
    }
  }
""",
    "preventive-scheduler",
)

mstart = merged.index("  void staffAndTasks() {")
mend = merged.index("  void guests() {", mstart)
merged = merged[:mstart] + fn + merged[mend:]
path.write_text(merged, encoding="utf-8")
