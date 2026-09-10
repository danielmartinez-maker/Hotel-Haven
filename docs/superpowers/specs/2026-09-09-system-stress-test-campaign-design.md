# Hotel Haven — System-Wide Stress Test Campaign Design

Date: 2026-09-09
Status: Approved design, implementation not started
Branch: `feature/system-stress-test-campaign`

## 1. Purpose

Build a comprehensive, deterministic stress-testing program for every implemented Hotel Haven system. The campaign must expose correctness failures, state corruption, nondeterminism, unbounded growth, deadlocks/hangs, save/load divergence, catastrophic performance regressions, and cross-system failures under sustained or adversarial load.

The campaign is a hardening layer around the existing game. It must not reduce simulation fidelity, weaken assertions, change gameplay rules merely to make tests pass, or replace meaningful acceptance criteria with proxy metrics.

## 2. Repository topology and ownership

Hotel Haven is currently not a single linear implementation branch. Stress coverage must follow authoritative ownership rather than pretending all systems already coexist on `main`.

As of this design:

- `main` contains the integrated foundation, FINAL-03 staff/organization work, and merged FINAL-04 service/logistics work.
- `feature/final-01-construction-building-systems` and `feature/final-02-guest-ai-psychology` contain the construction/building and guest-AI line that remains separate from the newer integrated main line.
- `feature/final-05-fnb-amenities-events` is the base for the newer hospitality-content stack.
- `feature/final-06-economy-market-competition` is stacked on FINAL-05.
- `feature/final-07-ingame-ui-management` is stacked on FINAL-06.
- renderer/runtime-asset work and Asset Pipeline V2 remain separately evolving feature lines.

Therefore the stress program will use a **branch matrix**. Each subsystem receives tests on the branch that owns the implementation. A central orchestration workflow runs the full matrix. No branch reconciliation or merge is part of this stress-test task unless explicitly requested later.

## 3. Design principles

### 3.1 Determinism first

All randomized stress scenarios use explicit 64-bit seeds and Hotel Haven's deterministic RNG conventions. Tests must avoid standard-library distribution behavior that can differ across implementations when exact replay matters. A failing run prints:

- suite name;
- seed;
- scenario/phase;
- simulation tick/day;
- invariant or assertion that failed;
- authoritative state hash where available;
- the recent bounded action trace needed to reproduce the failure.

A failure that cannot be replayed from its reported seed is itself a test-harness defect.

### 3.2 Invariants over expected anecdotes

Long stress campaigns validate authoritative invariants continuously instead of checking only a final snapshot. Core invariants include, where applicable:

- finite numeric state: no NaN/Inf in gameplay values;
- bounded domain values and enum/state validity;
- unique stable identifiers;
- monotonic simulation time and legal state transitions;
- resource conservation for physical inventories and queues;
- no impossible negative physical counts;
- no duplicated ownership/assignment of exclusive resources;
- no orphaned references after entity removal;
- queue, event, memory/history, and pending-work collections remain bounded by explicit gameplay/data limits or by the number of legitimate live entities;
- save/load round-trips preserve authoritative state and deterministic continuation;
- identical seed + identical commands produce identical authoritative hashes on repeated runs and, for systems promising cross-platform determinism, on Linux and MSVC.

### 3.3 Correctness and performance are separate gates

Correctness gates must be deterministic and platform-stable. Performance gates must catch catastrophic regressions without making CI flaky.

- CTest timeouts act as hard hang/runaway detectors.
- Stress scenarios enforce algorithmic/state-size ceilings where the design provides meaningful bounds.
- Extended workflows record throughput and wall-clock metrics.
- Wall-clock failure thresholds are coarse guardrails, not tight microbenchmark claims.
- A platform-specific timing difference must never be used to excuse a correctness failure.

### 3.4 Tests must remain diagnostic

Massive scenarios are useless if failures only say "stress test failed." Stress helpers will isolate phases, preserve a recent fixed-size action log, and report exact subsystem context. Large scenarios will be decomposed into named campaigns so a failing phase can be reproduced directly.

## 4. Stress tiers

### Tier A — PR stress

Runs on every relevant pull request and should remain practical for normal CI.

Purpose:
- thousands to tens of thousands of operations per subsystem;
- repeated deterministic seeds;
- representative saturation and adversarial edges;
- save/load continuity;
- sanitizer-compatible targeted tests.

Tier A must be strict enough to catch ordinary regressions before merge.

### Tier B — extended soak

Runs from the central stress workflow on schedule and by `workflow_dispatch`.

Purpose:
- dozens of deterministic seeds;
- multi-year simulated campaigns where appropriate;
- high entity counts;
- repeated crisis injection;
- longer save/load cycles;
- bounded-state and leak/runaway checks;
- Linux ASan/UBSan and Windows/MSVC parity.

### Tier C — exhaustive campaign

Manual/dispatchable heavy mode for pre-release and hardening milestones.

Purpose:
- hundreds of seeds;
- very long headless campaigns;
- extreme occupancy/entity/task counts up to supported or intentionally test-defined stress limits;
- repeated adversarial command generation;
- cross-system crisis combinations;
- deterministic replay artifacts for any failure.

Tier C is allowed to be expensive. It is not required on every PR.

## 5. Shared stress harness

Each branch-level suite will follow a common contract even when concrete APIs differ.

The harness will provide:

- stable seed parsing and deterministic bounded sampling;
- scenario phase naming;
- operation counters;
- periodic invariant callbacks;
- authoritative hash comparison helpers;
- a bounded recent-action trace;
- deterministic temporary save locations;
- scale profiles (`pr`, `extended`, `exhaustive`);
- failure output in a consistent one-line summary plus detailed diagnostics.

Environment/CLI configuration will support at minimum:

- `HH_STRESS_SEED` — reproduce one exact seed;
- `HH_STRESS_SCALE` — `pr`, `extended`, or `exhaustive`;
- `HH_STRESS_SCENARIO` — run one named scenario when supported.

The default CTest registration uses deterministic fixed seeds. Scheduled/manual workflows add additional seed matrices.

## 6. Subsystem campaigns

### 6.1 Foundation / simulation clock / command processing

Stress:
- long headless advancement;
- rapid speed/pause transitions where exposed;
- deterministic command bursts;
- repeated entity creation/removal;
- boundary values around tick/day transitions;
- repeated identical run/hash verification.

Reject:
- skipped/duplicated authoritative time transitions;
- state-hash divergence for identical inputs;
- collection growth unrelated to live state;
- hangs or nonterminating command processing.

### 6.2 Construction and building systems — FINAL-01/02 line

Stress:
- dense place/cancel/rebuild cycles;
- large construction queues;
- utility connect/disconnect churn;
- accessibility/elevator/topology changes;
- room validation invalidation/revalidation;
- fire/security/building-state transitions where implemented;
- save/load during partially completed construction.

Adversarial cases include invalid placements, rapid cancellation, capacity saturation, disconnected utilities, and repeated topology mutation.

Reject illegal completed structures, stale validity, orphaned jobs/materials, impossible accessibility state, resource duplication, or continuation divergence.

### 6.3 Guest AI and psychology — FINAL-02 line

Stress:
- all 13 archetypes at scale;
- large simultaneous arrivals/departures;
- group and discretionary-goal selection under congestion;
- need decay/recovery over long stays;
- high-frequency positive/negative experiences;
- memory and complaint generation/expiry;
- loyalty/repeat-intent evolution;
- path/service failures and unavailable-goal fallbacks;
- save/load mid-stay and deterministic continuation.

Reject unbounded memory/complaint growth, invalid need/satisfaction ranges, nondeterministic goal selection, orphaned group references, impossible guest state transitions, or loss of attributable experience causality.

### 6.4 Staff, departments, workforce, scheduler and optimizer

Stress:
- thousands of employees and large task queues at heavy scale;
- shift boundaries, breaks, fatigue, morale, training and absence churn;
- sudden absence spikes;
- department/manager reassignment;
- station coverage saturation;
- adversarial optimizer proposals;
- deterministic fallback scheduler activation;
- repeated rolling-horizon planning.

Reject duplicate employee/task assignment, overlapping exclusive work, unavailable or ineligible assignment, invalid plan metadata, planner/fallback nondeterminism, starvation that violates explicit scheduler priorities, or unbounded pending-plan state.

### 6.5 Service and logistics — FINAL-04

Stress:
- full-hotel checkout/turnover waves;
- saturated housekeeping queues;
- conserved laundry cycles under shortage and surplus;
- receiving-dock and storage-capacity saturation;
- high-frequency internal stock transfers;
- waste generation/removal spikes;
- preventive/corrective maintenance storms;
- room-service order/transport/tray-return bursts;
- efficient vs pathological layouts over long campaigns;
- save/load during active multi-stage jobs.

Reject resource creation/destruction, negative physical inventory, impossible job-stage transitions, orphaned carts/trays/stock moves, stale blocked jobs, or loss of the existing meaningful layout acceptance contract.

### 6.6 F&B, amenities and events — FINAL-05 line

Stress every authoritative FINAL-05 system discovered during implementation, including high request/order/event concurrency, capacity saturation, inventory/service dependency failures, event setup/teardown churn, and guest/service interactions.

The implementation pass must inventory the actual FINAL-05 APIs before coding. Undefined mechanics must not be invented merely to create a stress scenario.

### 6.7 Economy, market, competition and revenue management — FINAL-06

Stress:
- high-volume segmented demand generation;
- large future reservation books;
- sellout and explicit overbooking limits;
- cancellation/no-show storms;
- competitor-rate shocks;
- pricing-rule overlap and priority boundaries;
- contracted-rate preservation;
- commissions and channel mix;
- relocation/recovery under oversell;
- marketing/reputation feedback;
- corporate/group feasibility;
- loans, debt service, covenant distress/default/receivership;
- exact-cent ledger conservation;
- multi-year and decade-scale campaigns;
- repeated save/load and continuation hash checks.

Reject capacity violations beyond explicit allowance, floating/cents drift where exact-cent accounting is authoritative, duplicate bookings, impossible inventory availability, rule-order nondeterminism, ledger imbalance, or unbounded market/reservation history.

### 6.8 In-game UI, management tools and player command routing — FINAL-07

Stress:
- very high snapshot refresh counts;
- repeated panel/overlay open-close and selection churn;
- rapid inspector target changes;
- build preview create/update/cancel loops;
- operations/economy dashboard refresh under changing simulation state;
- alert creation/expiry/deduplication;
- objective/accessibility state transitions;
- large typed command bursts through the application routing boundary;
- stale/invalid command attempts.

Reject UI mutation of simulation authority, stale snapshot references, invalid command acceptance, unbounded alert/history growth, controller state leakage, or crashes after repeated navigation cycles.

### 6.9 Renderer and runtime assets

Stress portable renderer-side logic and Windows renderer smoke paths separately.

Portable stress:
- scene composition with very large instance counts;
- floor visibility mode churn;
- occlusion/visibility extreme inputs;
- camera boundary values;
- runtime asset registry repeated lookup/load lifecycle;
- malformed/truncated/edge-case GLB and cooked-asset inputs where parsers accept external bytes.

Windows stress:
- repeated device/window initialization where safe;
- scene rebuild churn;
- large draw-instance batches;
- smoke launches with packaged assets.

Reject crashes, invalid memory access, integer overflow, stale handles, parser out-of-bounds access, incorrect registry identity, or uncontrolled per-frame/per-rebuild growth.

### 6.10 Save/load, migration and replay

Every branch that owns authoritative save state receives dedicated stress coverage.

Stress:
- repeated save/load cycles;
- saves during active jobs, crises and partial transitions;
- supported older-version migrations;
- large valid state;
- malformed/truncated files where a parser boundary exists;
- deterministic continuation after reload;
- repeated save output stability where canonical serialization is promised.

Reject silent partial loads, dangling references, version misinterpretation, state-hash mismatch after valid round trip, or replay divergence.

### 6.11 Content pipeline and asset production

Stress the shipping/current asset count on the owning branch and the 800-asset target on Asset Pipeline V2 when present.

Stress:
- repeated deterministic generation/cook/validation;
- full-manifest scans;
- dependency-graph depth and fan-out;
- malformed metadata/definition rejection;
- duplicate IDs and missing dependencies;
- repeated fingerprint/hash generation;
- packaged asset audits.

Reject nondeterministic cooked identity where determinism is promised, dependency cycles that should be invalid, duplicate IDs, missing required assets, parser crashes, or package/library count mismatch.

## 7. Cross-system catastrophe campaigns

The highest-value failures often occur at boundaries. Where the owning branch contains all required systems, add combined campaigns such as:

1. **Checkout storm** — near-full occupancy, synchronized departures, housekeeping saturation, laundry shortage, maintenance faults and new arrivals.
2. **Demand shock** — competitor price movement, marketing spike, reservation burst, cancellation/no-show reversal and overbooking recovery.
3. **Workforce shock** — heavy occupancy plus sudden staff absences, optimizer fallback, service queues and guest dissatisfaction pressure.
4. **Infrastructure failure** — active guests and services during utility/elevator/building accessibility disruption on branches where those systems coexist.
5. **Financial crisis** — weak cash position, debt service, demand decline, cancellations and expensive recovery/relocation events.
6. **Save-in-crisis** — serialize and reload at the peak of each supported crisis, then prove deterministic continuation.

A catastrophe scenario must never fabricate a subsystem absent from that branch.

## 8. CI architecture

Add a dedicated `.github/workflows/system-stress.yml` orchestration workflow.

### PR jobs

Branch-local changes run their normal test suites plus Tier A stress targets. Linux sanitizer jobs target the most stateful portable campaigns. Windows/MSVC runs deterministic parity and client/package smoke paths where applicable.

### Extended jobs

A scheduled/manual matrix checks out the authoritative branches and invokes each branch's stress target with additional seeds and `HH_STRESS_SCALE=extended`.

The central workflow records the exact branch SHA for every matrix leg so results remain attributable even while feature branches move.

### Exhaustive jobs

`workflow_dispatch` exposes an exhaustive scale and optional seed/scenario override. Exhaustive failures upload logs/state summaries needed for deterministic reproduction. Binary dumps are optional and must not become required to understand the failure.

## 9. Sanitizers and compiler hardening

Portable C++ stress targets will run with strict warnings and, where compatible with the branch:

- AddressSanitizer;
- UndefinedBehaviorSanitizer;
- leak detection through the sanitizer/runtime available on CI;
- MSVC Release parity for deterministic behavior.

ThreadSanitizer is added only if the exercised code actually has meaningful concurrent execution; it is not included ceremonially for single-threaded simulation code.

## 10. Failure policy

A failing stress test is treated as a product defect or a test-harness defect and investigated to root cause.

Forbidden responses to failure:

- increase tolerances without proving the contract was wrong;
- reduce entity counts solely to hide a scalability defect;
- disable the failing seed;
- skip Windows/Linux parity without a documented platform limitation;
- suppress warnings/sanitizer findings;
- replace authoritative outcomes with proxy metrics;
- remove legitimate gameplay behavior to make the test green.

The smallest correct production fix is preferred. If a stress assumption is invalid, fix the test and document why.

## 11. Completion criteria

This campaign is complete only when:

- every implemented gameplay subsystem listed above has at least one branch-authoritative Tier A stress suite;
- stateful core systems have extended soak coverage;
- deterministic failures are replayable by seed;
- save/load continuation is stressed on every branch that owns save state;
- Linux and Windows deterministic parity gates pass for systems that claim cross-platform determinism;
- sanitizer jobs pass for the selected high-value portable campaigns;
- renderer/runtime assets and content pipeline have dedicated load/fuzz-style stress coverage appropriate to their parsers and registries;
- the central branch-matrix workflow can run all active stacks without merging them;
- baseline normal CTest suites remain green;
- no assertions, fidelity, acceptance contracts, or gameplay rules were weakened to obtain GREEN.

## 12. Non-goals

This task does not:

- merge FINAL stacks;
- reconcile divergent gameplay architectures;
- invent missing FINAL mechanics;
- perform unrelated refactors;
- replace profiling with microbenchmark theater;
- claim hardware-independent frame-rate guarantees from shared CI runners.

If stress testing reveals a structural defect that requires architectural change, implementation stops at that defect boundary long enough to document the root cause and apply the smallest justified correction.