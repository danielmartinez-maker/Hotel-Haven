# Hotel Haven Guest Lifecycle and Psychology

**Document ID:** HMG-010 implementation design
**Version:** 0.1
**Status:** Approved after conversation review
**Target:** Windows 11 x64 native client; portable deterministic simulation
**Language:** C++20

## 1. Purpose

Complete the guest lifecycle and psychology subsystem defined in [`HMG_010_GUEST_AI_AND_PSYCHOLOGY_V0_1.md`](../../specifications/HMG_010_GUEST_AI_AND_PSYCHOLOGY_V0_1.md). Extend the current playable simulation without replacing its command/snapshot boundary, deterministic operation, or existing hotel-management loops.

The first implementation milestone establishes the guest model and integration contracts. Later milestones provide the broader hotel objects, physical facilities, market, noise sources, and service events that populate those contracts.

## 2. Product context and staged delivery

The user selected the existing HMG documents as the definition of the full game and approved a dependency-first sequence:

1. Guest lifecycle and psychology.
2. Shared object construction, utilities, movement, elevators, and safety foundations.
3. Physical service chains, including laundry, food, room service, and events.
4. Competitor-aware demand, pricing, financing, and distress.
5. Campaign progression, authored production content, accessibility, scale benchmarks, and Windows release verification.

This document covers stage 1 only. Each stage is intended to be an independently buildable and reviewable addition to the game.

## 3. Current repository baseline

The repository is `danielmartinez-maker/Hotel-Haven`. The main branch contains a C++20 simulation with a portable headless runner and a Windows Direct3D 11 client. The simulation already tracks reservations, rooms, guest movement, hunger, rest, satisfaction, patience, a current goal, reviews, and service work. It exposes commands and a read-only `SimulationView`; rendering consumes snapshots.

Guest state is currently represented by basic fields on `Person` in `game/src/Simulation.cpp`. The public view does not yet expose the HMG-010 profile, needs, category expectations, memories, complaints, or reasoned goal-selection data. The current balance file defines only business and leisure archetypes.

The source currently writes `HHGS 9` saves and accepts versions 2 through 9. `README.md` describes v7, and `docs/IMPLEMENTATION_STATUS.md` describes v8. The implementation must use the loader and writer as the compatibility authority and correct the stale documentation when the new format is integrated.

## 4. Goals

- Represent the 13 guest archetypes as varied profiles rather than fixed templates.
- Implement explicit lifecycle, group behavior, needs, perceptions, goals, expectations, experience memories, complaints, recovery, sleep/noise reactions, and reviews from HMG-010.
- Keep simulation outcomes deterministic for a seed and save state.
- Make guest motivations and outcomes inspectable and causally explainable in the client.
- Provide stable interfaces for later venue, market, environment, and service systems.
- Keep guest decision work bounded as the hotel grows.

## 5. Non-goals for this milestone

- Building restaurants, bars, banquet venues, exercise spaces, or other new service facilities.
- Implementing competitor hotels, a segmented market simulation, lending, or financial covenants.
- Replacing the current construction model, route system, renderer, or frontend.
- Claiming the 500-room, 1,000-guest, 300-employee target is met before the project-wide benchmark milestone.
- Adding online play, multiplayer, or external services.

Future goal types and experience categories may be defined now, but a guest may only select an action when a real matching activity is available, reachable, and accepting service.

## 6. Architecture

### 6.1 Ownership boundary

`Simulation` remains the public command and read-only snapshot boundary. The simulation owns all authoritative guest data, transition validation, decision timing, experience processing, and save/load behavior. The client renders snapshots and sends commands; it does not calculate needs, satisfaction, or goal utility.

Extract guest-domain data and deterministic helper logic into a focused guest-model module under `game/include/hh/game` and `game/src`. `Simulation::Impl` coordinates IDs, simulation time, hotel state, and calls into this module. The guest model must not depend on Win32, Direct3D, or UI code.

### 6.2 Guest-domain components

- **Profile factory:** creates an archetype distribution, identity, budget, preferences, sensitivities, expectations, and 0–3 traits from a deterministic per-guest random stream.
- **Lifecycle and group model:** validates the HMG-010 lifecycle state machine; stores group members, leader, shared reservation, cohesion, shared itinerary, and group budget where applicable.
- **Need model:** stores the eight needs (energy, hunger, hygiene, comfort, entertainment, social, privacy, safety) and four operational perceptions (service confidence, cleanliness confidence, environment comfort, value perception). Values remain in the specified 0–100 range.
- **Goal selector:** ranks only valid candidates using the HMG-010 utility and need-pressure formulas, with mandatory lifecycle goals taking precedence. It returns the selected goal and the factors that explain the choice.
- **Experience and memory model:** records typed events, updates category satisfaction, applies memory decay and resolution, and raises eligible complaints.
- **Review generator:** derives probability, score, and text from the completed stay and its strongest actual memories.
- **Guest snapshot:** provides stable, read-only diagnostic values for inspection without exposing mutable simulation internals.

### 6.3 Deterministic decisions

Guest profile generation and discretionary choices use an explicitly specified deterministic random stream derived from the simulation seed and stable guest identity. Do not use `std::hash`, implementation-dependent distributions, or iteration order as a source of outcomes. Guest-local draws must not consume the existing shared simulation stream, so adding profile or review draws cannot silently change staff, economy, or demand decisions.

Goal candidates and any tie-breaks use stable ordering. Lifecycle transitions are validated by one canonical transition function. Invalid transitions are rejected and surfaced through the existing status/diagnostic path rather than partially mutating a guest.

## 7. Behavior and data flow

1. A booking or walk-in creates a guest identity and profile from the archetype and balance definitions. Guests in a shared booking receive a stable group identity and one leader.
2. Arrival and mandatory hotel lifecycle states override discretionary behavior. Each transition is validated and recorded.
3. Needs and operational perceptions update from elapsed simulation time and the guest's current activity. Need values are clamped to 0–100.
4. On lifecycle changes, goal completion, invalid targets, excessive waits, critical needs, group-plan changes, or scheduled reassessment, the selector evaluates available activities. Normal discretionary reassessment follows the specified 60-simulation-second cadence; critical needs below 15 and event triggers can reassess sooner.
5. A candidate includes service type, reachability, current availability, expected wait, budget compatibility, time compatibility, group compatibility, and distance. The selector uses the HMG-010 utility formula and need pressure `((100 - N) / 100)^2 * 2`.
6. The selected action uses the existing movement/task/service systems. A missing or unavailable service creates no fabricated completion; the guest retains an explainable unmet need or chooses a valid alternative.
7. Systems report factual outcomes as typed `GuestExperienceEvent` values containing timestamp, location/source, category, observed and expected values, raw impact, salience, and resolution state. Guest logic alone updates memory, satisfaction, and complaints.
8. At checkout, the review generator evaluates category scores and memories. Review text may reference only events recorded for that guest. The stay summary and review history remain available after the live guest leaves the hot simulation set.
9. `SimulationView` publishes the inspection data. The client displays it without changing simulation state.

## 8. HMG-010 behavior contract

Use the HMG-010 document as the source of truth for state names, archetypes, trait effects, formulas, event categories, defaults, and recovery values. The implementation includes:

- All 13 archetypes: Budget Leisure, Backpacker, Business Traveler, Executive Business, Couple Leisure, Family Leisure, Luxury Leisure, Conference Delegate, Group/Tour Traveler, Airport/Transit Traveler, Wellness Traveler, VIP/Celebrity, and Critic/Reviewer.
- Zero to three individually sampled numeric traits, including Patient, Impatient, Neat, Messy, Light Sleeper, Heavy Sleeper, Foodie, Workaholic, Social, Private, Frugal, Status Conscious, Fitness Focused, Early Riser, Night Owl, Complaint Prone, and Forgiving. Each implemented trait must have at least one explicit numeric effect; values are balance-tunable.
- The canonical lifecycle states from `Prospective` through `CompletedStay`, plus `Cancelled`, `NoShow`, and `WalkedRelocated`. Children cannot independently check in or reassign rooms.
- Need decay, need-pressure goal utility, guest-specific expectations, room/service/cleanliness/value category satisfaction, queue tolerance and abandonment, and group action acceptance.
- Experience memory valence, magnitude, salience, category, half-life, and resolution. Critical incidents retain their specified in-stay weight.
- Complaint thresholds and duplicate-incident prevention; service-recovery options and response-speed, hospitality, and forgiveness modifiers.
- Five-minute sleep/noise sampling when a noise input exists, energy-gain interruption, disruption memories, and repeat-disturbance complaints.
- Checkout review chance and modifiers, a 1–10 score, deterministic score noise, and text based on the strongest actual positive and negative memories. Critics use the specified forced-review probability and reputation impact multiplier.
- VIP expectations, privacy/security weighting, and failure visibility as variants of the shared satisfaction and memory model, not a separate scoring system.

Where HMG-010 depends on a later subsystem, this milestone defines an input boundary. The market supplies optional competitor-relative price position; if it is absent, the guest receives no invented competitor comparison and the inspector indicates that comparison is unavailable. Environment systems supply optional measured room noise; without a sample, no noise incident is fabricated. Venue systems provide activity candidates. The later milestones populate these interfaces.

## 9. Experience, complaint, and recovery boundary

Operations modules report what happened; they do not directly assign satisfaction or review scores. A stable event type and category allow new facilities to integrate without duplicating psychology rules. The source entity and location are retained so the guest inspector can explain each memory.

Complaint resolution is an explicit simulation command. It validates the complaint, guest, option, timing, and any required service or room availability before changing state. Financial refunds use the existing integer-currency ledger. A recovery memory is recorded separately from the original incident; unresolved negative memory is reduced only by the HMG-010 configured effect and modifier formulas.

## 10. Balance data and player diagnostics

Extend the existing balance definitions with optional, versioned guest-profile and psychology tables. The current `balance.v1` file must remain loadable unchanged, with documented defaults used for omitted fields. Archetype defaults, trait modifiers, need rates, queue tolerances, expectation weights, memory half-lives, complaint thresholds, review settings, and recovery values are not scattered as unexplained literals through update code.

Selecting a guest shows archetype, budget band, lifecycle state, current goal and selection factors, all need scores, category satisfaction and expectations, active memories with source and timestamp, queue tolerance when queued, likely review range, group/leader context, and complaint/recovery status. Missing external inputs are labeled unavailable. No information relies only on color.

## 11. Persistence and compatibility

Introduce simulation save version 10 for the expanded guest profile, lifecycle, group, need/perception, goal, memory, complaint, recovery, and per-guest RNG state. The current writer is version 9, and the current loader accepts versions 2–9.

Loading any supported legacy save must create valid guest defaults deterministically from stable saved identity without drawing from the shared simulation RNG. Existing room, reservation, employee, task, service, economy, and history references must retain the loader's graph validation. New saves round-trip all guest state and continue to the same result after load as uninterrupted simulation.

Reject malformed ranges, unknown enum values, duplicate IDs, broken guest/group/reservation references, invalid memory sources, non-finite values, and oversized collections using the existing explicit load-error behavior. Update the README and implementation-status version claims to match the actual writer and loader.

## 12. Performance and failure handling

- Do not evaluate every venue utility for every guest every simulation second.
- Use the specified event-triggered and 60-second decision cadence; critical need thresholds and invalidated targets may trigger sooner.
- Cache guest path queries until the target or route-invalidating hotel state changes.
- Prefilter candidates by facility type and reachable floor/zone before full utility calculation.
- Guests with no urgent or visible interaction may use coarse updates without changing authoritative outcomes.
- Missing candidates remain visible as unmet needs; unreachable actions never complete.
- A failed optional market or environment input cannot corrupt the simulation. It is displayed as unavailable and cannot produce fabricated memories.
- Review text falls back to a neutral, event-backed result when no usable memory statements exist.

## 13. Verification

Add portable tests for:

- Deterministic generation of every archetype and trait distribution for fixed seeds; distinct guests vary without depending on platform library behavior.
- Allowed and rejected lifecycle transitions; group leader and family restrictions; group action acceptance and fallback.
- Need bounds/decay, goal utility, stable tie-breaking, mandatory goal precedence, availability/reachability filtering, queue tolerance, and critical-need reassessment.
- Category expectations with and without supplied market context; noise reactions with and without a measured noise input.
- Experience event attribution, memory decay/resolution, complaint eligibility and duplicate prevention, recovery modifiers, and integer-currency refunds.
- Review probability, critic behavior, deterministic score, and event-backed text that never names an unexperienced service.
- Guest inspection snapshots and stable ordering.
- Version-10 save round-trip, deterministic save continuation, and migration tests for every supported legacy version 2–9 without perturbing non-guest random outcomes.
- A bounded guest-decision soak that exercises the HMG target of 1,000 simultaneous guests and reports decision work and save size. Timing thresholds for release claims are established from measured baselines rather than guessed.

Use the existing `hh_game_tests`/CTest integration and add focused guest-model tests to `game/CMakeLists.txt`. Keep the Windows client smoke workflow and add a guest-inspector rendering check. Full 500-room and 1,300-moving-character claims remain a later project-wide benchmark gate.

## 14. Acceptance criteria

The milestone is ready when:

1. All guest-owned HMG-010 profile, lifecycle, group, need, goal, queue, expectation, memory, complaint/recovery, sleep/noise, review, VIP/critic, performance, and diagnostic rules are implemented. Dependency-owned market, environment, and venue inputs have explicit contracts, with their providers delivered in their respective later milestones.
2. Guests do not choose unavailable services, skip required lifecycle transitions, or complete actions without their dependencies.
3. Repeated seeded runs and save/load continuations reproduce the same guest state, review output, economy, and non-guest simulation state.
4. The client exposes the required selected-guest diagnostics through read-only snapshots.
5. Every supported legacy save version 2–9 loads, and version-10 guest state survives save/load with graph validation.
6. Portable CTest, the focused guest tests, and the Windows client smoke/inspection check pass in CI.
7. Documentation accurately reports the actual save version and clearly distinguishes the completed guest subsystem from remaining full-game work.

## 15. Related project documents

- [`HMG-000 Foundation`](../../specifications/HMG_000_FOUNDATION_V0_1.md)
- [`HMG-010 Guest AI and Psychology`](../../specifications/HMG_010_GUEST_AI_AND_PSYCHOLOGY_V0_1.md)
- [`HMG-020 Construction and Room Rules`](../../specifications/HMG_020_CONSTRUCTION_AND_ROOM_RULES_V0_1.md)
- [`HMG-030 Hotel Economics and Market Demand`](../../specifications/HMG_030_HOTEL_ECONOMICS_AND_MARKET_DEMAND_V0_1.md)
- [`HMG-040 Staff and Task Scheduling`](../../specifications/HMG_040_STAFF_AND_TASK_SCHEDULING_V0_1.md)
- [`HMG-050 Service and Logistics Simulation`](../../specifications/HMG_050_SERVICE_AND_LOGISTICS_SIMULATION_V0_1.md)
- [`Implementation status`](../../IMPLEMENTATION_STATUS.md)


