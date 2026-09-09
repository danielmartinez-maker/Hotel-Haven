# Full-Game NVIDIA Integration Implementation Plan

**Goal:** Wire the reusable NVIDIA optimization foundation into the authoritative Hotel Haven simulation without adding CUDA, Python, WSL2, cuOpt, or OpenUSD as shipping runtime dependencies.

**Base:** `feature/full-game-integration`

**Branch:** `feature/full-game-nvidia-integration`

## Authority contract

- `Simulation::Impl` remains authoritative for shifts, inventory/resource blocking, path reachability, task execution, fatigue, wages, room/task state transitions, and completion.
- `hh_optimization` receives immutable snapshots and returns proposals only.
- Every proposed plan is validated before commit.
- Invalid/stale/unavailable optimizer results fall back to the deterministic native optimizer.
- OpenUSD remains authoring/build tooling; runtime continues to consume Hotel Haven cooked assets.

## Task 1 — Build integration seam (TDD)

- [ ] Add a failing simulation test proving higher-priority guest-facing work wins a contested staff assignment.
- [ ] Verify RED on Integrated Game CI.
- [ ] Add `optimization/` to the root build and link `hh_game` privately to `hh_optimization`.
- [ ] Add `buildOptimizerSnapshot()` at the assignment seam.
- [ ] Use deterministic optimizer output to propose assignments.
- [ ] Validate the plan and commit only valid assignments using existing state transitions.
- [ ] Verify GREEN on Linux/Windows Integrated Game CI plus Optimization CI.

## Task 2 — Safety/fallback coverage (TDD)

- [ ] Add failing tests for blocked-task exclusion, role/shift eligibility, one-worker/one-live-task capacity, and deterministic save continuation with optimizer enabled.
- [ ] Verify RED where new behavior is missing.
- [ ] Implement only the minimal bridge behavior needed for GREEN.
- [ ] Verify all game/optimizer tests GREEN.

## Task 3 — Balance Lab real simulation output (TDD)

- [ ] Add a failing headless-run test for deterministic Balance Lab JSON output.
- [ ] Extend `hotel_haven_headless` with an opt-in balance-summary output containing operating cost, guest satisfaction, fatigue load, excess wait, and GOP.
- [ ] Verify Balance Lab can consume the generated summary without surrogate simulation logic.

## Task 4 — Reconciliation

- [ ] Compare PR #6 against `feature/full-game-integration` and PR #5.
- [ ] Confirm no renderer/content-pipeline regressions or accidental runtime NVIDIA dependency.
- [ ] Run current-head CI and review changed files.
- [ ] Mark PR ready only after all required checks are green.
