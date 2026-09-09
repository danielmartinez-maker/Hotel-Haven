#!/usr/bin/env python3
"""Pareto-analysis tooling for deterministic Hotel Haven balance runs."""

from __future__ import annotations

import argparse
import csv
import itertools
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

OBJECTIVES = (
    "operating_cost",
    "guest_dissatisfaction",
    "fatigue_load",
    "excess_wait",
    "negative_gop",
)


@dataclass(frozen=True)
class BalancePoint:
    run_id: str
    values: dict[str, float]
    source: dict[str, Any]


def _finite_number(run: dict[str, Any], key: str) -> float:
    value = run.get(key)
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{run.get('run_id', '<unknown>')}: {key} must be numeric")
    value = float(value)
    if not math.isfinite(value):
        raise ValueError(f"{run.get('run_id', '<unknown>')}: {key} must be finite")
    return value


def to_point(run: dict[str, Any]) -> BalancePoint:
    run_id = run.get("run_id")
    if not isinstance(run_id, str) or not run_id.strip():
        raise ValueError("run_id must be a non-empty string")
    satisfaction = _finite_number(run, "guest_satisfaction")
    if satisfaction < 0.0 or satisfaction > 100.0:
        raise ValueError(f"{run_id}: guest_satisfaction must be in [0, 100]")
    values = {
        "operating_cost": _finite_number(run, "operating_cost_minor"),
        "guest_dissatisfaction": 100.0 - satisfaction,
        "fatigue_load": _finite_number(run, "fatigue_load"),
        "excess_wait": _finite_number(run, "excess_wait_minutes"),
        "negative_gop": -_finite_number(run, "gop_minor"),
    }
    return BalancePoint(run_id=run_id, values=values, source=dict(run))


def dominates(a: BalancePoint, b: BalancePoint, *, tolerance: float = 1e-9) -> bool:
    no_worse = all(a.values[name] <= b.values[name] + tolerance for name in OBJECTIVES)
    strictly_better = any(a.values[name] < b.values[name] - tolerance for name in OBJECTIVES)
    return no_worse and strictly_better


def _point_sort_key(point: BalancePoint) -> tuple[Any, ...]:
    return tuple(point.values[name] for name in OBJECTIVES) + (point.run_id,)


def pareto_frontier(points: Iterable[BalancePoint]) -> list[BalancePoint]:
    materialized = list(points)
    ids = [point.run_id for point in materialized]
    if len(ids) != len(set(ids)):
        raise ValueError("run_id values must be unique")
    frontier = [candidate for candidate in materialized if not any(other.run_id != candidate.run_id and dominates(other, candidate) for other in materialized)]
    return sorted(frontier, key=_point_sort_key)


def _serialize_point(point: BalancePoint) -> dict[str, Any]:
    return {"run_id": point.run_id, **{name: point.values[name] for name in OBJECTIVES}}


def payoff_table(points: Iterable[BalancePoint]) -> dict[str, dict[str, Any]]:
    materialized = list(points)
    if not materialized:
        raise ValueError("cannot build payoff table from no points")
    table: dict[str, dict[str, Any]] = {}
    for objective in OBJECTIVES:
        best = min(materialized, key=lambda point: (point.values[objective], _point_sort_key(point)))
        table[objective] = _serialize_point(best)
    return table


def normalize_points(points: Iterable[BalancePoint]) -> dict[str, dict[str, float]]:
    materialized = list(points)
    if not materialized:
        raise ValueError("cannot normalize no points")
    mins = {name: min(point.values[name] for point in materialized) for name in OBJECTIVES}
    maxs = {name: max(point.values[name] for point in materialized) for name in OBJECTIVES}
    normalized: dict[str, dict[str, float]] = {}
    for point in materialized:
        normalized[point.run_id] = {}
        for objective in OBJECTIVES:
            span = maxs[objective] - mins[objective]
            normalized[point.run_id][objective] = 0.0 if abs(span) <= 1e-12 else (point.values[objective] - mins[objective]) / span
    return normalized


def select_knee_candidate(points: Iterable[BalancePoint]) -> dict[str, Any]:
    materialized = list(points)
    normalized = normalize_points(materialized)
    best = min(materialized, key=lambda point: (math.sqrt(sum(normalized[point.run_id][name] ** 2 for name in OBJECTIVES)), _point_sort_key(point)))
    result = _serialize_point(best)
    result["normalized_distance_to_ideal"] = math.sqrt(sum(normalized[best.run_id][name] ** 2 for name in OBJECTIVES))
    return result


def _levels(low: float, high: float, steps: int) -> list[float]:
    if steps < 2:
        raise ValueError("steps must be >= 2")
    if abs(high - low) <= 1e-12:
        return [low] * steps
    return [low + (high - low) * index / (steps - 1) for index in range(steps)]


def generate_epsilon_grid(points: Iterable[BalancePoint], *, primary_objective: str, steps: int = 4) -> list[dict[str, Any]]:
    materialized = list(points)
    if not materialized:
        raise ValueError("cannot generate epsilon grid from no points")
    if primary_objective not in OBJECTIVES:
        raise ValueError(f"unknown primary objective {primary_objective}")
    bounded = [name for name in OBJECTIVES if name != primary_objective]
    levels_by_objective = {name: _levels(min(point.values[name] for point in materialized), max(point.values[name] for point in materialized), steps) for name in bounded}
    grid: list[dict[str, Any]] = []
    for index, values in enumerate(itertools.product(*(levels_by_objective[name] for name in bounded))):
        grid.append({"sweep_id": f"eps-{index:05d}", "primary_objective": primary_objective, "epsilon_bounds": {name: value for name, value in zip(bounded, values, strict=True)}})
    return grid


def _load_runs(input_dir: Path) -> list[BalancePoint]:
    points: list[BalancePoint] = []
    for path in sorted(input_dir.glob("*.json"), key=lambda value: value.name):
        with path.open("r", encoding="utf-8") as handle:
            payload = json.load(handle)
        if not isinstance(payload, dict):
            raise ValueError(f"{path}: root must be an object")
        points.append(to_point(payload))
    if not points:
        raise ValueError(f"{input_dir}: no JSON run summaries found")
    return points


def _write_json(path: Path, payload: Any) -> None:
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _write_frontier_csv(path: Path, frontier: list[BalancePoint]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle, lineterminator="\n")
        writer.writerow(("run_id",) + OBJECTIVES)
        for point in frontier:
            writer.writerow([point.run_id] + [point.values[name] for name in OBJECTIVES])


def run_frontier(input_dir: Path, output_dir: Path) -> None:
    points = _load_runs(input_dir)
    frontier = pareto_frontier(points)
    output_dir.mkdir(parents=True, exist_ok=True)
    _write_json(output_dir / "pareto_frontier.json", [_serialize_point(point) for point in frontier])
    _write_frontier_csv(output_dir / "pareto_frontier.csv", frontier)
    _write_json(output_dir / "payoff_table.json", payoff_table(frontier))
    _write_json(output_dir / "knee_candidate.json", select_knee_candidate(frontier))


def run_epsilon_grid(input_dir: Path, output: Path, primary_objective: str, steps: int) -> None:
    frontier = pareto_frontier(_load_runs(input_dir))
    payload = {"schema": 1, "primary_objective": primary_objective, "steps": steps, "sweeps": generate_epsilon_grid(frontier, primary_objective=primary_objective, steps=steps)}
    output.parent.mkdir(parents=True, exist_ok=True)
    _write_json(output, payload)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Hotel Haven multi-objective balance laboratory")
    subparsers = parser.add_subparsers(dest="command", required=True)
    frontier_parser = subparsers.add_parser("frontier", help="compute non-dominated run frontier")
    frontier_parser.add_argument("--input-dir", required=True)
    frontier_parser.add_argument("--output-dir", required=True)
    epsilon_parser = subparsers.add_parser("epsilon-grid", help="generate epsilon-constraint sweep manifest")
    epsilon_parser.add_argument("--input-dir", required=True)
    epsilon_parser.add_argument("--output", required=True)
    epsilon_parser.add_argument("--primary-objective", choices=OBJECTIVES, required=True)
    epsilon_parser.add_argument("--steps", type=int, default=4)
    args = parser.parse_args(argv)
    if args.command == "frontier":
        run_frontier(Path(args.input_dir), Path(args.output_dir))
    else:
        run_epsilon_grid(Path(args.input_dir), Path(args.output), args.primary_objective, args.steps)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
