# Implementation status

This file distinguishes the integrated gameplay build from the full HMG product specification. It is not a declaration that the full game is complete.

## Integrated in this branch

- Root CMake build joining simulation, renderer and existing content pipeline.
- Native Windows client with validated construction previews, room inspection, exact-cent rates, staff hiring/shifts/wages, guest travel/queue diagnostics, supply ordering, task diagnostics, finances, reviews and introductory guide.
- Three-floor starter property, room blueprints, corridors, walls, doors, an explicit lobby, reception, bathrooms, staff rooms, supply closets and stairs.
- Furnished procedural scene with beds, desks, bath fixtures, plants, characters, floor selection, cutaway and diagnostic colors.
- Deterministic one-second simulation with price/reputation demand, fixed arrival and departure windows, physical check-in and checkout, day/night room stays, turnover resources, part-consuming maintenance failures, task-weighted fatigue, payroll, reviews and cash flow.
- Workforce organization with deterministic daily applicant pools, exact-cent employment contracts/onboarding, policy-priority breaks, fatigue/morale effects, training, seeded shift-boundary absence, departments/managers, fixed-bucket staffing forecasts, immutable optimizer snapshots, validation, and deterministic native fallback planning.
- Atomic, cash-limited construction and state-aware room/infrastructure commands that preserve reservations, repairs, service tasks and routing invariants.
- Integer smallest-currency authority for rates, wages, utilities and all ledger totals. Save format v8 round-trips exact deterministic state, validates the complete entity-reference graph and migrates v2-v7 saves.
- Long-running campaigns keep departed guests and completed reservations out of the per-second hot path, retain the newest 128 completed tasks for diagnostics, and preserve complete reservation/review history in saves. A 365-day, six-room Release verification run completed in 2.12 seconds (2.11 seconds on repeat) with 954 stays and zero blocked tasks in the current Linux container; repeated outputs are byte-identical.
- The portable benchmark target covers small, normal, large, and 300-employee simulation tiers plus deterministic 64/256/512 employee-task fallback-planner scaling. Renderer GPU metrics and direct 500-room/1,000-guest injection remain unavailable through the current portable API.
- Portable campaign runner, behavioral/save/world tests, software-rendering Windows smoke-test path and Windows packaging workflow.

The HMG-000 first-playable layout criterion has an automated 20-day, same-seed comparison. The two hotels have identical room capacity, prices, constructed corridor length, staffing and inventory; only reception and closet placement differ. The test requires the poor layout to produce more guest travel, more queue time, lower satisfaction, fewer completed stays and lower operating profit.

Local verification covers the portable Linux build. The native Direct3D client is configured for a Windows CI compile, forced-WARP smoke launch, save round-trip and two screenshot captures, but that Windows job must run before claiming a verified Windows binary.

## Full-spec gaps that remain material

The supplied HMG-000 through HMG-050 documents describe a substantially larger simulation than these integrated systems. In particular, this branch does not yet provide restaurant/bar/banquet production, the physical washer/dryer/folding laundry chain, elevator-car dispatch, fire/security systems, all 13 guest archetypes with their complete trait/memory/group model and discretionary service goals, financing/covenants/receivership, competitor hotels and segmented demand, full utility-network simulation, all global overlays, or the production asset catalog in HMG-070 through HMG-079.

Room blueprints and procedural furnishings are a construction interface for this implementation; they do not constitute the complete object-level construction editor, physical construction-labor/material chain or authored commercial art library. The tutorial is an in-game guide rather than a fully scripted campaign with contextual objectives.

No 500-room / 1,000-guest / 300-employee performance claim is made without a corresponding benchmark.
