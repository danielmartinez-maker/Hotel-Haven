# Main Core and Service Stress Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stress the integrated `main` simulation, workforce/departments/optimizer, and FINAL-04 service/logistics systems with deterministic high-load, soak, crisis, and replay coverage.

**Architecture:** Create `stress/main-core-service` from the current `main` head and keep all stress-only helpers under `game/tests/stress/`. A header-only stress harness supplies deterministic sampling, scale parsing, action tracing, and diagnostics; three independent CTest executables cover core simulation, workforce/optimizer, and service/logistics so failures remain attributable.

**Tech Stack:** C++20, CMake/CTest, existing `hh_game` APIs, GitHub Actions, Ubuntu ASan+UBSan, MSVC Release.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Branch from the current `main` head and record that SHA; never push stress changes directly to `main`.
- Preserve the existing FINAL-04 meaningful layout acceptance contract and all existing simulation rules.
- Use exact scale names `pr`, `extended`, `exhaustive`; default `pr`.
- Honor `HH_STRESS_SEED`, `HH_STRESS_SCALE`, and `HH_STRESS_SCENARIO`.
- Random selection uses raw deterministic engine output with rejection/modulo helpers, not implementation-specific standard distributions where replay parity matters.
- Every failure prints suite, seed, phase/scenario, tick/day when available, invariant, state hash when available, and bounded recent actions.
- Do not hide load defects by reducing counts solely to get GREEN.
- No merge is part of this plan.

---

## File Structure

- Create: `game/tests/stress/StressHarness.h` — shared test-only deterministic harness.
- Create: `game/tests/stress/StressHarnessTests.cpp` — harness contract tests.
- Create: `game/tests/stress/CoreSimulationStressTests.cpp` — long-run simulation/time/command/hash checks.
- Create: `game/tests/stress/WorkforceStressTests.cpp` — workforce/departments/optimizer saturation.
- Create: `game/tests/stress/ServiceLogisticsStressTests.cpp` — housekeeping/laundry/logistics/engineering/room-service catastrophe and soak tests.
- Modify: `game/CMakeLists.txt` — register portable stress targets on both GCC/Clang and MSVC.
- Create: `.github/workflows/main-core-service-stress.yml` — Tier A, extended, sanitizer, and Windows parity jobs.
- Create: `docs/stress/main-core-service-verification.md` — exact parent/head/run record.

### Task 1: Add the deterministic test harness

**Interfaces:**
- Produces: `hh::stress::Config`, `hh::stress::Rng`, `hh::stress::Trace`, and `hh::stress::RunContext` used by later stress files.

- [ ] **Step 1: Write harness tests first**

Create `StressHarnessTests.cpp` to assert: default scale is `pr`; explicit seed parsing accepts decimal and `0x` hex; two `Rng` instances with the same seed produce the same 1,000 bounded values; bounds are respected; trace retains only the latest 64 actions.

Core test shape:

```cpp
#include "StressHarness.h"
#include <cassert>

int main() {
    hh::stress::Rng a{0x123456789abcdef0ULL};
    hh::stress::Rng b{0x123456789abcdef0ULL};
    for (int i = 0; i < 1000; ++i) {
        const auto av = a.bounded(37);
        const auto bv = b.bounded(37);
        assert(av == bv);
        assert(av < 37);
    }
    hh::stress::Trace trace{64};
    for (int i = 0; i < 100; ++i) trace.push("op=" + std::to_string(i));
    assert(trace.size() == 64);
}
```

- [ ] **Step 2: Wire only the failing target and prove RED**

Add `hh_stress_harness_tests` to CMake before creating `StressHarness.h`.

Run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target hh_stress_harness_tests
```

Expected: compile FAIL because `StressHarness.h` does not exist.

- [ ] **Step 3: Implement the header-only harness**

`StressHarness.h` must expose exactly:

```cpp
namespace hh::stress {
enum class Scale { Pr, Extended, Exhaustive };
struct Config {
    Scale scale;
    std::uint64_t seed;
    std::string scenario;
};
Config configFromEnvironment(std::uint64_t fallbackSeed);
class Rng {
public:
    explicit Rng(std::uint64_t seed);
    std::uint64_t next();
    std::uint64_t bounded(std::uint64_t exclusiveUpperBound);
    bool chance(std::uint32_t numerator, std::uint32_t denominator);
private:
    std::uint64_t state_;
};
class Trace {
public:
    explicit Trace(std::size_t capacity = 64);
    void push(std::string action);
    std::size_t size() const;
    std::string dump() const;
private:
    std::size_t capacity_;
    std::deque<std::string> entries_;
};
struct RunContext {
    std::string suite;
    Config config;
    Trace trace{64};
    std::string phase;
    [[noreturn]] void fail(std::string_view invariant,
                           std::uint64_t tickOrDay,
                           std::string_view stateHash = {}) const;
};
}
```

Use a fully specified test-only SplitMix64 sequence for `next()`. Implement `bounded()` with rejection sampling so it is unbiased and platform-independent. `configFromEnvironment()` must reject unknown scale values with a non-zero test failure rather than silently falling back.

- [ ] **Step 4: Run harness target GREEN on Linux and Windows**

```bash
cmake --build build --config Release --target hh_stress_harness_tests
ctest --test-dir build -C Release -R StressHarness --output-on-failure
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add game/tests/stress/StressHarness.h game/tests/stress/StressHarnessTests.cpp game/CMakeLists.txt
git commit -m "test: add deterministic stress harness"
```

### Task 2: Stress the simulation core

**Files:**
- Create: `game/tests/stress/CoreSimulationStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `hh::stress::RunContext` and the public `Simulation` API already exercised by `game/tests/SimulationTests.cpp`.
- Produces: CTest `StressCoreSimulation` and executable `hh_core_simulation_stress_tests`.

- [ ] **Step 1: Add the target and a failing deterministic replay assertion**

Create a test runner with fixed scenario names `long_run`, `burst_commands`, and `repeat_hash`. Reuse the same public construction/setup calls already used by `SimulationTests.cpp`; do not add test-only production hooks.

The repeat contract is:

```cpp
const auto first = runScenario(config.seed, operationBudget(config.scale));
const auto second = runScenario(config.seed, operationBudget(config.scale));
if (first.authoritativeHash != second.authoritativeHash) {
    ctx.fail("identical seed and command stream diverged", first.finalTick,
             first.authoritativeHash + "/" + second.authoritativeHash);
}
```

Before implementing `runScenario`, wire `hh_core_simulation_stress_tests` and prove RED on an undefined symbol/fixture.

- [ ] **Step 2: Implement deterministic scenario generation**

Use budgets:

```cpp
std::size_t operationBudget(hh::stress::Scale scale) {
    switch (scale) {
        case hh::stress::Scale::Pr: return 25'000;
        case hh::stress::Scale::Extended: return 250'000;
        case hh::stress::Scale::Exhaustive: return 2'000'000;
    }
    std::abort();
}
```

Generate only commands/states that the existing main public API supports. Every 256 operations validate monotonic time, finite numeric snapshots, stable unique IDs exposed by the API, and bounded live collections relative to legitimate live entities.

- [ ] **Step 3: Add a coarse hang budget**

Register CTest property `TIMEOUT 180` for Tier A. Extended/exhaustive runs use workflow-level timeouts instead of changing correctness expectations.

- [ ] **Step 4: Run twice with exact replay seed**

```bash
HH_STRESS_SCALE=pr HH_STRESS_SEED=0xC0FFEE1234ABCDEF ctest --test-dir build -C Release -R StressCoreSimulation --output-on-failure
HH_STRESS_SCALE=pr HH_STRESS_SEED=0xC0FFEE1234ABCDEF ctest --test-dir build -C Release -R StressCoreSimulation --output-on-failure
```

Expected: PASS both runs with identical reported final hash.

- [ ] **Step 5: Commit**

```bash
git add game/tests/stress/CoreSimulationStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress deterministic simulation core"
```

### Task 3: Stress workforce, departments, and optimizer

**Files:**
- Create: `game/tests/stress/WorkforceStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `Workforce.h`, `Departments.h`, `StaffOptimization.h`, and existing setup patterns from `WorkforceTests.cpp`, `DepartmentTests.cpp`, `OptimizerIntegrationTests.cpp`.
- Produces: CTest `StressWorkforce`.

- [ ] **Step 1: Write the RED saturation campaign**

Create named phases `roster_growth`, `shift_churn`, `absence_spike`, `task_saturation`, `optimizer_replan`, `fallback_replay`. Tier A creates at least 2,000 employees and 20,000 task/assignment operations; extended multiplies operations by 10; exhaustive by 50.

Continuous assertions must reject:

```cpp
assert(noDuplicateExclusiveAssignments(snapshot));
assert(allAssignmentsReferenceLiveEmployees(snapshot));
assert(allAssignedEmployeesAreEligible(snapshot));
assert(noOverlappingExclusiveWork(snapshot));
assert(pendingPlanCount(snapshot) <= legitimatePendingWork(snapshot));
```

Implement these as test-local scans over public snapshots/containers; do not add production caches solely for testing.

- [ ] **Step 2: Prove RED before production changes**

Wire the executable and run it. Expected RED may be compile/link because the stress fixture is incomplete or a real invariant failure. Record the first genuine product failure separately if one appears.

- [ ] **Step 3: Finish the fixture and deterministic optimizer replay**

Run identical seeded planning twice and compare the ordered assignment/result representation exposed by existing tests. If optimizer code intentionally returns an unordered container, canonicalize only in the test by sorting stable IDs before comparison; do not change gameplay order unless nondeterminism is authoritative.

- [ ] **Step 4: Make existing workforce tests run on MSVC too**

The current main `game/CMakeLists.txt` registers workforce/department/optimizer tests only inside the non-MSVC branch. Move target registration outside the compiler-warning conditional while keeping compiler flags platform-specific. Verify Windows now executes baseline workforce tests as well as `StressWorkforce`.

- [ ] **Step 5: Run targeted and baseline tests**

```bash
ctest --test-dir build -C Release -R "(Workforce|Department|Optimizer|StressWorkforce)" --output-on-failure
```

Expected: all GREEN.

- [ ] **Step 6: Commit**

```bash
git add game/tests/stress/WorkforceStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress workforce and optimizer saturation"
```

### Task 4: Stress FINAL-04 service and logistics

**Files:**
- Create: `game/tests/stress/ServiceLogisticsStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: public Housekeeping, Laundry, Logistics, Engineering, RoomService, ServiceLogistics APIs and existing FINAL-04 test fixtures.
- Produces: CTest `StressServiceLogistics`.

- [ ] **Step 1: Write catastrophe phases before implementation**

The executable must support these exact scenarios:

```text
checkout_storm
laundry_starvation
receiving_saturation
maintenance_storm
room_service_burst
combined_crisis
save_in_crisis
```

Tier A minimum load: 500 rooms/turn jobs where the API supports that count, 10,000 service operations, and at least 30 simulated days. Extended: at least 365 days. Exhaustive: at least 2,000 days or the largest lower duration justified by a documented hard game limit.

- [ ] **Step 2: Assert physical conservation continuously**

Every simulation window, compute test-local conservation equations from exposed state. For each conserved resource, require:

```cpp
const auto accounted = clean + dirty + inProcess + staged + assigned;
if (accounted != initial + received - permanentlyConsumedOrRemoved) {
    ctx.fail("physical resource conservation violated", day);
}
```

Also assert nonnegative physical quantities, legal job stages, live references, and bounded blocked/pending queues.

- [ ] **Step 3: Preserve layout acceptance**

Reuse the existing efficient-vs-poor layout fixture and keep the meaningful directionality: poor layout must increase travel/wait pressure, lower satisfaction, and reduce real operating profit. Do not substitute queue length or another proxy for profit/satisfaction.

- [ ] **Step 4: Stress save/load only through existing authoritative save API**

For `save_in_crisis`, serialize at the peak of an active crisis, reload, continue both original and restored states with the same subsequent command stream, and compare authoritative hashes/snapshots exposed by the branch. If main does not expose a valid save API for one subservice, omit only that subservice from the round-trip; do not invent serialization.

- [ ] **Step 5: Run Tier A and extended**

```bash
HH_STRESS_SCALE=pr ctest --test-dir build -C Release -R StressServiceLogistics --output-on-failure
HH_STRESS_SCALE=extended HH_STRESS_SEED=0x5EED5EED5EED5EED ctest --test-dir build -C Release -R StressServiceLogistics --output-on-failure
```

Expected: GREEN; existing `ServiceLogisticsSoak` remains GREEN.

- [ ] **Step 6: Commit**

```bash
git add game/tests/stress/ServiceLogisticsStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress service logistics catastrophe paths"
```

### Task 5: Add Linux sanitizer and Windows parity CI

**Files:**
- Create: `.github/workflows/main-core-service-stress.yml`
- Create: `docs/stress/main-core-service-verification.md`

- [ ] **Step 1: Add CI after local tests are GREEN**

Workflow jobs:

```text
ubuntu-release-pr-stress
ubuntu-asan-ubsan-extended
windows-release-pr-stress
```

Ubuntu sanitizer configure flags:

```bash
-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
-DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
```

Run `StressCoreSimulation`, `StressWorkforce`, and `StressServiceLogistics`. Set `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

- [ ] **Step 2: Run full baseline suite on both platforms**

```bash
ctest --test-dir build -C Release --output-on-failure
```

Expected: zero baseline regressions.

- [ ] **Step 3: Record exact verification**

Write parent SHA, verified stress head SHA, workflow run IDs, Tier A result, extended result, and replay seed `0x5EED5EED5EED5EED` to `docs/stress/main-core-service-verification.md`.

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/main-core-service-stress.yml docs/stress/main-core-service-verification.md
git commit -m "ci: verify main core service stress suite"
```

## Completion Gate

This branch is complete only when the three stress CTests pass on Release, selected extended tests pass under ASan+UBSan, workforce baseline tests execute on MSVC, exact replay is stable, FINAL-04 conservation/layout contracts remain intact, and the complete pre-existing CTest suite is GREEN.