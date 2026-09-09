#!/usr/bin/env python3
"""Optional NVIDIA cuOpt MILP worker for Hotel Haven staff scheduling.

The shipping game never imports this module. The native simulation exports a
canonical request, invokes this worker in a compatible development environment
(typically WSL2/Linux with cuOpt installed), validates the returned proposal,
and records the accepted plan as an authoritative simulation event.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

OBJECTIVES = (
    "critical_service",
    "guest_wait_lateness",
    "labor_travel_cost",
    "fatigue_zone_switch",
)
ACCEPTED_STATUS_NAMES = {"Optimal", "PrimalFeasible"}


def _require_int(obj: dict[str, Any], key: str, *, minimum: int | None = None) -> int:
    value = obj.get(key)
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{key} must be an integer")
    if minimum is not None and value < minimum:
        raise ValueError(f"{key} must be >= {minimum}")
    return value


def validate_request(request: dict[str, Any]) -> None:
    if request.get("schema") != 1:
        raise ValueError("unsupported request schema")
    _require_int(request, "optimization_epoch", minimum=0)
    _require_int(request, "simulation_second", minimum=0)
    _require_int(request, "snapshot_fingerprint", minimum=0)
    _require_int(request, "bucket_minutes", minimum=1)
    _require_int(request, "horizon_buckets", minimum=1)

    objectives = request.get("lexicographic_objectives")
    if objectives != list(OBJECTIVES):
        raise ValueError("lexicographic_objectives must match Hotel Haven objective order")

    employees = request.get("employees")
    tasks = request.get("tasks")
    candidates = request.get("candidates")
    service_stations = request.get("service_stations")
    station_candidates = request.get("station_candidates")
    if not all(isinstance(value, list) for value in (employees, tasks, candidates, service_stations, station_candidates)):
        raise ValueError("employees, tasks, candidates, service_stations, and station_candidates must be arrays")

    employee_ids: set[int] = set()
    for employee in employees:
        if not isinstance(employee, dict):
            raise ValueError("employee must be an object")
        employee_id = _require_int(employee, "id", minimum=1)
        if employee_id in employee_ids:
            raise ValueError(f"duplicate employee id {employee_id}")
        employee_ids.add(employee_id)
        if not isinstance(employee.get("available"), bool):
            raise ValueError("employee available must be boolean")
        _require_int(employee, "available_from_bucket", minimum=0)
        _require_int(employee, "available_until_bucket", minimum=0)

    task_ids: set[int] = set()
    for task in tasks:
        if not isinstance(task, dict):
            raise ValueError("task must be an object")
        task_id = _require_int(task, "id", minimum=1)
        if task_id in task_ids:
            raise ValueError(f"duplicate task id {task_id}")
        task_ids.add(task_id)
        if task.get("state") not in {"Created", "Blocked", "Ready", "Assigned", "Traveling", "Executing", "Completed", "Cancelled", "Failed"}:
            raise ValueError(f"invalid task state for {task_id}")

    station_ids: set[int] = set()
    horizon = int(request["horizon_buckets"])
    for station in service_stations:
        if not isinstance(station, dict):
            raise ValueError("service station must be an object")
        station_id = _require_int(station, "id", minimum=1)
        if station_id in station_ids:
            raise ValueError(f"duplicate station id {station_id}")
        station_ids.add(station_id)
        _require_int(station, "guest_impact_weight", minimum=1)
        _require_int(station, "minimum_coverage", minimum=0)
        demand = station.get("demand_seconds_by_bucket")
        if not isinstance(demand, list) or len(demand) != horizon:
            raise ValueError(f"station {station_id} demand_seconds_by_bucket must match horizon")
        for value in demand:
            if isinstance(value, bool) or not isinstance(value, int) or value < 0:
                raise ValueError(f"station {station_id} demand must be non-negative integers")

    station_candidate_keys: set[tuple[int, int]] = set()
    for candidate in station_candidates:
        if not isinstance(candidate, dict):
            raise ValueError("station candidate must be an object")
        employee_id = _require_int(candidate, "employee_id", minimum=1)
        station_id = _require_int(candidate, "station_id", minimum=1)
        if employee_id not in employee_ids:
            raise ValueError(f"station candidate references unknown employee {employee_id}")
        if station_id not in station_ids:
            raise ValueError(f"station candidate references unknown station {station_id}")
        key = (employee_id, station_id)
        if key in station_candidate_keys:
            raise ValueError(f"duplicate station candidate employee/station pair {key}")
        station_candidate_keys.add(key)
        _require_int(candidate, "service_capacity_seconds_per_bucket", minimum=1)
        _require_int(candidate, "zone_penalty_seconds", minimum=0)

    candidate_keys: set[tuple[int, int]] = set()
    for candidate in candidates:
        if not isinstance(candidate, dict):
            raise ValueError("candidate must be an object")
        employee_id = _require_int(candidate, "employee_id", minimum=1)
        task_id = _require_int(candidate, "task_id", minimum=1)
        if employee_id not in employee_ids:
            raise ValueError(f"candidate references unknown employee {employee_id}")
        if task_id not in task_ids:
            raise ValueError(f"candidate references unknown task {task_id}")
        key = (employee_id, task_id)
        if key in candidate_keys:
            raise ValueError(f"duplicate candidate employee/task pair {key}")
        candidate_keys.add(key)
        _require_int(candidate, "duration_buckets", minimum=1)


def build_decision_slots(request: dict[str, Any]) -> list[dict[str, int]]:
    """Expand eligible employee/task pairs into feasible start-bucket decisions."""
    horizon = int(request["horizon_buckets"])
    employees = {int(e["id"]): e for e in request["employees"]}
    ready = {int(t["id"]) for t in request["tasks"] if t["state"] == "Ready"}
    slots: list[dict[str, int]] = []
    for candidate in sorted(request["candidates"], key=lambda c: (int(c["employee_id"]), int(c["task_id"]))):
        employee_id = int(candidate["employee_id"])
        task_id = int(candidate["task_id"])
        if task_id not in ready:
            continue
        employee = employees[employee_id]
        if not employee["available"]:
            continue
        duration = int(candidate["duration_buckets"])
        first = max(0, int(employee["available_from_bucket"]))
        last_exclusive = min(horizon, int(employee["available_until_bucket"]))
        for start in range(first, max(first, last_exclusive - duration + 1)):
            if start + duration <= last_exclusive:
                slots.append({"employee_id": employee_id, "task_id": task_id, "start_bucket": start, "duration_buckets": duration})
    return slots


def build_station_slots(request: dict[str, Any]) -> list[dict[str, int]]:
    """Expand eligible employee/station pairs into feasible coverage buckets."""
    horizon = int(request["horizon_buckets"])
    employees = {int(e["id"]): e for e in request["employees"]}
    slots: list[dict[str, int]] = []
    for candidate in sorted(request["station_candidates"], key=lambda c: (int(c["employee_id"]), int(c["station_id"]))):
        employee_id = int(candidate["employee_id"])
        station_id = int(candidate["station_id"])
        employee = employees[employee_id]
        if not employee["available"]:
            continue
        first = max(0, int(employee["available_from_bucket"]))
        last_exclusive = min(horizon, int(employee["available_until_bucket"]))
        for bucket in range(first, last_exclusive):
            slots.append({"employee_id": employee_id, "station_id": station_id, "bucket": bucket})
    return slots


def normalize_success_response(request: dict[str, Any], assignments: list[dict[str, int]], objective_values: dict[str, float], station_coverage: list[dict[str, int]] | None = None) -> dict[str, Any]:
    ordered_assignments = sorted(assignments, key=lambda a: (int(a["task_id"]), int(a["employee_id"]), int(a["start_bucket"])))
    ordered_station_coverage = sorted(station_coverage or [], key=lambda a: (int(a["station_id"]), int(a["employee_id"]), int(a["bucket"])))
    return {
        "schema": 1,
        "status": "ok",
        "optimization_epoch": int(request["optimization_epoch"]),
        "simulation_second": int(request["simulation_second"]),
        "snapshot_fingerprint": int(request["snapshot_fingerprint"]),
        "source": "cuopt",
        "assignments": ordered_assignments,
        "station_coverage": ordered_station_coverage,
        "objective_values": {key: float(objective_values[key]) for key in sorted(objective_values)},
    }


def failure_response(request: dict[str, Any] | None, error: str, *, code: str = "solver_error") -> dict[str, Any]:
    response: dict[str, Any] = {"schema": 1, "status": "error", "error_code": code, "error": error}
    if request:
        for key in ("optimization_epoch", "simulation_second", "snapshot_fingerprint"):
            if isinstance(request.get(key), int):
                response[key] = request[key]
    return response


def _candidate_map(request: dict[str, Any]) -> dict[tuple[int, int], dict[str, Any]]:
    return {(int(c["employee_id"]), int(c["task_id"])): c for c in request["candidates"]}


def solve_with_cuopt(request: dict[str, Any], *, time_limit: float = 5.0, mip_gap: float = 0.0) -> dict[str, Any]:
    """Build and solve the rolling-horizon MILP with NVIDIA cuOpt."""
    validate_request(request)
    ready_tasks = [t for t in request["tasks"] if t["state"] == "Ready"]
    has_station_demand = any(any(int(value) > 0 for value in station["demand_seconds_by_bucket"]) for station in request["service_stations"])
    if not ready_tasks and not has_station_demand:
        return normalize_success_response(request, [], {name: 0.0 for name in OBJECTIVES}, station_coverage=[])

    try:
        from cuopt.linear_programming.problem import CONTINUOUS, INTEGER, MINIMIZE, Problem
        from cuopt.linear_programming.solver_settings import SolverSettings
    except ImportError as exc:
        raise RuntimeError("NVIDIA cuOpt is not installed in this Python environment") from exc

    problem = Problem("HotelHavenStaffRollingHorizon")
    settings = SolverSettings()
    settings.set_parameter("time_limit", float(time_limit))
    settings.set_parameter("mip_relative_gap", float(mip_gap))

    slots = build_decision_slots(request)
    employees = {int(e["id"]): e for e in request["employees"]}
    tasks = {int(t["id"]): t for t in ready_tasks}
    candidates = _candidate_map(request)
    stations = {int(s["id"]): s for s in request["service_stations"]}
    station_candidates = {(int(c["employee_id"]), int(c["station_id"])): c for c in request["station_candidates"]}
    station_slots = build_station_slots(request)
    horizon = int(request["horizon_buckets"])
    bucket_seconds = int(request["bucket_minutes"]) * 60

    x: dict[tuple[int, int, int], Any] = {}
    slot_by_key: dict[tuple[int, int, int], dict[str, int]] = {}
    for slot in slots:
        key = (slot["employee_id"], slot["task_id"], slot["start_bucket"])
        x[key] = problem.addVariable(lb=0, ub=1, vtype=INTEGER, name=f"x_e{key[0]}_t{key[1]}_b{key[2]}")
        slot_by_key[key] = slot

    y: dict[tuple[int, int, int], Any] = {}
    for slot in station_slots:
        key = (slot["employee_id"], slot["station_id"], slot["bucket"])
        y[key] = problem.addVariable(lb=0, ub=1, vtype=INTEGER, name=f"station_e{key[0]}_s{key[1]}_b{key[2]}")

    station_shortfall: dict[tuple[int, int], Any] = {}
    station_coverage_shortfall: dict[tuple[int, int], Any] = {}
    for station_id, station in sorted(stations.items()):
        for bucket in range(horizon):
            shortfall = problem.addVariable(lb=0, vtype=CONTINUOUS, name=f"station_short_s{station_id}_b{bucket}")
            coverage_shortfall = problem.addVariable(lb=0, vtype=CONTINUOUS, name=f"station_covshort_s{station_id}_b{bucket}")
            station_shortfall[(station_id, bucket)] = shortfall
            station_coverage_shortfall[(station_id, bucket)] = coverage_shortfall
            coverage_vars = [var for (employee_id, sid, b), var in y.items() if sid == station_id and b == bucket]
            capacity = sum(int(station_candidates[(employee_id, station_id)]["service_capacity_seconds_per_bucket"]) * var for (employee_id, sid, b), var in y.items() if sid == station_id and b == bucket)
            demand = int(station["demand_seconds_by_bucket"][bucket])
            problem.addConstraint(shortfall >= demand - capacity, name=f"station_demand_s{station_id}_b{bucket}")
            minimum_coverage = int(station["minimum_coverage"]) if demand > 0 else 0
            problem.addConstraint(coverage_shortfall >= minimum_coverage - sum(coverage_vars), name=f"station_coverage_s{station_id}_b{bucket}")

    unserved = {task_id: problem.addVariable(lb=0, ub=1, vtype=INTEGER, name=f"u_t{task_id}") for task_id in sorted(tasks)}
    lateness = {task_id: problem.addVariable(lb=0, vtype=CONTINUOUS, name=f"late_t{task_id}") for task_id in sorted(tasks)}
    for task_id in sorted(tasks):
        task_vars = [var for (employee_id, tid, start), var in x.items() if tid == task_id]
        problem.addConstraint(sum(task_vars) + unserved[task_id] == 1, name=f"task_once_{task_id}")
        deadline = int(tasks[task_id].get("deadline_bucket", -1))
        if deadline >= 0:
            completion = sum((start + slot_by_key[(employee_id, tid, start)]["duration_buckets"]) * var for (employee_id, tid, start), var in x.items() if tid == task_id)
            problem.addConstraint(lateness[task_id] >= completion - deadline * sum(task_vars), name=f"late_{task_id}")
        else:
            problem.addConstraint(lateness[task_id] == 0, name=f"late_zero_{task_id}")

    break_vars: dict[tuple[int, int], Any] = {}
    break_duration = max(1, math.ceil(30 / int(request["bucket_minutes"])))
    for employee_id, employee in sorted(employees.items()):
        feasible_break_starts: list[int] = []
        if employee.get("needs_break") and employee.get("available"):
            first = max(0, int(employee["available_from_bucket"]))
            last = min(horizon, int(employee["available_until_bucket"]))
            feasible_break_starts = list(range(first, max(first, last - break_duration + 1)))
            if not feasible_break_starts:
                raise ValueError(f"employee {employee_id} requires a break but has no 30-minute break window")
            for start in feasible_break_starts:
                break_vars[(employee_id, start)] = problem.addVariable(lb=0, ub=1, vtype=INTEGER, name=f"break_e{employee_id}_b{start}")
            problem.addConstraint(sum(break_vars[(employee_id, start)] for start in feasible_break_starts) == 1, name=f"break_once_{employee_id}")

        for bucket in range(horizon):
            active_work = [var for (eid, task_id, start), var in x.items() if eid == employee_id and start <= bucket < start + slot_by_key[(eid, task_id, start)]["duration_buckets"]]
            active_break = [var for (eid, start), var in break_vars.items() if eid == employee_id and start <= bucket < start + break_duration]
            active_station = [var for (eid, station_id, b), var in y.items() if eid == employee_id and b == bucket]
            if active_work or active_break or active_station:
                problem.addConstraint(sum(active_work) + sum(active_break) + sum(active_station) <= 1, name=f"capacity_e{employee_id}_b{bucket}")

    critical_terms = []
    wait_terms = []
    labor_terms = []
    fatigue_terms = []
    for station_id, station in sorted(stations.items()):
        impact = int(station["guest_impact_weight"])
        for bucket in range(horizon):
            critical_terms.append(station_coverage_shortfall[(station_id, bucket)] * impact * 100_000)
            wait_terms.append(station_shortfall[(station_id, bucket)] * impact)

    for task_id, task in sorted(tasks.items()):
        priority = int(task.get("priority", 0))
        guest_impact = int(task.get("guest_impact", 0))
        revenue_impact = int(task.get("revenue_impact", 0))
        if priority >= 80:
            critical_terms.append(unserved[task_id] * ((priority - 79) * 100_000 + guest_impact * 1_000 + revenue_impact * 100))
        wait_terms.append(unserved[task_id] * (priority * 1_000 + guest_impact * 100 + revenue_impact * 10 + 1))
        wait_terms.append(lateness[task_id] * max(1, priority + guest_impact))

    for (employee_id, task_id, start), var in x.items():
        candidate = candidates[(employee_id, task_id)]
        employee = employees[employee_id]
        wage = int(employee["overtime_wage_minor_per_hour"] if employee.get("overtime") else employee["regular_wage_minor_per_hour"])
        work_seconds = int(candidate.get("effective_work_seconds", 0))
        travel_seconds = int(candidate.get("travel_seconds", 0))
        labor_terms.append(var * (wage * work_seconds + travel_seconds * 3600))
        fatigue_score = int(candidate.get("fatigue_penalty_seconds", 0)) + int(candidate.get("zone_penalty_seconds", 0)) + int(candidate.get("task_switch_penalty_seconds", 0)) - int(candidate.get("skill_bonus_seconds", 0)) + int(employee.get("fatigue", 0)) * int(slot_by_key[(employee_id, task_id, start)]["duration_buckets"])
        fatigue_terms.append(var * fatigue_score)

    for (employee_id, station_id, bucket), var in y.items():
        employee = employees[employee_id]
        candidate = station_candidates[(employee_id, station_id)]
        wage = int(employee["overtime_wage_minor_per_hour"] if employee.get("overtime") else employee["regular_wage_minor_per_hour"])
        labor_terms.append(var * wage * bucket_seconds)
        fatigue_terms.append(var * (int(employee.get("fatigue", 0)) + int(candidate.get("zone_penalty_seconds", 0))))

    anchor = next(iter(unserved.values()), None) or next(iter(y.values()), None)
    if anchor is None:
        raise RuntimeError("MILP contains neither ready tasks nor station decisions")
    zero_expression = anchor * 0
    objective_expressions = [
        ("critical_service", sum(critical_terms, zero_expression)),
        ("guest_wait_lateness", sum(wait_terms, zero_expression)),
        ("labor_travel_cost", sum(labor_terms, zero_expression)),
        ("fatigue_zone_switch", sum(fatigue_terms, zero_expression)),
    ]

    objective_values: dict[str, float] = {}
    for index, (name, expression) in enumerate(objective_expressions):
        problem.setObjective(expression, sense=MINIMIZE)
        problem.solve(settings)
        status_name = getattr(problem.Status, "name", str(problem.Status))
        if status_name not in ACCEPTED_STATUS_NAMES:
            raise RuntimeError(f"cuOpt solve for {name} returned {status_name}")
        value = float(problem.ObjValue)
        objective_values[name] = value
        if index + 1 < len(objective_expressions):
            tolerance = max(1e-6, abs(value) * 1e-9)
            problem.addConstraint(expression <= value + tolerance, name=f"fix_{name}")

    assignments = [dict(slot_by_key[key]) for key, variable in x.items() if float(variable.getValue()) > 0.5]
    station_coverage = [{"employee_id": employee_id, "station_id": station_id, "bucket": bucket} for (employee_id, station_id, bucket), variable in y.items() if float(variable.getValue()) > 0.5]
    return normalize_success_response(request, assignments, objective_values, station_coverage=station_coverage)


def _load_json(path: str | None) -> dict[str, Any]:
    if path:
        with Path(path).open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    else:
        data = json.load(sys.stdin)
    if not isinstance(data, dict):
        raise ValueError("request root must be an object")
    return data


def _write_json(payload: dict[str, Any], path: str | None) -> None:
    text = json.dumps(payload, sort_keys=True, separators=(",", ":")) + "\n"
    if path:
        Path(path).write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Hotel Haven optional NVIDIA cuOpt staff optimizer")
    parser.add_argument("--input", help="request JSON path; defaults to stdin")
    parser.add_argument("--output", help="response JSON path; defaults to stdout")
    parser.add_argument("--validate-only", action="store_true", help="validate/expand request without importing cuOpt")
    parser.add_argument("--time-limit", type=float, default=5.0)
    parser.add_argument("--mip-gap", type=float, default=0.0)
    args = parser.parse_args(argv)

    request: dict[str, Any] | None = None
    try:
        request = _load_json(args.input)
        validate_request(request)
        if args.validate_only:
            payload = {"schema": 1, "status": "ok", "optimization_epoch": request["optimization_epoch"], "snapshot_fingerprint": request["snapshot_fingerprint"], "decision_slot_count": len(build_decision_slots(request)), "station_slot_count": len(build_station_slots(request))}
        else:
            payload = solve_with_cuopt(request, time_limit=args.time_limit, mip_gap=args.mip_gap)
        _write_json(payload, args.output)
        return 0
    except ValueError as exc:
        _write_json(failure_response(request, str(exc), code="invalid_request"), args.output)
        return 2
    except Exception as exc:
        _write_json(failure_response(request, str(exc), code="solver_error"), args.output)
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
