# Elevator Bank Dispatch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace FINAL-01's single-request elevator servicing with deterministic capacity-aware multi-car bank dispatch and persistent transport diagnostics.

**Architecture:** Extend `BuildingSystems` in place. Bank assignment is a pure deterministic selection over existing elevator snapshots; each car then performs directional capacity-aware batching while preserving the existing public state enum. `Simulation` owns command validation, elapsed simulation time, save-version migration, and invokes a bank tick once per authoritative one-second step.

**Tech Stack:** C++20, CMake/CTest, existing Hotel Haven text save format.

**Spec:** `docs/superpowers/specs/2026-09-10-elevator-bank-dispatch-design.md`

## Global Constraints

- Extend FINAL-01; do not create a parallel transport/navigation system.
- Preserve existing `requestElevator(EntityId, int, int)` compatibility.
- Dispatch must use integer authoritative state only: no RNG, wall clock, floating-point score, or unordered iteration order.
- Passenger and service banks remain isolated by exact `ElevatorKind`.
- Capacity is a hard invariant.
- Elevators never become fire-egress authority.
- Save format becomes HHGS 12 while HHGS 2-11 remain loadable.
- Completed-trip diagnostics are bounded to 128 records.

---

### Task 1: RED contracts for assignment, capacity and batching

**Files:**
- Modify: `game/tests/BuildingSystemsTests.cpp`
- Modify: `game/CMakeLists.txt` only if the existing target does not already compile `BuildingSystemsTests.cpp`.

**Interfaces:**
- Consumes: existing `Simulation::installElevator`, `Simulation::requestElevator`, `BuildingSystemsSnapshot`.
- Produces required API: `Simulation::requestElevator(ElevatorKind kind, int pickupFloor, int destinationFloor)` bank overload and richer elevator/request snapshots.

- [ ] Add a failing test that installs two passenger cars, issues a bank request, and requires deterministic assignment to the minimum `(ETA, queue depth, elevatorId)` car.
- [ ] Add a failing test with capacity 2 and three same-floor/up-direction requests; after boarding, require exactly two boarded and one waiting.
- [ ] Add a failing test requiring compatible same-floor requests to share one boarding door cycle rather than serial door cycles.
- [ ] Run the building-system target and confirm RED failures are specifically missing bank dispatch/capacity behavior.
- [ ] Commit RED tests.

### Task 2: Snapshot model and deterministic bank selector

**Files:**
- Modify: `game/include/hh/game/BuildingSystems.h`
- Modify: `game/src/BuildingSystems.cpp`
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/src/Simulation.cpp`

**Interfaces:**
- Produces: `ElevatorDirection { Idle, Up, Down }`; request assignment/timestamps; `detail::assignElevatorRequest(BuildingSystemsSnapshot&, ElevatorKind, pickup, destination, requestId, elapsedSeconds)`; bank overload on `Simulation::requestElevator`.

- [ ] Implement request timestamp/assignment fields plus per-car direction/onboard/diagnostic counters using integer fields.
- [ ] Implement a deterministic projected-ETA helper and selection key `(etaSeconds, requests.size(), elevator.id)` over cars in stable ID order.
- [ ] Reject degenerate requests and bank requests with no compatible car.
- [ ] Keep direct-car requests by setting `assignedElevatorId` to that car immediately.
- [ ] Run Task 1 tests to GREEN for assignment while capacity/batching tests remain RED only where expected.
- [ ] Commit selector/model implementation.

### Task 3: Capacity-aware directional car sweep

**Files:**
- Modify: `game/src/BuildingSystems.cpp`
- Modify: `game/tests/BuildingSystemsTests.cpp`

**Interfaces:**
- Consumes request ordering `(requestedAtSeconds, id)` and assigned car IDs.
- Produces: `detail::tickElevator(ElevatorSnapshot&, std::int64_t elapsedSeconds, std::vector<ElevatorTripSnapshot>& completedTrips)`.

- [ ] Write/retain failing assertions for capacity saturation, batched boarding, batched alighting, and opposite-direction deferral.
- [ ] Change car stop selection to deterministic directional sweeps while preserving the five public elevator states.
- [ ] At boarding, board oldest compatible requests up to `capacity`; never exceed capacity.
- [ ] At alighting, complete all onboard requests at the current destination in one door cycle and record wait/ride seconds.
- [ ] When the directional sweep empties, choose the oldest remaining assigned request and establish the next direction.
- [ ] Bound completed trip history to the newest 128 entries.
- [ ] Run building-system tests to GREEN.
- [ ] Commit batching implementation.

### Task 4: Service isolation, rejection and diagnostics RED->GREEN

**Files:**
- Modify: `game/tests/BuildingSystemsTests.cpp`
- Modify: `game/src/BuildingSystems.cpp`
- Modify: `game/src/Simulation.cpp`

**Interfaces:**
- Snapshot diagnostics: `onboardCount`, `completedTrips`, `cumulativeWaitSeconds`, `cumulativeRideSeconds`, global bounded `completedElevatorTrips`.

- [ ] Add a failing test proving a passenger bank request never uses a service car and vice versa.
- [ ] Add failing rejection tests for `pickup == destination`, unserved floors, and absent compatible car.
- [ ] Add a failing diagnostics test whose exact wait/ride duration follows configured floor and door timing.
- [ ] Implement any missing validation/counter updates needed for those tests.
- [ ] Run the complete building-system suite to GREEN.
- [ ] Commit diagnostics/isolation changes.

### Task 5: Save v12 and legacy migration RED->GREEN

**Files:**
- Modify: `game/src/Simulation.cpp`
- Modify: `game/tests/ConstructionSaveTests.cpp`
- Modify: `game/tests/SimulationTestsV11.cpp` only if it contains the existing version-11 fixture helpers.

**Interfaces:**
- HHGS 12 writes new request/car/trip fields.
- HHGS 2-11 loader remains accepted.

- [ ] Add a failing mid-trip save/load test: step to a moving/boarding state, save, load, continue both simulations, and require identical elevator snapshots and byte-identical final saves.
- [ ] Add a failing HHGS 11 migration test requiring existing elevator requests to be assigned to their containing car without changing RNG-dependent simulation state.
- [ ] Bump writer/current max reader version to 12.
- [ ] Serialize/parse new car/request/trip fields for v12; preserve the v11 field parser exactly for legacy saves.
- [ ] During v11 migration, assign each request to its containing elevator and derive stable migration timestamps from `elapsed` plus request-ID ordering.
- [ ] Validate timestamp order, assignment ownership, capacity, counters, and ID uniqueness.
- [ ] Run save/construction and building-system tests to GREEN.
- [ ] Commit persistence changes.

### Task 6: Deterministic burst/soak regression

**Files:**
- Create: `game/tests/ElevatorDispatchSoakTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Uses only public `Simulation` commands and immutable snapshots.

- [ ] Add a deterministic scenario with at least 4 cars, 16 served floors, mixed passenger/service banks, and several hundred stable request events.
- [ ] Require identical same-seed final snapshots and save bytes across two simulations.
- [ ] Assert no car ever reports `onboardCount > capacity`, all completed trips have nonnegative ordered timestamps, completed history never exceeds 128, and service requests never cross bank type.
- [ ] Run the soak target twice and confirm identical results.
- [ ] Commit soak coverage.

### Task 7: Full verification and status

**Files:**
- Modify: `docs/IMPLEMENTATION_STATUS.md`

**Interfaces:** None.

- [ ] Update the status document to remove elevator-car dispatch from the explicit material-gap list and accurately describe what is now implemented; do not claim actor-level elevator path integration unless implemented.
- [ ] Configure a fresh Release build.
- [ ] Build all available portable targets with warnings treated as errors where supported.
- [ ] Run full CTest and record exact pass/fail count.
- [ ] Run `git diff --check` equivalent via PR diff review and inspect every changed file for accidental scope expansion.
- [ ] Open a draft PR against `feature/final-01-construction-building-systems`; do not merge it.
