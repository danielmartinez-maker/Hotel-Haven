# Hotel Haven Elevator Bank Dispatch Design

**Status:** Approved and implemented 2026-09-10

## Goal

Extend FINAL-01's existing authoritative building/elevator model into deterministic multi-car elevator-bank dispatch without introducing a parallel navigation or transport architecture.

## Existing seam

FINAL-01 already owns `ElevatorKind`, `ElevatorState`, `ElevatorSpec`, `ElevatorSnapshot`, request persistence, car timing, floor ranges, construction/accessibility state, and one-second simulation ticks. The baseline `tickElevator()` selected the smallest request ID and served that request alone; car capacity was not used for batching and cars did not coordinate.

## Authority and boundaries

- `BuildingSystemsSnapshot` remains authoritative for elevator state.
- HMG-020 remains authoritative for shaft/landing/accessibility rules.
- Elevators never count as fire egress.
- Existing direct `requestElevator(elevatorId, ...)` remains supported for compatibility and explicit-car diagnostics/tests.
- New bank requests use `requestElevator(ElevatorKind, pickupFloor, destinationFloor)` and deterministically select one compatible installed car.
- Rendering remains snapshot-driven and non-authoritative.
- Guest/staff pathfinding is not automatically routed through elevator calls in this scope. This subsystem supplies deterministic car/request behavior for that later integration.
- No escalator, destination-control UI, or physical passenger animation system is added.

## Deterministic bank assignment

A bank request is eligible for a car only when:

1. car kind exactly matches the requested kind;
2. pickup and destination are within the car's served floor range;
3. the request is non-degenerate (`pickup != destination`).

Each compatible car receives an integer ETA derived only from authoritative snapshot state:

- current phase seconds remaining;
- travel time from current/projected target floor to the requested pickup;
- a deterministic door-cycle charge for already queued requests.

Selection key is `(etaSeconds, queuedRequestCount, elevatorId)`. No RNG, wall-clock time, floating-point comparison, or unordered-container iteration order participates in dispatch.

## Capacity and directional batching

A car may carry at most `capacity` boarded requests. At a boarding stop it collects waiting requests for the same pickup floor and active direction in stable request-ID order until capacity is exhausted.

The first waiting request establishes the sweep direction. Same-floor requests whose destinations continue in that direction may share the door cycle. Opposite-direction requests remain waiting for a later sweep. Multiple onboard riders sharing a destination alight in the same door cycle.

## Car state machine

The existing public states remain unchanged: `Idle`, `MovingToPickup`, `Boarding`, `MovingToDestination`, `Alighting`.

- Idle: select the oldest waiting request and travel to its pickup.
- Boarding: batch compatible riders up to capacity and choose the nearest onboard destination in the current direction.
- MovingToDestination: travel using the existing integer `travelSecondsPerFloor` timing.
- Alighting: remove all onboard requests for the current floor in one door cycle; continue the sweep if riders remain, otherwise service the oldest waiting request.

Doors continue to use `doorSeconds`. Car movement remains floor-discrete and simulation-authoritative.

## Live diagnostics

The immutable elevator snapshot exposes derived `direction`, `onboardCount`, and per-request `assignedElevatorId`. Queue depth is the active request-vector size. These fields are diagnostic views of existing authoritative HHGS 11 car/request state; they do not create a second persistence authority.

Historical wait/ride telemetry is intentionally deferred. Adding behavior-irrelevant historical state would require a save-format migration without improving dispatch correctness.

## Save compatibility

The implementation deliberately keeps **HHGS 11** unchanged.

The existing save already persists every behavior-critical dispatch field: containing elevator/car identity, floor range, current/target floors, capacity, travel/door timing, public state, phase time, active request ID, each request's pickup/destination, and whether it has boarded. `assignedElevatorId`, direction, and onboard count are reconstructed deterministically from that state.

This avoids an unnecessary HHGS 12 migration and preserves FINAL-01's existing v2-v11 loading behavior. A dedicated regression saves during a two-rider trip, reloads, continues both simulations, and requires byte-identical final saves plus equal authoritative building-system snapshots.

## Validation and rejection

Bank commands reject:

- invalid elevator kind;
- pickup equal to destination;
- floors not jointly served by a compatible car;
- absence of a passenger/service car of the requested kind.

Existing FINAL-01 load validation remains authoritative for saved elevator ranges, state enum, timing, request floors, active request references and entity-ID uniqueness.

## Verification contracts

Tests cover:

1. deterministic nearest-car assignment;
2. stable `(ETA, queue depth, elevator ID)` tie breaking;
3. capacity saturation with deferred riders;
4. same-floor/same-direction batching;
5. opposite-direction deferral;
6. passenger/service bank isolation;
7. degenerate/unserved request rejection;
8. mid-trip HHGS 11 save/load continuation;
9. same-seed deterministic behavior;
10. a four-car, sixteen-floor, 400-request burst followed by a 20,000-second drain soak with continuous capacity checks and byte-identical same-seed final saves.
