# HMG-020 Construction and Room Topology Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace blueprint-owned room rectangles with authoritative construction geometry and topology-derived rooms while preserving the playable hotel, deterministic simulation, and existing saves.

**Architecture:** A portable C++20 `ConstructionWorld` owns floor tiles, canonical wall and door edges, placed logical objects, and detected room regions. `Simulation` remains the public authority for gameplay commands, room operations, routing, economy, and persistence; immutable snapshots feed the native UI and renderer.

**Tech Stack:** C++20, CMake, standard library only for the new module, existing native Win32 client and renderer adapters.

**Spec:** [`docs/superpowers/specs/2026-09-30-hmg-020-construction-room-topology-design.md`](../specs/2026-09-30-hmg-020-construction-room-topology-design.md)

## Global Constraints

- Use the repository's C++20 toolchain and keep `hh_game` portable across Windows and Linux.
- A floor tile represents 1 m × 1 m; walls and doors occupy canonical edges between grid cells.
- Keep simulation state authoritative; renderers and UI consume snapshots and issue commands only.
- Preserve deterministic guest-local random streams and exact integer-cent ledger values.
- Continue to load every HHGS save version v2 through v10; write the new construction representation as HHGS v11.
- Preserve current gameplay routes, room bookings, work tasks, room rates, and service registrations when construction changes are accepted or rejected.
- Do not make construction correctness depend on HMG-070 art assets or renderer asset IDs.
- This milestone is the first HMG-020 slice. Planning ghosts/jobs, utilities, full safety, environment, quality, and renovation remain required follow-on work in the full-game objective.

## Review Focus

- Opposite sides of one grid boundary must canonicalize to one edge, including the outer map perimeter; pin this in Task 1 with `canonical_edge_has_one_identity_from_either_cell` and `perimeter_edges_are_valid_and_outside_edges_rejected`.
- Region IDs must survive ties, multi-region edits, and unrelated-floor edits deterministically; pin this in Task 2 with `split_id_tie_uses_first_tile_order`, `merge_keeps_oldest_room_id`, and `edit_preserves_other_floor_room_ids`.
- Legacy room doors at corners and legacy solid wall tiles must migrate without changing room or route references; pin this in Task 4 with `v10_corner_door_migrates_deterministically` and `v10_wall_tiles_become_blocking_edges`.
- Editing a room used by an active guest, reservation, or service task must reject atomically when it would remove the room or its required route; pin this in Task 3 with `occupied_or_reserved_room_geometry_cannot_be_invalidated` and `rejected_build_preserves_economy_and_ids`.
- Rotated object footprints and interaction nodes must stay within the map, avoid collisions, and remain reachable; pin this in Tasks 1 and 3 with `rotated_footprint_and_interaction_nodes_are_canonical` and `unreachable_required_object_blocks_room_readiness`.

---

### Task 1: Define construction geometry and object types

**Files:**
- Create: `game/include/hh/game/ConstructionTypes.h`
- Create: `game/include/hh/game/ConstructionWorld.h`
- Create: `game/src/ConstructionWorld.cpp`
- Create: `game/tests/ConstructionWorldTests.cpp`
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- `ConstructionTypes.h` owns the shared `Position` and `TileKind` definitions moved from `Simulation.h`, imports `EntityId`/`RoomId` from `ServiceTypes.h`, and defines `GridSide { North, East, South, West }`, `EdgeAxis { Vertical, Horizontal }`, and `GridEdge { int floor; int x; int y; EdgeAxis axis; }`.
- `GridEdge edgeForSide(Position tile, GridSide side) noexcept` maps North/South to horizontal grid lines and East/West to vertical grid lines. For a horizontal segment, `x` is its column and `y` is its grid line; for a vertical segment, `x` is its grid line and `y` is its row. Coordinates may equal `width` or `height` only on the corresponding perimeter and may not lie beyond it.
- `ConstructionObjectKind` contains `SingleBed`, `DoubleBed`, `Toilet`, `Sink`, `Shower`, `Bath`, `Light`, `Desk`, `Chair`, `Nightstand`, and `Plant`.
- `ConstructionObjectDefinition` contains the kind, integer width/height, bed capacity, purchase cost in cents, logical room-capability flags, and interaction offsets. Definitions are static simulation data and contain no renderer asset IDs. Initial footprints are 1×2 for single beds, 2×2 for double beds, 1×1 for plumbing fixtures/lights/decor, and 1×1 for desks/chairs/nightstands/plants. Bed capacity is 1 for `SingleBed`, 2 for `DoubleBed`, and 0 for other kinds. Initial direct-placement cost is 15,000 cents per occupied footprint tile, matching the current furnished-room per-area rate. Functional objects expose one interaction node immediately outside their footprint; decor has no interaction node.
- `const ConstructionObjectDefinition& objectDefinition(ConstructionObjectKind) noexcept`, `std::vector<Position> footprintTiles(const ConstructionObject&)`, and `std::vector<Position> interactionNodes(const ConstructionObject&)` provide the canonical definition and oriented integer coordinates used by placement, diagnostics, and tests.
- `ConstructionObject` contains `EntityId id`, kind, anchor `Position`, and quarter-turn orientation. `ConstructionResult` contains `bool ok`, exact `std::string message`, and optional created `EntityId`.
- `ConstructionWorld` is a copyable value type constructed as `ConstructionWorld(int width, int height, int floors)` and exposes read-only dimensions, tile, edge, object, and region views. Mutation and region APIs are defined in later tasks.
- `ConstructionWorld::validEdge(GridEdge) const noexcept` accepts segments within the map boundary, including perimeter lines, and rejects all coordinates beyond that boundary.
- Add an `hh_construction_world_tests` executable and a `ConstructionWorld` CTest entry under `HH_GAME_BUILD_TESTS`.

- [x] **Step 1: Write failing type and edge tests**

Add these cases to `ConstructionWorldTests.cpp` and register the target in `game/CMakeLists.txt`:

```cpp
canonical_edge_has_one_identity_from_either_cell
perimeter_edges_are_valid_and_outside_edges_rejected
rotated_footprint_and_interaction_nodes_are_canonical
```

Assert that the East side of `{floor=0,x=3,y=4}` equals the West side of `{floor=0,x=4,y=4}`, that a North edge at row 0 and an East edge at column `width` are valid perimeter segments while out-of-range lines are rejected, and that a 1×2 bed rotated one quarter turn occupies 2×1 tiles with its interaction node still outside the footprint.

- [x] **Step 2: Register the test target and run it to confirm the API is missing**

Run: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`

Run: `cmake --build build --config Release --target hh_construction_world_tests`

Expected: compilation fails because the construction types and geometry helpers are not defined.

- [x] **Step 3: Add the shared types and the copyable `ConstructionWorld` shell**

Move `Position` and `TileKind` without changing enumerator order or values, include `ConstructionTypes.h` from `Simulation.h`, implement `edgeForSide`, and add bounds-aware edge identity helpers. Keep all object data renderer-independent.

- [x] **Step 4: Run the focused tests**

Run: `cmake --build build --config Release --target hh_construction_world_tests`

Run: `ctest --test-dir build -C Release -R '^ConstructionWorld$' --output-on-failure`

Expected: all three named geometry tests pass.

- [x] **Step 5: Commit**

```bash
git add game/include/hh/game/ConstructionTypes.h game/include/hh/game/ConstructionWorld.h game/src/ConstructionWorld.cpp game/tests/ConstructionWorldTests.cpp game/include/hh/game/Simulation.h game/CMakeLists.txt
git commit -m "feat: define construction geometry types"
```

### Task 2: Detect enclosed rooms and reconcile stable IDs

**Files:**
- Modify: `game/include/hh/game/ConstructionWorld.h`
- Modify: `game/src/ConstructionWorld.cpp`
- Modify: `game/tests/ConstructionWorldTests.cpp`

**Interfaces:**
- `ConstructionRegion` contains `RoomId id`, sorted `std::vector<Position> tiles`, sorted boundary `GridEdge` values, and sorted `GridEdge` door entrances.
- `ConstructionWorld::setTile(Position, TileKind) -> ConstructionResult` changes one floor tile only when in bounds and unoccupied by a conflicting structure/object.
- `ConstructionWorld::setWall(GridEdge, bool enabled) -> ConstructionResult` and `setDoor(GridEdge, bool enabled) -> ConstructionResult` mutate canonical edges. Enabling a door changes an existing solid wall segment to a traversable door segment; disabling it restores a solid wall. `removeEdge(GridEdge) -> ConstructionResult` removes either kind of segment.
- `ConstructionWorld::placeObject(ConstructionObject) -> ConstructionResult` and `removeObject(EntityId) -> ConstructionResult` validate and mutate rotated footprints atomically.
- `ConstructionWorld::rebuildRegions(std::function<RoomId()> allocateRoomId)` updates only affected floors. On split, the old ID follows the largest overlap; ties use the smallest first tile in `(floor,y,x)` order. On merge, the smallest contributing ID survives.
- `ConstructionWorld::regions() const -> const std::vector<ConstructionRegion>&` returns stable, sorted region data for the simulation adapter.

- [x] **Step 1: Write failing room-region tests**

Add these cases to `ConstructionWorldTests.cpp`:

```cpp
closed_boundary_detects_one_room_and_door_remains_an_entrance
open_boundary_does_not_report_an_enclosed_room
split_keeps_old_id_on_larger_region
split_id_tie_uses_first_tile_order
merge_keeps_oldest_room_id
edit_preserves_other_floor_room_ids
object_footprints_validate_and_reject_atomically
```

Assert exact sorted tile sets, selected IDs, and door-edge membership for each fixture. Flood fill uses underlying interior floor geometry independent of object occupancy; navigation separately treats placed footprints as blocked cells.

- [x] **Step 2: Run `ConstructionWorld` tests and confirm the region behavior is missing**

Run: `cmake --build build --config Release --target hh_construction_world_tests`

Expected: compilation fails because the region mutation and query APIs are not implemented yet. Do not run CTest until the target builds, since an existing executable could otherwise report a stale pass.

- [x] **Step 3: Implement affected-floor flood fill and ID reconciliation**

Traverse interior floor geometry in `(floor,y,x)` order independent of furniture occupancy; block region connectivity across wall and door edges; treat door/window edges as boundary entrances, and reject unclosed exterior regions. For each old region, propose its ID to the new region with the greatest tile overlap, breaking ties by the new region's first tile. A merged new region keeps the smallest proposed ID; regions with no proposal receive newly allocated IDs. Recompute only floors touched by the edit.

- [x] **Step 4: Run the focused topology tests**

Run: `cmake --build build --config Release --target hh_construction_world_tests`

Run: `ctest --test-dir build -C Release -R '^ConstructionWorld$' --output-on-failure`

Expected: all ten geometry, topology, and object-footprint cases pass with stable ordering.

- [x] **Step 5: Commit**

```bash
git add game/include/hh/game/ConstructionWorld.h game/src/ConstructionWorld.cpp game/tests/ConstructionWorldTests.cpp
git commit -m "feat: detect rooms from construction boundaries"
```

### Task 3: Integrate construction commands, room state, and routing

**Files:**
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/src/Simulation.cpp`
- Modify: `game/tests/SimulationTests.cpp`
- Create: `game/tests/SimulationConstructionTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Add `Simulation::setConstructionWall(GridEdge, bool enabled)`, `Simulation::setConstructionDoor(GridEdge, bool enabled)`, `Simulation::removeConstructionEdge(GridEdge)`, `Simulation::placeConstructionObject(ConstructionObjectKind, Position, int quarterTurns = 0)`, and `Simulation::removeConstructionObject(EntityId)`; return the existing `CommandResult` with exact player-facing diagnostics. Use the exact diagnostics 'Place a wall before adding a door', 'Construction edge is outside the property', 'Object footprint overlaps existing construction', and 'Required object is unreachable' for those respective cases.
- `Simulation::Impl` owns one `ConstructionWorld`; replace its duplicate authoritative tile map with world reads/writes. Keep service, guest, staff, economy, and reservation records in their current owners.
- Add `Simulation::Impl::reconcileConstructionRooms()` to preserve service/runtime room fields by stable `RoomId` while updating footprint, area, bed capacity, bathroom capability, primary door, and diagnostics from `ConstructionRegion`.
- Keep `RoomBlueprint.beds` as the requested count of single-bed objects. A detected room's `beds` capacity is the sum of its placed objects' definition capacities, and `area` is its number of topology tiles.
- `Simulation::Impl::path` must reject neighbor transitions across wall edges and permit transitions through door edges, while vertical stair transitions retain current behavior.
- `buildFurnishedRoom(const RoomBlueprint&)` remains source-compatible and emits room perimeter edges, floor tiles, a door edge, one single-bed object per requested bed, one toilet/sink/shower set per requested bath, and one controllable light in one candidate-world transaction. Fit optional decor only where it does not overlap required objects or interaction nodes. Its aggregate charge remains `width * height * 15000` cents for save/game balance compatibility; direct objects charge their `ConstructionObjectDefinition::purchaseCostCents`. Rooms too small to fit their requested contents return 'Room blueprint cannot fit its contents' without mutation.
- `buildTile(Position, TileKind)` remains the tile API for floors, lobby, entrance, reception, closets, stairs, bathroom/staff zones, and removal. New wall/door placement uses edge commands; legacy `TileKind::Wall`/`Door` calls return 'Walls and doors are placed on edges' so callers cannot create tile-wide barriers in the new model.
- A construction command validates on a candidate `ConstructionWorld` and candidate next-ID value. Commit geometry, room reconciliation, service registrations, cash, and construction totals only after all geometry, occupancy, reservation, task, and path constraints pass.

- [x] **Step 1: Write failing simulation integration tests**

Add these cases to `SimulationConstructionTests.cpp`:

```cpp
wall_edge_blocks_shared_path_and_door_edge_restores_it
blueprint_creates_same_guest_ready_profile_and_capacity
rejected_build_preserves_economy_and_ids
occupied_or_reserved_room_geometry_cannot_be_invalidated
unreachable_required_object_blocks_room_readiness
```

Assert exact cash/ledger and `nextId` stability after rejected commands (read `nextId` from the serialized header), route outcomes before/after an edge change, guest-ready fields and capacity for a 6×6 blueprint, and the exact diagnostic 'Required object is unreachable' when a required interaction node has no path. Update existing room fixtures in `SimulationTests.cpp` from 3×3/4×4 to 5×5 or larger where they need a guest-ready blueprint with distinct required objects.

- [x] **Step 2: Register the test target and run it to confirm integration is absent**

Add `hh_simulation_construction_tests` and CTest name `SimulationConstruction` under `HH_GAME_BUILD_TESTS`.

Run: `cmake --build build --config Release --target hh_simulation_construction_tests`

Run: `ctest --test-dir build -C Release -R '^SimulationConstruction$' --output-on-failure`

Expected: the target fails to compile because the new simulation commands and world integration are not present.

- [x] **Step 3: Route tile, wall, door, and object mutations through candidate-world transactions**

Implement the five public commands, object-footprint/interaction validation, exact `ConstructionObjectDefinition` costs, and candidate-world commit rules. Update `buildTile` and `buildFurnishedRoom` to use the same authority while preserving their signatures and current aggregate room cost.

- [x] **Step 4: Reconcile rooms and make pathfinding use canonical edge transitions**

Build `RoomView` from detected regions, preserving existing rate, reservation, cleanliness, condition, and operational status for retained IDs. Update `passable`/`path` and `refreshReachability` so wall and door edges govern adjacency consistently.

- [x] **Step 5: Run simulation construction and existing gameplay tests**

Run: `cmake --build build --config Release --target hh_simulation_construction_tests hh_game_tests`

Run: `ctest --test-dir build -C Release -R '^(SimulationConstruction|hh_game_tests)$' --output-on-failure`

Expected: all five new integration cases and the existing game simulation suite pass.

- [x] **Step 6: Commit**

```bash
git add game/include/hh/game/Simulation.h game/src/Simulation.cpp game/tests/SimulationTests.cpp game/tests/SimulationConstructionTests.cpp game/CMakeLists.txt
git commit -m "feat: integrate topology-backed construction commands"
```

### Task 4: Persist v11 construction state and migrate v2-v10 saves

**Files:**
- Modify: `game/src/Simulation.cpp`
- Modify: `game/tests/SimulationSaveMigrationTests.cpp`

**Interfaces:**
- The writer emits `HHGS 11` and retains the existing base, `FINAL04`, and `GUEST10` records unchanged.
- Append `CONSTRUCTION11 1 <byteCount>\n<payload>\n`; the payload stores canonical edges, sorted object placements, and room-ID reconciliation metadata in stable order.
- The v11 loader parses `GUEST10` as it does for v10, then parses exactly one construction section and rejects truncation, duplicate IDs, out-of-bounds edges/footprints, invalid kinds/orientations, and trailing bytes.
- The v2-v10 loader preserves legacy room IDs/references and migrates room rectangles, `TileKind::Wall`, and `TileKind::Door` to edge geometry. Corner doors use the approved deterministic side order from the design spec. Reject construction payloads larger than 32 MiB before allocating vectors.

- [x] **Step 1: Add failing HHGS v11 persistence and migration cases**

Extend `SimulationSaveMigrationTests.cpp` with:

```cpp
v10_corner_door_migrates_deterministically
v10_wall_tiles_become_blocking_edges
v11_round_trip_preserves_edges_objects_and_room_ids
v11_rejects_duplicate_object_ids_and_invalid_footprints
migrated_room_references_and_guest_continuation_remain_valid
```

Assert preserved room/reservation/task IDs, object/edge equality, deterministic repeated load/save continuation, and rejection of malformed construction payloads.

- [x] **Step 2: Run save migration tests and confirm the new format is unsupported**

Run: `cmake --build build --config Release --target hh_simulation_save_migration_tests`

Run: `ctest --test-dir build -C Release -R '^GuestSaveMigration$' --output-on-failure`

Expected: new v11 and construction migration cases fail against the v10-only writer/loader.

- [x] **Step 3: Add the v11 construction extension and legacy geometry migration**

Keep the v10 guest section byte-compatible; parse it for versions 10 and 11, then parse the length-delimited construction extension for v11. Convert legacy walls/doors and room furnishings deterministically without rewriting existing room references or ledger values.

- [x] **Step 4: Run save migration tests**

Run: `cmake --build build --config Release --target hh_simulation_save_migration_tests`

Run: `ctest --test-dir build -C Release -R '^GuestSaveMigration$' --output-on-failure`

Expected: all existing v2-v10 migrations and five new v11 construction cases pass.

- [x] **Step 5: Commit**

```bash
git add game/src/Simulation.cpp game/tests/SimulationSaveMigrationTests.cpp
git commit -m "feat: persist construction geometry in saves"
```

### Task 5: Publish construction snapshots in the Build and Rooms UI

**Files:**
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/src/Simulation.cpp`
- Modify: `game/app/Client.h`
- Modify: `game/app/GameMain.cpp`
- Modify: `game/app/Panels.cpp`
- Modify: `game/app/WorldView.h`
- Modify: `game/app/WorldView.cpp`
- Modify: `game/app/WorldViewTests.cpp`

**Interfaces:**
- Add sorted `constructionWalls`, `constructionDoors`, and `constructionObjects` arrays to `SimulationView`. Each object view exposes only simulation-owned ID, kind, position, orientation, and footprint.
- Add `area`, sorted `tiles`, capacity, primary door edge, and stable diagnostics to `RoomView`; keep `x/y/width/height` as the bounding box compatibility fields used for camera targeting and room lists. GameMain room selection checks membership in `tiles`, not the bounding rectangle, so void cells inside an irregular room's bounding box do not select it.
- `WorldView::worldScene` renders wall/door edges and the explicit object list. Remove bed/bath/desk/plant placement inferred from room rectangles; keep only non-gameplay garden scenery procedural.
- The Build panel supports floor tile, wall edge, door edge, and supported furnishing tools. A visible edge-side selector cycles North/East/South/West; an object selector cycles definitions, and a rotation control advances quarter turns. Pointer previews use the same `Simulation` command path on a copied simulation as final placement.
- The Rooms panel displays area, capacity, reachability, and the stable list of missing/unreachable requirements from `RoomView`.

- [x] **Step 1: Add failing snapshot and renderer tests**

Add these cases to `WorldViewTests.cpp`:

```cpp
construction_edges_render_from_snapshot_coordinates
furnishing_placements_render_from_object_views
room_bounds_are_derived_from_topology_tiles
```

Assert explicit edge transforms, object-kind presentation, and bounding-box values computed from non-rectangular room tiles.

- [x] **Step 2: Implement construction and room diagnostic snapshot fields**

Populate stable sorted arrays from `ConstructionWorld` and detected regions. Preserve every existing `SimulationView` field consumed by clients.

- [x] **Step 3: Render explicit edge and object data**

Update `WorldView::worldScene` and its tests. Keep selection, status/cleanliness/condition overlays, and camera bounds working for irregular room footprints.

- [x] **Step 4: Add edge/object tools and room diagnostics to the native client**

Update the `Tool` enum, preview, click dispatch, Build controls, and Rooms inspector. Wall side selection must be visible and identical in preview and commit.

- [x] **Step 5: Run world-view tests and build the Windows client**

Run: `cmake --build build --config Release --target hh_world_view_tests hotel_haven`

Run: `ctest --test-dir build -C Release -R '^hh_world_view_tests$' --output-on-failure`

Expected: all three rendering cases pass and `hotel_haven` builds with the new Build and Rooms controls.

Local result: all world-view tests passed, and both modified client source files compiled. The local final link is blocked because this Zig toolchain cannot locate the `d3dcompiler` import library. Integrated Game run #1236 passed the complete client build, all CTest tests, and Windows client smoke on GitHub Actions.

- [x] **Step 6: Commit**

```bash
git add game/include/hh/game/Simulation.h game/src/Simulation.cpp game/app/Client.h game/app/GameMain.cpp game/app/Panels.cpp game/app/WorldView.h game/app/WorldView.cpp game/app/WorldViewTests.cpp
git commit -m "feat: expose object construction in the hotel editor"
```

### Task 6: Close the milestone against its acceptance boundary

**Files:**
- Modify: `docs/IMPLEMENTATION_STATUS.md`
- Modify: `README.md`
- Modify: `docs/superpowers/plans/2026-09-30-hmg-020-construction-room-topology.md`

**Interfaces:**
- Documentation records the new geometry/topology slice as integrated and explicitly lists construction jobs, full utility/safety, quality/environment, and renovation as remaining HMG-020 work.
- The integrated game remains the verification authority for simulation, persistence, renderer, and native client behavior.

- [x] **Step 1: Run the complete game build and CTest suite**

Run: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`

Run: `cmake --build build --config Release --parallel 4`

Run: `ctest --test-dir build -C Release --output-on-failure`

Expected: every configured test passes on Ubuntu and Windows CI.

Verified in Integrated Game run #1236: full Release builds and all configured CTest tests passed on Ubuntu and Windows. The local Windows Zig toolchain ran 22 of 23 tests; `hh_renderer_tests` and the final client link require the unavailable `d3dcompiler` import library.

- [x] **Step 2: Verify Windows smoke artifacts and package integrity in Integrated Game CI**

Expected: Windows client smoke test exits 0; hotel, Build, and Rooms screenshots are present; the release package integrity verifier succeeds.

Run #1236's Windows client smoke test exited 0. The `Hotel-Haven-Visual-QA` artifact contains `smoke-guide.bmp`, `smoke-hotel.bmp`, `smoke-build.bmp`, `smoke-rooms.bmp`, and `smoke-guests.bmp`. The `Hotel-Haven-Windows` ZIP passed the package integrity verifier.

- [x] **Step 3: Update the implementation ledger with only verified claims**

List which HMG-020 portions are implemented and retain every deferred HMG-020 section in the full-spec gaps.

- [x] **Step 4: Commit**

```bash
git add docs/IMPLEMENTATION_STATUS.md README.md docs/superpowers/plans/2026-09-30-hmg-020-construction-room-topology.md
git commit -m "docs: record construction topology milestone"
```

- [x] **Step 5: Prepare the GitHub draft PR after local verification**

HMG-010 PR #77 remains open, so HMG-020 draft [PR #78](https://github.com/danielmartinez-maker/Hotel-Haven/pull/78) targets `feature/guest-lifecycle-psychology-design`. It is attached to this Codex task and remains unmerged.
