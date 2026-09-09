# Hotel Haven cuOpt Worker

This development-only worker solves HMG-040 staff assignment proposals with NVIDIA cuOpt. It is deliberately outside the shipping C++ runtime: Windows builds remain playable without Python, CUDA, WSL2, or cuOpt.

## Request flow

1. `optimization::buildMilpRequestJson()` emits a canonical rolling-horizon request.
2. A development launcher invokes this worker in a cuOpt-capable Linux/WSL2 environment.
3. The worker constructs binary employee/task/start decisions, service-station coverage decisions, task coverage, employee capacity, optional 30-minute break occupancy, and lateness/coverage shortfall variables.
4. It solves four objectives sequentially: critical service, guest wait/lateness, labor/travel cost, fatigue/zone/switch cost.
5. The native game revalidates the returned proposal with `validatePlan()` before HMG-040 may commit it.
6. Accepted plan events are recorded for deterministic replay; replay does not re-run cuOpt.

## Baseline validation without cuOpt

```bash
python hotel_haven_cuopt_worker.py --input request.json --validate-only
python -m unittest -v
```

## NVIDIA host

Install a current NVIDIA cuOpt environment following the `cuopt-install` skill. Then invoke the same worker without `--validate-only`. The worker imports `cuopt.linear_programming.problem.Problem` and `SolverSettings` lazily.

Solver settings exposed by this wrapper:

```text
--time-limit SECONDS
--mip-gap FRACTION
```

A missing/failed solver returns structured JSON with `status=error`; the native simulation must fall back to `DeterministicFallbackOptimizer` rather than mutating state from a partial result.
