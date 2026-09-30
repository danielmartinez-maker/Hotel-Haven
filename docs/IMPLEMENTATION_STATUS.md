# Implementation status

This file distinguishes the integrated gameplay build from the full HMG product specification. It is not a declaration that the full game is complete.

## Integrated in this branch

- Root CMake build joining simulation, renderer and existing content pipeline.
- Native Windows client with validated construction previews, room inspection, exact-cent rates, staff hiring/shifts/wages, guest travel/queue diagnostics, supply ordering, task diagnostics, finances, reviews, the four-tab guest inspector and introductory guide.
- Three-floor starter property, room blueprints, corridors, walls, doors, an explicit lobby, reception, bathrooms, staff rooms, supply closets and stairs.
- Furnished procedural scene with beds, desks, bath fixtures, plants, characters, floor selection, cutaway and diagnostic colors.
- Deterministic one-second simulation with price/reputation demand, fixed arrival and departure windows, physical check-in and checkout, day/night room stays, turnover resources, part-consuming maintenance failures, task-weighted fatigue, payroll, reviews and cash flow.
- Workforce organization with deterministic daily applicant pools, exact-cent employment contracts/onboarding, policy-priority breaks, fatigue/morale effects, training, seeded shift-boundary absence, departments/managers, fixed-bucket staffing forecasts, immutable optimizer snapshots, validation, and deterministic native fallback planning.
- HMG-010 guest profiles for all 13 archetypes, guest-local deterministic decisions, lifecycle/groups, needs and perceptions, available-service goal selection, event-backed memories, complaint recovery, reviews, and v10 guest persistence. CI verifies a repeatable 1,000-profile decision soak plus 1,000-memory/complaint save continuation.
- Atomic, cash-limited construction and state-aware room/infrastructure commands that preserve reservations, repairs, service tasks and routing invariants.
- Integer smallest-currency authority for rates, wages, utilities and all ledger totals. The HHGS 10 writer round-trips exact deterministic state, validates the complete entity-reference graph and migrates v2-v10 saves.
- Long-running campaigns keep departed guests and completed reservations out of the per-second hot path, retain the newest 128 completed tasks for diagnostics, and preserve complete reservation/review history in saves. A 365-day, six-room Release reference run completed in 3.80 seconds with 941 stays and zero blocked tasks in the current Linux container.
- Portable campaign runner, behavioral/save/world tests, software-rendering Windows smoke-test path and Windows packaging workflow.

The HMG-000 first-playable layout criterion has an automated 20-day, same-seed comparison. The two hotels have identical room capacity, prices, constructed corridor length, staffing and inventory; only reception and closet placement differ. The test requires the poor layout to produce more guest travel, more queue time, lower satisfaction, fewer completed stays and lower operating profit.

Integrated Game CI builds and tests Linux and Windows, launches the Windows client under forced WARP, checks save round-trip, captures Guide, hotel, and guest-inspector screenshots, packages the Windows release, and verifies package integrity. The guest screenshots and release archive are available as workflow artifacts.

## Full-spec gaps that remain material

The supplied HMG-000 through HMG-050 documents describe a substantially larger simulation than these integrated systems. In particular, this branch does not yet provide restaurant/bar/banquet production, the physical washer/dryer/folding laundry chain, elevator-car dispatch, fire/security systems, financing/covenants/receivership, competitor hotels and segmented demand, full utility-network simulation, all global overlays, or the production asset catalog in HMG-070 through HMG-079. Guest market-rate comparisons, measured environmental noise, and additional venue/service providers await their owning later systems; the current guest model reports those inputs as unavailable.

Room blueprints and procedural furnishings are a construction interface for this implementation; they do not constitute the complete object-level construction editor, physical construction-labor/material chain or authored commercial art library. The tutorial is an in-game guide rather than a fully scripted campaign with contextual objectives.

The 1,000-profile decision soak is not a benchmark of 1,000 simultaneously active hotel guests. No 500-room, 1,000-active-guest, 300-employee, or 1,300-moving-character performance claim is made without its corresponding project-wide benchmark.
