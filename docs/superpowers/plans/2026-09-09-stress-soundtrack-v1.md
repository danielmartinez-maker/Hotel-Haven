# Soundtrack V1 Stress Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stress the initial Hotel Haven soundtrack catalog, exact asset-integrity contract, missing/unreadable-track handling, sequential loop state, and Windows WinMM/MCI player lifecycle without allowing audio to affect simulation determinism or smoke-test behavior.

**Architecture:** Create `stress/soundtrack-v1` from the current `feature/hotel-soundtrack-v1` head. Add portable catalog/state-machine stress beside existing soundtrack tests and Windows-only player lifecycle stress behind `WIN32`. Stress tests use the existing four exact MP3s and temporary copies/missing paths; they never mutate the checked-in binaries.

**Tech Stack:** C++20, CMake/CTest, existing SoundtrackCatalog/SoundtrackPlayer, WinMM/MCI on Windows, existing `VerifySoundtrackAssets.cmake`, GitHub Actions Ubuntu/Windows.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Branch from the current soundtrack head and record its exact parent SHA.
- Preserve the exact four source MP3 hashes and byte sizes already verified by PR #16.
- Audio is non-authoritative. Stress operations must never change simulation/save/replay state.
- `--smoke-test` must continue to disable music.
- Missing/unreadable tracks are skipped safely and must not block client launch.
- Do not create synthetic replacement tracks or alter volume/sequence product behavior to satisfy tests.
- Honor `HH_STRESS_SEED`, `HH_STRESS_SCALE`, and `HH_STRESS_SCENARIO` for randomized catalog/path ordering tests.
- No merge is part of this plan.

---

## File Structure

- Create: `game/audio/SoundtrackStressTests.cpp`
- Create: `game/audio/SoundtrackPlayerStressTests.cpp`
- Modify: root `CMakeLists.txt` or the soundtrack registration section that currently builds `hh_soundtrack_catalog_tests`/player.
- Modify: `.github/workflows/soundtrack-ci.yml` or create `.github/workflows/soundtrack-stress.yml`; prefer a separate stress workflow if normal soundtrack CI would become materially slower.
- Create: `docs/stress/soundtrack-v1-verification.md`

### Task 1: Stress portable catalog parsing/order/fallback behavior

**Files:**
- Create: `game/audio/SoundtrackStressTests.cpp`
- Modify: soundtrack CMake registration.

**Interfaces:**
- Consumes: `SoundtrackCatalog.h`, `game/data/audio/soundtrack.json`, existing portable catalog tests.
- Produces: executable `hh_soundtrack_stress_tests`, CTest `StressSoundtrackCatalog`.

- [ ] **Step 1: Register the source before creating it and prove RED**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target hh_soundtrack_stress_tests
```

Expected: FAIL because `SoundtrackStressTests.cpp` does not exist.

- [ ] **Step 2: Add repeated catalog load/order checks**

Tier A loads/parses/resolves the catalog at least 10,000 times; extended 100,000; exhaustive 500,000. Every valid load must preserve the approved order:

```text
Morning_in_the_Atrium.mp3
Sunlight_on_Marble.mp3
The_Concierge_Desk.mp3
First_Light_on_Marble.mp3
```

and the existing default volume contract of 35%.

- [ ] **Step 3: Stress missing/unreadable path resolution with temp fixtures**

Create temporary catalog roots where deterministic subsets of the four expected track paths are absent or inaccessible. Catalog/player selection logic must skip unavailable tracks through the existing behavior, retain the relative order of remaining tracks, and never alter authoritative game state.

- [ ] **Step 4: Assert bounded sequencing state**

Simulate at least 100,000 next-track/loop decisions in Tier A using the portable sequencing logic exposed by the catalog/player boundary. The current track index/state must remain within the four-track catalog and loop exactly according to current product semantics; no history collection may grow per transition unless an explicit bounded diagnostic history already exists.

- [ ] **Step 5: Run asset integrity verification alongside stress**

```bash
ctest --test-dir build -C Release -R "(soundtrack|StressSoundtrackCatalog)" --output-on-failure
cmake -P game/audio/VerifySoundtrackAssets.cmake
```

Expected: all four checked-in MP3 hashes/sizes remain exact and stress test passes.

- [ ] **Step 6: Commit**

```bash
git add game/audio/SoundtrackStressTests.cpp CMakeLists.txt
git commit -m "test: stress soundtrack catalog and sequencing"
```

### Task 2: Stress Windows player lifecycle and failure recovery

**Files:**
- Create: `game/audio/SoundtrackPlayerStressTests.cpp`
- Modify: soundtrack CMake registration under `WIN32`.

**Interfaces:**
- Consumes: `SoundtrackPlayer.h` and the same Windows WinMM/MCI implementation already compiled in soundtrack CI.
- Produces: Windows-only CTest `StressSoundtrackPlayer`.

- [ ] **Step 1: Add a Windows-only missing-file RED target**

Non-Windows CMake must not build/link WinMM player stress. Windows build should fail until the new source exists.

- [ ] **Step 2: Exercise repeated open/start/advance/stop/close lifecycle**

Use real checked-in tracks for successful cases and temporary missing paths for failure cases. Tier A performs at least 1,000 safe lifecycle cycles; extended performs at least 10,000. Each cycle must close any MCI alias/device it opens before the next iteration.

- [ ] **Step 3: Verify failure isolation**

For missing/unreadable tracks, player operations must return/continue according to existing skip semantics without throwing across the client boundary, hanging, or leaving an alias that prevents subsequent valid playback.

- [ ] **Step 4: Verify smoke-test disable contract**

Use the existing client/soundtrack setup seam to prove a `--smoke-test` launch never initiates player playback. Keep this as a state/interaction assertion; do not infer it only from absence of audible output.

- [ ] **Step 5: Keep simulation authority unchanged**

Around player lifecycle stress, snapshot/hash the available authoritative simulation fixture before and after audio-only operations. Require equality. If the soundtrack code has no simulation dependency at all, assert that architectural separation through compilation/link dependencies rather than adding a fake simulation callback.

- [ ] **Step 6: Run Windows targeted tests and commit**

```powershell
ctest --test-dir build -C Release -R "(soundtrack|StressSoundtrack)" --output-on-failure
```

Expected: GREEN.

```bash
git add game/audio/SoundtrackPlayerStressTests.cpp CMakeLists.txt
git commit -m "test: stress Windows soundtrack player lifecycle"
```

### Task 3: Add dedicated stress CI and exact verification

**Files:**
- Create: `.github/workflows/soundtrack-stress.yml`
- Create: `docs/stress/soundtrack-v1-verification.md`

**Interfaces:**
- Produces: Ubuntu portable catalog stress, Windows catalog+player stress, exact binary verification, client smoke test.

- [ ] **Step 1: Add Ubuntu portable job**

Configure/build Release, run current soundtrack catalog tests, `StressSoundtrackCatalog`, and `VerifySoundtrackAssets.cmake`. Use `HH_STRESS_SCALE=extended` for scheduled/manual stress and `pr` for PR/push execution.

- [ ] **Step 2: Add Windows player job**

Configure/build Release, run current catalog/player compile/link tests, `StressSoundtrackCatalog`, `StressSoundtrackPlayer`, exact asset verification, and Windows client `--smoke-test`.

- [ ] **Step 3: Verify package still contains exactly four tracks**

When the normal packaging target is available on this branch, inspect the generated package using the same package-verification approach already established by PR #16. Require exactly four MP3 files with these existing SHA-256 values:

```text
Morning_in_the_Atrium.mp3  af78af388491faf33449efad632dfc6d09f53406d3ccf05cbad91dace933e69d
Sunlight_on_Marble.mp3      cc0018a839e3fa9b0f0775b128a9774d8cb5d13802682bda4317b80347199460
The_Concierge_Desk.mp3      89b2308da2b26d58f7186516c592810e83660ec86944707f0885aa94057939b7
First_Light_on_Marble.mp3   06031b117eb1b1af6bd6c7f0cf72d955d07c176f38c06df97e32f577a3e51823
```

Do not update expected hashes unless the project owner explicitly supplies replacement source tracks in a separate content change.

- [ ] **Step 4: Run exact deterministic sequencing replay**

Seed `0xC0FFEE1234ABCDEF` on Ubuntu and Windows portable catalog/sequencing stress. Require identical sequence summaries because selection/order logic is portable; do not compare Windows MCI timing.

- [ ] **Step 5: Record verification**

`docs/stress/soundtrack-v1-verification.md` must record parent SHA, verified child SHA, four asset hashes/sizes, Ubuntu/Windows run IDs, package result, smoke-test result, and replay seed.

- [ ] **Step 6: Commit**

```bash
git add .github/workflows/soundtrack-stress.yml docs/stress/soundtrack-v1-verification.md
git commit -m "ci: verify soundtrack stress suite"
```

## Completion Gate

Complete only when repeated portable catalog/sequence stress, missing-track recovery, Windows WinMM/MCI lifecycle churn, smoke-test disable behavior, exact four-file asset verification, package verification, portable replay parity, and all existing soundtrack tests are GREEN without changing audio content or simulation authority.