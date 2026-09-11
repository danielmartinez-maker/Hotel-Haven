# Elevator Bank Dispatch Implementation Plan

**Status:** Implemented; final CI verification pending on the current head.

**Goal:** Replace FINAL-01's single-request elevator servicing with deterministic capacity-aware multi-car bank dispatch while preserving FINAL-01 persistence compatibility.

**Architecture:** Extend `BuildingSystems` in place. Bank assignment is a pure deterministic selection over existing elevator snapshots; each car performs directional capacity-aware batching while preserving the existing public state enum. The existing HHGS 11 car/request fields remain the complete behavior authority, so no save-version migration is required.

**Tech Stack:** C++20, CMake/CTest, existing Hotel Haven HHGS 11 text save format.

**Spec:** `docs/superpowers/specs/2026-09-10-elevator-bank-dispatch-design.md`

## Constraints

- Extend FINAL-01; no parallel transport/navigation system.
- Preserve direct `requestElevator(EntityId, ...)` compatibility.
- Add bank `requestElevator(ElevatorKind, ...)` selection.
- Integer deterministic scoring only; no RNG, wall clock, floating dispatch score, or unordered iteration authority.
- Passenger/service banks isolated by exact `ElevatorKind`.
- Capacity is a hard invariant.
- Elevators remain excluded from fire-egress authority.
- HHGS 11 remains unchanged; new live diagnostics must be derivable from persisted state.
- Automatic guest/staff path integration is out of scope and must not be claimed.

## Completed implementation sequence

### 1. RED dispatch contracts

- [x] Added failing bank-selection, capacity/batching, bank-isolation and rejection contracts before production API existed.
- [x] Opened branch-isolated draft PR #25 against FINAL-01 while the feature was RED.

### 2. Deterministic bank selector

- [x] Added `ElevatorDirection` and derived `assignedElevatorId`/`onboardCount` diagnostics.
- [x] Added `Simulation::requestElevator(ElevatorKind, pickup, destination)` in a small dedicated translation unit rather than expanding `Simulation.cpp`.
- [x] Implemented exact-kind/floor eligibility and selection key `(etaSeconds, queueDepth, elevatorId)`.
- [x] Rejected same-floor and no-compatible-car bank requests.

### 3. Capacity-aware directional batching

- [x] Preserved existing five public car states.
- [x] Batched same-floor/same-direction waiting requests in stable request-ID order.
- [x] Enforced remaining capacity before boarding.
- [x] Selected nearest onboard destination in the current direction with stable ID tie breaking.
- [x] Alighted every onboard request sharing the reached destination in one door cycle.
- [x] Returned to the oldest waiting request after the onboard sweep drained.

### 4. Persistence safety

- [x] Kept HHGS 11 writer/reader unchanged because all behavior-critical elevator state was already persisted.
- [x] Kept assignment/direction/onboard diagnostics derived and outside persistence equality.
- [x] Added mid-trip batched save/load continuation requiring byte-identical final saves and equal authoritative snapshots.

### 5. Burst/soak regression

- [x] Added `ElevatorDispatchSoakTests.cpp`.
- [x] Scenario uses four cars, sixteen floors, passenger/service banks and 400 deterministic requests.
- [x] Capacity is asserted after each burst tick and through a 20,000-second drain.
- [x] Same-seed simulations must match at every burst step and end with identical snapshots/save bytes and no stranded requests.

### 6. Remaining verification checklist

- [ ] Current-head Ubuntu Integrated Game build + CTest green.
- [ ] Current-head Windows Integrated Game build + CTest + smoke/package green.
- [ ] Adjacent Balance Lab, Optimization and OpenUSD workflows green.
- [ ] Review PR diff for unintended scope expansion.
- [ ] Update implementation status and PR body with exact verified evidence.
- [ ] Keep PR draft and unmerged.
