# Hotel Haven 500 Asset Library Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the approved unified 500-asset Hotel Haven library, its animation mappings, style/material contracts, batch manifests, and validation artifacts on top of the existing HMG-070 content pipeline.

**Architecture:** The library is data-driven. A canonical master manifest owns the exact 500 gameplay-facing assets, while separate style, material, animation, and batch files provide reusable production rules. Assets are produced in ten 50-asset batches, validated against the same style/technical contract, then exported through the existing HMG-070 sidecar/cooker flow.

**Tech Stack:** JSON metadata, Markdown production documentation, existing native C++20 HMG-070 content pipeline, GitHub branch `feature/500-asset-library`.

**Spec:** `docs/superpowers/specs/2026-09-08-hotel-haven-500-asset-library-design.md`

## Global Constraints

- Exactly 500 gameplay-facing assets in the master manifest.
- Ten production batches of exactly 50 assets each.
- Architectural Diorama Realism is the single authoritative visual direction.
- Orthographic/isometric 2.5D readability is required at gameplay camera distances.
- Shared metric scale, pivot grammar, interaction anchors, cutaway policies, material families, and naming rules apply to all assets.
- Animation clips and animation sets are supporting runtime assets and do not consume the 500 gameplay-facing asset count.
- Runtime-bound assets must remain compatible with the existing HMG-070 source/export/sidecar/cook separation.
- No source asset is loaded directly by shipping runtime.
- No batch advances while blocker-level style or metadata validation issues remain unresolved.

---

### Task 1: Add the authoritative art-style contract

**Files:**
- Create: `GameData/AssetDefinitions/art_style_contract_v1.json`
- Create: `Art/Reference/StyleGuide/README.md`

**Interfaces:**
- Consumes: approved 500-asset design spec.
- Produces: stable keys used by every manifest entry: `style_contract`, `camera_profile`, `scale_profile`, `pivot_profile`, `lighting_profile`, `cutaway_profile`.

- [ ] **Step 1: Create `art_style_contract_v1.json`** with schema version 1 and explicit values for camera, scale, proportions, geometry language, material response, palette guidance, lighting, silhouette/readability, texel/detail density, pivot placement, interaction-anchor naming, cutaway behavior, and forbidden style drift.
- [ ] **Step 2: Create `Art/Reference/StyleGuide/README.md`** documenting the same rules in artist-readable language and the batch review checklist.
- [ ] **Step 3: Validate JSON syntax** with `python -m json.tool GameData/AssetDefinitions/art_style_contract_v1.json`.
- [ ] **Step 4: Commit** with `git commit -m "feat(art): lock Hotel Haven unified art style"`.

### Task 2: Define shared material families

**Files:**
- Create: `GameData/AssetDefinitions/material_families_v1.json`

**Interfaces:**
- Consumes: `art_style_contract_v1.json`.
- Produces: material IDs referenced by all 500 manifest entries.

- [ ] **Step 1: Define material families** for painted plaster, warm/dark wood, polished/brushed brass, stone, marble light/dark, ceramic tile, carpet standard/luxury, upholstery, leather, glass clear/frosted, stainless/service metal, painted service metal, vegetation, exterior paving, fabric/linen, plastic/rubber, emissive practical lights, and signage.
- [ ] **Step 2: For each family include** roughness range, metallic rule, saturation/value guidance, normal/detail intensity guidance, and allowed variants.
- [ ] **Step 3: Validate JSON syntax** with `python -m json.tool GameData/AssetDefinitions/material_families_v1.json`.
- [ ] **Step 4: Commit** with `git commit -m "feat(art): add shared material families"`.

### Task 3: Define reusable animation sets

**Files:**
- Create: `GameData/AssetDefinitions/animation_sets_v1.json`

**Interfaces:**
- Consumes: approved character/mechanical animation strategy.
- Produces: `animation_set_id`, `skeleton_id`, clip lists, loop/root-motion flags, and interaction-event names referenced by animated assets.

- [ ] **Step 1: Define skeleton families** `SK_HumanoidAdult`, `SK_HumanoidSmall`, `SK_ServiceProp`, and `SK_MechanicalSimple`.
- [ ] **Step 2: Define reusable sets** for guest locomotion, guest room interaction, front desk, housekeeping, restaurant service, kitchen, maintenance, security, bell service, ambient idles, and mechanical props.
- [ ] **Step 3: Define clip/event semantics** for idle, walk, turn, sit, stand, wait, talk, inspect, sleep, eat, drink, check-in, check-out, carry luggage, push cart, clean surface, make bed, vacuum, serve tray, cook prep, repair, open/close, and elevator-door states.
- [ ] **Step 4: Validate JSON syntax** with `python -m json.tool GameData/AssetDefinitions/animation_sets_v1.json`.
- [ ] **Step 5: Commit** with `git commit -m "feat(art): define reusable animation sets"`.

### Task 4: Create the exact 500-asset master manifest

**Files:**
- Create: `GameData/AssetDefinitions/hotel_haven_asset_manifest_v1.json`

**Interfaces:**
- Consumes: style contract, material families, animation sets.
- Produces: canonical list of exactly 500 gameplay-facing assets.

- [ ] **Step 1: Populate 500 unique entries** with stable IDs `HH_A001` through `HH_A500`.
- [ ] **Step 2: Enforce category totals**: architecture/construction 70; floors/walls/finishes 40; guest-room furniture/fixtures 75; lobby/front-of-house/public 60; restaurant/bar/food 45; housekeeping/maintenance/logistics 55; amenities/events 35; decor/clutter/signage 45; exterior/landscaping 25; guests/staff 50.
- [ ] **Step 3: Every entry must include** `asset_id`, `display_name`, `family`, `subcategory`, `batch`, `asset_type`, `units`, `style_contract`, `material_family`, `lod_policy`, `collision_policy`, `cutaway_policy`, `pivot_profile`, `footprint`, `interaction_anchors`, `tags`, `dependencies`, `animation_set` or null, and `review_status`.
- [ ] **Step 4: Validate count and uniqueness** with a script or one-shot Python command that asserts 500 entries, 500 unique IDs, and exact family totals.
- [ ] **Step 5: Commit** with `git commit -m "feat(art): add canonical 500 asset manifest"`.

### Task 5: Create ten production batch manifests

**Files:**
- Create: `GameData/AssetDefinitions/Batches/batch_01.json`
- Create: `GameData/AssetDefinitions/Batches/batch_02.json`
- Create: `GameData/AssetDefinitions/Batches/batch_03.json`
- Create: `GameData/AssetDefinitions/Batches/batch_04.json`
- Create: `GameData/AssetDefinitions/Batches/batch_05.json`
- Create: `GameData/AssetDefinitions/Batches/batch_06.json`
- Create: `GameData/AssetDefinitions/Batches/batch_07.json`
- Create: `GameData/AssetDefinitions/Batches/batch_08.json`
- Create: `GameData/AssetDefinitions/Batches/batch_09.json`
- Create: `GameData/AssetDefinitions/Batches/batch_10.json`

**Interfaces:**
- Consumes: canonical master manifest.
- Produces: ten ordered sets of exactly 50 asset IDs plus production/review state.

- [ ] **Step 1: Split the canonical manifest** into ten batches of exactly 50 IDs according to the approved production order.
- [ ] **Step 2: Add per-batch fields** `batch_id`, `purpose`, `asset_ids`, `style_gate`, `technical_gate`, `simulation_gate`, `completion_gate`, and `status`.
- [ ] **Step 3: Validate** that batch union equals the master manifest exactly, with no duplicate or omitted IDs.
- [ ] **Step 4: Commit** with `git commit -m "feat(art): add ten production batch manifests"`.

### Task 6: Add machine-checkable manifest validation

**Files:**
- Create: `Tools/ContentPipeline/tests/AssetLibraryManifestTests.cpp`
- Modify: `Tools/ContentPipeline/CMakeLists.txt`

**Interfaces:**
- Consumes: JSON files from Tasks 1-5 and existing `hh::assets::Json` parser/test framework.
- Produces: CI-enforced checks for count, uniqueness, allowed batch range, required fields, and animation references.

- [ ] **Step 1: Write failing tests** asserting master count 500, unique IDs, exact family totals, exactly ten batches, exactly 50 assets per batch, and no unknown animation-set references.
- [ ] **Step 2: Run content-pipeline tests** and confirm the new target/test fails before implementation if helper support is missing.
- [ ] **Step 3: Add the minimum loader/helper code in the test file or existing JSON utilities** needed to make those assertions against repository-relative files without altering gameplay authority.
- [ ] **Step 4: Run CMake/CTest** for `Tools/ContentPipeline` in Release and confirm all tests pass.
- [ ] **Step 5: Commit** with `git commit -m "test(art): validate 500 asset library manifests"`.

### Task 7: Produce and register Batch 01 visual reference assets

**Files:**
- Create: `Art/Reference/StyleGuide/batch_01_architecture_reference.md`
- Create: `Art/Validation/batch_01_validation.md`
- Create or register generated reference sheet under `Art/Generated/Batch01/` when binary upload path is available.

**Interfaces:**
- Consumes: Batch 01 manifest and unified style contract.
- Produces: approved visual grammar for architecture/construction core.

- [ ] **Step 1: Generate a single Batch 01 reference board** showing all 50 assets in the locked style and stable ID order.
- [ ] **Step 2: Record each asset's visual notes** in `batch_01_architecture_reference.md`: silhouette, materials, scale cues, cutaway/readability concerns, and whether animation is required.
- [ ] **Step 3: Run Gate A-D review** and record pass/fail per asset in `batch_01_validation.md`.
- [ ] **Step 4: Ensure all blocker issues are resolved before setting Batch 01 status to `APPROVED_FOR_PRODUCTION`**.
- [ ] **Step 5: Commit text metadata/reference documentation** with `git commit -m "art: approve batch 01 architecture reference"`.

### Task 8: Repeat visual production and validation for Batches 02-09

**Files:**
- Create: `Art/Reference/StyleGuide/batch_02_reference.md` through `batch_09_reference.md`
- Create: `Art/Validation/batch_02_validation.md` through `batch_09_validation.md`

**Interfaces:**
- Consumes: the same style/material contracts plus each batch manifest.
- Produces: approved reference boards and validation records for assets HH_A051-HH_A450.

- [ ] **Step 1: Generate each 50-asset batch board** using the same camera, lighting, presentation scale, and material grammar as Batch 01.
- [ ] **Step 2: Review every asset against Gate A-D** and document deviations.
- [ ] **Step 3: Regenerate or revise any blocker-level failures** before approving the batch.
- [ ] **Step 4: Update batch status to `APPROVED_FOR_PRODUCTION` only after the 50/50 asset review passes.**
- [ ] **Step 5: Commit each batch independently** so regressions can be isolated.

### Task 9: Produce characters/staff and animation coverage for Batch 10

**Files:**
- Create: `Art/Reference/StyleGuide/batch_10_characters_reference.md`
- Create: `Art/Validation/batch_10_validation.md`
- Modify: `GameData/AssetDefinitions/animation_sets_v1.json` only if a required clip is absent and the addition does not contradict the approved design.

**Interfaces:**
- Consumes: humanoid style proportions, skeleton families, reusable animation sets.
- Produces: 50 approved guest/staff gameplay assets with complete animation mappings.

- [ ] **Step 1: Generate character reference board(s)** covering body proportions, uniform language, guest archetypes, and staff roles.
- [ ] **Step 2: Assign an animation set to every character asset** and ensure no character has a required interaction without a mapped clip/event.
- [ ] **Step 3: Validate silhouette/readability at gameplay camera** and role recognition without reliance on labels.
- [ ] **Step 4: Record pass/fail and animation coverage** in Batch 10 validation.
- [ ] **Step 5: Commit** with `git commit -m "art: approve batch 10 characters and animation mappings"`.

### Task 10: Final library audit and release gate

**Files:**
- Create: `Art/Validation/library_release_audit_v1.md`
- Modify: batch manifests and master manifest review statuses as necessary.

**Interfaces:**
- Consumes: all Tasks 1-9.
- Produces: release-ready declaration for the 500-asset library metadata/reference layer.

- [ ] **Step 1: Run all content-pipeline tests** in Release.
- [ ] **Step 2: Run JSON syntax validation** on every file under `GameData/AssetDefinitions`.
- [ ] **Step 3: Re-run count/union/reference checks** and require exactly 500 unique gameplay-facing asset IDs and ten complete 50-asset batches.
- [ ] **Step 4: Audit validation notes** and require zero unresolved BLOCKER, CRITICAL, or MAJOR issues for the library milestone.
- [ ] **Step 5: Write `library_release_audit_v1.md`** containing the final counts, batch statuses, animation coverage summary, known MINOR issues, and commit identifiers.
- [ ] **Step 6: Commit** with `git commit -m "chore(art): complete 500 asset library release audit"`.

## Verification Checklist

- `hotel_haven_asset_manifest_v1.json` has exactly 500 unique entries.
- Family totals equal 500 and match the approved allocation exactly.
- `batch_01.json` through `batch_10.json` each contain exactly 50 unique IDs.
- Union of all batch IDs equals the master manifest set exactly.
- All material references resolve to `material_families_v1.json`.
- All non-null animation references resolve to `animation_sets_v1.json`.
- Every character asset has an animation set.
- Every mechanical asset requiring state motion has an animation set.
- All JSON parses successfully.
- All content-pipeline tests pass.
- All ten validation documents show zero unresolved blocker-level failures.
