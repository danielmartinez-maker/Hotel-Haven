# FINAL-02 Construction and Guest Stress Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stress the authoritative FINAL-01 construction/building implementation and FINAL-02 guest AI/psychology implementation under dense topology mutation, large guest populations, long stays, complaint/memory pressure, save/load, and deterministic replay.

**Architecture:** Create `stress/final02-construction-guest` from the current `feature/final-02-guest-ai-psychology` head. Reuse the same header-only stress harness contract as the main stress branch, but keep this branch independent. Two CTest executables isolate construction/building failures from guest-AI failures; a third combined executable stresses save/continuation across both systems where the branch exposes authoritative integration.

**Tech Stack:** C++20, CMake/CTest, FINAL-01 construction/building APIs, FINAL-02 guest psychology/goals/reviews/archive APIs, Ubuntu ASan+UBSan, MSVC Release.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Branch from the current FINAL-02 head, not from `main`, and record the exact parent SHA.
- Do not import FINAL-03/04/05/06/07 systems into this branch.
- Do not fabricate staff, service, economy, or UI authority absent from this line.
- Use exact stress environment contract: `HH_STRESS_SEED`, `HH_STRESS_SCALE`, `HH_STRESS_SCENARIO`.
- Preserve all current guest determinism, expectation, satisfaction, review, construction validity, building-system, and save tests.
- Randomized failures must be exactly replayable and diagnostic.
- No merge is part of this plan.

---

## File Structure

- Create: `game/tests/stress/StressHarness.h`
- Create: `game/tests/stress/StressHarnessTests.cpp`
- Create: `game/tests/stress/ConstructionBuildingStressTests.cpp`
- Create: `game/tests/stress/GuestPsychologyStressTests.cpp`
- Create: `game/tests/stress/Final02ContinuationStressTests.cpp`
- Modify: `game/CMakeLists.txt`
- Create: `.github/workflows/final02-stress.yml`
- Create: `docs/stress/final02-construction-guest-verification.md`

### Task 1: Port and verify the shared stress harness contract

**Interfaces:** Same `hh::stress::{Scale,Config,Rng,Trace,RunContext}` interface defined by the central campaign plan.

- [ ] **Step 1: Copy the approved header contract and its tests into this isolated branch**

Use the same SplitMix64 algorithm, rejection-sampling `bounded()`, scale parser, 64-entry trace, and failure format as `stress/main-core-service`. Do not link to another branch or create a production dependency.

- [ ] **Step 2: Prove target RED before copying implementation**

Wire `hh_stress_harness_tests` in `game/CMakeLists.txt`, place `StressHarnessTests.cpp`, and run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target hh_stress_harness_tests
```

Expected: FAIL before `StressHarness.h` is added.

- [ ] **Step 3: Add harness and verify GREEN**

```bash
cmake --build build --config Release --target hh_stress_harness_tests
ctest --test-dir build -C Release -R StressHarness --output-on-failure
```

Expected: PASS.

- [ ] **Step 4: Commit**

```bash
git add game/tests/stress/StressHarness.h game/tests/stress/StressHarnessTests.cpp game/CMakeLists.txt
git commit -m "test: add FINAL-02 stress harness"
```

### Task 2: Stress construction and building systems

**Files:**
- Create: `game/tests/stress/ConstructionBuildingStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: public APIs used by `ConstructionTests.cpp`, `BuildJobTests.cpp`, `BuildingSystemsTests.cpp`, `ConstructionSaveTests.cpp`.
- Produces: executable `hh_construction_building_stress_tests`, CTest `StressConstructionBuilding`.

- [ ] **Step 1: Write named stress phases and wire RED target**

Required scenarios:

```text
dense_place_cancel
topology_churn
utility_churn
validation_churn
job_queue_saturation
partial_build_save
```

Tier A must perform at least 25,000 valid/invalid construction commands and repeatedly exercise cancellation and rebuild. Extended must perform at least 250,000 operations. Exhaustive must perform at least 1,000,000 operations unless an explicit branch hard limit is reached first and documented.

- [ ] **Step 2: Add continuous construction invariants**

Implement test-local scans over public state to enforce:

```cpp
assert(allCompletedStructuresAreValid(snapshot));
assert(noBuildJobReferencesMissingTarget(snapshot));
assert(noMaterialCountIsNegative(snapshot));
assert(noExclusiveConstructionTargetHasTwoActiveJobs(snapshot));
assert(accessibilityStateMatchesCurrentTopology(snapshot));
assert(utilityStateContainsNoDanglingEndpoint(snapshot));
```

Where the public API uses different names, implement adapters only inside the stress test; do not add production aliases simply to match these helper names.

- [ ] **Step 3: Exercise boundary and adversarial commands**

Generate a deterministic stream containing legal placements, illegal placements, repeated cancel attempts, disconnected/reattached utility states, topology changes around room/building validity, and save operations while jobs are incomplete. Invalid commands must fail cleanly without corrupting authoritative state.

- [ ] **Step 4: Prove save/load continuation**

Using the save contract already verified by `ConstructionSaveTests.cpp`:

```cpp
const auto before = canonicalConstructionState(sim);
saveSimulation(sim, path);
auto restored = loadSimulation(path);
assert(canonicalConstructionState(restored) == before);
applyDeterministicTail(sim, seed);
applyDeterministicTail(restored, seed);
assert(canonicalConstructionState(restored) == canonicalConstructionState(sim));
```

`canonicalConstructionState`, `saveSimulation`, and `loadSimulation` are test-local wrappers around the exact existing public calls; they must not bypass production serialization.

- [ ] **Step 5: Run Tier A and baseline construction tests**

```bash
HH_STRESS_SCALE=pr ctest --test-dir build -C Release -R "(StressConstructionBuilding|construction|build_job|building_systems)" --output-on-failure
```

Expected: GREEN.

- [ ] **Step 6: Commit**

```bash
git add game/tests/stress/ConstructionBuildingStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress construction and building systems"
```

### Task 3: Stress guest AI and psychology

**Files:**
- Create: `game/tests/stress/GuestPsychologyStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `GuestPsychology.h`, `GuestGoals.h`, `GuestReviews.h`, `GuestPsychologyArchive.h`, Simulation guest integration used by current FINAL-02 tests.
- Produces: executable `hh_guest_psychology_stress_tests`, CTest `StressGuestPsychology`.

- [ ] **Step 1: Add RED target with all archetypes represented**

The fixture must construct every one of the 13 implemented archetypes and cycle them deterministically across the population. Tier A minimum population is 5,000 guest lifecycles; extended is 50,000; exhaustive is 250,000.

Required phases:

```text
mass_arrival
need_pressure
congested_goal_selection
memory_pressure
complaint_pressure
loyalty_evolution
group_reference_churn
unavailable_goal_fallback
```

- [ ] **Step 2: Assert guest-domain bounds every simulation window**

For each live guest, assert all need, trait, satisfaction, loyalty/repeat-intent values remain in the exact ranges already defined by production or existing tests. Also assert group references resolve, current goals are legal for current state, memory/complaint collections remain within their designed bounds, and attributable experiences retain valid causal references.

Use helpers shaped like:

```cpp
void assertGuestInvariants(const GuestSnapshot& g, hh::stress::RunContext& ctx) {
    if (!allNeedValuesInRange(g)) ctx.fail("guest need out of range", g.tick);
    if (!memoryBounded(g)) ctx.fail("guest memory growth unbounded", g.tick);
    if (!complaintsBounded(g)) ctx.fail("guest complaint growth unbounded", g.tick);
    if (!goalIsLegal(g)) ctx.fail("illegal guest goal", g.tick);
}
```

The concrete snapshot type and accessors must be the existing branch types, not a new production DTO.

- [ ] **Step 3: Add deterministic goal-selection replay**

Run the same guest fixture and command/event sequence twice with seed `0xC0FFEE1234ABCDEF`. Canonicalize only non-authoritative unordered test output, then compare goal choices, satisfaction breakdowns, complaint/review outcomes, and final authoritative guest hash/state.

- [ ] **Step 4: Add long-stay memory expiry pressure**

Advance enough simulated time for repeated memory creation and expiry. The collection must reach a bounded steady regime rather than grow with total simulated history. If a current implementation violates the documented bound, retain the failing seed and fix the smallest production defect before GREEN.

- [ ] **Step 5: Run all guest baselines plus stress**

```bash
HH_STRESS_SCALE=pr ctest --test-dir build -C Release -R "(StressGuestPsychology|guest_)" --output-on-failure
```

Expected: current guest determinism, satisfaction, expectation, review, psychology, and goal tests all remain GREEN.

- [ ] **Step 6: Commit**

```bash
git add game/tests/stress/GuestPsychologyStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress guest psychology at scale"
```

### Task 4: Stress integrated FINAL-02 continuation

**Files:**
- Create: `game/tests/stress/Final02ContinuationStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: authoritative FINAL-02 Simulation integration and its save/archive path.
- Produces: CTest `StressFinal02Continuation`.

- [ ] **Step 1: Build a combined construction-plus-guests fixture**

Use only systems present on this branch: mutable construction/building topology plus guest psychology/goals. Create a deterministic sequence that changes building validity/accessibility while guests are active, generates positive and negative experiences, saves mid-transition, reloads, and continues.

- [ ] **Step 2: Compare canonical authoritative continuation**

At checkpoints every 1,024 operations, compare an original run and a restored run after the save point. The canonical comparison must include construction/building state and all guest state currently persisted by FINAL-02.

- [ ] **Step 3: Add combined crisis scenario selection**

Support `HH_STRESS_SCENARIO=topology_guest_crisis` and `save_in_crisis`. Unknown non-empty scenario names must fail with a clear list of valid values.

- [ ] **Step 4: Run exact replay twice**

```bash
HH_STRESS_SCALE=extended HH_STRESS_SCENARIO=save_in_crisis HH_STRESS_SEED=0x5EED5EED5EED5EED ctest --test-dir build -C Release -R StressFinal02Continuation --output-on-failure
HH_STRESS_SCALE=extended HH_STRESS_SCENARIO=save_in_crisis HH_STRESS_SEED=0x5EED5EED5EED5EED ctest --test-dir build -C Release -R StressFinal02Continuation --output-on-failure
```

Expected: GREEN with identical final hash/state summaries.

- [ ] **Step 5: Commit**

```bash
git add game/tests/stress/Final02ContinuationStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress FINAL-02 save continuation"
```

### Task 5: Add sanitizer and MSVC parity CI

**Files:**
- Create: `.github/workflows/final02-stress.yml`
- Create: `docs/stress/final02-construction-guest-verification.md`

- [ ] **Step 1: Add three jobs**

Required jobs:

```text
ubuntu-release-pr-stress
ubuntu-asan-ubsan-extended
windows-release-pr-stress
```

The sanitizer job runs `StressConstructionBuilding`, `StressGuestPsychology`, and `StressFinal02Continuation` with `HH_STRESS_SCALE=extended`; Windows runs Tier A plus the entire existing FINAL-02 CTest suite.

- [ ] **Step 2: Run full baseline test suite**

```bash
ctest --test-dir build -C Release --output-on-failure
```

Expected: zero regressions, including construction and all guest tests.

- [ ] **Step 3: Record exact verification metadata**

Write parent SHA, verified child SHA, CI run IDs, sanitizer result, Windows result, and replay seeds to `docs/stress/final02-construction-guest-verification.md`.

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/final02-stress.yml docs/stress/final02-construction-guest-verification.md
git commit -m "ci: verify FINAL-02 stress suite"
```

## Completion Gate

Complete only when dense construction/topology mutation, all 13 guest archetypes, bounded memories/complaints, deterministic goal selection, save-in-crisis continuation, ASan+UBSan extended runs, MSVC Tier A, and the full existing FINAL-02 suite are all GREEN without importing systems from other FINAL branches.