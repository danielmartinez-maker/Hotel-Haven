# Implementation status

This file distinguishes the integrated gameplay build from the full HMG product specification. It is not a declaration that the full game is complete.

## Integrated in this branch

- Root CMake build joining simulation, renderer and existing content pipeline.
- Native Windows client with validated construction previews, room inspection, exact-cent rates, staff hiring/shifts/wages, guest archetype/trait/budget/travel/queue diagnostics, supply ordering, task diagnostics, finances, reviews and introductory guide.
- Three-floor starter property, room blueprints, corridors, walls, doors, an explicit lobby, reception, bathrooms, staff rooms, supply closets and stairs.
- Furnished procedural scene with beds, desks, bath fixtures, plants, characters, floor selection, cutaway and diagnostic colors.
- Deterministic one-second simulation with all 13 baseline guest archetype profiles, 0–3 non-conflicting traits, individual budgets/sensitivities/patience, segment-aware price/reputation demand and probabilistic 1.0–10.0 reviews with deterministic score variation. It also models fixed arrival and departure windows, physical check-in, check-in abandonment and room release, checkout, day/night room stays, turnover resources, part-consuming maintenance failures, task-weighted fatigue, payroll and cash flow.
- FINAL-01 authoritative object construction with atomic multi-object placement/removal, exact-cent cost validation, stable reason codes, floor/wall support rules, movement/access protection and immutable object snapshots.
- FINAL-01 physical build jobs that reserve cash and construction materials, create maintenance-labor tasks, consume materials at work start, mutate world state only after labor completion and preserve deterministic cancellation/refund behavior.
- FINAL-01 power/water connectivity and capacity graphs, exact room-sale blockers for power/water/egress/accessibility, authoritative fire/security installation coverage, and read-only utility/construction presentation overlays.
- Deterministic elevator-bank dispatch on the FINAL-01 building-system authority: passenger/service bank isolation, integer ETA car selection with stable car-ID tie breaking, hard capacity enforcement, same-floor/same-direction boarding batches and multi-destination onboard sweeps. Existing request ownership and `boarded` state provide queue/onboard diagnostics; explicit-car requests remain supported and the HHGS 11 elevator schema is unchanged.
- Atomic, cash-limited construction and state-aware room/infrastructure commands that preserve reservations, repairs, service tasks and routing invariants.
- Integer smallest-currency authority for rates, wages, utilities and all ledger totals. Save format v11 round-trips exact deterministic state including FINAL-01 construction/build-job/building-system state. The FINAL-01 migration gate loads prior v10 saves with safe defaults for appended state, while earlier supported migration behavior remains intact. Mid-trip batched elevator saves resume byte-identically.
- Long-running campaigns keep departed guests and completed/walked reservations out of the per-second hot path, retain the newest 128 completed tasks for diagnostics, and preserve complete reservation/review history in saves. With seed `20260909`, a 365-day, six-room Release reference run completed in 4.40 seconds with 1,071 stays, 10,419,302 cents cash, 78.37 reputation and zero blocked tasks in the current Linux container.
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

The supplied HMG-000 through HMG-050 documents describe a substantially larger simulation than FINAL-01. Remaining cross-stack/full-product gaps include restaurant/bar/banquet production, the physical washer/dryer/folding laundry chain, automatic guest/staff path routing into elevator-bank requests, fire/security incident-response simulation beyond FINAL-01 installation/coverage state, utility domains beyond the current power/water network, complete guest memories/groups/complaints/discretionary service goals, departmental managers and forecasts, financing/covenants/receivership, competitor hotels and full future-date inventory, all global management overlays, and the production asset catalog in HMG-070 through HMG-079.

Room blueprints and procedural furnishings remain convenience construction commands alongside the FINAL-01 object-level editor and physical build-job chain; they do not represent the final breadth of the authored construction catalog or commercial art library. Staffing does not yet include applicants, breaks, morale, training, managers or absence. The tutorial is an in-game guide rather than a fully scripted campaign with contextual objectives.

No 500-room / 1,000-guest / 300-employee performance claim is made without a corresponding benchmark.
