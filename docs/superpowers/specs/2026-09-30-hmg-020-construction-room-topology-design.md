# HMG-020 Construction and Room Topology Design

## Status and scope

This design defines the first implementation slice of the authoritative construction subsystem in [`HMG_020_CONSTRUCTION_AND_ROOM_RULES_V0_1.md`](../../specifications/HMG_020_CONSTRUCTION_AND_ROOM_RULES_V0_1.md). It advances the full Hotel Haven goal without claiming to implement all of HMG-020.

The first slice replaces rectangle-owned rooms with construction geometry and topology-derived rooms. It adds object placement, enclosed-room detection, deterministic room identity across splits and merges, and exact placement/room diagnostics. Later slices will add the planning and construction-job lifecycle, complete room validity, utilities and safety, room quality and environmental simulation, and renovation.

## Product intent

The player should be able to shape a hotel from floor tiles, wall segments, doors, and furnishing objects. Rooms should emerge from that layout, and a selected room should explain what is missing or unreachable. The existing six-room tutorial and guest-management loop must continue to work while players gain more direct control of the hotel layout.

The authoritative HMG-020 specification remains the product contract. The project currently has atomic tile placement and a rectangular furnished-room blueprint, but room identity and footprint are stored directly on that blueprint-created room. The UI presents construction as room and tile tools, the simulation owns routing and cash, and the renderer consumes snapshots.

## Approaches considered

1. **Incremental construction authority (recommended).** Add a focused construction-world module and route existing simulation commands through it. Keep `Simulation` as the public gameplay facade and retain room blueprints as convenience commands. This stages the migration while establishing the geometry and topology that later HMG-020 systems need.
2. **Replace construction in one pass.** Rewrite the simulation, UI, saves, and renderer around object-level construction, construction jobs, utilities, and environmental quality together. This reaches farther in one change but couples many unfinished systems and makes migration and regression review difficult.
3. **Extend only the current blueprint UI.** Add more presets and controls while keeping rooms as stored rectangles. This is faster initially but leaves the authoritative room and construction model short of HMG-020.

The first approach is selected. It creates a durable path to the full subsystem while keeping the existing gameplay integration usable at every milestone.

## Architecture

### Ownership

- Add a `ConstructionWorld` module under `game/include/hh/game` and `game/src`. It owns floor tiles, canonical wall-edge geometry, door-edge occupancy, placed logical objects, and the detected room regions. The model can represent future window boundaries, but window authoring is deferred.
- `Simulation::Impl` owns `ConstructionWorld`. `Simulation` public commands validate gameplay-sensitive constraints and delegate construction mutations to it; existing guest, staff, economy, and service owners continue to consume the resulting room and route views.
- Keep `SimulationView` as the immutable boundary consumed by the native UI and renderer. Add construction-object and room-diagnostic data to that snapshot; the renderer may display the data but never infer construction or room validity.
- Keep `buildTile` for corridors, lobby, stairs, and other existing tile-oriented commands. Add explicit wall-edge and object-placement commands for the new editor. Keep `buildFurnishedRoom` as a convenience command that emits the same geometry and logical furniture through the new construction authority.

### Geometry and room detection

- A floor tile is the 1 m × 1 m walkable-space unit. Walls and doors occupy a canonical edge between adjacent grid cells, so an edge has exactly one identity regardless of which side addresses it. The topology can treat a future window edge as a closed room boundary, but this slice does not expose window placement.
- Placed logical objects use integer tile footprints and declared interaction nodes. The initial definitions cover the existing baseline guest-room bed and bathroom capabilities; visual assets remain presentation data and can arrive through the later HMG-070 asset pipeline.
- Room regions are connected sets of interior floor tiles. A region is enclosed when every outward boundary edge is a structural wall, closed boundary, door, or window, consistent with HMG-020. A door fills a boundary edge without breaking enclosure. The first editor authors structural walls and doors; window boundaries are reserved for a later slice.
- Navigation continues to use one node per walkable tile. A path transition between adjacent nodes is legal only when the shared edge is not blocked by a wall; a door edge remains traversable under the current access rules. This keeps room enclosure and guest/staff routing on the same authoritative geometry.
- A topology edit invalidates detection on its affected floor. The first implementation recomputes that floor and reconciles its regions; it does not scan unrelated floors. This preserves a clear upgrade path to finer-grained incremental updates if measured map sizes require them.
- On a split, the previous `RoomID` remains with the resulting region that contains the most of the old region's tiles; a tie goes to the region whose first tile in stable `(floor, y, x)` order is smallest. Other regions receive new IDs.
- On a merge, the region keeps the oldest contributing `RoomID`; because IDs are allocated monotonically, the smallest ID is the deterministic oldest ID. New regions receive IDs from the existing monotonic entity-ID source. Save migration must preserve existing room IDs and references.

### Commands, validation, and diagnostics

- Placement is atomic. A rejected command leaves geometry, room identities, money, reservations, service tasks, and routing unchanged.
- Validate map/buildable bounds, footprint overlap, wall-edge uniqueness, door-to-wall adjacency, existing structural constraints, and current occupancy/routing invariants before committing a placement.
- Return one exact, player-readable reason for each rejected command. A room inspection exposes its area, capacity, connected door/path state, and a stable list of currently known missing or unreachable baseline requirements.
- The first slice keeps the current immediate-completion construction behavior. It does not yet expose planning ghosts or claim the HMG-020 construction-job state machine; that lifecycle is a follow-on slice. Existing playable rooms remain functional after a successful atomic placement.
- A room is guest-ready only when it satisfies the current baseline room contract and has a reachable guest door. The richer utility, egress, accessibility, quality, cleanliness, condition, and environment rules remain explicit follow-on work rather than hidden assumptions.

### UI and data flow

The Build panel presents floor-tile, wall-edge, door, and supported furnishing tools. Input is translated into `Simulation` commands; the simulation returns success or an exact diagnostic. The Rooms panel reads detected regions and their diagnostics from `SimulationView`. World rendering consumes explicit edge and object presentation records instead of inferring room furniture from a rectangular room footprint. The client and renderer contain no authoritative placement, enclosure, or room-validity logic.

The data path is:

```text
Build input -> Simulation command -> ConstructionWorld validation/mutation
            -> affected-floor room detection -> SimulationView snapshot
            -> Build/Rooms UI and renderer presentation
```

The existing room blueprint action becomes a convenience macro through this path, so the tutorial can keep its starter hotel while regular play moves toward object-level construction.

## Save compatibility and determinism

- Preserve loading for every existing HHGS save version from v2 through v10.
- The first persisted construction-object representation advances the writer to v11. The v10 migration converts each stored room rectangle and its legacy bed/bath counts into deterministic boundary geometry and logical baseline objects while preserving room IDs, reservations, reviews, tasks, economy totals, and guest state.
- Legacy perimeter wall tiles become the canonical boundary edges of their stored room. A legacy door tile is translated to the perimeter edge identified by the room bounds and passable adjacent tiles; ambiguous corner cases use a fixed north/east/south/west side order. Legacy wall tiles outside rooms become blocking edges against adjacent walkable cells. The compatibility tile projection keeps existing floor, corridor, and door navigation coordinates stable.
- New save records serialize canonical edge keys and object placements in stable sorted order. Room regions are derived from saved geometry on load; the save retains identity reconciliation metadata needed to preserve the same room IDs.
- Save/load continuation remains deterministic for a migrated save, and adding construction state must not change guest decision RNG ownership or existing exact-cent ledger values.

## Scope boundaries

Included in this slice:

- Authoritative floor-tile, wall-edge, door-edge, and logical furnishing placement; window placement remains later work.
- Deterministic enclosed-room detection and RoomID reconciliation on topology edits.
- Atomic build validation, room readiness for the current baseline, and player-facing diagnostics.
- Snapshot and Build/Rooms UI integration, including the existing blueprint convenience path.
- Save migration and persistence for the new construction geometry and logical objects.

Deferred to subsequent HMG-020 slices:

- Free ghost planning, commit/reserve-funds workflow, construction task states, materials, labor, demolition, and renovation holds.
- Full room-type definitions and all HMG-020 validity, occupancy, nested-bathroom, and status rules.
- Utility networks, climate, accessibility, elevators, fire/egress jurisdiction rules, and construction safety constraints beyond current invariants.
- Room-quality components, decor/style coherence, views, noise propagation, per-tile dirt, object condition, design aging, and their complete diagnostics.
- Authored commercial art and the full HMG-070 through HMG-079 content catalog.

These items remain part of the full Hotel Haven objective; the boundaries above only sequence implementation.

## Acceptance criteria

1. The tutorial hotel is created through the construction authority and retains the same guest-ready rooms, IDs, routes, capacities, rates, and startup behavior.
2. Players can place and remove supported structural edges and logical objects through simulation commands; overlap, bounds, occupied-space, and invalid wall/door operations are rejected atomically with exact messages.
3. Enclosing floor regions become rooms without a room-blueprint rectangle. Removing one boundary can split a room; adding one can merge regions; IDs follow the specified deterministic rules.
4. A room inspection reports area, capacity, reachability, and missing or inaccessible baseline requirements from authoritative simulation data.
5. Existing HHGS v2-v10 saves load with their references intact; v11 saves round-trip construction geometry, objects, room identities, and gameplay state deterministically.
6. Guest, staff, service, economy, and renderer systems continue to read the same authoritative simulation snapshot boundaries.

## Verification approach

The implementation plan will define focused behavior checks for placement rejection atomicity, enclosure creation, split/merge identity, deterministic save continuation, v10 migration, tutorial equivalence, and client presentation. Verification must cover the complete native game integration on supported Windows and Linux CI paths before this milestone is reported complete.

## Review notes

- The first slice is a foundation, not a claim that all HMG-020 requirements are complete.
- The public simulation facade protects later subsystems from depending on the internal construction representation.
- Logical objects and art assets remain separate so construction correctness does not depend on renderer or content-pipeline changes.
- Legacy saves and tutorial construction use the same new geometry path after migration, avoiding a permanent parallel room model.
