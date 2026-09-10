# Renderer and Content Pipeline Stress Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stress Hotel Haven's portable renderer logic, runtime asset registry/load boundaries, GLB parser, scene composition/visibility, and C++ content pipeline under large inputs, malformed inputs, repeated lifecycle churn, and sanitizer instrumentation.

**Architecture:** Create `stress/renderer-content` from the current `main` head. Keep renderer stress inside `renderer/tests/` using the existing test framework and content-pipeline stress inside `Tools/ContentPipeline/tests/`. Parser tests generate deterministic in-memory/temp-file corpora rather than checking in huge fuzz corpora. Windows D3D11 receives lifecycle/smoke churn, while portable parser/registry/scene tests run Ubuntu ASan+UBSan and Windows Release.

**Tech Stack:** C++20, renderer test framework, ContentPipeline test framework, CMake/CTest, D3D11/Win32 on Windows, ASan+UBSan on Ubuntu.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Branch from current `main`, record SHA, and do not modify renderer/content feature branches directly.
- Do not retrieve or mutate GPU state from portable tests; Windows-specific D3D11 stress stays Windows-only.
- Malformed input tests must verify clean rejection, never reinterpret rejection as a valid asset.
- No unchecked integer arithmetic on synthetic size/offset fields in test builders.
- Stress inputs are deterministic and replayable; parser campaigns print seed/case index on failure.
- Do not loosen parser validation or asset identity rules to accept malformed test data.
- No merge is part of this plan.

---

## File Structure

- Create: `renderer/tests/RendererStressTests.cpp`
- Create: `renderer/tests/GlbLoaderStressTests.cpp`
- Create: `renderer/tests/RuntimeAssetRegistryStressTests.cpp`
- Create: `renderer/tests/D3D11LifecycleStressTests.cpp`
- Modify: `renderer/CMakeLists.txt`
- Create: `Tools/ContentPipeline/tests/ContentPipelineStressTests.cpp`
- Modify: `Tools/ContentPipeline/CMakeLists.txt`
- Create: `.github/workflows/renderer-content-stress.yml`
- Create: `docs/stress/renderer-content-verification.md`

### Task 1: Stress portable scene/camera/floor/visibility logic

**Files:**
- Create: `renderer/tests/RendererStressTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**
- Consumes: `Camera`, `FloorVisibility`, `Occlusion`, `SceneComposer`, `Visibility`, existing `RenderScene` structures.
- Produces: CTest `StressRendererCore`.

- [ ] **Step 1: Register missing-file test and prove RED**

Add the source to a dedicated stress test executable using the same test framework/library dependencies as existing renderer tests, then run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target hh_renderer_stress_tests
```

Expected: FAIL until `RendererStressTests.cpp` exists.

- [ ] **Step 2: Generate large deterministic scenes**

Required phases:

```text
camera_extremes
floor_mode_churn
visibility_saturation
occlusion_extremes
scene_rebuild_churn
```

Tier A builds/rebuilds scenes totaling at least 250,000 instances across cycles; extended at least 2,500,000; exhaustive at least 10,000,000 cumulative instances. Keep any single scene within existing container/index type limits.

- [ ] **Step 3: Assert renderer-core invariants**

Require finite transforms/camera outputs for valid finite inputs, floor visibility results only reference valid floors/instances, visible indices are in range, stable asset handles remain stable across composition, and repeated scene rebuilds do not retain references to prior destroyed input storage.

- [ ] **Step 4: Run baseline plus stress**

```bash
ctest --test-dir build -C Release -R "(Camera|FloorVisibility|Occlusion|SceneComposer|Visibility|StressRendererCore)" --output-on-failure
```

Expected: GREEN.

- [ ] **Step 5: Commit**

```bash
git add renderer/tests/RendererStressTests.cpp renderer/CMakeLists.txt
git commit -m "test: stress portable renderer core"
```

### Task 2: Stress GLB parser boundaries

**Files:**
- Create: `renderer/tests/GlbLoaderStressTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**
- Consumes: existing `GlbLoader` byte/file interface used by `GlbLoaderTests.cpp`.
- Produces: CTest `StressGlbLoader`.

- [ ] **Step 1: Add deterministic corpus builder**

Build valid minimal GLB bytes using the same format expectations as current tests, then derive deterministic mutations:

```text
truncated_header
wrong_magic
oversized_declared_length
truncated_json_chunk
truncated_bin_chunk
chunk_length_overflow
invalid_accessor_bounds
invalid_buffer_view_bounds
huge_but_valid_metadata_count
repeated_valid_load
```

Each case records seed and mutation index.

- [ ] **Step 2: Prove RED with at least one new boundary case before fixes**

Wire the test and run under ASan+UBSan. If all current parser cases reject safely, RED may be the intentionally missing test source/target; do not manufacture a product bug.

- [ ] **Step 3: Execute large mutation campaigns**

Tier A: 10,000 deterministic mutations. Extended: 100,000. Exhaustive: 1,000,000. Valid inputs must either load to internally valid bounded structures or fail only when the format is actually invalid; malformed inputs must fail without crash, OOB access, integer overflow, or runaway allocation.

- [ ] **Step 4: Fix only demonstrated parser defects**

If sanitizer or invariant failure identifies a production defect in `renderer/src/GlbLoader.cpp`, add the smallest checked arithmetic/bounds validation necessary and keep the failing seed as a regression case.

- [ ] **Step 5: Commit after GREEN**

```bash
git add renderer/tests/GlbLoaderStressTests.cpp renderer/CMakeLists.txt renderer/src/GlbLoader.cpp
git commit -m "test: harden GLB loader under stress"
```

If no production change is required, omit `renderer/src/GlbLoader.cpp` from the commit.

### Task 3: Stress runtime asset registry lifecycle

**Files:**
- Create: `renderer/tests/RuntimeAssetRegistryStressTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**
- Consumes: `RuntimeAssetRegistry`, stable `AssetHandle` identity, existing cooked runtime asset audit semantics.
- Produces: CTest `StressRuntimeAssetRegistry`.

- [ ] **Step 1: Generate repeated register/lookup/rebuild cycles**

Tier A executes at least 100,000 lookup/lifecycle operations across the shipping asset set or deterministic synthetic registry fixtures supported by the current API. Extended executes 1,000,000.

- [ ] **Step 2: Require identity and bounds invariants**

Same valid asset identity must resolve consistently; missing identities fail cleanly; no duplicate identity silently aliases a different asset; repeated rebuild/load cycles must not grow registry cardinality without corresponding live unique assets.

- [ ] **Step 3: Stress malformed cooked metadata boundary where registry parses it**

Only mutate bytes/metadata at the layer that actually accepts external data. If parsing belongs to ContentPipeline/Hasset rather than registry, keep malformed-data coverage in Task 4 and do not duplicate authority.

- [ ] **Step 4: Commit**

```bash
git add renderer/tests/RuntimeAssetRegistryStressTests.cpp renderer/CMakeLists.txt
git commit -m "test: stress runtime asset registry lifecycle"
```

### Task 4: Stress the C++ content pipeline

**Files:**
- Create: `Tools/ContentPipeline/tests/ContentPipelineStressTests.cpp`
- Modify: `Tools/ContentPipeline/CMakeLists.txt`

**Interfaces:**
- Consumes: Catalog, Cooker, DependencyGraph, Fingerprint, Hasset, Json, Metadata APIs.
- Produces: CTest `StressContentPipeline`.

- [ ] **Step 1: Register the new test source and prove RED**

Use the existing ContentPipeline test executable/framework pattern. Build must fail before the source is created.

- [ ] **Step 2: Add deterministic graph/metadata/parser campaigns**

Required phases:

```text
large_acyclic_dependency_graph
deep_dependency_chain
high_fanout_graph
duplicate_id_rejection
cycle_rejection
missing_dependency_rejection
metadata_roundtrip
hasset_roundtrip
truncated_hasset
json_boundary_mutation
fingerprint_repeatability
cook_repeatability
```

Tier A uses at least 10,000 generated asset definitions/operations cumulatively; extended uses at least 100,000.

- [ ] **Step 3: Verify deterministic fingerprints/cooked identity**

For identical canonical input, repeated fingerprint/cook operations must return identical identity/bytes where existing pipeline semantics promise determinism. If output intentionally embeds non-authoritative path/time data, compare only the canonical deterministic payload already defined by production.

- [ ] **Step 4: Enforce clean rejection**

Cycles, duplicate IDs, missing dependencies, invalid metadata, and truncated Hasset/JSON inputs must report failure through existing error APIs and never crash or create partial accepted catalog state.

- [ ] **Step 5: Run all content baselines plus stress**

```bash
ctest --test-dir build -C Release -R "(Content|Cooker|DependencyGraph|Fingerprint|Hasset|Metadata|StressContentPipeline)" --output-on-failure
```

Use the actual registered CTest names when the existing suite naming differs; do not rename baseline tests solely for this command.

- [ ] **Step 6: Commit**

```bash
git add Tools/ContentPipeline/tests/ContentPipelineStressTests.cpp Tools/ContentPipeline/CMakeLists.txt
git commit -m "test: stress content pipeline parsers and graph"
```

### Task 5: Add Windows D3D11 lifecycle stress

**Files:**
- Create: `renderer/tests/D3D11LifecycleStressTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**
- Consumes: same Windows renderer/window initialization and smoke fixture used by `D3D11SmokeTests.cpp`.
- Produces: Windows-only CTest `StressD3D11Lifecycle`.

- [ ] **Step 1: Add missing-source RED target under `WIN32`**

Ensure non-Windows configure does not reference or require this source.

- [ ] **Step 2: Repeat safe lifecycle operations**

Perform repeated renderer/device/window initialization, resize/rebuild paths supported by the existing smoke fixture, scene uploads, and teardown. Tier A uses at least 100 lifecycle cycles unless D3D11 fixture constraints require a lower stable count; any lower count must be justified by an existing platform/API limitation, not runtime slowness alone.

- [ ] **Step 3: Keep timing non-authoritative**

Use only workflow/CTest timeout to detect hangs. Do not assert a frame-rate threshold on shared CI hardware.

- [ ] **Step 4: Commit**

```bash
git add renderer/tests/D3D11LifecycleStressTests.cpp renderer/CMakeLists.txt
git commit -m "test: stress D3D11 renderer lifecycle"
```

### Task 6: Add sanitizer/Windows CI and verification record

**Files:**
- Create: `.github/workflows/renderer-content-stress.yml`
- Create: `docs/stress/renderer-content-verification.md`

- [ ] **Step 1: Ubuntu ASan+UBSan extended job**

Run `StressRendererCore`, `StressGlbLoader`, `StressRuntimeAssetRegistry`, and `StressContentPipeline` with `HH_STRESS_SCALE=extended`. Set sanitizer halt-on-error and leak detection.

- [ ] **Step 2: Windows Release job**

Run portable stress targets plus `StressD3D11Lifecycle`, normal renderer tests, runtime asset audit after cooking assets, and ContentPipeline baselines.

- [ ] **Step 3: Exact replay**

Run portable deterministic corpus/scene tests with `HH_STRESS_SEED=0xC0FFEE1234ABCDEF` on Ubuntu and Windows and require matching deterministic summaries for logic that promises platform parity.

- [ ] **Step 4: Record verification and commit**

```bash
git add .github/workflows/renderer-content-stress.yml docs/stress/renderer-content-verification.md
git commit -m "ci: verify renderer and content stress suite"
```

## Completion Gate

Complete only when large scene/visibility churn, 100k+ extended parser mutations, registry lifecycle stress, content dependency/parser stress, Ubuntu sanitizers, Windows D3D11 lifecycle, cooked asset audit, and all existing renderer/content tests are GREEN without relaxing input validation.