# Asset Pipeline V2 Stress Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stress the manifest-driven 800-asset production pipeline, quality enrichment, batch generation, Meshy import normalization, and full-library validation for determinism, bounded resource use, duplicate/dependency rejection, and repeated full-run stability.

**Architecture:** Create `stress/asset-pipeline-v2` from the current `feature/asset-pipeline-v2-800-assets` head. Add pytest stress modules under `Tools/ArtGeneration/tests/stress/` and a small shared Python stress helper that uses a fully specified deterministic PRNG seed and temporary directories. Stress runs call production manifest/generator/validator APIs and CLI entry points; they do not generate new gameplay asset definitions or alter the 800-asset contract.

**Tech Stack:** Python 3.13, pytest, existing ArtGeneration/ContentPipeline Python tooling, GitHub Actions on Ubuntu and Windows.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Branch from the current Asset Pipeline V2 head and record the exact SHA.
- Preserve the 800-asset manifest and quality contract; stress tests must not lower quality gates or expected counts.
- Do not add Meshy runtime coupling; only the existing authoring/import normalization boundary is tested.
- Use `HH_STRESS_SEED`, `HH_STRESS_SCALE`, and `HH_STRESS_SCENARIO`; supported scales are `pr`, `extended`, `exhaustive`.
- Use temporary directories for generated/corrupted fixtures. Do not write generated stress artifacts into checked-in production directories.
- Failures print seed, scenario, iteration/asset ID, and the validation/generation stage.
- No merge is part of this plan.

---

## File Structure

- Create: `Tools/ArtGeneration/tests/stress/__init__.py`
- Create: `Tools/ArtGeneration/tests/stress/stress_support.py`
- Create: `Tools/ArtGeneration/tests/stress/test_manifest_stress.py`
- Create: `Tools/ArtGeneration/tests/stress/test_generation_stress.py`
- Create: `Tools/ArtGeneration/tests/stress/test_quality_meshy_stress.py`
- Create: `.github/workflows/asset-pipeline-v2-stress.yml`
- Create: `docs/stress/asset-pipeline-v2-verification.md`

### Task 1: Add deterministic Python stress configuration

**Files:**
- Create: `Tools/ArtGeneration/tests/stress/stress_support.py`
- Create: `Tools/ArtGeneration/tests/stress/test_manifest_stress.py` initially with support-contract tests.

**Interfaces:**
- Produces: `StressConfig`, deterministic `SplitMix64`, `bounded()`, scale budgets, and diagnostic context.

- [ ] **Step 1: Write support tests before helper**

Start `test_manifest_stress.py` with:

```python
from .stress_support import SplitMix64, load_stress_config


def test_splitmix64_is_replayable():
    a = SplitMix64(0x123456789ABCDEF0)
    b = SplitMix64(0x123456789ABCDEF0)
    assert [a.bounded(37) for _ in range(1000)] == [b.bounded(37) for _ in range(1000)]


def test_default_scale_is_pr(monkeypatch):
    monkeypatch.delenv('HH_STRESS_SCALE', raising=False)
    assert load_stress_config().scale == 'pr'
```

- [ ] **Step 2: Prove RED**

```bash
python -m pytest Tools/ArtGeneration/tests/stress/test_manifest_stress.py -q
```

Expected: FAIL because `stress_support.py` does not exist.

- [ ] **Step 3: Implement support helper**

Expose exactly:

```python
@dataclass(frozen=True)
class StressConfig:
    scale: str
    seed: int
    scenario: str

class SplitMix64:
    def __init__(self, seed: int): ...
    def next_u64(self) -> int: ...
    def bounded(self, upper: int) -> int: ...

def load_stress_config(default_seed: int = 0x5EED5EED5EED5EED) -> StressConfig: ...
def iterations(config: StressConfig, pr: int, extended: int, exhaustive: int) -> int: ...
```

Implement 64-bit wraparound explicitly with `& 0xFFFFFFFFFFFFFFFF`. Reject invalid scales and out-of-range seeds rather than silently normalizing them.

- [ ] **Step 4: Run GREEN and commit**

```bash
python -m pytest Tools/ArtGeneration/tests/stress/test_manifest_stress.py -q
git add Tools/ArtGeneration/tests/stress
git commit -m "test: add deterministic asset stress support"
```

### Task 2: Stress the 800-asset manifest and dependency contract

**Files:**
- Modify: `Tools/ArtGeneration/tests/stress/test_manifest_stress.py`

**Interfaces:**
- Consumes: `asset_manifest.py`, `hotel_haven_asset_manifest_v2.json`, batch 01-16 manifests, quality contract V2.
- Produces: repeatable manifest load/mutation stress coverage.

- [ ] **Step 1: Assert full production manifest invariants repeatedly**

Every clean iteration must prove exactly 800 unique production asset IDs, all referenced batch entries resolve, required quality metadata exists, and batch aggregation is stable.

```python
def assert_manifest_invariants(manifest):
    ids = [asset['id'] for asset in manifest.assets]
    assert len(ids) == 800
    assert len(set(ids)) == 800
    assert all_required_dependencies_resolve(manifest)
    assert all_assets_meet_required_metadata_shape(manifest)
```

Use the actual loaded manifest objects/dicts in implementation; helper names are test-local.

- [ ] **Step 2: Add deterministic mutation cases**

In a temp copy, mutate one condition at a time:

```text
duplicate_asset_id
missing_batch_file
missing_asset_dependency
invalid_quality_tier
malformed_json
unknown_required_factory
batch_count_mismatch
manifest_total_count_mismatch
```

Production validation must reject each mutation with a non-success result/exception already used by current tests. It must not partially accept a corrupted manifest.

- [ ] **Step 3: Scale mutation count**

Tier A: 1,000 mutations; extended: 10,000; exhaustive: 50,000. Each failure message includes seed + mutation index + mutation name.

- [ ] **Step 4: Run clean V2 tests plus stress**

```bash
HH_STRESS_SCALE=pr python -m pytest \
  Tools/ArtGeneration/tests/test_asset_pipeline_v2_manifest.py \
  Tools/ArtGeneration/tests/test_batches_11_16_v2.py \
  Tools/ArtGeneration/tests/stress/test_manifest_stress.py -q
```

Expected: GREEN.

- [ ] **Step 5: Commit**

```bash
git add Tools/ArtGeneration/tests/stress/test_manifest_stress.py
git commit -m "test: stress 800 asset manifest contract"
```

### Task 3: Stress full generation determinism and repeated runs

**Files:**
- Create: `Tools/ArtGeneration/tests/stress/test_generation_stress.py`

**Interfaces:**
- Consumes: `generate_all_assets.py`, `asset_manifest.py`, factories/batches 01-16, geometry validation entry points.
- Produces: deterministic repeated-generation stress tests in temp roots.

- [ ] **Step 1: Add missing-file RED through pytest collection**

Add the intended path to the branch stress workflow/test command before creating the file; verify pytest fails because the file is absent or expected generation stress test is missing from collection.

- [ ] **Step 2: Generate the same selected subset twice and compare canonical outputs**

Select asset IDs deterministically from the real 800 manifest. For output produced from identical source definitions, compare canonical file hashes/metadata already expected to be deterministic. Exclude only fields explicitly documented as non-deterministic by the existing pipeline.

Core pattern:

```python
first = generate_selection(tmp_path / 'a', selected_ids)
second = generate_selection(tmp_path / 'b', selected_ids)
assert canonical_output_hashes(first) == canonical_output_hashes(second)
```

- [ ] **Step 3: Stress repeated factory execution**

Tier A cumulatively generates/validates at least 1,000 selected assets; extended at least 8,000 asset-generation operations (ten complete-library equivalents); exhaustive at least 40,000. Reuse temporary roots between iterations only when testing idempotent overwrite behavior; otherwise isolate them.

- [ ] **Step 4: Stress `generate_all_assets.py` idempotence**

Run a full 800-asset generation twice in the same temp project root. The second run must not change canonical asset identity, duplicate manifest entries, or leave stale unexpected files.

- [ ] **Step 5: Validate geometry after generation**

Call the existing geometry validator over generated selections/full runs. Any geometry validation failure remains a product/asset defect; do not relax the quality contract for stress.

- [ ] **Step 6: Run baselines and commit**

```bash
HH_STRESS_SCALE=pr python -m pytest \
  Tools/ArtGeneration/tests/test_generate_all_assets.py \
  Tools/ArtGeneration/tests/test_manifest_driven_generation_v2.py \
  Tools/ArtGeneration/tests/stress/test_generation_stress.py -q

git add Tools/ArtGeneration/tests/stress/test_generation_stress.py
git commit -m "test: stress manifest driven asset generation"
```

### Task 4: Stress quality enrichment and Meshy normalization boundary

**Files:**
- Create: `Tools/ArtGeneration/tests/stress/test_quality_meshy_stress.py`

**Interfaces:**
- Consumes: `quality_enrichment.py`, `meshy_import_adapter.py`, `meshy_authoring_contract_v1.json`, quality contract V2.
- Produces: quality/normalization mutation stress tests.

- [ ] **Step 1: Generate legal and invalid authoring metadata deterministically**

Required cases:

```text
valid_normalization
missing_required_source_metadata
invalid_scale
invalid_axis_or_orientation
invalid_material_mapping
unknown_asset_id
quality_metadata_loss
repeated_normalization
```

- [ ] **Step 2: Prove authoring boundary does not leak into runtime contract**

A normalized valid import may enrich/normalize authoring data only through the existing adapter. Stress tests must assert that generated runtime/cooked references still use normal Hotel Haven asset IDs and do not require a Meshy service/client at runtime.

- [ ] **Step 3: Stress quality enrichment idempotence**

Apply quality enrichment repeatedly to valid definitions. Canonical enriched output after the first successful application must remain stable and must not accumulate duplicated tags/metadata.

- [ ] **Step 4: Scale operations**

Tier A: 5,000 normalization/enrichment operations. Extended: 50,000. Exhaustive: 250,000.

- [ ] **Step 5: Run existing quality/Meshy tests plus stress**

```bash
HH_STRESS_SCALE=pr python -m pytest \
  Tools/ArtGeneration/tests/test_meshy_import_adapter_v1.py \
  Tools/ArtGeneration/tests/test_quality_enrichment_v2.py \
  Tools/ArtGeneration/tests/stress/test_quality_meshy_stress.py -q
```

Expected: GREEN.

- [ ] **Step 6: Commit**

```bash
git add Tools/ArtGeneration/tests/stress/test_quality_meshy_stress.py
git commit -m "test: stress asset quality and Meshy normalization"
```

### Task 5: Add cross-platform pipeline stress CI

**Files:**
- Create: `.github/workflows/asset-pipeline-v2-stress.yml`
- Create: `docs/stress/asset-pipeline-v2-verification.md`

- [ ] **Step 1: Add Ubuntu and Windows Tier A jobs**

Both jobs use Python 3.13, install `Tools/ArtGeneration/requirements.txt` plus pytest, run the complete existing ArtGeneration test suite, and run all files under `Tools/ArtGeneration/tests/stress/` with `HH_STRESS_SCALE=pr`.

- [ ] **Step 2: Add scheduled/manual extended job**

Extended run executes full manifest mutations, ten-equivalent full generation load, geometry validation, and quality/Meshy stress. Use workflow timeout as runaway guard and upload only logs/reports needed for diagnosis, not entire temporary generated libraries unless a failure artifact is small and useful.

- [ ] **Step 3: Run one exact cross-platform replay**

Seed `0xC0FFEE1234ABCDEF`. Require identical selected asset IDs/mutation sequence and identical canonical deterministic hashes for the same generated fixtures on Ubuntu/Windows where existing pipeline semantics promise platform independence.

- [ ] **Step 4: Record verified counts and SHAs**

Verification doc must state parent SHA, child SHA, production asset count `800`, Tier A and extended run IDs, replay seed, and clean full-library validation result.

- [ ] **Step 5: Commit**

```bash
git add .github/workflows/asset-pipeline-v2-stress.yml docs/stress/asset-pipeline-v2-verification.md
git commit -m "ci: verify Asset Pipeline V2 stress suite"
```

## Completion Gate

Complete only when the clean 800-asset contract, thousands of deterministic manifest corruptions, repeated full/partial generation, geometry validation, quality enrichment, Meshy authoring normalization, Ubuntu/Windows parity, and the complete existing V2 test suite are GREEN without reducing asset count or quality requirements.