# OpenUSD Hotel Scene Pipeline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate deterministic OpenUSD hotel assembly stages with floor payloads and aggressive safe instancing while preserving the HMG-070 cooked-runtime boundary.

**Architecture:** A standard-library Python tool validates a JSON scene manifest and writes text USDA stages plus a Hotel Haven cook manifest. Reusable rooms/furniture use scenegraph references and `instanceable`; high-volume simple props may use `PointInstancer`.

**Tech Stack:** Python 3.11 standard library; optional external `usdchecker`/Omniverse tools for enhanced validation.

**Spec:** `docs/superpowers/specs/2026-09-09-nvidia-optimization-openusd-design.md`

## Global Constraints

- 1 engine unit = 1 meter.
- Floors are payload boundaries.
- Repeated assets above the HMG-070 threshold of 100 instances must use an instance-friendly mode unless explicitly exempted.
- Runtime loads cooked Hotel Haven assets rather than authoring USD.

---

### Task 1: Manifest validation and deterministic stage generation

**Files:**
- Create: `Tools/OpenUsdPipeline/hh_usd.py`
- Create: `Tools/OpenUsdPipeline/test_hh_usd.py`

- [ ] Write failing tests for duplicate IDs, unknown asset references, high-count uninstanced assets, stable output ordering, and transform validation.
- [ ] Run RED.
- [ ] Implement manifest validation and deterministic serialization.
- [ ] Run GREEN.

### Task 2: Payloads, scenegraph instances, PointInstancer, and cook manifest

**Files:**
- Modify: `Tools/OpenUsdPipeline/hh_usd.py`
- Modify: `Tools/OpenUsdPipeline/test_hh_usd.py`
- Create: `Tools/OpenUsdPipeline/README.md`

- [ ] Add failing tests for per-floor payloads, `instanceable = true` references, PointInstancer prototype emission, and logical asset cook mapping.
- [ ] Run RED.
- [ ] Implement stage/floor generation and cook manifest output.
- [ ] Run GREEN.

### Task 3: CI

**Files:**
- Create: `.github/workflows/openusd-pipeline-ci.yml`

- [ ] Run unit tests on Windows and Ubuntu.
- [ ] If `usdchecker` exists, validate generated fixtures without making it a baseline dependency.
