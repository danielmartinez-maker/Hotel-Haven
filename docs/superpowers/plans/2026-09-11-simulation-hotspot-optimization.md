# Simulation Hotspot Optimization Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reduce measured CPU cost in the authoritative simulation's navigation and staff/task update paths while preserving exact gameplay behavior, save compatibility, and deterministic benchmark output.

**Architecture:** Keep the existing `Simulation` ownership model and public interfaces. First capture a fresh profile of the current implementation. Then make localized changes in `game/src/Simulation.cpp` only where the profile and source trace identify repeated allocations or scans. Extend the existing deterministic simulation benchmark and regression tests so each change has a reproducible before/after measurement and checksum guard.

**Tech Stack:** C++17, CMake, CTest, GCC/gprof, existing `hh_simulation_benchmarks`, existing deterministic simulation tests, GitHub PR #24.

**Spec:** The change must not reduce simulation fidelity, alter authoritative ordering, weaken assertions, disable tests, or introduce a cache without an explicit invalidation path. Existing benchmark checksum outputs are compatibility guards.

## Global Constraints

- Preserve the current branch and do not merge PR #24.
- Use fixed seeds and identical scenarios for every comparison.
- Do not claim a speedup unless repeated measurements beat normal run-to-run noise.
- For each production change, run the smallest relevant test first, then the broader suite.
- Keep route and assignment tie-breaking identical to the current implementation.
- Invalidate any derived navigation/service data on every existing map mutation and load path.

---

## Task 1: Capture the current hotspot evidence

- [x] Run the existing Release simulation benchmark three times and save raw JSON output under `/tmp` with the current commit recorded.
- [x] Run the existing profile-enabled benchmark from an isolated working directory and inspect inclusive and self CPU cost for `Simulation::path`, `neighbors`, `guests`, `staffAndTasks`, `getPerson`, `getRoom`, `getReservation`, and `locate`.
- [x] Trace every caller and mutation path for navigation and special-tile lookup before changing either implementation.
- [x] Record the exact baseline medians, checksum values, compiler, build directory, and command lines in the working notes used for the final report.

## Task 2: Add deterministic regression coverage for navigation behavior

- [x] Add a focused simulation test that exercises repeated route queries across a reachable route, an unreachable destination, and a map mutation, asserting the same reachability and authoritative positions before and after the repeated-query sequence.
- [x] Run that test before the optimization and keep its output as the RED/GREEN evidence if the test exposes an existing defect; otherwise use it as a behavior guard alongside the performance baseline.
- [x] Run the existing save/load and deterministic benchmark tests before modifying route-query internals.

## Task 3: Remove confirmed per-query navigation overhead

- [x] Implement the smallest safe reduction of temporary allocation in `Simulation::neighbors`/`Simulation::path`, retaining the current neighbor ordering and passability rules.
- [x] Confirm no shared mutable scratch state is introduced; ensure failed, unreachable, and `from == to` queries leave no stale route state.
- [x] If route reuse is justified by the profile, key it by all navigation-affecting state and invalidate it from every existing construction/removal/load path; otherwise do not add a route cache.
- [x] Run the focused navigation test, deterministic benchmark checksum checks, save/load tests, and sanitizer build after the change.

## Task 4: Remove confirmed staff/task candidate-scan overhead

- [x] Based on the profile, build the smallest per-update candidate index that preserves the existing employee eligibility, distance, ID tie-break, priority, reservation, and assignment ordering.
- [x] Add or extend a staff/task regression test that compares assignment outcomes for ties, unavailable workers, cancelled tasks, and mixed roles against the pre-change deterministic expectations.
- [x] Benchmark staff/task-heavy scenarios at the existing `small`, `normal`, `large`, and `stress` tiers and reject the change if checksums or gameplay state diverge.

## Task 5: Verify and document the optimization

- [x] Run clean Debug and Release builds, the complete CTest suite, Werror, ASan/UBSan with leak detection disabled only for the documented ptrace limitation, frontend/renderer portable tests, and the existing asset tests.
- [x] Rerun the simulation benchmark three times, compare per-tier medians and checksums to Task 1, and inspect the updated gprof profile.
- [x] Run save/load round trips, the 365-day deterministic campaign, the existing stress scenarios, and memory/RSS checks.
- [x] Update `docs/performance/baseline-2026-09-10.json` only if the benchmark schema supports the new evidence; otherwise add a dated comparison artifact without rewriting prior baseline data.
- [x] Update `docs/reports/2026-09-10-hotel-haven-optimization-debugging-report.md` with confirmed measurements, limitations, files changed, exact final commands, and unresolved risks.
- [x] Review `git diff`, confirm no tests or assertions were weakened, commit the coherent milestone, mirror it to PR #24 through GitHub, and poll GitHub CI without merging.
