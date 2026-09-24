#!/usr/bin/env python3
"""Validate Hotel Haven headless output against the Balance Lab contract."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

REQUIRED = {
    "run_id",
    "seed",
    "simulation_second",
    "operating_cost_minor",
    "guest_satisfaction",
    "fatigue_load",
    "excess_wait_minutes",
    "gop_minor",
    "completed_stays",
    "reputation",
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("summary", type=Path)
    parser.add_argument("--expected-run-id", required=True)
    parser.add_argument("--min-guest-satisfaction", type=float, default=0.0)
    args = parser.parse_args()

    data = json.loads(args.summary.read_text(encoding="utf-8"))
    missing = sorted(REQUIRED - data.keys())
    if missing:
        raise SystemExit(f"missing required Balance Lab fields: {', '.join(missing)}")
    if data["run_id"] != args.expected_run_id:
        raise SystemExit("run_id does not match requested deterministic scenario")

    for key in ("guest_satisfaction", "fatigue_load", "excess_wait_minutes", "reputation"):
        value = data[key]
        if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
            raise SystemExit(f"{key} must be finite numeric output")
    if not 0.0 <= float(data["guest_satisfaction"]) <= 100.0:
        raise SystemExit("guest_satisfaction must use Hotel Haven's 0..100 scale")
    if float(data["guest_satisfaction"]) < args.min_guest_satisfaction:
        raise SystemExit("guest_satisfaction appears to be using the 1..10 review scale")
    if float(data["fatigue_load"]) < 0.0 or float(data["excess_wait_minutes"]) < 0.0:
        raise SystemExit("fatigue/wait objectives must be non-negative")
    if not 0.0 <= float(data["reputation"]) <= 100.0:
        raise SystemExit("reputation must remain on the 0..100 simulation scale")
    if not isinstance(data["completed_stays"], int) or isinstance(data["completed_stays"], bool):
        raise SystemExit("completed_stays must be an integer throughput metric")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
