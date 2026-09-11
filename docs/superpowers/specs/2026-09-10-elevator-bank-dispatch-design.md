# Hotel Haven Elevator Bank Dispatch Design

**Status:** Approved 2026-09-10

## Goal

Extend FINAL-01's existing authoritative building/elevator model into deterministic multi-car elevator-bank dispatch without introducing a parallel navigation or transport architecture.

## Existing seam

FINAL-01 already owns `ElevatorKind`, `ElevatorState`, `ElevatorSpec`, `ElevatorSnapshot`, request persistence, car timing, floor ranges, construction/accessibility state, and one-second simulation ticks. The current `tickElevator()` selects the smallest request ID and serves that request alone; car capacity is not used for batching and cars do not coordinate.

## Authority and boundaries

- `BuildingSystemsSnapshot` remains authoritative for elevator state.
- HMG-020 remains authoritative for shaft/landing/accessibility rules.
- Elevators never count as fire egress.
- Existing direct `requestElevator(elevatorId, ...)` remains supported for compatibility and explicit-car diagnostics/tests.
- New bank requests select an `ElevatorKind` rather than a car. They are deterministically assigned to one compatible installed car.
- Rendering remains snapshot-driven and non-authoritative.
- No speculative escalator, destination-control UI, or physical passenger animation system is added.

## Request model

`ElevatorRequestSnapshot` gains:

- `requestedAtSeconds`: authoritative request timestamp.
- `assignedElevatorId`: selected car ID; zero only before bank assignment.
- `boardedAtSeconds`: zero until boarding.
- `completedAtSeconds`: zero until alighting/removal from the active queue.

`boarded` remains for backward-readable state and explicit state inspection.

`BuildingSystemsSnapshot` gains bounded completed-trip diagnostics rather than retaining an unbounded active-request history. A completed trip records request ID, elevator ID, pickup/destination floors, request/board/complete timestamps, derived wait seconds and ride seconds.

## Deterministic bank assignment

A bank request is eligible for a car only when:

1. car kind exactly matches the requested kind;
2. pickup and destination are within the car's served floor range;
3. the request is non-degenerate (`pickup != destination`).

Each compatible car receives an integer ETA score computed only from authoritative snapshot state:

- seconds remaining in the current phase;
- floor-distance travel time from the car's projected service end to the new pickup;
- door-cycle cost for stops already committed ahead of the request;
- queued compatible pickup batching credit when the car will naturally pass/stop at the pickup in its current direction.

Selection key is `(etaSeconds, queuedRequestCount, elevatorId)`. No RNG, wall-clock time, floating-point comparison, or container iteration order participates in dispatch.

## Capacity and batching

A car may carry at most `capacity` boarded requests. At a boarding stop it boards the oldest compatible waiting requests for that floor, ordered by `(requestedAtSeconds, requestId)`, up to available capacity.

A compatible request is one whose destination continues in the active travel direction. When a car is empty, the oldest boarded/boarding request establishes direction. Requests requiring the opposite direction remain queued for a later sweep.

Multiple riders with the same pickup/direction may therefore share one door cycle. Multiple onboard riders with the same destination alight in the same door cycle.

## Car state machine

Keep the existing public states (`Idle`, `MovingToPickup`, `Boarding`, `MovingToDestination`, `Alighting`) to preserve compatibility. Internally, each transition chooses the next deterministic stop from assigned requests:

- Idle: select the oldest assigned waiting request and travel to its pickup.
- Boarding: board every capacity-compatible request at this floor; then select the nearest destination in the active direction.
- MovingToDestination: arrive at the selected destination.
- Alighting: complete every onboard request for this floor; then continue the current directional sweep if possible, otherwise reverse/serve the oldest remaining assigned request.

Movement stays floor-discrete with `travelSecondsPerFloor`; doors remain `doorSeconds`.

## Diagnostics

`BuildingSystemsSnapshot` exposes per-car queue depth, onboard count, total completed trips, cumulative wait seconds, and cumulative ride seconds. Derived average values are computed by UI/diagnostics rather than stored as floating-point authority.

Completed-trip history is bounded to the newest 128 entries so long-running hotels do not accumulate transport diagnostics indefinitely.

## Save compatibility

Bump the save format from HHGS 11 to HHGS 12.

- HHGS 12 serializes all new request timestamps/assignment data and per-car/bounded-trip metrics.
- HHGS 2-11 continue to load.
- Legacy FINAL-01 elevator requests are treated as directly assigned to their containing elevator.
- Legacy requests receive deterministic timestamps based on load-time simulation elapsed seconds and stable request-ID order; this migration changes no RNG state.
- Save/load in the middle of movement, boarding, or alighting must produce byte-identical continuation to an uninterrupted simulation.

## Validation

Load-time validation rejects:

- request references to the wrong/nonexistent elevator;
- timestamp ordering violations;
- boarded requests exceeding car capacity;
- boarded requests outside served floors;
- impossible active request references;
- negative counters or cumulative times;
- duplicate IDs across active and completed trip records.

## Tests

RED->GREEN tests cover:

1. capacity saturation and deferred riders;
2. same-floor/same-direction batching;
3. deterministic selection between multiple cars;
4. stable car-ID tie breaking;
5. passenger/service bank isolation;
6. out-of-range/no-compatible-car rejection;
7. wait/ride diagnostics;
8. bounded completed-trip history;
9. mid-trip save/load deterministic continuation;
10. legacy HHGS 11 migration;
11. sustained burst/soak dispatch invariants.
