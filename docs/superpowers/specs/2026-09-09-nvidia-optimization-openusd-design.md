# Hotel Haven NVIDIA Optimization + OpenUSD Integration Design

## Status

Approved for implementation on 2026-09-09.

## Scope

This design adds three development subsystems without changing existing HMG authority boundaries:

1. a deterministic staff/task optimization boundary with a native fallback and optional NVIDIA cuOpt MILP worker;
2. a balance laboratory that computes Pareto frontiers across operating cost, guest dissatisfaction, employee fatigue, service waiting, and profitability;
3. an OpenUSD authoring/assembly layer for large hotels that preserves the existing HMG-070 `.hasset` shipping boundary.

## Authority boundaries

- HMG-040 remains authoritative for employee availability, assignment commits, schedules, fatigue, morale, breaks, overtime, and department labor capacity.
- HMG-050 remains authoritative for task generation, dependencies, inventory/resource claims, logistics chains, and blocked-task reasons.
- HMG-010 remains authoritative for guest reactions, memories, satisfaction, complaints, and reviews.
- HMG-030 remains authoritative for revenue, payroll, operating costs, demand, and profitability.
- NVIDIA solvers may propose plans; they never mutate authoritative simulation state directly.
- OpenUSD is authoring/interchange data. Runtime gameplay loads cooked Hotel Haven assets, not USD source stages.

## Staff optimization architecture

A platform-neutral C++20 `optimization/` library defines immutable optimizer snapshots, plans, validation, deterministic fallback assignment, and a MILP request format. The library does not depend on CUDA or cuOpt.

The optional NVIDIA path is a Python worker under `Tools/CuOptWorker/`. It reads a deterministic JSON request, builds a cuOpt MILP using the current numerical-optimization Python API, performs sequential lexicographic solves, and emits a plan JSON. Windows integration is intentionally process-isolated so the shipping executable does not acquire a hard WSL2/CUDA dependency.

The rolling-horizon model uses five-minute buckets over a default 60-minute horizon. Binary assignment variables select employee/task/start-bucket combinations. Hard constraints cover eligibility, one assignment per task, employee capacity, blocked-task exclusion, and fixed availability. Optional break and service-station coverage inputs are supported. Objectives are solved sequentially: critical/SLA failure, guest-weighted lateness/waiting, labor/travel cost, fatigue/zone/switch penalties.

## Determinism contract

Every snapshot is canonicalized by stable 64-bit IDs. Time is integer simulation seconds. Model inputs avoid wall-clock state. Solver output is validated and sorted before it can become a `SchedulerPlan`. Live solver nondeterminism is isolated by recording the accepted plan as an ordered authoritative simulation event; replay consumes the recorded event instead of re-solving. If the solver is unavailable, invalid, stale, infeasible, or times out, Hotel Haven uses the native deterministic fallback.

## Balance Lab

`Tools/BalanceLab/` is a dependency-light Python CLI. It consumes deterministic headless-run summaries, converts all five objectives to minimization form, computes anchor/payoff data, removes dominated points, identifies a normalized knee candidate, and emits JSON/CSV artifacts. It also generates epsilon-constraint sweep manifests so scenario runners can execute systematic balance experiments.

The framework does not replace the game simulation with a surrogate objective. Long-horizon effects such as reviews, reputation, demand, and profit remain outcomes produced by the actual deterministic game simulation.

## OpenUSD pipeline

`Tools/OpenUsdPipeline/` consumes a Hotel Haven scene manifest and emits deterministic USDA assembly stages. Floors are payload boundaries. Reusable rooms/furniture are authored as references with `instanceable = true`. High-count simple repeated props may use `UsdGeomPointInstancer`. The tool validates the HMG-070 rule that assets expected to appear more than 100 times must be instance-friendly unless explicitly exempted.

Generated USD lives under authoring/generated content and maps back to stable Hotel Haven asset IDs. A cook manifest records which logical Hotel Haven assets appear in the stage. The shipping renderer continues to consume cooked `.hasset`/future `.hpak` outputs.

## Error handling

- Optimizer request validation errors fail before invoking cuOpt.
- Worker import/runtime errors produce structured failure responses and never partially commit assignments.
- Stale snapshot hashes reject returned plans.
- Balance input rows with non-finite or missing objective values fail with file/field diagnostics.
- USD manifests fail on duplicate IDs, unknown references, invalid transforms, invalid instance modes, or high-count unique duplication.
- Generated files are written deterministically and atomically where practical.

## Testing

TDD coverage includes:

- canonical snapshot ordering and stable hashing;
- deterministic fallback tie-breaking;
- blocked/ineligible task rejection;
- capacity/non-overlap plan validation;
- MILP request generation and lexicographic objective metadata;
- Pareto dominance, frontier extraction, payoff table, and knee selection;
- epsilon sweep generation;
- deterministic USDA output;
- per-floor payloads;
- scenegraph instancing and PointInstancer generation;
- HMG-070 high-count instancing validation.

CI runs portable C++ tests plus Python unit tests on feature branches and pull requests. cuOpt/Omniverse installation is not required for baseline CI; NVIDIA-specific integration tests are opt-in on compatible GPU/WSL2 development hosts.

## Non-goals

- No CUDA/cuOpt dependency in the shipping executable.
- No duplication of HMG-010/030/040/050 simulation authority.
- No direct USD loading in the shipping runtime.
- No attempt to solve guest AI or pathfinding inside MILP.
- No claim that CI without an NVIDIA GPU validates cuOpt kernel/runtime performance.
