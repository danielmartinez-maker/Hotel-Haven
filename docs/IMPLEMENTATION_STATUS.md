# Implementation status

This file distinguishes the integrated gameplay build from the full HMG product specification. It is not a declaration that the full game is complete.

## Integrated in this branch

- Root CMake build joining simulation, renderer and existing content pipeline.
- Native Windows client with validated construction previews, room inspection, exact-cent rates, staff hiring/shifts/wages, guest archetype/trait/budget/travel/queue diagnostics, supply ordering, task diagnostics, finances, reviews and introductory guide.
- Three-floor starter property, room blueprints, corridors, walls, doors, an explicit lobby, reception, bathrooms, staff rooms, supply closets and stairs.
- Furnished procedural scene with beds, desks, bath fixtures, plants, characters, floor selection, cutaway and diagnostic colors.
- Deterministic one-second simulation with all 13 baseline guest archetype profiles, 0–3 non-conflicting traits, individual budgets/sensitivities/patience, segment-aware price/reputation demand and probabilistic 1.0–10.0 reviews with deterministic score variation. It also models fixed arrival and departure windows, physical check-in, check-in abandonment and room release, checkout, day/night room stays, turnover resources, part-consuming maintenance failures, task-weighted fatigue, payroll and cash flow.
- Atomic, cash-limited construction and state-aware room/infrastructure commands that preserve reservations, repairs, service tasks and routing invariants.
- Integer smallest-currency authority for rates, wages, utilities and all ledger totals. Save format v10 round-trips exact deterministic state, guest profiles, complete check-in diagnostics, walked reservations and ten-point reviews, validates the complete entity-reference graph and migrates v2-v9 saves.
- Long-running campaigns keep departed guests and completed/walked reservations out of the per-second hot path, retain the newest 128 completed tasks for diagnostics, and preserve complete reservation/review history in saves. With seed `20260909`, a 365-day, six-room Release reference run completed in 4.40 seconds with 1,071 stays, 10,419,302 cents cash, 78.37 reputation and zero blocked tasks in the current Linux container.
- Portable campaign runner, behavioral/save/world tests, software-rendering Windows smoke-test path and Windows packaging workflow.

The HMG-000 first-playable layout criterion has an automated same-seed comparison with a controlled six-guest arrival rush followed by a 20-day operating window. The two hotels have identical room capacity, prices, constructed corridor length, staffing and inventory; only reception and closet placement differ. With seed `89`, the efficient/poor hotels produce 12/3,060 seconds of arrival-to-desk travel, 4,810/6,302 seconds of complete check-in queue time, 63.94/56.39 launch-cohort satisfaction, 1/3 walked relocations, 37/35 completed stays and 198,000/100,000 cents operating profit.

Local verification covers the portable Linux build. The native Direct3D client is configured for a Windows CI compile, forced-WARP smoke launch, save round-trip and two screenshot captures, but that Windows job must run before claiming a verified Windows binary.

## Full-spec gaps that remain material

The supplied HMG-000 through HMG-050 documents describe a substantially larger simulation than these integrated systems. In particular, this branch does not yet provide restaurant/bar/banquet production, the physical washer/dryer/folding laundry chain, elevator-car dispatch, fire/security systems, complete guest memories/groups/complaints/discretionary service goals, departmental managers and forecasts, financing/covenants/receivership, competitor hotels and full future-date inventory, full utility-network simulation, all global overlays, or the production asset catalog in HMG-070 through HMG-079.

Room blueprints and procedural furnishings are a construction interface for this implementation; they do not constitute the complete object-level construction editor, physical construction-labor/material chain or authored commercial art library. Staffing does not yet include applicants, breaks, morale, training, managers or absence. The tutorial is an in-game guide rather than a fully scripted campaign with contextual objectives.

No 500-room / 1,000-guest / 300-employee performance claim is made without a corresponding benchmark.
