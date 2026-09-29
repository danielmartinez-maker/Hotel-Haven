# Asset Library 4200 Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce and release exactly HH_A001–HH_A4200 with deterministic generation, meaningful variants, animation dependencies, and selective runtime loading.

**Architecture:** The active manifest enumerates 84 shards and their generator families. Existing batches retain their generators; new families use registry dispatch and focused semantic builders. Offline cook, stage, and package gates derive gameplay counts from that manifest.

**Tech Stack:** Python, Trimesh, CMake, C++17, GitHub Actions.

**Spec:** `docs/superpowers/specs/2026-09-28-asset-library-4200-expansion-design.md`

## Global Constraints

- Exactly 4,200 canonical gameplay IDs in 84 batches of 50; animation support assets counted separately.
- Family targets and variant distinctness must pass unchanged quality gates.
- Shipping runtime selectively loads reachable assets and preserves simulation authority.
- No paid external generation or generated binary assets committed.

## Review Focus

- An undeclared shard must never enter generation or animation binding.
- A missing or duplicate declared batch must fail before generation.
- A generated batch with 49 or 51 assets must fail the exact count gate.
- Existing A700 runtime bindings must still resolve after A800 is activated.
- A stale packaged count must fail CI rather than silently publishing.

---

### Task 1: Active manifest and A800 port

**Files:** Create `Tools/ArtGeneration/asset_manifest.py`, `GameData/AssetDefinitions/hotel_haven_asset_manifest_v2.json`, batch 15–16 shards and generators, and focused manifest tests. Modify `Tools/ArtGeneration/generate_all_assets.py`.

**Interfaces:** `load_active_manifest(root) -> AssetManifest`, `AssetManifest.batch_entries`, `batch_path`, `iter_rows`, `asset_count`.

- [ ] Write tests for declared-only enumeration, duplicate/missing batches, and A800 contiguous IDs; run red.
- [ ] Port the A701–A800 semantic factories and data, make generation use the active manifest, and run green.
- [ ] Run asset manifest validation and generation tests; commit.

### Task 2: Generator registry and curated catalog

**Files:** Create focused factory registry and batch 17–84 shards; modify active manifest and generator orchestration.

**Interfaces:** `generator_family` in each batch spec, registry family builder callable with `(name, subcategory, material, asset_id, profile)`.

- [ ] Write tests for 84 exact shards, family targets, unknown generator family, and meaningful variant signatures; run red.
- [ ] Add domain builders and curated data through A4200; run green.
- [ ] Generate all assets, run semantic, geometry, and variant audits; commit.

### Task 3: Animation coverage

**Files:** Modify animation definitions and generators, dependency linker, animation tests.

- [ ] Write tests for mechanical and role-specific humanoid sets; run red.
- [ ] Generate and link all declared dependencies; run green.
- [ ] Verify zero deferred dependencies across the full library; commit.

### Task 4: Cook, stage, CI and package authority

**Files:** Modify `cmake/StageRuntimeAssets.cmake`, asset verification scripts and workflows, renderer audit expectations.

- [ ] Write tests for exact manifest-derived count and missing/extra cooked or staged assets; run red.
- [ ] Wire counts through CMake and CI without weakening audits; run green.
- [ ] Run full generation, cook, stage, renderer smoke, and package checks; commit.
