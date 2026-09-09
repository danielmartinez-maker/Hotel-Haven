# cuOpt Staff Optimizer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic C++ staff-optimization boundary with native fallback and an optional NVIDIA cuOpt MILP worker.

**Architecture:** The portable C++ library owns snapshots, plan contracts, validation, deterministic fallback, and cuOpt request serialization. NVIDIA cuOpt runs out-of-process and returns a proposed plan; HMG-040 remains the only commit authority.

**Tech Stack:** C++20, CMake 3.25+, Python 3.11+, NVIDIA cuOpt Python API when available.

**Spec:** `docs/superpowers/specs/2026-09-09-nvidia-optimization-openusd-design.md`

## Global Constraints

- Windows 11 x64 remains the product target.
- Shipping runtime must not require CUDA, WSL2, Python, or cuOpt.
- Stable 64-bit entity IDs and integer simulation seconds are mandatory.
- Solver plans are proposals and require deterministic validation before commit.

---

### Task 1: Optimizer contracts and deterministic fallback

**Files:**
- Create: `optimization/CMakeLists.txt`
- Create: `optimization/include/hh/optimization/Types.h`
- Create: `optimization/include/hh/optimization/Optimizer.h`
- Create: `optimization/src/Optimizer.cpp`
- Create: `optimization/tests/OptimizerTests.cpp`

**Interfaces:**
- Produces: `OptimizerSnapshot`, `SchedulerPlan`, `IStaffOptimizer`, `DeterministicFallbackOptimizer`.

- [ ] Write tests for canonical ordering, blocked/ineligible exclusion, priority ordering, and stable tie-breaks.
- [ ] Run tests and confirm RED because optimizer implementation is absent.
- [ ] Implement the minimal portable optimizer contracts and fallback bidder.
- [ ] Run tests and confirm GREEN.

### Task 2: Plan validation and MILP request builder

**Files:**
- Create: `optimization/include/hh/optimization/PlanValidator.h`
- Create: `optimization/include/hh/optimization/MilpRequest.h`
- Create: `optimization/src/PlanValidator.cpp`
- Create: `optimization/src/MilpRequest.cpp`
- Modify: `optimization/tests/OptimizerTests.cpp`

**Interfaces:**
- Produces: `validatePlan(const OptimizerSnapshot&, const SchedulerPlan&)` and deterministic JSON request generation.

- [ ] Add failing tests for stale snapshot hash, duplicate employee overlap, ineligible assignment, blocked task assignment, and request ordering.
- [ ] Run tests and confirm RED.
- [ ] Implement validation and request generation.
- [ ] Run tests and confirm GREEN.

### Task 3: Optional cuOpt worker

**Files:**
- Create: `Tools/CuOptWorker/hotel_haven_cuopt_worker.py`
- Create: `Tools/CuOptWorker/test_worker.py`
- Create: `Tools/CuOptWorker/README.md`

**Interfaces:**
- Consumes deterministic request JSON from `MilpRequest`.
- Produces structured plan JSON or structured failure JSON.

- [ ] Write failing Python tests for request validation and deterministic response normalization without requiring cuOpt.
- [ ] Run tests and confirm RED.
- [ ] Implement schema validation, lazy cuOpt import, MILP construction, sequential lexicographic solves, and deterministic output sorting.
- [ ] Run baseline tests and confirm GREEN.

### Task 4: CI

**Files:**
- Create: `.github/workflows/optimization-ci.yml`

- [ ] Configure Windows CMake build/test for `optimization/`.
- [ ] Configure Python unit tests for `Tools/CuOptWorker`.
- [ ] Keep GPU/cuOpt runtime tests opt-in.
