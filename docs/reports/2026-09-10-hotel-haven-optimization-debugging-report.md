# HOTEL HAVEN — OPTIMIZATION & DEBUGGING REPORT

## Repository and verification status

Repository: `danielmartinez-maker/Hotel-Haven`  
Branch: `hardening/extensive-optimization-2026-09-10`  
Code-under-test commit: `a18b7b6278605ec386325ad3a62cc90c87bae97b`  
Baseline commit: `17703b9935d48bb20c55b74e62e3d824bc876cf2`  
Final status at the verified source snapshot: **PARTIALLY GREEN**.

The portable authoritative simulation, content pipeline, frontend-core, renderer-core, Release, warning-clean, ASan+UBSan, debug-iterator, save/load, deterministic campaign, and asset-library checks are green. The native Windows client and Direct3D 11 renderer are not buildable in this Linux runner because their repository targets are explicitly `WIN32`-gated and `DirectXMath.h` is unavailable. Consequently, GPU frame time, average FPS, 1% lows, draw calls, native asset-buffer creation, and screenshot/WARP evidence remain unverified. LeakSanitizer is also unavailable under the runner's ptrace restriction; address and undefined-behavior checks pass with leak detection disabled.

No branch was merged and no existing pull request was modified.

## Executive summary

This pass produced nine focused changes supported by regression tests or deterministic measurements:

- prevented closed-room turnover work from being lost or completed against an unavailable room;
- excluded `OutOfOrder` rooms from the occupancy denominator;
- rejected negative guest need-loss definitions;
- changed transient simulation cleanup from an unconditional per-minute scan to correct dirty-state cleanup while preserving load-time cleanup and bounded diagnostic history;
- removed repeated shift/unavailability copying and sorting from each staff-task candidate evaluation;
- bounded hostile `.hasset` dependency counts before allocation;
- made multi-asset cooker state publication one atomic publication after all outputs succeed;
- corrected the documented Hotel Haven Z-up asset-space to renderer-space GLB conversion; and
- removed redundant menu transition retargets and double-applied idle camera offsets.

The most repeatable performance result is the scheduler optimization: with identical plan checksums and assignment counts, the measured fallback-planner time improved by 83.54% at 64 employees/tasks, 94.24% at 256, and 96.31% at 512. The simulation cleanup change removed the profiled `compactTransientState` hot function from the post-change profile; its end-to-end Release timing improvement is positive in the paired measurements but not statistically conclusive for the largest portable tiers on the final rerun.

## Method and scope

The repository was inspected before editing, including root and subsystem CMake files, source ownership, renderer and frontend targets, specifications, Superpowers plans, save/load code, asset generation and cooking scripts, tests, CI workflows, recent Git history, branches, and open GitHub pull requests. The work followed the required evidence → reproduction → root cause → minimal experiment → regression test → fix → verification sequence. Fixes were kept localized; no new authority boundary or speculative subsystem was introduced.

GitHub reconnaissance confirmed:

- authenticated account: `danielmartinez-maker`;
- repository: `danielmartinez-maker/Hotel-Haven`;
- default branch: `main`;
- authenticated permission: admin/push;
- the hardening branch was absent from the remote before handoff; and
- existing PRs #8, #9, #13, #15, #17–#23 remain separate draft/integration or stress branches and were not merged or changed.

## Baseline

### Environment

| Field | Baseline value |
| --- | --- |
| Platform | Linux x86_64, kernel `6.18.35` |
| Compiler | GCC 13.3.0, `/usr/bin/c++` |
| CMake / CTest | CMake/CTest 3.31.10 from the repository's `hh-tools` Python environment |
| Python | 3.12.14 |
| Renderer | Native renderer/frontend targets unavailable on Linux; portable renderer policy is testable |
| Timing | `std::chrono::steady_clock` for in-process benchmarks; Bash `time -p` for campaigns |

### Pre-change build and test state

| Configuration | Result | Evidence |
| --- | --- | --- |
| Baseline Release | 7/7 tests passed | CTest total 2.66 s |
| Baseline debug iterators/assertions | 7/7 tests passed | CTest total 468.58 s |
| Baseline ASan | 0/7 under default leak detection | Environmental failure: “LeakSanitizer does not work under ptrace” |

### Pre-change campaign and profile

The fixed tutorial/restock campaign ran for 365 simulated days twice.

| Metric | Baseline |
| --- | ---: |
| First wall time | 2.28 s |
| Repeat wall time | 2.12 s |
| Repeat output | byte-identical |
| Final CSV row | `365,7839802,27950000,17273198,0.333333,74.0842,954,26,0` |
| Save/load equivalence | pass |
| Save SHA-256 | `4003a8690e800b8ca2def7eae1be339b9a74f7a644693190303eab34cef71a4c` |
| Child maximum RSS | 8448 KiB |

The initial `gprof -pg` profile identified these self-time hotspots:

| Function/region | Self time |
| --- | ---: |
| `Simulation::Impl::guests` | 31.08% |
| `Simulation::Impl::minute` | 23.87% |
| `Simulation::Impl::staffAndTasks` | 19.37% |
| staff assignment lambda | 11.71% |
| `Simulation::Impl::compactTransientState` | 10.36% |
| `Simulation::Impl::neighbors` | 1.80% |
| `Simulation::step` | 1.35% |

The baseline benchmark record is preserved in [`docs/performance/baseline-2026-09-10.json`](../performance/baseline-2026-09-10.json).

## Confirmed defects, root causes, and fixes

### Simulation state and correctness

1. **Closed-room turnover could be consumed while the room was closed.** The task scheduler did not make room closure a blocking condition for queued turnover work. The fix leaves the task unassigned, changes it to `Blocked` with the reason `Room is closed`, and allows deterministic continuation after reopening. A save/load regression test verifies task retention, exact round-trip bytes, and post-reopen completion.

2. **`OutOfOrder` rooms diluted occupancy.** The occupancy calculation counted every non-closed, non-incomplete room as available, including rooms under repair. The denominator now excludes `OutOfOrder`; the regression test fills every genuinely available room and requires occupancy `1.0`.

3. **Negative guest need-loss rates were accepted.** Definition validation rejected some invalid negative rates but omitted hunger and rest loss. Both now reject values below zero while still accepting zero, preventing invalid state from entering the simulation.

4. **Transient cleanup ran on every minute even when nothing was transient.** Completed guests, reservations, and tasks were scanned and compacted unconditionally. Completion/check-out sites now mark transient state dirty; cleanup returns immediately when clean, still runs after load, and resets the dirty flag after compaction. This retains all authoritative history and the bounded 128-item diagnostic task history.

### Staff scheduling

5. **Fallback scheduling repeatedly copied and sorted employee windows and rebuilt blocker vectors for every employee-task candidate.** The root cause was per-candidate preparation work inside the hot nested loop. Employee shift/unavailability windows are now prepared once per plan, and each employee keeps an ordered assignment list. The merge preserves the original tie/order behavior; the added ordering regression test and exact plan checksums confirm equivalence.

### Asset format and cooker

6. **A hostile `.hasset` dependency count could request an oversized allocation before the parser discovered truncation.** The reader now checks the minimum trailing layout and bounds the dependency count by remaining bytes before reserving. The test mutates a valid payload to an infeasible count and requires the diagnostic `hasset dependency count exceeds remaining bytes`.

7. **Multi-asset cooking published intermediate state.** State was written from each individual cook operation, exposing partial results if a later asset failed. State publication is now performed once, atomically, after the complete `cook_all`, `cook_changed`, or runtime cook succeeds. Test-only publication observation proves one publication on success and none on failed multi-asset cooking; the prior valid output is preserved on failed recook.

### Renderer and frontend

8. **GLB coordinates violated the documented asset-space contract.** Hotel Haven assets are authored X/Y/Z with Z up, while the renderer is Y up. The old path only negated Z for handedness. The loader now flattens the node transform and applies one central `(x,y,z) → (x,z,y)` asset-to-renderer conversion, retaining one index-winding reversal. Positions, normals, bounds, alpha, and malformed accessor handling are regression-tested.

9. **Menu input caused redundant transition retargets and the demo double-applied idle camera offsets.** Mouse motion/click now retargets only when the selected item changes. The demo applies the already composed scene-controller yaw and zoom values directly. Deterministic pose tests cover the exact composed values.

## Performance measurements and optimizations

### Portable simulation tiers

The new benchmark target uses fixed seeds, 86,400 simulated seconds, three repetitions per process, and times `Simulation::step` only. Public API constraints intentionally limit the tiers to 6/12/18 rooms and 3/12/100/300 staff; it does not mutate private or serialized authority to fabricate a 500-room/1,000-guest scenario.

The final artifact was run in three fresh Release processes. The table uses the median of each process's three internal samples, then the median across the three process medians. Baseline values are the three-process pre-change medians recorded during the campaign.

| Scenario | Metric | Baseline | Final | Change | Result |
| --- | --- | ---: | ---: | ---: | --- |
| Small: 6 rooms / 3 staff | `Simulation::step`, 86,400 s | 6.473 ms | 5.852 ms | -9.59% | Positive, but process variance is material |
| Normal: 12 rooms / 12 staff | `Simulation::step`, 86,400 s | 15.965 ms | 15.329 ms | -3.98% | Positive, modest |
| Large: 18 rooms / 100 staff | `Simulation::step`, 86,400 s | 51.535 ms | 51.790 ms | +0.49% | Within measurement noise; no win claimed |
| Stress: 18 rooms / 300 staff | `Simulation::step`, 86,400 s | 118.437 ms | 118.676 ms | +0.20% | Within measurement noise; no win claimed |

An immediately paired post-compaction measurement, retained during the change campaign, recorded 5.698/14.905/48.284/112.753 ms for the same four tiers, or -11.97%/-6.64%/-6.31%/-4.80%. The final rerun is reported separately because its large/stress results do not reproduce that magnitude. The stronger claim supported by both profiles is that the unconditional compaction work was eliminated from clean minutes; the final end-to-end large/stress result should be treated as neutral until a lower-noise benchmark environment or larger supported fixture is available.

### Scheduler scaling

This is the clearest repeatable optimization. Old timings were collected from the pre-optimization implementation using the same benchmark fixture; final timings are the fresh final Release artifact. Assignment counts and plan checksums are identical.

| Scenario | Metric | Baseline | Final | Change | Result |
| --- | --- | ---: | ---: | ---: | --- |
| 64 employees / 64 tasks | fallback planner | 0.158 ms | 0.026 ms | -83.54% | pass; checksum `0x4e14af0c44bdcd00` |
| 256 employees / 256 tasks | fallback planner | 5.608 ms | 0.323 ms | -94.24% | pass; checksum `0x6101ecc6c1dcdb4a` |
| 512 employees / 512 tasks | fallback planner | 36.009 ms | 1.328 ms | -96.31% | pass; checksum `0x5752b5cb18840828` |

The post-change `gprof` run measured `compactTransientState` at 0.00% self time. `staffAndTasks` remained the dominant benchmark region at 53.45%, followed by `guests` at 41.38%; the total sampled profile time was effectively unchanged within profiler noise. This is why no unrelated cadence or simulation-fidelity change was made.

### Campaign result

The final Release campaign ran twice at 2.12 s and 2.11 s, compared with the baseline first run of 2.28 s and repeat of 2.12 s. The conservative interpretation is a first-run improvement of 7.02% and a repeat improvement of 0.47%, with both final CSV outputs byte-identical. The final row remains `365,7839802,27950000,17273198,0.333333,74.0842,954,26,0`.

## Memory, lifetime, and diagnostics

- Final child maximum RSS was 8448 KiB for the 365-day campaign, matching the baseline measurement.
- Repeated campaign output and save/load checks showed no state or output growth visible through the available process-level measurement.
- ASan+UBSan completed 8/8 CTest tests with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`; no address or undefined-behavior diagnostic was emitted.
- LeakSanitizer with default leak detection failed before test execution in the baseline environment because it cannot operate under ptrace. No leak-free claim is made.
- `valgrind`, `clang-tidy`, and `cppcheck` are not installed. The repository's portable build was compiled with `-Wall -Wextra -Wpedantic -Werror`; that build passed all targets.
- The portable runtime contains no `std::thread`, `std::jthread`, mutex, condition-variable, or asynchronous simulation/asset worker path. The only new atomic is the test-only cooker publication observer, so a thread sanitizer run was not applicable to an active runtime worker system.

## Simulation, AI, and navigation findings

The authoritative simulation remained one-second deterministic. The fixed-seed 365-day campaign produced identical bytes across repeated runs, and the 14-day save + 7-day load sequence matched a direct 21-day sequence byte-for-byte at the CSV output level and final state.

The layout acceptance test exercised movement and service consequences: efficient versus poor layout produced travel `150/5982` seconds, wait `5124/8172` seconds, satisfaction `65.8667/61.18`, completed stays `40/38`, and operating profit `30000/2000` cents. The benchmark stress tier exercised 300 staff and 86,400 simulated seconds, but the public API does not expose direct large guest-population injection. Therefore no unsupported claim is made for 500 rooms, 1,000 guests, or a GPU-backed navigation workload.

No AI behavior was homogenized, no decision fidelity was reduced, and no navigation timeout was increased. The observed post-change `guests` profile share of 41.38% is a supported next profiling target, not an implemented optimization.

## Renderer and UI findings

The strict portable renderer-core suite passed 18/18 tests covering floor visibility, occlusion, scene composition, asset-format linkage, GLB transforms/material alpha/malformed accessors, and runtime registry handles. The strict portable frontend-core suite passed 34/34 tests covering menu state, transitions, input, stress navigation, layout, and deterministic scene-controller motion.

The native CMake targets are intentionally Windows-only. The Linux runner lacks `DirectXMath.h`, so the full renderer/frontend CMake targets and D3D11 smoke tests could not be configured. No graphical quality, default visibility, animation, shadow, batching, or draw behavior was reduced to obtain the portable results. Native frame time, GPU time, draw-call count, visible-object count, rapid camera movement, WARP buffer creation, and screenshot checks remain open verification items.

## Asset pipeline findings

The Python art-generation suite passed 69 tests. The manifest validator passed with 500 unique production assets across ten 50-asset batches and resolved mappings. In an isolated temporary fixture containing the repository's `GameData` and `Tools` inputs:

- generation produced 500 gameplay GLBs and 591 total generated records, including animation/support records;
- geometry QA passed all 500 assets with `max_faces=3968`, `colors=86`, `nonwhite=1.0`, anchors `184/184`, and contract/placement/semantic checks passing;
- the C++ CLI validated all 591 records;
- runtime cooking produced 500 `.hasset` mesh envelopes;
- a repeat runtime cook completed without changing the valid output; and
- `cook --changed` processed 91 changed/support records and ended with 591 cooked records.

The native renderer's full 500-asset registry/buffer audit could not run on Linux. Corrupt/missing asset error paths are covered by portable registry and parser tests, while native GPU upload and all-asset renderer lifetime coverage remain CI responsibilities.

## Save/load findings

Save format v8 retains integer currency, validates entity references, preserves full reservation/review history, keeps bounded recent-task diagnostics, and migrates v2–v7. The final split-vs-direct check used:

- 14 days with `--restock --save`;
- load that save for 7 days with `--restock`; and
- a direct 21-day `--restock` run.

All three commands exited 0. The split sequence matched the direct sequence, both ended at day 21 with `2091302,1539500,967598,0.166667,73.8591,54,10,0`, and the saved file hash was `4003a8690e800b8ca2def7eae1be339b9a74f7a644693190303eab34cef71a4c`.

## Stress-test matrix coverage

| Dimension | Evidence | Status |
| --- | --- | --- |
| Tiny / normal / large / 300-staff hotel | Portable benchmark tiers | Green |
| Long accelerated simulation | 365-day campaign; repeated byte-identical output | Green |
| Rapid construction/room validity | Simulation regression suite and layout acceptance | Green for exposed API |
| Staff/task pressure | 64/256/512 scheduler scaling; 300-staff simulation tier | Green for exposed API |
| Unreachable/closed/unavailable room states | Closed-room turnover and `OutOfOrder` occupancy regressions | Green |
| Save/load and malformed data | v8 round trip, migration/reference tests, malformed `.hasset` tests | Green for tested formats |
| 500-asset generation/cook | Python QA, 591-record validation, 500 runtime cook | Green |
| Native renderer/GPU/rapid camera | Windows/D3D11 target unavailable here | Unverified |
| 500 rooms / 1,000 guests | No supported public fixture injection | Unverified; no unsupported claim |
| Leak sanitizer / external static analyzers | LSan blocked by ptrace; tools absent | Unverified |

## Tests and final verification commands

The following commands were executed against the final source snapshot. The CMake binary is given explicitly because CMake is not on the container `PATH`.

```bash
cmake -S . -B build-hardening-release-final \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-hardening-release-final --parallel 4
ctest --test-dir build-hardening-release-final --output-on-failure
```

Result: Release build passed; CTest passed 8/8 in 3.34 s.

```bash
cmake -S . -B build-hardening-werror-final \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS=-Werror \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build-hardening-werror-final --parallel 4
```

Result: all portable targets built with warnings treated as errors.

```bash
cmake -S . -B build-hardening-asan-final \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-hardening-asan-final --parallel 4
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
ctest --test-dir build-hardening-asan-final --output-on-failure
```

Result: 8/8 passed in 41.95 s; no ASan/UBSan findings. Leak detection was disabled only because the runner cannot start LSan under ptrace.

```bash
cmake -S . -B build-hardening-debug-iter-final \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS='-D_GLIBCXX_DEBUG -D_GLIBCXX_ASSERTIONS'
cmake --build build-hardening-debug-iter-final --parallel 4
ctest --test-dir build-hardening-debug-iter-final --output-on-failure
```

Result: 8/8 passed. Recorded per-test times sum to approximately 404.5 s; the expensive `hh_game_tests` and `Workforce` tests completed without iterator/assertion failures.

```bash
for i in 1 2 3
do
  build-hardening-release-final/game/hh_simulation_benchmarks \
    > "/tmp/hh-benchmark-final-${i}.json"
done
```

Result: all repetitions were deterministic; simulation checksums were `0x9330f3776492d946`, `0xe2d3ec7750f70f2d`, `0xfe07900e7fe3d39a`, and `0xff7b896af4218d94`. Scheduler plan checksums were stable at all three scales.

```bash
{ time -p build-hardening-release-final/hotel_haven_headless \
    --days 365 --restock > /tmp/hh-final-campaign-1.csv; } \
  2> /tmp/hh-final-campaign-time-1.txt
{ time -p build-hardening-release-final/hotel_haven_headless \
    --days 365 --restock > /tmp/hh-final-campaign-2.csv; } \
  2> /tmp/hh-final-campaign-time-2.txt
sha256sum /tmp/hh-final-campaign-1.csv /tmp/hh-final-campaign-2.csv
cmp -s /tmp/hh-final-campaign-1.csv /tmp/hh-final-campaign-2.csv
```

Result: exit 0, identical SHA-256 `20160db34e021c59f69e0d7077c382cec3efe2fbf64dc492b4d6b57698bcac1d`, final row `365,7839802,27950000,17273198,0.333333,74.0842,954,26,0`, times 2.12 s and 2.11 s.

```bash
git diff --check
git status --short --branch
git diff --stat 17703b9935d48bb20c55b74e62e3d824bc876cf2
```

The portable frontend and renderer-core strict commands were also run directly with `/usr/bin/c++ -std=c++20 -Wall -Wextra -Wpedantic -Werror`; results were 34/34 and 18/18 respectively. The Python art-generation command was `PYTHONPATH=/workspace/scratch/6e1d6f1acb1d/hh-tools/lib/python3.12/site-packages python3 -m pytest Tools/ArtGeneration/tests -q`; it passed 69 tests.

No tests were disabled or weakened. `HH_SKIP_LONG_WORKFORCE_GATES` was not set for the CTest runs.

## Files modified

| File | Change |
| --- | --- |
| `game/src/Simulation.cpp` | transient dirty tracking, closed-room task blocking, occupancy denominator, definition validation |
| `game/tests/SimulationTests.cpp` | negative-rate, room-closure, save/load, and occupancy regressions |
| `game/src/StaffOptimization.cpp` | prepared windows and ordered per-employee assignment blockers |
| `game/tests/OptimizerIntegrationTests.cpp` | ordering-semantics regression |
| `game/benchmarks/SimulationBenchmarks.cpp` | deterministic simulation and scheduler benchmark harness |
| `game/CMakeLists.txt` | benchmark target and CTest registration |
| `Tools/ContentPipeline/src/Hasset.cpp` | bounded hostile dependency parsing |
| `Tools/ContentPipeline/tests/HassetTests.cpp` | infeasible dependency-count regression |
| `Tools/ContentPipeline/src/Cooker.cpp` | one atomic state publication per batch |
| `Tools/ContentPipeline/tests/CookerTests.cpp` | publication and failed-recook regressions |
| `renderer/include/hh/renderer/GlbLoader.h` | explicit Z-up asset contract |
| `renderer/src/GlbLoader.cpp` | centralized asset-to-renderer basis conversion |
| `renderer/tests/GlbLoaderTests.cpp` | transformed geometry/bounds/normal coverage |
| `frontend/app/MainMenuDemo.cpp` | transition guard and composed camera application |
| `frontend/tests/MenuSceneControllerTests.cpp` | composed yaw/zoom regressions |
| `docs/performance/baseline-2026-09-10.json` | machine-readable baseline measurements |
| `README.md` | v8 save/migration and benchmark documentation |
| `docs/IMPLEMENTATION_STATUS.md` | verified campaign and benchmark scope |
| `docs/reports/2026-09-10-hotel-haven-optimization-debugging-report.md` | this report |

## Remaining technical risks

| Priority | Issue | System | Severity | Status | Evidence | Recommended Next Action |
| --- | --- | --- | --- | --- | --- | --- |
| P0 | Native D3D11 client, WARP smoke, screenshots, GPU/frame metrics not run | Renderer/frontend | High | Open | Linux configure is platform-gated; `DirectXMath.h` unavailable | Run Windows CI and a fixed-hardware GPU capture; record CPU/GPU frame time, 1% lows, draw calls, visible objects, and native asset audit |
| P1 | No supported 500-room/1,000-guest benchmark fixture | Simulation/AI/navigation | High | Open | Public API cannot inject initial cash or guest population without mutating authority | Add a documented, authoritative stress-fixture seam only if the product specification permits it, then profile guests/pathfinding at scale |
| P1 | Leak detection not available | Lifetime/memory | Medium | Open | LSan fails under ptrace before tests; Valgrind unavailable | Run ASan+LSan and a heap profiler in a non-ptraced Linux/Windows CI environment |
| P1 | Guest and staff hot paths still dominate | Simulation/AI | Medium | Confirmed hotspot | Post-change `staffAndTasks` 53.45%, `guests` 41.38% in gprof benchmark profile | Capture a supported high-population profile before selecting indexing or cadence changes |
| P2 | Full all-asset native registry/GPU-buffer audit not run | Asset/runtime renderer | Medium | Portable pipeline green; native open | 500 generated GLBs and 591 cooked records pass portable validation; native audit is Windows-only | Run `hh_runtime_asset_library_audit.exe` in the existing Windows workflow and retain logs/artifacts |
| P2 | Static analyzers unavailable in runner | Tooling | Low/Medium | Open | `clang-tidy`, `cppcheck`, and `valgrind` not installed | Add them to CI or run their repository-equivalent configurations on a toolchain host |
| P2 | Final large/stress simulation timing is neutral within noise | Benchmark methodology | Low | Open measurement question | Final 3-process medians were +0.49% and +0.20% despite earlier paired post-change gains | Pin benchmark CPU affinity or use a dedicated runner, then repeat with a supported larger fixture |

## CI status

Local Linux CI-equivalent portable verification is green. Repository workflows define separate Ubuntu/Windows jobs for game, frontend, renderer, content pipeline, runtime asset cooking/audit, WARP smoke, and packaging. The Windows jobs were not executable in this environment. Draft PR [#24](https://github.com/danielmartinez-maker/Hotel-Haven/pull/24) is open; at the final GitHub status poll its head had no reported workflow runs or commit status entries yet, so remote CI is **PENDING**, not claimed green.

## Final commit and working-tree record

The source snapshot tested before report-only files were added was `a18b7b6278605ec386325ad3a62cc90c87bae97b`. The local report handoff was first committed as `af4ec05` before the authenticated GitHub mirror update. The normal Git push was unavailable because this runner has no Git HTTPS credential; the authenticated GitHub connector published the equivalent tree and report-only update.

- local report handoff commit before the mirror-only update: `af4ec05`;
- remote branch initial code/report head: `hardening/extensive-optimization-2026-09-10` at `dbdd8d1af616ec4a241818e46826cd87412f4d3f`;
- first remote report-only mirror commit: `a543b57218bbadbeaa3f524f10312c8571b27f29`; and
- draft PR: [#24](https://github.com/danielmartinez-maker/Hotel-Haven/pull/24), open against `main`, with no merge performed.

## RECOMMENDED NEXT OPTIMIZATION TARGETS

1. Profile the `guests` region at a supported high-population fixture. It accounts for 41.38% self time in the post-change benchmark profile, but the current public API cannot safely create the requested 1,000-guest scale without an explicit authority-preserving fixture.

2. Break down `staffAndTasks` beyond the optimized planner. It remains 53.45% self time in the benchmark profile; the next smallest diagnostic step is a region-level profile separating task discovery, eligibility filtering, movement, and completion before changing data structures or update cadence.

3. Run native Windows renderer profiling on a fixed WARP/GPU environment. Portable scene composition is green, but the highest-impact unknowns are GPU submission, asset-buffer creation, visibility, batching, and UI composition under dense hotels.

4. Execute the existing 500-asset native runtime audit and capture memory/resource lifetime over repeated load/unload cycles. Portable generation/cooking is green; the missing evidence is renderer-owned resource behavior.
