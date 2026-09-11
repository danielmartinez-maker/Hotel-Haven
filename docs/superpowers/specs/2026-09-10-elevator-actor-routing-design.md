# Hotel Haven — Elevator Actor Routing Design

Date: 2026-09-10
Status: Approved design; implementation not started
Branch: `feature/elevator-actor-routing-final02`
Parent: FINAL-02 Guest AI & Psychology (`8e83358b01ad2befb7f1afaea2269c4e7b0c4c4f`)

## Purpose

Integrate the verified deterministic elevator-bank simulation with authoritative guest and staff movement so actors complete cross-floor travel through physical elevator landings rather than treating elevators as a management-only subsystem.

The implementation preserves the existing tile pathfinder, stair behavior, elevator dispatch/capacity logic, deterministic simulation, optimizer authority boundaries, and legacy save compatibility. Elevators are not instantaneous teleport edges and never count as fire egress.

## Existing authority

Actor movement currently uses a tile BFS: horizontal passable tiles plus vertical movement only between stacked `TileKind::Stairs` cells. Elevator dispatch separately owns Passenger/Service car selection, queues, direction, batching, capacity, pickup travel, ride travel, boarding, and alighting.

FINAL-02 owns guest psychology and HHGS 12. The verified elevator work is on the updated FINAL-01/elevator line. This convergence branch starts from FINAL-02 and selectively ports that verified elevator behavior before adding actor integration.

## Non-goals

This change does not merge any draft PR, replace stairs, make elevators fire egress, add escalators or moving walkways, add predictive traffic optimization, invent new psychology penalties, change external-optimizer authority, or add elevator rendering/art.

## Physical elevator contract

A route-capable elevator receives an optional landing coordinate `(landingX, landingY)`, shared vertically across its served floors. Elevators without physical coordinates remain valid for the existing direct elevator API but automatic routing ignores them.

A physical elevator is route-eligible only when its landing is inside the map; pickup and destination floors are served; pickup and destination landing tiles are passable; the actor can reach the pickup landing with a same-floor-only walk; and the destination landing can reach the final target with a same-floor-only walk.

Installation rejects an explicitly physical landing that is outside the map or lacks a passable landing tile on any floor the car claims to serve. Legacy abstract installations remain valid.

Cars with the same `ElevatorKind`, `landingX`, and `landingY` form one physical bank. Actor routing selects a bank; the existing dispatcher selects the car. With multiple valid banks, selection is deterministic: shortest same-floor approach path, then lowest landing `y`, then lowest landing `x`. Cross-bank speculative queue prediction is deliberately out of scope.

## Actor transport state

`PersonState` remains unchanged. Elevator travel is an authoritative substate of a `Traveling` person.

Each active elevator traveler stores selected kind, landing coordinates, assigned car ID, request ID, pickup floor, destination floor, and phase: `WalkingToLanding`, `WaitingForElevator`, or `RidingElevator`. The existing `destination` remains the ultimate tile target.

Actor-created elevator requests persist `riderId`; manual/direct requests use `riderId == 0`. A rider may own at most one live request, and a rider-owned request must reference an existing traveling actor. Save/load rejects duplicate ownership, missing riders, impossible floors, mismatched bindings, and bindings to nonphysical cars.

## Movement flow

A shared authoritative traveler routine serves guests and staff.

For same-floor travel, it uses existing tile movement and clears vertical state.

For a cross-floor trip with an eligible bank it: selects a bank; walks to the landing using a same-floor path; submits a rider-owned request through the bank dispatcher; waits physically at the landing; stops tile walking while boarded; lets the elevator simulation own the ride; places the actor on the destination-floor landing only after the request completes alighting; clears vertical state; then resumes ordinary walking to the original destination.

If no valid bank of the required kind exists, the actor uses the existing stair path. If neither route exists, existing unreachable behavior is preserved rather than inventing a new failure policy.

Guests automatically use only Passenger banks. Non-guest staff automatically use only Service banks. If the required kind is unavailable, stairs are the fallback. Guest traffic never silently enters service cars and staff traffic never silently enters passenger cars.

## Pathfinding boundary

The general BFS remains static and tile-based; moving elevators are not BFS neighbors. Add a same-floor helper using normal passability but no vertical stair edges, preventing an elevator approach from routing through another floor.

Staff optimizer travel estimation may recognize a valid elevator route, but estimation is deterministic, non-mutating, and conservative. The optimizer never reserves or assigns a car; real elevator state changes occur only after native task assignment is committed.

## Dispatch integration

Before actor routing, port the verified bank-dispatch behavior and preserve exact kind isolation, integer-only stable selection, committed-work ETA projection, deterministic IDs, oldest-waiter selection, capacity, same-direction batching, opposite-direction deferral, directional onboard sweeps, request-ID tie breaks, and the direct-car API.

Add a physical-bank selector that filters candidates by exact kind and landing coordinate and then delegates car choice to the same dispatch scoring authority. Guest/staff code may not implement a second car-selection algorithm.

## Tick ordering

Use one explicit per-second authority order: existing work generation/assignment; staff traveler advancement; guest traveler advancement; one elevator tick per car; rider completion reconciliation/alighting placement; transient-state compaction.

No actor may observe elevator completion before the authoritative car tick that completes it. If compatibility evidence requires an adjacent ordering detail to remain as currently implemented, preserve it and encode the invariant in tests rather than changing timing casually.

## Save format — HHGS 13

The convergence branch emits HHGS 13. It preserves the established underlying HHGS 11 construction/building/elevator representation and FINAL-02 psychology/group data, then appends a required vertical-transport section for state older schemas cannot express: physical landing metadata keyed by elevator ID, rider ownership for live requests, and per-person vertical travel substate.

This avoids silently changing the old HHGS 11 field order.

Load remains compatible with supported v2-v12 saves. v2-v11 migrate through existing legacy paths with abstract elevators/no rider state. v12 restores FINAL-02 exactly with no fabricated physical transport state. v13 requires and validates the transport appendix. Malformed v13 transport state is rejected rather than heuristically repaired. Save/load continuation must remain byte-deterministic.

## Reachability and fire safety

Operational room reachability may be satisfied by the existing stair-connected route or by a valid passenger-elevator route with walkable same-floor approach/egress legs. Fire/egress validation is unchanged: elevator-only connectivity never satisfies fire egress. Accessibility policy beyond existing room-system flags is not expanded here.

## Determinism invariants

- One live rider-owned elevator request per actor and one owner per rider-owned request.
- Automatic bank/car selection has stable deterministic tie breaks and no RNG.
- Elevator capacity is never exceeded.
- A riding actor cannot tile-walk.
- An actor cannot change floors through an elevator before completed alighting.
- Abstract elevators are never used automatically.
- Guests use Passenger banks; staff use Service banks.
- Elevators never satisfy fire egress.
- Save/load preserves actor, request, car, queue, and task state exactly.

## TDD verification matrix

Required RED→GREEN coverage:

1. Guest walks to Passenger landing, waits, rides, alights, and reaches another-floor room.
2. Guest never automatically uses Service elevator.
3. Staff uses Service bank and completes a cross-floor task.
4. Staff never automatically uses Passenger elevator.
5. Stairs remain fallback when no physical bank is valid.
6. Abstract legacy elevators are ignored by automatic routing.
7. Multi-car bank uses existing deterministic dispatch authority.
8. Multiple riders queue safely and capacity is never exceeded.
9. Opposite-direction deferral remains intact.
10. Multiple banks select shortest approach with stable coordinate tie breaks.
11. Actor floor does not change before real alighting completion.
12. Room operational reachability recognizes Passenger elevator while fire egress rejects elevator-only egress.
13. HHGS 13 mid-wait save/load continuation is byte-deterministic.
14. HHGS 13 mid-ride save/load continuation is byte-deterministic.
15. v12 FINAL-02 saves load without fabricated physical elevator state.
16. Malformed/duplicate v13 rider ownership is rejected.
17. Same-seed mixed guest/staff multi-floor soak produces identical snapshots/save bytes and drains queues.
18. Relevant Linux and Windows integrated suites stay green.

## Branch and implementation strategy

`feature/elevator-actor-routing-final02` starts from FINAL-02 head `8e83358b01ad2befb7f1afaea2269c4e7b0c4c4f`.

Implementation order: selectively port verified elevator dispatch from the FINAL-01/elevator line; verify that port independently; add physical landing/rider contracts under TDD; integrate the shared traveler routine into staff and guest movement; add HHGS 13 persistence/migration; run deterministic/soak/save compatibility suites on Linux and Windows; open/update a draft stacked PR without merging.

FINAL-03 remains separate. The traveler seam is generic so workforce/department reconciliation can consume it later without creating another navigation authority.

## Completion criteria

Complete only when cross-floor guests/staff physically use the correct bank when available; stairs remain fallback; elevator dispatch remains the sole queue/car/capacity authority; operational reachability can use Passenger elevators without changing fire egress; HHGS 13 validates new state and safely loads v2-v12; deterministic save/load and mixed multi-floor soak pass; exact-head Linux and Windows CI are green; and the PR remains draft/unmerged unless explicitly instructed otherwise.
