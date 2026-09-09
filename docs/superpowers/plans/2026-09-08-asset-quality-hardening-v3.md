# Asset Quality Hardening V3 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Carry Hotel Haven's canonical interaction-anchor and placement/render policy metadata through the native content pipeline into cooked `.hasset` files so runtime consumers do not depend on source JSON sidecars.

**Architecture:** Extend `AssetMetadata` with the normalized gameplay fields that are currently ignored by C++, then extend the deterministic Hasset container to version 2 with a compact policy envelope. `parse_hasset` remains able to read version 1 files. The cooker copies metadata into the container; Python generation continues to normalize all 500 gameplay sidecars from the authoritative profile manifest.

**Tech Stack:** C++20, Python 3, CMake/CTest, deterministic HMG-070 content pipeline, GitHub Actions.

**Spec:** `GameData/AssetDefinitions/hotel_haven_asset_manifest_v1.json`

## Global Constraints

- Gameplay-facing asset count remains exactly 500.
- HMG-070 sidecar schema remains 1.
- 3D units remain meters.
- Existing art direction and generated geometry are unchanged by this pass.
- `.hasset` v1 files remain readable.
- `.hasset` v2 serialization is deterministic and repository-relative.
- Runtime gameplay authority remains with owning game-data systems; this pass transports metadata and does not invent simulation rules.

---

### Task 1: Native metadata transport fields

**Files:**
- Modify: `Tools/ContentPipeline/include/hh/assets/Metadata.h`
- Modify: `Tools/ContentPipeline/src/Metadata.cpp`
- Modify: `Tools/ContentPipeline/tests/MetadataTests.cpp`
- Modify: `Tools/ArtGeneration/generate_all_assets.py`
- Test: `Tools/ArtGeneration/tests/test_quality_hardening_v2.py`

**Interfaces:**
- Consumes: normalized sidecar fields `interaction_anchors` and profile contract `pivot_profile`.
- Produces: `AssetMetadata::interaction_anchors` and `AssetMetadata::pivot_profile`.

- [ ] **Step 1: Write failing metadata tests**

Add interaction anchors and `pivot_profile` to the valid sidecar fixture, then assert `load_metadata` preserves them and `canonicalize_metadata` emits them deterministically.

- [ ] **Step 2: Verify RED**

Run the native content-pipeline tests in CI; expected failure is missing `AssetMetadata` fields or dropped canonical fields.

- [ ] **Step 3: Implement minimal metadata support**

Add:

```cpp
std::vector<std::string> interaction_anchors;
std::optional<std::string> pivot_profile;
```

Load `interaction_anchors` as an optional string array for backward compatibility and `pivot_profile` as an optional string. Canonicalization must include both fields when present.

- [ ] **Step 4: Normalize pivot profile in generated gameplay sidecars**

`normalize_gameplay_sidecar_data` must set:

```python
normalized['pivot_profile'] = contract['pivot_profile']
```

- [ ] **Step 5: Verify GREEN and commit**

All Python art-generator tests and native metadata tests must pass.

---

### Task 2: Hasset v2 policy envelope with v1 read compatibility

**Files:**
- Modify: `Tools/ContentPipeline/include/hh/assets/Hasset.h`
- Modify: `Tools/ContentPipeline/src/Hasset.cpp`
- Modify: `Tools/ContentPipeline/tests/HassetTests.cpp`

**Interfaces:**
- Consumes: cooked metadata strings/vectors.
- Produces: parsed `HassetDocument` fields `units`, `lod_policy`, `collision_policy`, `cutaway_policy`, `pivot_profile`, and `interaction_anchors`.

- [ ] **Step 1: Write failing Hasset tests**

Extend the sample document with policy fields and anchors. Assert deterministic v2 round-trip. Add a hand-built v1 byte fixture and assert it still parses with empty new fields. Change unsupported-version coverage to version 3.

- [ ] **Step 2: Verify RED**

Native Hasset tests must fail because the document lacks the new fields/version handling.

- [ ] **Step 3: Implement v2 serialization**

Serialize after provenance paths and before payload:

```text
units
lod_policy
collision_policy
cutaway_policy
pivot_profile
interaction_anchor_count
interaction_anchor strings (sorted/unique)
```

Keep magic unchanged; write version 2.

- [ ] **Step 4: Implement v1/v2 parsing**

Version 1 follows the existing layout. Version 2 reads the policy envelope. Any other version fails.

- [ ] **Step 5: Verify GREEN and commit**

All Hasset tests must pass, including deterministic bytes, truncation rejection, absolute-path rejection, and v1 compatibility.

---

### Task 3: Cooker populates runtime policy metadata

**Files:**
- Modify: `Tools/ContentPipeline/src/Cooker.cpp`
- Modify: `Tools/ContentPipeline/tests/CookerTests.cpp`

**Interfaces:**
- Consumes: `AssetRecord::metadata` fields from Task 1.
- Produces: populated Hasset v2 documents from Task 2.

- [ ] **Step 1: Write failing cooker test**

Create a fixture sidecar with `interaction_anchors`, `pivot_profile`, cutaway, LOD and collision policies; cook it; parse the `.hasset`; assert all policy fields exactly match the sidecar.

- [ ] **Step 2: Verify RED**

Expected failure: cooked Hasset policy fields are empty.

- [ ] **Step 3: Populate HassetDocument**

Copy metadata into the document before serialization. Convert optional `CutawayPolicy` to its canonical string; absent optionals become empty strings.

- [ ] **Step 4: Verify GREEN and commit**

Run native content-pipeline tests and ensure deterministic recook/no-op tests remain green.

---

### Task 4: Full-library cooked metadata verification

**Files:**
- Modify: `Tools/ContentPipeline/scripts/validate_asset_library.py`
- Modify: `.github/workflows/content-pipeline-ci.yml` only if the existing validation invocation cannot inspect cooked outputs.
- Modify: `Art/Validation/library_release_audit_v1.md` from generated evidence only.

**Interfaces:**
- Consumes: all 500 normalized gameplay sidecars and cooked `.hasset` files.
- Produces: release gate proving cooked policy/anchor parity.

- [ ] **Step 1: Add failing full-library validation**

For each gameplay asset, compare cooked Hasset policy/anchor metadata against normalized sidecar metadata. Aggregate counts for `cooked_policy_assets` and `cooked_interaction_anchor_bindings`.

- [ ] **Step 2: Verify RED before cooker implementation is accepted**

The validator must detect missing cooked policy metadata on the old container format.

- [ ] **Step 3: Run complete generation, native build, cook and validation**

Required gates: 500/500 gameplay assets, 591 total generated records, 87/87 animation links, 0 deferred, 184/184 sidecar anchors, 0 profile/placement failures, and 500/500 cooked policy parity.

- [ ] **Step 4: Record exact generated audit**

Update the version-controlled audit only from the successful final CI artifact.

- [ ] **Step 5: Verify exact branch head**

Re-run Content Pipeline CI on the audit commit and confirm every workflow step succeeds on the latest branch SHA.
