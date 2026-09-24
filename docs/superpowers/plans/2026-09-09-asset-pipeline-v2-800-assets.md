# Hotel Haven Asset Pipeline V2 — 800-Asset Quality Expansion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand Hotel Haven from 500 to 800 gameplay-facing assets while applying a stricter, data-driven quality bar across the entire library and preserving the cooked `.hasset` runtime contract.

**Architecture:** Keep the existing deterministic Python authoring pipeline and C++ cooked runtime. Introduce an active V2 manifest and quality contract, make generation/preview/validation manifest-driven, add reusable family factories for six new batches, and validate all 800 assets through one structural/geometry/material/semantic/variant/preview gate. Meshy remains an optional offline authoring source whose outputs must enter through the same sidecar/quality/cook path.

**Tech Stack:** Python 3, trimesh, numpy, Pillow/matplotlib preview tooling, JSON manifests/contracts, C++20 content pipeline, GitHub Actions, existing `.hasset` cooker/runtime registry.

**Spec:** `docs/superpowers/specs/2026-09-09-asset-pipeline-v2-800-assets-design.md`

## Global Constraints

- Preserve logical IDs `HH_A001` through `HH_A500`; add exactly `HH_A501` through `HH_A800`.
- Active production library contains exactly 800 gameplay-facing assets in 16 batches of 50.
- Preserve Architectural Diorama Realism V1.
- Preserve deterministic generation and deterministic `.hasset` cooking.
- Shipping executables must not gain Python, Blender, CUDA, Omniverse, OpenUSD, or Meshy dependencies.
- Simulation, room, economy, placement, reachability, task, and interaction authority remain in owning gameplay systems.
- Generic placeholder geometry is release-blocking unless explicitly allowed by a family contract.
- Quality gates are explicit blockers; no aggregate score may override a blocker.
- Meshy is authoring-only. Meshy-generated GLB/FBX/OBJ content must be normalized into the same asset sidecar and validation flow before cook.

---

### Task 1: Add the V2 manifest and declarative quality contract

**Files:**
- Create: `GameData/AssetDefinitions/hotel_haven_asset_manifest_v2.json`
- Create: `GameData/AssetDefinitions/asset_quality_contract_v2.json`
- Create: `GameData/AssetDefinitions/Manifest/asset_batch_11.json`
- Create: `GameData/AssetDefinitions/Manifest/asset_batch_12.json`
- Create: `GameData/AssetDefinitions/Manifest/asset_batch_13.json`
- Create: `GameData/AssetDefinitions/Manifest/asset_batch_14.json`
- Create: `GameData/AssetDefinitions/Manifest/asset_batch_15.json`
- Create: `GameData/AssetDefinitions/Manifest/asset_batch_16.json`
- Test: `Tools/ArtGeneration/tests/test_asset_pipeline_v2_manifest.py`

**Interfaces:**
- Produces: master manifest keys `asset_count`, `batches`, `profiles`, `quality_contract`; batch rows retain the V1 seven-column tuple contract.
- Produces: quality contract layers `profiles`, `families`, `variant_groups`, `material_families`, `preview`, `meshy_authoring`.

- [ ] Write tests asserting 800 unique IDs, 16 batches, 50 rows per batch, continuity from `HH_A001` to `HH_A800`, valid profile references, and a resolvable quality-contract path.
- [ ] Run `python -m pytest Tools/ArtGeneration/tests/test_asset_pipeline_v2_manifest.py -v`; expect RED because V2 data does not exist.
- [ ] Add V2 master/quality manifests and batches 11–16 with 300 concrete assets aligned to the approved batch themes.
- [ ] Re-run the test and require GREEN.
- [ ] Commit as `feat(assets): define 800-asset v2 production manifest`.

### Task 2: Make generation scope manifest-driven

**Files:**
- Create: `Tools/ArtGeneration/asset_manifest.py`
- Modify: `Tools/ArtGeneration/generate_all_assets.py`
- Modify: `Tools/ArtGeneration/link_animation_dependencies.py`
- Test: `Tools/ArtGeneration/tests/test_generate_all_assets.py`
- Test: `Tools/ArtGeneration/tests/test_asset_pipeline_v2_manifest.py`

**Interfaces:**
- `load_active_manifest(repo_root: Path) -> AssetManifest`
- `AssetManifest.batch_numbers: tuple[int, ...]`
- `AssetManifest.asset_count: int`
- `AssetManifest.batch_path(batch: int) -> Path`
- `AssetManifest.profile(profile_id: str) -> dict`

- [ ] Add failing tests proving generation is not controlled by `range(1, 11)`, literal `500`, or globbing stale batch files outside the active manifest.
- [ ] Run targeted tests and confirm RED.
- [ ] Implement the manifest loader and route generation, metadata normalization, animation-link expectations, and generated-record totals through it.
- [ ] Re-run targeted tests and require GREEN.
- [ ] Commit as `refactor(assets): drive generation from active manifest`.

### Task 3: Replace selected hard-coded quality checks with a V2 quality engine

**Files:**
- Create: `Tools/ArtGeneration/quality_contract_v2.py`
- Modify: `Tools/ArtGeneration/semantic_asset_quality.py`
- Modify: `Tools/ArtGeneration/validate_generated_geometry.py`
- Modify: `Tools/ArtGeneration/render_asset_previews.py`
- Test: `Tools/ArtGeneration/tests/test_quality_contract_v2.py`
- Test: `Tools/ArtGeneration/tests/test_batches_08_10_and_release_completion.py`

**Interfaces:**
- `load_quality_contract(repo_root: Path, manifest: AssetManifest) -> dict`
- `quality_failures(asset_meta: dict, metrics: dict, contract: dict) -> list[str]`
- `variant_failures(asset_metrics: dict[str, dict], contract: dict) -> list[str]`

- [ ] Add RED tests for placeholder `Body` rejection, missing family semantic nodes, face-budget overflow, floor/pivot violations, disallowed materials, missing interaction anchors, and duplicate required variants.
- [ ] Implement contract-driven profile/family checks while retaining V1 compatibility helpers for old tests.
- [ ] Add preview occupancy diagnostics and deterministic silhouette/perceptual signatures.
- [ ] Run targeted art-generation tests and require GREEN.
- [ ] Commit as `feat(assets): enforce v2 production quality contract`.

### Task 4: Apply a reusable geometry/material quality layer to all generated assets

**Files:**
- Create: `Tools/ArtGeneration/quality_enrichment.py`
- Modify: `Tools/ArtGeneration/generate_all_assets.py`
- Modify: `Tools/ArtGeneration/service_asset_factory.py`
- Modify: `Tools/ArtGeneration/amenity_decor_factory.py`
- Modify: `Tools/ArtGeneration/character_factory.py`
- Test: `Tools/ArtGeneration/tests/test_quality_enrichment.py`

**Interfaces:**
- `enrich_generated_asset(asset_path: Path, sidecar_path: Path, asset_meta: dict, contract: dict) -> dict`
- Returned metrics include `material_slots`, `semantic_nodes`, `bounds`, `floor_offset`, `quality_revision`.

- [ ] Add RED tests that representative V1 assets from architecture, furniture, service, amenity, prop, and character profiles receive V2 quality metadata without changing their logical IDs or gameplay bindings.
- [ ] Implement deterministic sidecar enrichment: quality revision, family/profile tags, material-response normalization metadata, semantic-role tags, and authored-source provenance.
- [ ] Upgrade shared factories so repeated furniture/service/amenity/character assets expose stronger silhouettes, small consistent manufactured-edge bevel geometry where appropriate, and role-specific semantic subcomponents within profile budgets.
- [ ] Re-run existing batch tests plus the enrichment tests.
- [ ] Commit as `feat(assets): raise shared asset geometry and material quality`.

### Task 5: Implement production factories for Batches 11–15

**Files:**
- Create: `Tools/ArtGeneration/architecture_factory.py`
- Create: `Tools/ArtGeneration/guestroom_factory.py`
- Create: `Tools/ArtGeneration/public_space_factory.py`
- Create: `Tools/ArtGeneration/food_beverage_factory.py`
- Extend: `Tools/ArtGeneration/service_asset_factory.py`
- Create: `Tools/ArtGeneration/batch11_generate.py`
- Create: `Tools/ArtGeneration/batch12_generate.py`
- Create: `Tools/ArtGeneration/batch13_generate.py`
- Create: `Tools/ArtGeneration/batch14_generate.py`
- Create: `Tools/ArtGeneration/batch15_generate.py`
- Test: `Tools/ArtGeneration/tests/test_batches_11_15.py`

**Interfaces:**
- Each batch module exposes `generate_package(manifest_path: Path, out_dir: Path, source_path: str) -> list[Path]`.
- Family factories return deterministic trimesh scenes plus semantic node names and sidecar-compatible material metadata.

- [ ] Add RED tests asserting each new batch generates exactly 50 paired exports/sidecars with unique geometry signatures and required family semantics.
- [ ] Implement architecture/elevator/circulation, guest-room/bathroom, lobby/public-space, F&B/banqueting, and housekeeping/engineering/logistics families using reusable primitives instead of per-asset placeholder boxes.
- [ ] Enforce floor contact and profile budgets during generation.
- [ ] Run the new batch tests and quality-contract tests.
- [ ] Commit as `feat(assets): generate v2 batches 11 through 15`.

### Task 6: Implement Batch 16 amenities/exterior/human variety

**Files:**
- Extend: `Tools/ArtGeneration/amenity_decor_factory.py`
- Extend: `Tools/ArtGeneration/character_factory.py`
- Create: `Tools/ArtGeneration/batch16_generate.py`
- Test: `Tools/ArtGeneration/tests/test_batch16_v2.py`

**Interfaces:**
- Uses the same `generate_package(...)` batch contract.
- Character variants remain compatible with the existing humanoid skeleton/animation-set dependencies.

- [ ] Add RED tests for 50 assets, character skeleton compatibility, accessory differentiation, exterior/weather-compatible material families, and zero missing animation dependencies.
- [ ] Implement amenity/exterior/accessibility/luggage variants and additional staff/guest presentation variants.
- [ ] Run Batch 16 plus humanoid/animation dependency tests.
- [ ] Commit as `feat(assets): add v2 amenities exterior and character variety`.

### Task 7: Add the Meshy authoring adapter contract without runtime coupling

**Files:**
- Create: `GameData/AssetDefinitions/meshy_authoring_contract_v1.json`
- Create: `Tools/ArtGeneration/meshy_import_adapter.py`
- Create: `docs/art/MESHY_AUTHORING_GUIDE.md`
- Test: `Tools/ArtGeneration/tests/test_meshy_import_adapter.py`

**Interfaces:**
- `normalize_meshy_asset(source_model: Path, target_asset_id: str, manifest: AssetManifest, output_dir: Path) -> tuple[Path, Path]`
- Adapter validates source extension, logical ID, profile/family, scale, floor contact, material count, provenance, and sidecar metadata before returning an importable source/sidecar pair.

- [ ] Add RED tests proving a Meshy-authored model cannot bypass V2 quality or cooked-runtime validation.
- [ ] Implement authoring metadata fields for Meshy task ID, source format, PBR enablement, intended family, and quality-review state; never serialize Meshy/API credentials.
- [ ] Document the recommended Meshy workflow: image/text design -> GLB -> normalization -> V2 QA -> `.hasset` cook.
- [ ] Run adapter/quality tests.
- [ ] Commit as `feat(assets): add meshy authoring normalization contract`.

### Task 8: Make previews and release audit fully V2/manifest-driven

**Files:**
- Modify: `Tools/ArtGeneration/render_asset_previews.py`
- Modify: `Tools/ArtGeneration/validate_generated_geometry.py`
- Create/Update: `Art/Validation/library_release_audit_v2.md`
- Test: `Tools/ArtGeneration/tests/test_preview_release_v2.py`

**Interfaces:**
- Preview QC expected batch/asset totals come exclusively from the active manifest.
- Release audit emits per-gate counts, 16 batch totals, and deterministic hashes.

- [ ] Add RED tests for 16 preview boards, 800 nonblank previews, occupancy bounds, and deterministic variant signatures.
- [ ] Remove hard-coded 10-batch/500-asset assumptions from preview and audit logic.
- [ ] Generate release-audit metrics from the quality engine.
- [ ] Run preview/release tests.
- [ ] Commit as `feat(assets): validate 800-asset v2 visual release`.

### Task 9: Preserve deterministic cook/runtime behavior at 800 assets

**Files:**
- Modify only if needed: `Tools/ContentPipeline/scripts/validate_asset_library.py`
- Modify only if needed: `.github/workflows/content-pipeline-ci.yml`
- Add/Modify: renderer/runtime asset tests already used by cooked-runtime CI

**Interfaces:**
- Cooked runtime continues to consume `.hasset` only.
- Runtime registry remains catalog-size agnostic.

- [ ] Add RED checks for active-manifest count 800 and representative cooked assets from batches 11–16.
- [ ] Update authoring/release validators to derive expected scope from V2 manifest rather than literal 500.
- [ ] Keep runtime registry/container format unchanged unless a failing test demonstrates a real compatibility gap.
- [ ] Run native content-pipeline, runtime registry, and representative renderer tests.
- [ ] Commit as `test(assets): verify v2 cook and runtime compatibility`.

### Task 10: Full RED→GREEN verification and release record

**Files:**
- Update: `Art/Validation/library_release_audit_v2.md`
- Update: `docs/superpowers/plans/2026-09-09-asset-pipeline-v2-800-assets.md` checkboxes only after evidence exists.

- [ ] Run the complete Python art-generation suite.
- [ ] Generate the full 800-asset library from a clean `Art/Exports` state.
- [ ] Run geometry/material/semantic/interaction/animation/variant/preview QC.
- [ ] Run native content-pipeline tests and deterministic cook checks.
- [ ] Run cooked-runtime registry/renderer smoke tests.
- [ ] Confirm exactly 800 gameplay assets, 16x50 batches, zero deferred animation dependencies, zero missing generated dependencies, and zero unresolved release blockers.
- [ ] Record final commit SHA, CI runs, asset/record counts, max face count, preview pass counts, and artifact hash in `library_release_audit_v2.md`.
- [ ] Commit as `docs(assets): record v2 800-asset release verification`.
