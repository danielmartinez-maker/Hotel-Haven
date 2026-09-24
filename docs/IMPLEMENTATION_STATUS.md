# Implementation status

This file distinguishes the integrated gameplay build from the full HMG product specification. It is not a declaration that the full game is complete.

## Integrated in this branch

- Root CMake build joining simulation, renderer and existing content pipeline.
- Native Windows client with validated construction previews, room inspection, exact-cent rates, staff hiring/shifts/wages, guest travel/queue diagnostics, supply ordering, task diagnostics, finances, reviews and introductory guide.
- Three-floor starter property, room blueprints, corridors, walls, doors, an explicit lobby, reception, bathrooms, staff rooms, supply closets and stairs.
- Hybrid asset-backed world scene with cooked meshes for stairs, doors, beds, room furniture, bathroom fixtures, reception, service storage, plants and people; 49 startup bindings cover all 30 guest variants plus receptionist, housekeeper and maintenance variants, with procedural fallbacks, floor selection, cutaway and diagnostic colors.
- Deterministic one-second simulation with price/reputation demand, fixed arrival and departure windows, physical check-in and checkout, day/night room stays, turnover resources, part-consuming maintenance failures, task-weighted fatigue, payroll, reviews and cash flow.
- FINAL-05 food-and-beverage production with recipe inventory, constrained stations, restaurant/bar/breakfast venue lifecycles, exact-once room-service handoff to FINAL-04, event quoting/reservations/operations, and gym/spa/pool reservations with cleanliness, condition, outcomes and exact-cent revenue.
- FINAL-04 housekeeping, engineering and logistics are reconciled with the physical room-work scheduler: room cleans/repairs create mirrored service jobs, canonical supplies are claimed once at physical pickup, service progress is labor-gated, room sellability waits for both physical and service completion, and integrated room wear is the sole maintenance-failure source. The legacy inventory payload remains only for save-format compatibility and is not exposed as live stock authority.
- Atomic, cash-limited construction and state-aware room/infrastructure commands that preserve reservations, repairs, service tasks and routing invariants.
- Integer smallest-currency authority for rates, wages, utilities and all ledger totals. Save format v9 round-trips exact deterministic state, validates the complete entity-reference graph, persists FINAL-04/FINAL-05 state in length-prefixed sections and migrates v2-v8 saves.
- Long-running campaigns keep departed guests and completed reservations out of the per-second hot path, retain the newest 128 completed tasks for diagnostics, and preserve complete reservation/review history in saves. A 365-day, six-room Release reference run completed in 3.80 seconds with 941 stays and zero blocked tasks in the current Linux container.
- HMG-070 production-content foundation with a deterministic 500 gameplay-asset catalog, ten 50-asset batches, generated GLB/sidecar output, cooked `.hasset` packaging, semantic/geometry/preview release gates and a full 500-asset renderer audit. The shipping client selectively decodes only its current world-view dependency set rather than all 500 meshes at startup.\n- Portable campaign runner, behavioral/save/world tests, software-rendering Windows smoke-test path and Windows packaging workflow.

The HMG-000 first-playable layout criterion has an automated 20-day, same-seed comparison. The two hotels have identical room capacity, prices, constructed corridor length, staffing and inventory; only reception and closet placement differ. The test requires the poor layout to produce more guest travel, more queue time, lower satisfaction, fewer completed stays and lower operating profit.

Local verification covers the portable Linux build. The native Direct3D client is configured for a Windows CI compile, forced-WARP smoke launch, save round-trip and two screenshot captures, but that Windows job must run before claiming a verified Windows binary.

## Full-spec gaps that remain material

The supplied HMG-000 through HMG-050 documents describe a substantially larger simulation than these integrated systems. In particular, this branch does not yet provide construction-authored placement and full UI control for the new FINAL-05 venues/amenities, elevator-car dispatch, fire/security systems, all 13 guest archetypes with their complete trait/memory/group model and discretionary service goals, departmental managers and forecasts, financing/covenants/receivership, competitor hotels and segmented demand, full utility-network simulation, all global overlays, or complete runtime consumption of the production asset catalog such as skeletal animation playback and object-level placement across every asset family.

Room blueprints and the current hybrid procedural/asset-backed furnishings are a construction interface for this implementation; they do not constitute the complete object-level construction editor, physical construction-labor/material chain or full gameplay placement coverage for the existing generated asset library. Staffing does not yet include applicants, breaks, morale, training, managers or absence. The tutorial is an in-game guide rather than a fully scripted campaign with contextual objectives.

No 500-room / 1,000-guest / 300-employee performance claim is made without a corresponding benchmark.
