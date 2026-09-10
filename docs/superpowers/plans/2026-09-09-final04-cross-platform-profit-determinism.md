# FINAL-04 Cross-Platform Profit Determinism Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the long layout campaign's real operating-profit acceptance criterion deterministic across MSVC and Linux without weakening the existing layout consequences contract, then complete the Windows release verification chain.

**Architecture:** Keep `std::mt19937_64` as the authoritative serialized engine, but stop feeding it through implementation-defined standard-library adapters in the booking path. Use explicit, fixed engine-to-unit-double conversion and Fisher-Yates room ordering so identical seeds consume identical engine values on MSVC and libstdc++; retain actual `EconomyView` revenue/cost accounting and the existing profit comparison.

**Tech Stack:** C++20, CMake/CTest, GitHub Actions, MSVC 19.51, GCC 13.

**Spec:** `HMG_FINAL_04_SERVICE_LOGISTICS(1).md` / existing FINAL-04 implementation and layout acceptance tests.

## Global Constraints

- Do not weaken or delete the travel, wait, satisfaction, throughput, viability, or operating-profit acceptance behavior.
- Preserve the existing `std::mt19937_64` simulation engine and save/load continuation contract.
- Do not merge PR #12.
- FINAL-04 is complete only after Windows CTest, client smoke launch, packaging, and packaged cooked-asset verification all pass on the final head.

---

### Task 1: Remove implementation-dependent RNG adapters from booking

**Files:**
- Modify: `game/src/Simulation.cpp`
- Test: `game/tests/SimulationTests.cpp`

**Interfaces:**
- Consumes: `Simulation::Impl::rng` (`std::mt19937_64`).
- Produces: fixed booking room order and fixed `[0,1)` booking draw from the same engine state on all supported standard libraries.

- [ ] **Step 1: Preserve the existing RED evidence**

Run the current PR head on Windows through CTest. Expected failure is `hh_game_tests` with the long layout campaign reporting equal efficient/poor operating profit and `poor layout did not reduce operating profit`.

- [ ] **Step 2: Add fixed RNG adapters**

In `Simulation.cpp`, add a fixed unit-double conversion using the upper 53 bits of one `mt19937_64` draw and a deterministic Fisher-Yates shuffle whose index selection is defined directly from engine output. No `std::shuffle`, `std::generate_canonical`, or standard distribution object may remain in `hourlyBookings`.

- [ ] **Step 3: Keep the real profit assertion unchanged**

`SimulationTests.cpp` must continue to assert both `efficient.operatingProfitCents > 0` and `poor.operatingProfitCents < efficient.operatingProfitCents`. Do not replace profit with a proxy or add a tolerance.

- [ ] **Step 4: Run targeted and integrated Linux verification**

Run `hh_game_tests`, FINAL-04 tests, and full CTest. Expected: all pass.

- [ ] **Step 5: Commit**

Commit the deterministic RNG adapter change on `feature/final-04-service-logistics` without merging PR #12.

### Task 2: Complete Windows release verification

**Files:**
- No production changes expected unless verification exposes a separate root cause.

**Interfaces:**
- Consumes: final Task 1 branch head.
- Produces: fresh Windows evidence for the complete shipping path.

- [ ] **Step 1: Run Windows Release configure/build and CTest**

Expected: all integrated CTest entries pass, including `hh_game_tests` and all FINAL-04 suites.

- [ ] **Step 2: Run the Windows client smoke launch**

Expected: `hotel_haven.exe --smoke-test` exits successfully and generates/validates its smoke artifact as required by the existing workflow.

- [ ] **Step 3: Package**

Run the existing CPack/package step. Expected: package creation succeeds.

- [ ] **Step 4: Verify packaged cooked assets**

Run the existing packaged-asset verifier against the generated package. Expected: all required cooked assets are present and valid.

- [ ] **Step 5: Mark FINAL-04 complete only after Steps 1-4 succeed**

Update PR #12 status/body only after the final head has fresh green evidence for every gate. Keep the PR draft/unmerged unless separately instructed.
