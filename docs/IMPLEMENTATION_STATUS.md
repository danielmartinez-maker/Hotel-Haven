# Implementation status

This file distinguishes the integrated gameplay build from the full HMG product specification. It is not a declaration that the full game is complete.

## Integrated in this branch

- Root CMake build joining simulation, renderer and existing content pipeline.
- Native Windows client with validated construction previews, room inspection, exact-cent rates, staff hiring/shifts/wages, guest archetype/trait/budget/travel/queue diagnostics, supply ordering, task diagnostics, finances, reviews and introductory guide.
- Three-floor starter property, room blueprints, corridors, walls, doors, an explicit lobby, reception, bathrooms, staff rooms, supply closets and stairs.
- Furnished procedural scene with beds, desks, bath fixtures, plants, characters, floor selection, cutaway and diagnostic colors.
- Deterministic one-second simulation with price/reputation demand, fixed arrival and departure windows, physical check-in and checkout, day/night room stays, turnover resources, part-consuming maintenance failures, task-weighted fatigue, payroll, reviews and cash flow.
- FINAL-05 food-and-beverage production with recipe inventory, constrained stations, restaurant/bar/breakfast venue lifecycles, exact-once room-service handoff to FINAL-04, event quoting/reservations/operations, and gym/spa/pool reservations with cleanliness, condition, outcomes and exact-cent revenue.
- FINAL-04 housekeeping, engineering and logistics are reconciled with the physical room-work scheduler: room cleans/repairs create mirrored service jobs, canonical supplies are claimed once at physical pickup, service progress is labor-gated, room sellability waits for both physical and service completion, and integrated room wear is the sole maintenance-failure source. The legacy inventory payload remains only for save-format compatibility and is not exposed as live stock authority.
- Atomic, cash-limited construction and state-aware room/infrastructure commands that preserve reservations, repairs, service tasks and routing invariants.
- Integer smallest-currency authority for rates, wages, utilities and all ledger totals. Save format v9 round-trips exact deterministic state, validates the complete entity-reference graph, persists FINAL-04/FINAL-05 state in length-prefixed sections and migrates v2-v8 saves.
- Long-running campaigns keep departed guests and completed reservations out of the per-second hot path, retain the newest 128 completed tasks for diagnostics, and preserve complete reservation/review history in saves. A 365-day, six-room Release reference run completed in 3.80 seconds with 941 stays and zero blocked tasks in the current Linux container.
- Portable campaign runner, behavioral/save/world tests, software-rendering Windows smoke-test path and Windows packaging workflow.

The HMG-000 first-playable layout criterion has an automated same-seed comparison with a controlled six-guest arrival rush followed by a 20-day operating window. The two hotels have identical room capacity, prices, constructed corridor length, staffing and inventory; only reception and closet placement differ. With seed `89`, the efficient/poor hotels produce 12/3,060 seconds of arrival-to-desk travel, 4,810/6,302 seconds of complete check-in queue time, 63.94/56.39 launch-cohort satisfaction, 1/3 walked relocations, 37/35 completed stays and 198,000/100,000 cents operating profit.

FINAL-01 verification is green on head `6f305f59e57bd78b0d5ae0b45c4d597a6ec7ee46`: Ubuntu Release build and the complete 15-test CTest suite passed, including construction, build jobs, building systems, v11 save/migration, mid-trip elevator continuation and deterministic elevator-dispatch soak coverage. Windows Release build, CTest, native client smoke launch and package generation also passed on the same head. Balance Lab, Optimization and OpenUSD Pipeline CI passed as well.

## FINAL-01 completion status

The FINAL-01 Construction & Building Systems implementation plan is complete on this branch:

- Object-level building is usable without blueprint-only shortcuts.
- Construction commits atomically and supports exact cash, material reservation/consumption and maintenance labor.
- Power, water, egress and accessibility can block room sale with exact authoritative reason codes.
- Passenger and service elevators operate as deterministic capacity-aware state machines with stable bank dispatch.
- Fire and security infrastructure have authoritative installation/coverage state suitable for later incident systems.
- Save/load round-trips FINAL-01 state and the v10-to-v11 migration path is covered.
- Construction/building-system state reaches the client through immutable snapshots; presentation does not own validity.

## Full-spec gaps that remain material

The supplied HMG-000 through HMG-050 documents describe a substantially larger simulation than these integrated systems. In particular, this branch does not yet provide construction-authored placement and full UI control for the new FINAL-05 venues/amenities, elevator-car dispatch, fire/security systems, all 13 guest archetypes with their complete trait/memory/group model and discretionary service goals, departmental managers and forecasts, financing/covenants/receivership, competitor hotels and segmented demand, full utility-network simulation, all global overlays, or the production asset catalog in HMG-070 through HMG-079.

Room blueprints and procedural furnishings are a construction interface for this implementation; they do not constitute the complete object-level construction editor, physical construction-labor/material chain or authored commercial art library. The tutorial is an in-game guide rather than a fully scripted campaign with contextual objectives.

No 500-room / 1,000-guest / 300-employee performance claim is made without a corresponding benchmark.
