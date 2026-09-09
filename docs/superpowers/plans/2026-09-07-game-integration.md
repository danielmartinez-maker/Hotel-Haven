# Hotel Haven Game Integration Implementation Plan

> **For agentic workers:** Use superpowers:subagent-driven-development. Execute continuously under the user's existing implementation authorization.

**Goal:** Turn the existing native renderer and pipeline into an integrated playable hotel management game, with auditable coverage of the larger HMG design.

**Architecture:** A portable C++20 authoritative simulation owns construction, guest lifecycle, staff dispatch, inventory, economics, and persistence. A Windows client converts snapshots to renderer DTOs and presents native management controls. A headless executable runs reproducible campaigns and integration tests.

**Tech Stack:** C++20, CMake, Win32, Direct3D 11; existing hh_assets JSON parser.

**Spec:** `docs/specifications/HMG_000_FOUNDATION_V0_1.md` and HMG-010 through HMG-050 in the same directory.

## Global Constraints
- Windows 11 x64; mouse + keyboard; single player.
- Authoritative money in integer smallest currency units; persistent IDs uint64_t.
- Deterministic simulation and RNG; save/load includes all authoritative state.
- Speeds 0, 1, 3, 8, 20; 1 real second = 60 simulation seconds at normal speed.
- Presentation does not own simulation rules.
- Room types, prices, roles, archetypes, and service timings reside in external data.
- Record additional balance decisions explicitly; never label unimplemented HMG sections complete.

### Task 1: Authoritative integrated simulation
**Files:** `game/include/hh/game/Simulation.h`, `game/src/*.cpp`, `game/data/*`, `game/tests/SimulationTests.cpp`, `game/CMakeLists.txt`.
**Interfaces:** `hh::game::Simulation` owns state; publish concrete header before client work. Provide construction commands, staff hiring, pricing, inventory orders, step(seconds), inspectable state, save/load.
- [ ] Write behavior tests for invalid construction, reachable routes, reservations and room turnover, resource blocking, money accounting, save continuation, determinism, and layout penalties.
- [ ] Demonstrate tests fail before implementation.
- [ ] Implement bounded deterministic navigation, room lifecycle, actual staff travel/work, guest stays/needs/reviews, hourly bookings, daily costs, supplier delivery, maintenance, progression, and serialization.
- [ ] Run tests and a multi-day campaign, fix failures, commit only task files.

### Task 2: Native game client and presentation
**Files:** `game/app/*`, root `CMakeLists.txt`, root `README.md`, `.github/workflows/game.yml`.
**Consumes:** Published Task 1 header, existing `RenderScene`, `OrthoCamera`, `D3D11Renderer`.
**Produces:** `hotel_haven.exe` and `hotel_haven_headless`.
- [ ] Add root build integrating pipeline and Windows-only renderer, portable simulation and headless execution.
- [ ] Add construction/management panels, time controls, room inspector, employees, guests, supplies, finance, reviews, tutorial, and save/load controls.
- [ ] Draw a furnished multistory hotel and moving guests/staff from authoritative state; selection, camera rotation, floor switching and cutaway.
- [ ] Verify Linux build/tests; verify Windows client through CI build and smoke test if accessible.

### Task 3: Review, repair, packaging and honest coverage
**Files:** `docs/IMPLEMENTATION_STATUS.md`, targeted fixes from review, release configuration.
- [ ] Independent review for spec compliance and code correctness.
- [ ] Repair load-bearing findings and rerun affected tests.
- [ ] Record implemented, simplified, and absent systems precisely.
- [ ] Commit and push isolated feature branch; create draft PR with test evidence and limitations.

## Rulings
- The prior simulation commit mentioned in conversation is not reachable from the three fetched remote branches. New simulation lives under `game/` to avoid pretending it has been recovered or overwriting it if later supplied.
- The authoritative HMG documents define a much larger product than the first playable. Implementation status must distinguish these; a passing first-playable gate is not full HMG compliance.
- Existing renderer uses procedural box geometry. Composed furniture/people meshes are a reproducible native art baseline; they are not claimed as completed HMG-070 art production deliverables.
