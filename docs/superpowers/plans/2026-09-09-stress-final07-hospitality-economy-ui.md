# FINAL-07 Hospitality, Economy, and UI Stress Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stress the stacked FINAL-05 F&B/amenities/events, FINAL-06 economy/market/competition, and FINAL-07 in-game UI/controller routing systems under large demand, service saturation, long financial campaigns, command churn, alert pressure, and cross-system crises.

**Architecture:** Create `stress/final07-hospitality-economy-ui` from the current `feature/final-07-ingame-ui-management` head. Keep portable gameplay stress targets under `game/tests/stress/`; keep UI/controller stress in the existing Windows-only frontend/app test topology. Use a copied header-only deterministic stress harness for portable gameplay. Cross-system catastrophe tests use only authority actually exposed on this branch; FINAL-02/03 guest/staff/construction state is not fabricated.

**Tech Stack:** C++20, CMake/CTest, FINAL-05/06 game APIs, FINAL-07 frontend/controller APIs, Windows/MSVC frontend, Ubuntu ASan+UBSan for portable gameplay/economy.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Branch from the current FINAL-07 head and record the exact parent SHA.
- FINAL-02 and FINAL-03 remain divergent; do not invent guest/staff/construction authority not exposed by this stack.
- Preserve exact-cent accounting and existing FINAL-06 overbooking/pricing/contract semantics.
- Preserve FINAL-05 service and event behavior; do not turn stress fixtures into new gameplay mechanics.
- UI remains presentation/controller state and must not become simulation authority.
- Frontend CMake is currently Windows-only; do not force a Linux frontend port merely to create parity. Portable game/economy stress still runs Linux + Windows.
- Honor `HH_STRESS_SEED`, `HH_STRESS_SCALE`, `HH_STRESS_SCENARIO` and deterministic replay diagnostics.
- No merge is part of this plan.

---

## File Structure

- Create: `game/tests/stress/StressHarness.h`
- Create: `game/tests/stress/StressHarnessTests.cpp`
- Create: `game/tests/stress/HospitalityStressTests.cpp`
- Create: `game/tests/stress/EconomyMarketStressTests.cpp`
- Create: `game/tests/stress/Final07CatastropheStressTests.cpp`
- Modify: `game/CMakeLists.txt`
- Create: `frontend/tests/GameUiStressTests.cpp`
- Create: `game/app/GameUiCommandRouterStressTests.cpp`
- Modify: `frontend/CMakeLists.txt`
- Modify: root `CMakeLists.txt` only if required to register the new app stress executable in the existing Windows test graph.
- Create: `.github/workflows/final07-stress.yml`
- Create: `docs/stress/final07-hospitality-economy-ui-verification.md`

### Task 1: Add the shared deterministic harness to the FINAL-07 branch

**Interfaces:** Same `hh::stress::{Scale,Config,Rng,Trace,RunContext}` contract used by the other C++ stress branches.

- [ ] **Step 1: Wire `hh_stress_harness_tests` without the header and prove RED**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target hh_stress_harness_tests
```

Expected: FAIL because `StressHarness.h` is missing.

- [ ] **Step 2: Add the exact approved header-only harness**

Use the same SplitMix64 sequence, unbiased rejection-sampling `bounded()`, three scales, environment parsing, 64-entry action trace, and deterministic failure format. Keep it test-only.

- [ ] **Step 3: Verify and commit**

```bash
ctest --test-dir build -C Release -R StressHarness --output-on-failure
git add game/tests/stress/StressHarness.h game/tests/stress/StressHarnessTests.cpp game/CMakeLists.txt
git commit -m "test: add FINAL-07 stress harness"
```

### Task 2: Stress FINAL-05 F&B, amenities, and events

**Files:**
- Create: `game/tests/stress/HospitalityStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `FoodService.h`, `Amenities.h`, `Events.h`, RoomService integration already present on the stacked branch.
- Produces: CTest `StressHospitality` and executable `hh_hospitality_stress_tests`.

- [ ] **Step 1: Write the stress target first**

Required named phases:

```text
food_order_burst
inventory_starvation
amenity_capacity_saturation
event_setup_teardown_churn
room_service_dependency_failure
combined_hospitality_peak
```

Tier A: at least 20,000 F&B/amenity/event operations. Extended: 200,000. Exhaustive: 1,000,000 or the highest valid load before an explicit documented branch capacity limit.

- [ ] **Step 2: Add invariants over existing public state**

At deterministic windows, reject impossible order/event states, negative conserved stock, duplicate exclusive reservations, references to removed amenities/events/orders, queue growth beyond legitimate active/pending work, and completed service without its required production/transport stages.

Test-local invariant shape:

```cpp
void assertHospitalityInvariants(const HospitalitySnapshot& s,
                                 hh::stress::RunContext& ctx) {
    if (!inventoryNonNegative(s)) ctx.fail("negative hospitality inventory", s.day);
    if (!orderStagesLegal(s)) ctx.fail("illegal food/order transition", s.day);
    if (!eventCapacityRespected(s)) ctx.fail("event capacity exceeded", s.day);
    if (!referencesResolve(s)) ctx.fail("orphaned hospitality reference", s.day);
}
```

Use existing branch snapshot/state types in the actual implementation; this helper name is test-local only.

- [ ] **Step 3: Exercise dependency failure and recovery**

Deterministically starve ingredients/capacity, create order/event pressure, restore resources, and verify blocked work either resumes or terminates through an existing legal failure path. No job may remain permanently stale merely because the transient shortage ended.

- [ ] **Step 4: Run current FINAL-05 baselines plus stress**

```bash
HH_STRESS_SCALE=pr ctest --test-dir build -C Release -R "(FoodService|FoodRoomServiceIntegration|Event|Amenity|SimulationFinal05|StressHospitality)" --output-on-failure
```

Expected: GREEN.

- [ ] **Step 5: Commit**

```bash
git add game/tests/stress/HospitalityStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress FINAL-05 hospitality systems"
```

### Task 3: Stress FINAL-06 economy, market, competition, reservations, and financing

**Files:**
- Create: `game/tests/stress/EconomyMarketStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `MarketDemand.h`, `RevenueInventory.h`, `RevenueManagement.h`, `HotelEconomics.h`, `Financing.h`, `Overbooking.h`, `CommercialDemand.h`, `EconomyRuntime.h`.
- Produces: CTest `StressEconomyMarket`.

- [ ] **Step 1: Write RED multi-year scenarios**

Required scenarios:

```text
demand_spike
future_book_saturation
cancellation_no_show_storm
competitor_rate_shock
pricing_rule_overlap
overbooking_recovery
marketing_reputation_feedback
corporate_group_pressure
debt_distress
multi_year_soak
save_in_financial_crisis
```

Tier A advances at least 180 simulated days and processes at least 50,000 market/reservation/economic operations. Extended advances at least 2,000 days. Exhaustive advances at least 10 simulated years unless a documented simulation date limit is reached first.

- [ ] **Step 2: Enforce exact-cent and inventory invariants continuously**

At each accounting window:

```cpp
if (!ledgerBalancesExactly(snapshot)) ctx.fail("economy ledger imbalance", snapshot.day);
if (!roomInventoryWithinCapacityAndExplicitOverbooking(snapshot))
    ctx.fail("reservation inventory exceeded explicit allowance", snapshot.day);
if (hasDuplicateBookingId(snapshot)) ctx.fail("duplicate booking id", snapshot.day);
if (!contractedRatesPreserved(snapshot)) ctx.fail("contracted rate mutated", snapshot.day);
if (!finiteNonAccountingMetrics(snapshot)) ctx.fail("non-finite economy metric", snapshot.day);
```

All money comparisons use the exact integer-cent representation already implemented; do not add floating tolerances for authoritative accounting.

- [ ] **Step 3: Stress rule priority determinism**

Generate overlapping revenue-management rules with deterministic legal inputs and compare selected rule/rate outcomes across two identical seeded runs. Priority ties must follow the branch's existing explicit semantics.

- [ ] **Step 4: Stress overbooking and relocation recovery**

Drive reservation inventory through sellout and only the explicitly configured overbooking allowance, then inject cancellations/no-shows/arrivals. Verify the existing relocation/recovery order, compensation posting, and final inventory consistency. Never increase overbooking allowance to make the test pass.

- [ ] **Step 5: Stress financing distress and persistence**

Use existing loan/default/covenant APIs to create a weak-cash scenario, advance through scheduled exact-cent debt service, save during distress, reload, continue with identical inputs, and compare authoritative economy hash/state.

- [ ] **Step 6: Run existing economy suite plus stress**

```bash
HH_STRESS_SCALE=pr ctest --test-dir build -C Release -R "(MarketDemand|RevenueInventory|RevenueManagement|HotelEconomics|Financing|Overbooking|CommercialDemand|EconomyIntegration|EconomySoak|EconomySpecReconciliation|StressEconomyMarket)" --output-on-failure
```

Expected: GREEN; existing 365-day soak remains intact.

- [ ] **Step 7: Commit**

```bash
git add game/tests/stress/EconomyMarketStressTests.cpp game/CMakeLists.txt
git commit -m "test: stress FINAL-06 economy and market"
```

### Task 4: Stress FINAL-07 UI models and controllers on Windows

**Files:**
- Create: `frontend/tests/GameUiStressTests.cpp`
- Modify: `frontend/CMakeLists.txt`

**Interfaces:**
- Consumes: `GameHudModel`, `GameHudController`, `InspectorModel`, `BuildCatalog`/BuildToolController, `OverlayModel`, `OperationsDashboard`, `EconomyDashboard`, `AlertCenter`, `ObjectiveUi`, `UiSettings`, `GameUiRuntime`.
- Produces: additional cases inside existing `hh_frontend_tests` and explicit CTest label `StressUi` if split into a dedicated executable is cleaner without duplicating the framework.

- [ ] **Step 1: Add the source to `hh_frontend_tests` before creating it and prove RED on Windows**

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target hh_frontend_tests
```

Expected: FAIL because `frontend/tests/GameUiStressTests.cpp` does not exist.

- [ ] **Step 2: Add deterministic UI churn tests**

Minimum Tier A operations inside the test: 100,000 model/controller actions, including snapshot refreshes, overlay open/close, inspector target changes, build preview create/update/cancel, operations/economy dashboard refresh, alerts, objective/accessibility transitions, and selection churn.

Required assertions:

```cpp
REQUIRE(alertCount <= configuredAlertBound);
REQUIRE(noStaleSelectionReferences(model));
REQUIRE(buildPreviewStateIsInternallyConsistent(model));
REQUIRE(snapshotGenerationIsMonotonic(runtime));
REQUIRE(uiActionsDidNotMutateAuthoritativeSimulationFixture());
```

Use the project's existing frontend test macros and public models. The last assertion uses a read-only authority fixture/hash around UI-only operations.

- [ ] **Step 3: Test alert dedupe/expiry pressure**

Generate repeated identical causal alerts plus distinct alerts beyond the normal display/history window. Verify deduplication and bounded expiry according to existing AlertCenter rules; do not invent a new cap.

- [ ] **Step 4: Run frontend tests repeatedly**

```powershell
1..10 | ForEach-Object { ctest --test-dir build -C Release -R hh_frontend_tests --output-on-failure; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }
```

Expected: 10/10 GREEN with no state leakage between process runs.

- [ ] **Step 5: Commit**

```bash
git add frontend/tests/GameUiStressTests.cpp frontend/CMakeLists.txt
git commit -m "test: stress FINAL-07 UI state churn"
```

### Task 5: Stress typed application command routing

**Files:**
- Create: `game/app/GameUiCommandRouterStressTests.cpp`
- Modify: root `CMakeLists.txt` or the existing app test registration location.

**Interfaces:**
- Consumes: `GameUiCommandRouter.h`, `GameUiBridge.h`, `InGameUiContract.h`.
- Produces: CTest `StressUiCommandRouter` on Windows.

- [ ] **Step 1: Wire missing-file RED target**

Register a test executable linked exactly as the existing `GameUiCommandRouterTests.cpp` target, but point it at the new stress source. Build must fail until the source exists.

- [ ] **Step 2: Generate 100,000+ typed command attempts**

Deterministically mix valid and stale/invalid commands exposed by the router. For each accepted command, require the same reason/result semantics as the normal router. Rejected commands must leave authoritative state unchanged.

```cpp
const auto before = authoritativeFixtureHash(sim);
const auto result = router.route(command);
if (!result.accepted) {
    REQUIRE(authoritativeFixtureHash(sim) == before);
}
```

Do not invent command kinds solely for stress coverage.

- [ ] **Step 3: Test repeated bridge snapshot/read cycles**

Interleave commands with bridge snapshot reads and prove snapshots never confer mutation authority or retain dangling references after the simulation state advances.

- [ ] **Step 4: Commit after GREEN**

```bash
git add game/app/GameUiCommandRouterStressTests.cpp CMakeLists.txt
git commit -m "test: stress FINAL-07 command routing"
```

### Task 6: Add cross-system catastrophe campaigns

**Files:**
- Create: `game/tests/stress/Final07CatastropheStressTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes only systems coexisting on FINAL-07: FINAL-04 service/logistics inherited in the stack, FINAL-05 hospitality, FINAL-06 economy.
- Produces: CTest `StressFinal07Catastrophe`.

- [ ] **Step 1: Implement supported combined crises**

Required scenarios:

```text
hospitality_demand_shock
inventory_and_service_crunch
overbooking_and_cancellation_reversal
financial_crisis_with_service_pressure
save_in_crisis
```

Do not implement the spec's workforce or construction catastrophe variants here because those authorities are absent/divergent on this branch.

- [ ] **Step 2: Validate all participating subsystem invariants at every checkpoint**

One combined failure must identify both the global phase and the specific violated subsystem invariant. Keep action trace entries prefixed with `service:`, `hospitality:`, or `economy:`.

- [ ] **Step 3: Save and replay at peak crisis**

Where the FINAL-07 stack persists the participating authoritative state, save at the highest queue/demand/distress point, reload, continue both branches, and compare authoritative persisted state. Do not claim persistence for UI-only state unless existing save behavior explicitly does so.

- [ ] **Step 4: Run Tier A and extended**

```bash
HH_STRESS_SCALE=pr ctest --test-dir build -C Release -R StressFinal07Catastrophe --output-on-failure
HH_STRESS_SCALE=extended HH_STRESS_SEED=0x5EED5EED5EED5EED ctest --test-dir build -C Release -R StressFinal07Catastrophe --output-on-failure
```

Expected: GREEN.

- [ ] **Step 5: Commit**

```bash
git add game/tests/stress/Final07CatastropheStressTests.cpp game/CMakeLists.txt
git commit -m "test: add FINAL-07 catastrophe campaigns"
```

### Task 7: Add CI and verification record

**Files:**
- Create: `.github/workflows/final07-stress.yml`
- Create: `docs/stress/final07-hospitality-economy-ui-verification.md`

- [ ] **Step 1: Add portable gameplay jobs**

Ubuntu Release Tier A and Ubuntu ASan+UBSan extended jobs run `StressHospitality`, `StressEconomyMarket`, and `StressFinal07Catastrophe` plus relevant baselines.

- [ ] **Step 2: Add Windows full-stack job**

Windows Release runs all portable game stress tests plus `hh_frontend_tests`, `StressUiCommandRouter`, the normal client smoke test, and the pre-existing full CTest suite. Use a workflow timeout as a hang guard; do not add tight UI wall-clock assertions.

- [ ] **Step 3: Run exact deterministic economy replay on Ubuntu and Windows**

Seed: `0xC0FFEE1234ABCDEF`. Compare the authoritative FINAL-06 hash/state summary printed by `StressEconomyMarket`; it must match where FINAL-06 claims cross-platform deterministic hashing.

- [ ] **Step 4: Record and commit verification**

```bash
git add .github/workflows/final07-stress.yml docs/stress/final07-hospitality-economy-ui-verification.md
git commit -m "ci: verify FINAL-07 stack stress suite"
```

## Completion Gate

Complete only when FINAL-05 hospitality load, FINAL-06 multi-year exact-cent/economy load, FINAL-07 UI and typed-router churn, supported cross-system catastrophe scenarios, sanitizer jobs, Windows full frontend/app verification, exact replay, and every pre-existing test on this stack are GREEN without importing divergent FINAL-02/03 authority.