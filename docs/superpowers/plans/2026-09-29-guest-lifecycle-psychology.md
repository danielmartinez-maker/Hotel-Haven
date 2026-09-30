# Guest Lifecycle and Psychology Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the HMG-010 guest lifecycle and psychology subsystem in Hotel Haven while preserving the deterministic simulation and existing command/snapshot boundary.

**Architecture:** Extract deterministic guest-domain logic into focused `GuestModel` and `GuestExperience` modules. Keep `Simulation` as the authoritative coordinator for reservations, groups, time, services, persistence, and commands; expose guest diagnostics as read-only snapshots to the Windows client.

**Tech Stack:** C++20, CMake, existing `hh::assets::Json`, portable CTest suite, Windows Direct3D 11 client and Win32 management panels.

**Spec:** `docs/superpowers/specs/2026-09-29-guest-lifecycle-psychology-design.md`

## Global Constraints

- Target Windows 11 x64 for the native client and keep the simulation portable and deterministic.
- Guest needs and perceptions use the HMG-010 0–100 convention; sensitivities and preferences use normalized 0.0–1.0 values.
- Use stable 64-bit entity IDs and deterministic per-guest random streams; do not use `std::hash` or implementation-dependent distributions.
- Mandatory lifecycle goals take precedence; discretionary decisions are event-triggered or on the HMG-010 60-simulation-second cadence.
- A guest can select a service only when a matching activity is available, accepting guests, reachable, and budget/time compatible.
- The existing `balance.v1` input remains valid without new fields.
- Write `HHGS 10` saves and continue loading every currently supported version, v2 through v9.
- Reviews use the HMG-010 1.0–10.0 scale and may mention only recorded guest experiences.

## Review Focus

- **Malformed guest balance data:** reject non-finite values, invalid weights, missing archetypes, and out-of-range numeric modifiers without partially applying definitions. Test in Task 1.
- **Lifecycle and group edge cases:** reject illegal state changes; prevent children from performing independent lifecycle actions; recover a valid leader when the prior leader leaves. Test in Task 2.
- **Missing venue, market, or noise inputs:** do not select unavailable services or invent price comparisons/noise memories. Test in Task 4.
- **Invalid experience and recovery references:** reject unknown guests, duplicate incidents, stale complaints, and recovery commands that could refund twice. Test in Tasks 3 and 4.
- **Legacy and oversized saves at scale:** migrate every supported old format, reject oversized guest collections, and exercise 1,000 simultaneous guest decisions without per-second full venue scoring. Test in Tasks 5 and 7.

---

## File Map

| File | Responsibility |
|---|---|
| `game/include/hh/game/GuestModel.h` | Guest archetype, trait, lifecycle, need, group, goal, and typed balance model. |
| `game/src/GuestModel.cpp` | Deterministic profile generation, transition rules, need updates, group acceptance, and goal selection. |
| `game/include/hh/game/GuestExperience.h` | Experience, memory, complaint, recovery, and review types and interfaces. |
| `game/src/GuestExperience.cpp` | Deterministic experience-to-memory, satisfaction, complaint, recovery, and review calculations. |
| `game/include/hh/game/Simulation.h` | Simulation guest commands and read-only guest/review view types. |
| `game/src/Simulation.cpp` | Booking/group lifecycle integration, event sources, balance loading, save/load integration, and simulation snapshots. |
| `game/data/balance.json` | Tunable archetype, trait, need, queue, memory, complaint, review, and recovery defaults. |
| `game/tests/GuestModelTests.cpp` | Portable deterministic guest profile, group, lifecycle, need, and goal tests. |
| `game/tests/GuestExperienceTests.cpp` | Portable memory, complaint, recovery, satisfaction, and review tests. |
| `game/tests/SimulationTests.cpp` | Existing campaign integration regression tests and new live guest-flow assertions. |
| `game/tests/SimulationSaveMigrationTests.cpp` | HHGS v2–v10 migration, graph validation, and round-trip tests. |
| `game/tests/fixtures/legacy-vN.hhsave` | Minimal known-good fixture for each supported legacy version v2–v9. |
| `game/app/Client.h` | Client-side selected guest identity for the Guests page. |
| `game/app/Panels.cpp` | Read-only guest inspector, group/memory/complaint display, recovery controls, and review rating display. |
| `game/app/GameMain.cpp` | Windows smoke-test navigation to and capture of the guest inspector. |
| `game/CMakeLists.txt` | Register portable guest-model, experience, and migration tests. |
| `README.md`, `docs/IMPLEMENTATION_STATUS.md` | Correct save-version claims and report HMG-010 completion separately from remaining full-game work. |

## Task 1: Deterministic guest profiles and archetypes

**Files:**
- Create: `game/include/hh/game/GuestModel.h`
- Create: `game/src/GuestModel.cpp`
- Create: `game/tests/GuestModelTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `GuestId` from `hh/game/ServiceTypes.h`.
- Produces: `GuestModelDefinitions defaultGuestModelDefinitions()` and `GuestProfile generateGuestProfile(std::uint64_t simulationSeed, GuestId guestId, const GuestModelDefinitions &definitions)`.
- Produces: `bool validateGuestModelDefinitions(const GuestModelDefinitions &definitions, std::string *error = nullptr)` so profile ranges, archetype weights, trait effects, and category weights can be rejected before definitions are applied.
- `GuestProfile` contains archetype, age band, travel purpose, wealth/budget, normalized sensitivities, activity and room preferences, expectation profile, and 0–3 traits.

- [x] **Step 1: Register the new CTest target and write failing profile tests** named `profile_generation_is_repeatable_for_seed_and_guest_id`, `profiles_vary_without_shared_rng_state`, `profile_sampling_covers_all_13_archetypes`, `trait_count_is_zero_to_three_and_every_trait_has_a_numeric_effect`, and `profile_definitions_reject_invalid_ranges`.
- [x] **Step 2: Run the new test target** with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` and `cmake --build build --config Release --target hh_guest_model_tests --parallel 4`. Expected: failure because the guest-model API is not implemented.
- [x] **Step 3: Implement profile enums, typed definitions, defaults, and `generateGuestProfile`; add `GuestModel.cpp` to `hh_game` in CMake**. Derive a guest-local stream from the simulation seed and guest ID with a fixed integer mixer; sample numeric variation from that stream and apply every selected trait’s configured modifiers.
- [x] **Step 4: Run `ctest --test-dir build -C Release -R '^GuestModel$' --output-on-failure`.** Expected: PASS, including all 13 archetypes across a fixed-seed sample and stable repeated profiles.
- [x] **Step 5: Commit** `feat: add deterministic guest profiles`.

## Task 2: Lifecycle, needs, groups, queues, and goal selection

**Files:**
- Modify: `game/include/hh/game/GuestModel.h`
- Modify: `game/src/GuestModel.cpp`
- Modify: `game/tests/GuestModelTests.cpp`

**Interfaces:**
- Produces: `bool canTransitionGuest(GuestLifecycleState from, GuestLifecycleState to) noexcept`.
- Produces: `double guestNeedPressure(double satisfiedScore) noexcept`.
- Produces: `GuestNeedState updateGuestNeeds(GuestNeedState current, const GuestProfile &profile, GuestActivity activity, double elapsedSimulationSeconds, const GuestModelDefinitions &definitions)`.
- Produces: a bounded perception update for cleanliness, service confidence, environment comfort, and value using only supplied operational evidence; absent external market/noise values remain unavailable.
- Produces: `double guestQueueToleranceMinutes(double baseToleranceMinutes, const GuestProfile &profile, double segmentModifier, double urgencyModifier) noexcept`.
- Produces: `bool groupAcceptsGoal(double proposedUtility, double bestAlternativeUtility) noexcept` and `std::optional<GuestGoalSelection> selectGuestGoal(const GuestProfile &profile, const GuestNeedState &needs, std::span<const GuestGoalCandidate> candidates, std::optional<GuestGoal> mandatoryGoal)`.
- A candidate carries its goal/target and the HMG utility factors: preference, availability, time, budget, group compatibility, distance, mood, expected wait, and required need.
- Produces: optional five-minute sleeping-noise sampling from an explicitly supplied measured noise value; absent noise input has no disruption, energy pause, or memory side effect.

- [x] **Step 1: Add the new tests to the registered model test target, then write failing tests** named `lifecycle_accepts_only_canonical_transitions`, `need_pressure_matches_hmg_formula_and_clamps_input`, `needs_decay_and_sleep_recovery_stay_in_range`, `children_cannot_accept_independent_lifecycle_goal`, `group_action_requires_utility_within_30_percent`, `selector_skips_unavailable_unreachable_and_over_budget_candidates`, `mandatory_goal_overrides_discretionary_goal`, `queue_tolerance_uses_patience_segment_and_urgency`, and `noise_sampling_requires_a_measured_sample_and_disrupts_for_five_minutes`.
- [x] **Step 2: Run `cmake --build build --config Release --target hh_guest_model_tests --parallel 4` and `ctest --test-dir build -C Release -R '^GuestModel$' --output-on-failure`.** Expected: new behavioral assertions fail.
- [x] **Step 3: Implement lifecycle validation, group leader/member rules, need and perception updates, need decay, queue tolerance, measured-noise sampling, the five-minute disruption/energy pause, need pressure, HMG utility scoring, stable tie-breaking, and candidate filtering** in `GuestModel.cpp`. Use `std::span` for candidate inputs; return no selection when no valid candidate exists.
- [x] **Step 4: Run `ctest --test-dir build -C Release -R '^GuestModel$' --output-on-failure`.** Expected: PASS with invalid ranges rejected and deterministic goal selections.
- [x] **Step 5: Commit** `feat: model guest goals and group behavior`.

## Task 3: Experience, memory, complaints, recovery, and reviews

**Files:**
- Create: `game/include/hh/game/GuestExperience.h`
- Create: `game/src/GuestExperience.cpp`
- Create: `game/tests/GuestExperienceTests.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: `GuestProfile`, `GuestCategory`, and deterministic guest-local random state from `GuestModel.h`.
- Produces: `GuestExperienceResult applyGuestExperience(const GuestProfile &profile, GuestExperienceState &state, const GuestExperienceEvent &event, const GuestExperienceDefinitions &definitions, EntityId memoryId, EntityId complaintId)`.
- Produces: `std::optional<GuestRecoveryOutcome> calculateGuestRecovery(const GuestMemory &memory, GuestRecoveryOption option, const GuestRecoveryContext &context, const GuestExperienceDefinitions &definitions)`.
- Produces: `GuestReview generateGuestReview(const GuestProfile &profile, const GuestExperienceState &state, int completedDay, std::uint64_t &guestRandomState, const GuestExperienceDefinitions &definitions)`.
- A review carries a 1.0–10.0 rating, 0–100 overall satisfaction for reputation, and text selected only from recorded memories.

- [ ] **Step 1: Register this test target in CMake, then write failing experience tests** named `memory_contribution_uses_valence_salience_and_half_life`, `critical_memories_keep_in_stay_weight`, `resolved_memory_and_complaint_are_updated_once`, `complaint_thresholds_and_duplicate_incidents_are_enforced`, `recovery_effectiveness_uses_hospitality_forgiveness_and_response_time`, `review_probability_and_critic_modifier_are_deterministic`, and `review_text_uses_only_actual_memory_statements`.
- [ ] **Step 2: Run `cmake --build build --config Release --target hh_guest_experience_tests --parallel 4` and `ctest --test-dir build -C Release -R '^GuestExperience$' --output-on-failure`.** Expected: the new tests fail before the experience module is implemented.
- [ ] **Step 3: Implement event validation, memory creation/decay/resolution, nine-category satisfaction aggregation (preserving both Quiet and ArrivalDeparture from the source sections), complaint eligibility, recovery outcomes, and deterministic review generation; add `GuestExperience.cpp` to `hh_game` in CMake**. Leave integer cash-ledger changes to the `Simulation` integration task. Every `GuestExperienceEvent` stores timestamp, location, optional source entity, category, observed/expected values, raw impact, salience, and resolution state.
- [ ] **Step 4: Run `ctest --test-dir build -C Release -R '^GuestExperience$' --output-on-failure`.** Expected: PASS; an unknown source or empty event cannot create an unsupported memory or review statement.
- [ ] **Step 5: Commit** `feat: model guest experience and reviews`.

## Task 4: Integrate guest profiles, lifecycle, groups, and goals into the simulation

**Files:**
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/src/Simulation.cpp`
- Modify: `game/tests/SimulationTests.cpp`

**Interfaces:**
- Consumes: profile, lifecycle, group, need, and goal APIs from Tasks 1–2.
- Produces: stable group/member state attached to reservations and runtime guests; current movement remains in `PersonState`, while hotel lifecycle uses `GuestLifecycleState`.

- [ ] **Step 1: Write failing campaign tests in the existing registered simulation target** named `reservation_creates_stable_guest_profile_and_group`, `group_checkin_and_checkout_run_once_for_all_members`, `guest_lifecycle_tracks_arrival_room_stay_and_departure`, `guest_goal_candidates_require_real_available_services`, and `guest_profiles_do_not_advance_shared_simulation_rng`.
- [ ] **Step 2: Run `cmake --build build --config Release --target hh_game_tests --parallel 4` and `ctest --test-dir build -C Release -R '^hh_game_tests$' --output-on-failure`.** Expected: the new integration assertions fail against the single-person reservation flow.
- [ ] **Step 3: Integrate profiles at booking, preserve globally unique guest IDs from booking through arrival, create multi-member groups for couple/family/group archetypes when room beds allow them, and coordinate one reservation check-in/checkout for all group members** in `Simulation.cpp`. Add lifecycle transitions around arrival, check-in, in-stay, checkout, and departure. Keep `PersonState` as movement/action state.
- [ ] **Step 4: Build and run `ctest --test-dir build -C Release -R '^hh_game_tests$' --output-on-failure`.** Expected: PASS; single and group reservations preserve existing room, task, and deterministic-campaign behavior.
- [ ] **Step 5: Commit** `feat: integrate guest lifecycle into campaigns`.

## Task 5: Wire experience events and balance definitions into current operations

**Files:**
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/src/Simulation.cpp`
- Modify: `game/data/balance.json`
- Modify: `game/tests/SimulationTests.cpp`

**Interfaces:**
- Produces: `CommandResult reportGuestExperience(const GuestExperienceEvent &event)` for trusted simulation operations to submit factual outcomes.
- Produces: `CommandResult resolveGuestComplaint(GuestId guestId, EntityId complaintId, GuestRecoveryOption option)`; successful refunds update the integer-currency ledger exactly once.
- Guest expectations accept optional market context; sleep/noise accepts optional measured room noise; later market/environment/venue systems populate those inputs.

- [ ] **Step 1: Write failing tests** named `balance_v1_loads_and_guest_tuning_is_atomic`, `invalid_guest_tuning_is_rejected_without_mutation`, `missing_market_or_noise_inputs_do_not_create_memories`, `recorded_queue_and_room_events_update_guest_memory`, `missing_food_venue_leaves_need_unserved_without_fake_completion`, `unknown_guest_experience_event_is_rejected`, and `complaint_recovery_cannot_refund_twice`.
- [ ] **Step 2: Run `cmake --build build --config Release --target hh_game_tests --parallel 4` and `ctest --test-dir build -C Release -R '^hh_game_tests$' --output-on-failure`.** Expected: the new API and event assertions fail.
- [ ] **Step 3: Add an optional `guestPsychology` table with its own version field to `balance.json` while retaining the current `balance.v1` payload; parse into a temporary definitions object and apply only after all fields validate. Add the two `Simulation` methods, event sources for existing queue/check-in/checkout/room-cleanliness facts, measured-noise sampling and five-minute energy interruption from Task 2, memory/complaint updates, completed-stay review generation, and recovery ledger handling** in `Simulation.cpp`.
- [ ] **Step 4: Run `ctest --test-dir build -C Release -R '^hh_game_tests$' --output-on-failure`.** Expected: PASS, including legacy balance input and no fabricated events when later-system inputs are absent.
- [ ] **Step 5: Commit** `feat: connect guest experiences to hotel operations`.

## Task 6: Version-10 guest persistence and legacy migration

**Files:**
- Create: `game/tests/SimulationSaveMigrationTests.cpp`
- Create: `game/tests/fixtures/legacy-v2.hhsave` through `legacy-v9.hhsave`
- Modify: `game/src/Simulation.cpp`
- Modify: `game/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 4 reservation/group/profile state and Task 5 experience, complaint, recovery, review, and guest-local random state.
- Produces: writer header `HHGS 10`; loader accepts versions 2–10; the v10 extension stores guest/group state without changing the legacy record layouts parsed for v2–v9.

- [ ] **Step 1: Add one valid, checked-in fixture captured from or generated against each legacy header version v2–v9, and write failing tests** named `every_legacy_version_migrates_to_v10`, `v10_roundtrip_preserves_guest_group_memories_and_random_state`, `corrupt_guest_references_are_rejected`, `guest_collection_limits_are_enforced`, `legacy_review_scores_migrate_to_hmg_rating`, and `legacy_migration_does_not_consume_shared_rng`. Include fixture provenance or a deterministic fixture-generation helper so each legacy payload is reproducible.
- [ ] **Step 2: Run `cmake --build build --config Release --target hh_simulation_save_migration_tests --parallel 4` and `ctest --test-dir build -C Release -R '^GuestSaveMigration$' --output-on-failure`.** Expected: legacy migration assertions fail until v10 guest state is serialized.
- [ ] **Step 3: Write `HHGS 10` and a length-bounded `GUEST10` extension after the existing service payload. Keep the v2–v9 parser layout unchanged; migrate legacy reservations to deterministic single-member groups and preserve existing live guest IDs. Reject duplicate/broken references and more than 100,000 total saved guest records, memories, or complaints** in `Simulation.cpp`.
- [ ] **Step 4: Run `ctest --test-dir build -C Release -R '^GuestSaveMigration$' --output-on-failure` and `ctest --test-dir build -C Release -R '^hh_game_tests$' --output-on-failure`.** Expected: PASS for v2–v9 migration, v10 round-trip, malformed-state rejection, and deterministic continuation.
- [ ] **Step 5: Commit** `feat: persist guest psychology in save v10`.

## Task 7: Guest inspection and recovery controls in the Windows client

**Files:**
- Modify: `game/include/hh/game/Simulation.h`
- Modify: `game/src/Simulation.cpp`
- Modify: `game/app/Client.h`
- Modify: `game/app/Panels.cpp`
- Modify: `game/app/GameMain.cpp`

**Interfaces:**
- Produces: `GuestView` snapshots with profile/archetype, group and leader, lifecycle, goal plus utility factors, needs, perceptions, category expectations/satisfaction, active memories, queue tolerance, review range, and complaint state.
- Produces: `SimulationView.guests` as a stable, read-only collection; the client selects guests by `GuestId` and sends recovery through `resolveGuestComplaint`.
- `ReviewView` exposes a 1.0–10.0 rating and 0–100 overall satisfaction; legacy v2–v9 ratings migrate from their prior 0–100 score.

- [ ] **Step 1: Add failing snapshot tests** named `guest_snapshot_contains_required_diagnostics`, `guest_snapshot_orders_members_and_memories_stably`, `guest_recovery_command_changes_only_the_selected_complaint`, and `review_view_exposes_rating_and_overall_satisfaction_separately`.
- [ ] **Step 2: Run `cmake --build build --config Release --target hh_game_tests --parallel 4` and `ctest --test-dir build -C Release -R '^hh_game_tests$' --output-on-failure`.** Expected: guest inspection fields and HMG review ratings are not present.
- [ ] **Step 3: Populate `GuestView` in `Simulation::view`, add selected-guest state and an inspector on the existing Guests page, display missing external inputs as unavailable, add complaint recovery buttons, and render reviews on the HMG-010 1–10 scale** in the listed files. Extend the Windows smoke path to select the Guests page and capture `smoke-guests.bmp` with at least one active guest.
- [ ] **Step 4: Run `cmake --build build --config Release --parallel 4` and `ctest --test-dir build -C Release --output-on-failure`; run the Windows client smoke test and verify `smoke-guests.bmp` is captured.** Expected: all portable tests pass and the Windows smoke process exits 0.
- [ ] **Step 5: Commit** `feat: inspect guest psychology in the client`.

## Task 8: Scale soak, full regression, and status documentation

**Files:**
- Create: `game/tests/GuestScaleTests.cpp`
- Modify: `game/CMakeLists.txt`
- Modify: `README.md`
- Modify: `docs/IMPLEMENTATION_STATUS.md`

**Interfaces:**
- Consumes: complete guest model, experience, simulation, save, and UI interfaces from Tasks 1–7.
- Produces: a bounded, repeatable 1,000-guest decision soak that reports goal-evaluation work and v10 save size without a guessed timing threshold.

- [ ] **Step 1: Write failing scale assertions** named `one_thousand_guest_decisions_use_filtered_candidates`, `memories_and_complaints_remain_within_save_limits`, and `guest_model_soak_is_repeatable_for_fixed_seed`.
- [ ] **Step 2: Run `cmake --build build --config Release --target hh_guest_scale_tests --parallel 4` and `ctest --test-dir build -C Release -R '^GuestScale$' --output-on-failure`.** Expected: scale target is not yet registered and the checks fail.
- [ ] **Step 3: Add the deterministic 1,000-guest decision soak and report decision evaluations plus serialized save bytes for that fixture; update `README.md` and `docs/IMPLEMENTATION_STATUS.md`** to state the verified HHGS 10 writer, v2–v10 loader, HMG-010 coverage, remaining HMG workstreams, and unclaimed full-scale targets.
- [ ] **Step 4: Run `cmake --build build --config Release --parallel 4` and `ctest --test-dir build -C Release --output-on-failure`; confirm the existing 14-day campaign and all guest tests pass.** Expected: all CTest tests pass; no 500-room or 1,300-moving-character claim is added without measurement.
- [ ] **Step 5: Commit** `test: verify guest decision scale and document coverage`.

## Execution notes

- Work through tasks in order because the simulation, persistence, and client all consume the new guest interfaces.
- Keep tests in the repository’s existing small C++ executable style and register them with CTest.
- Each task ends with its own passing test cycle and commit. No task silently expands into restaurants, competitor AI, financing, campaign content, or production asset completion; those remain later full-game projects.

