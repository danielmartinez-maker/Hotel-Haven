# Hotel Haven System-Wide Stress Test Campaign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add deterministic, replayable, branch-authoritative stress testing for every implemented Hotel Haven system, then orchestrate all stress branches from one CI campaign without merging the divergent gameplay stacks.

**Architecture:** The campaign is implemented as six isolated child stress branches, each created from the current authoritative implementation head at execution time. Each child branch owns its test harness copy, Tier A/extended/exhaustive stress targets, and branch-local CI; the central `feature/system-stress-test-campaign` branch owns only documentation and `.github/workflows/system-stress.yml`, which checks out and runs each child branch by ref.

**Tech Stack:** C++20, CMake/CTest, GitHub Actions, GCC/Clang sanitizers on Ubuntu, MSVC/Windows Server CI, Python 3.13/pytest for asset-pipeline stress tests, existing Hotel Haven deterministic simulation/data APIs.

**Spec:** `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md`

## Global Constraints

- Do not merge FINAL stacks or reconcile divergent gameplay architectures as part of this campaign.
- Do not invent mechanics absent from the branch being tested.
- Do not weaken assertions, gameplay fidelity, acceptance contracts, or simulation rules to obtain GREEN.
- Every randomized campaign must accept `HH_STRESS_SEED`, `HH_STRESS_SCALE`, and `HH_STRESS_SCENARIO` where the suite has named scenarios.
- Supported scale values are exactly `pr`, `extended`, and `exhaustive`; default is `pr`.
- Every randomized failure must print suite, seed, scenario/phase, tick/day when available, failed invariant, authoritative hash when available, and a bounded recent action trace.
- Exact replay uses deterministic bounded sampling from raw engine output; do not depend on implementation-specific `std::uniform_*` sequences when cross-platform equality matters.
- Correctness invariants are platform-stable. Wall-clock limits are coarse hang/runaway guards, not microbenchmark claims.
- Linux portable stress jobs use AddressSanitizer and UndefinedBehaviorSanitizer where compatible. Windows jobs provide MSVC Release parity and client-specific smoke paths where applicable.
- A failing stress test is investigated to root cause; do not disable the seed, relax a meaningful bound, or reduce load solely to hide the defect.
- No branch or PR is merged by this plan.

---

## Branch Matrix

| Stress branch | Authoritative parent at execution | Coverage |
| --- | --- | --- |
| `stress/main-core-service` | current `main` | foundation/simulation, workforce, departments, optimizer, FINAL-04 service/logistics, headless save/continuation paths exposed on main |
| `stress/final02-construction-guest` | current `feature/final-02-guest-ai-psychology` | FINAL-01 construction/building + FINAL-02 guest AI/psychology |
| `stress/final07-hospitality-economy-ui` | current `feature/final-07-ingame-ui-management` | FINAL-05 F&B/amenities/events + FINAL-06 economy/market + FINAL-07 UI/controller routing |
| `stress/renderer-content` | current `main` | renderer portable logic, runtime asset registry, GLB/cooked parser boundaries, C++ content pipeline |
| `stress/asset-pipeline-v2` | current `feature/asset-pipeline-v2-800-assets` | 800-asset manifest/generation/quality pipeline |
| `stress/soundtrack-v1` | current `feature/hotel-soundtrack-v1` | soundtrack catalog, asset integrity, missing/unreadable-track resilience, Windows player churn |

Observed planning snapshots on 2026-09-09 were: `main=b4bc16bb8ff159adeff42bae40f51010510ae94c`, FINAL-02 `8e83358b01ad2befb7f1afaea2269c4e7b0c4c4f`, FINAL-07 `4c6feb02434eb5368db236cd1e5747a2e5c50936`, Asset Pipeline V2 `39e9376b00819af85135cf61f1e2d338c5c359a5`, soundtrack `fc04dbccee7737ff088d2bb2bd1262d1854b146d`. These are audit references only; each stress branch must start from the then-current parent head, and that starting SHA must be recorded in its first commit message and PR body.

## File Structure

Central branch:

- Create: `.github/workflows/system-stress.yml` — scheduled/manual branch-matrix orchestrator.
- Create: `docs/stress/SYSTEM_STRESS_MATRIX.md` — records branch refs, target names, scale behavior, replay commands, and latest verified SHAs.
- Keep: `docs/superpowers/specs/2026-09-09-system-stress-test-campaign-design.md` — authoritative design.
- Keep this plan plus the six branch plans listed below.

Child plans:

- `docs/superpowers/plans/2026-09-09-stress-main-core-service.md`
- `docs/superpowers/plans/2026-09-09-stress-final02-construction-guest.md`
- `docs/superpowers/plans/2026-09-09-stress-final07-hospitality-economy-ui.md`
- `docs/superpowers/plans/2026-09-09-stress-renderer-content.md`
- `docs/superpowers/plans/2026-09-09-stress-asset-pipeline-v2.md`
- `docs/superpowers/plans/2026-09-09-stress-soundtrack-v1.md`

### Task 1: Execute the six branch plans independently

**Files:**
- Read: the six child plans listed above.
- Create: six stress branches listed in the branch matrix.

**Interfaces:**
- Consumes: the validated stress spec and each authoritative parent branch.
- Produces: six immutable-for-verification branch refs exposing documented CTest/pytest entry points.

- [ ] **Step 1: Resolve and record parent heads before branch creation**

Run:

```bash
git fetch origin
git rev-parse origin/main
git rev-parse origin/feature/final-02-guest-ai-psychology
git rev-parse origin/feature/final-07-ingame-ui-management
git rev-parse origin/feature/asset-pipeline-v2-800-assets
git rev-parse origin/feature/hotel-soundtrack-v1
```

Expected: five concrete 40-character SHAs. Save them in the execution notes and later PR bodies.

- [ ] **Step 2: Execute each child plan with RED→GREEN discipline**

Use one isolated worktree/branch per child plan. Do not modify the source feature branch directly. Each child plan defines its own failing-test commit, implementation/harness commit, CI commit, and full verification command.

- [ ] **Step 3: Require branch-local verification before orchestration**

For each child branch, verify its documented Tier A target is GREEN on every platform required by that child plan. Do not add a branch to the central workflow while its own stress suite is RED.

- [ ] **Step 4: Commit branch verification metadata**

Each child branch must end with a short `docs/stress/<branch>-verification.md` containing:

```text
Parent SHA: <40-char resolved parent>
Stress branch SHA: <40-char verified head>
Tier A: PASS
Extended: PASS or NOT-RUN
Exhaustive: PASS or NOT-RUN
Replay seed used for final deterministic check: 0x5EED5EED5EED5EED
```

`NOT-RUN` is acceptable only for optional heavy tiers; Tier A may never be `NOT-RUN`.

### Task 2: Add the central branch-matrix workflow

**Files:**
- Create: `.github/workflows/system-stress.yml`
- Create: `docs/stress/SYSTEM_STRESS_MATRIX.md`

**Interfaces:**
- Consumes: child branch target names and verified refs.
- Produces: scheduled/manual whole-repository stress campaign with `scale` and optional `seed` inputs.

- [ ] **Step 1: Add a workflow syntax test before the workflow**

Create `Tools/Stress/tests/test_system_stress_workflow.py` with:

```python
from pathlib import Path
import yaml


def test_system_stress_workflow_has_all_branch_legs():
    data = yaml.safe_load(Path('.github/workflows/system-stress.yml').read_text())
    jobs = data['jobs']
    matrix = jobs['stress']['strategy']['matrix']['include']
    names = {entry['name'] for entry in matrix}
    assert names == {
        'main-core-service',
        'final02-construction-guest',
        'final07-hospitality-economy-ui',
        'renderer-content',
        'asset-pipeline-v2',
        'soundtrack-v1',
    }
```

Add `PyYAML` only to this workflow's setup command (`python -m pip install pyyaml pytest`); do not add a runtime game dependency.

- [ ] **Step 2: Run the test to verify RED**

Run:

```bash
python -m pytest Tools/Stress/tests/test_system_stress_workflow.py -q
```

Expected: FAIL because `.github/workflows/system-stress.yml` does not exist.

- [ ] **Step 3: Create the workflow with explicit matrix entries**

Use this contract:

```yaml
name: System Stress Campaign
on:
  workflow_dispatch:
    inputs:
      scale:
        description: Stress scale
        required: true
        default: extended
        type: choice
        options: [pr, extended, exhaustive]
      seed:
        description: Optional uint64 replay seed
        required: false
        default: ''
  schedule:
    - cron: '17 7 * * *'

jobs:
  stress:
    strategy:
      fail-fast: false
      matrix:
        include:
          - { name: main-core-service, ref: stress/main-core-service, kind: cmake, filter: 'Stress(Main|Core|Workforce|Service)' }
          - { name: final02-construction-guest, ref: stress/final02-construction-guest, kind: cmake, filter: 'Stress(Final02|Construction|Guest)' }
          - { name: final07-hospitality-economy-ui, ref: stress/final07-hospitality-economy-ui, kind: cmake, filter: 'Stress(Final07|Hospitality|Economy|Ui)' }
          - { name: renderer-content, ref: stress/renderer-content, kind: cmake, filter: 'Stress(Renderer|Content)' }
          - { name: asset-pipeline-v2, ref: stress/asset-pipeline-v2, kind: python, filter: '' }
          - { name: soundtrack-v1, ref: stress/soundtrack-v1, kind: cmake, filter: 'StressSoundtrack' }
```

The job must use `actions/checkout@v4` with `ref: ${{ matrix.ref }}`, write the checked-out SHA to the log using `git rev-parse HEAD`, set `HH_STRESS_SCALE` from `github.event.inputs.scale || 'extended'`, and set `HH_STRESS_SEED` only when the input is non-empty. CMake legs run on the OS matrix required by their child plan; Python asset stress runs on Ubuntu and Windows; the soundtrack child plan defines the Windows-only player leg separately.

- [ ] **Step 4: Run syntax/contract test GREEN**

Run:

```bash
python -m pytest Tools/Stress/tests/test_system_stress_workflow.py -q
```

Expected: PASS.

- [ ] **Step 5: Add matrix documentation**

`docs/stress/SYSTEM_STRESS_MATRIX.md` must list each stress branch, authoritative parent, verified head SHA, CTest/pytest command, supported scale values, replay command, and whether ASan/UBSan and Windows parity apply.

Example replay command documented verbatim:

```bash
HH_STRESS_SCALE=extended HH_STRESS_SEED=0x5EED5EED5EED5EED ctest --test-dir build -C Release -R Stress --output-on-failure
```

- [ ] **Step 6: Commit central orchestration**

```bash
git add .github/workflows/system-stress.yml Tools/Stress/tests/test_system_stress_workflow.py docs/stress/SYSTEM_STRESS_MATRIX.md
git commit -m "ci: orchestrate system-wide stress campaign"
```

### Task 3: Run the complete campaign and freeze verification refs

**Files:**
- Modify: `docs/stress/SYSTEM_STRESS_MATRIX.md`

**Interfaces:**
- Consumes: all six verified child branches and central workflow.
- Produces: auditable final branch matrix tied to exact SHAs.

- [ ] **Step 1: Run Tier A across all child branches**

Dispatch the central workflow with `scale=pr` and seed `0x5EED5EED5EED5EED`.

Expected: all matrix legs GREEN. Any RED leg returns to its child plan; do not compensate in the central workflow.

- [ ] **Step 2: Run extended campaign**

Dispatch with `scale=extended` and no seed override so each suite executes its fixed multi-seed extended matrix.

Expected: all matrix legs GREEN with no sanitizer findings and no timeout/hang failures.

- [ ] **Step 3: Verify one exact replay on Linux and Windows where parity is promised**

Use seed `0xC0FFEE1234ABCDEF` on main-core-service, FINAL-02, FINAL-07 economy, and renderer portable logic. The suites must report identical authoritative hashes wherever the underlying system claims cross-platform deterministic hashing.

- [ ] **Step 4: Record exact verified SHAs**

Update `docs/stress/SYSTEM_STRESS_MATRIX.md` with `git rev-parse HEAD` for every child branch and the GitHub Actions run IDs for Tier A and extended.

- [ ] **Step 5: Run baseline regression suites**

On every child branch, run its full pre-existing CTest/pytest suite in addition to stress tests. Expected: zero pre-existing regressions.

- [ ] **Step 6: Commit verification record**

```bash
git add docs/stress/SYSTEM_STRESS_MATRIX.md
git commit -m "docs: record verified system stress matrix"
```

## Final Completion Gate

Do not call the campaign complete until all of the following are true:

- six child stress branches exist and are isolated from their authoritative source branches;
- every implemented gameplay subsystem has Tier A branch-authoritative stress coverage;
- core stateful systems have extended soak coverage;
- failing seeds are replayable;
- save/load continuation is stressed wherever the branch owns save state;
- Linux ASan/UBSan jobs are GREEN for selected portable stateful campaigns;
- MSVC parity is GREEN for cross-platform deterministic systems;
- renderer, runtime assets, C++ content pipeline, Asset Pipeline V2, and soundtrack each have dedicated stress coverage;
- central branch-matrix workflow runs all active stacks without merging them;
- all baseline test suites remain GREEN;
- no meaningful assertion, tolerance, gameplay rule, acceptance contract, or simulation fidelity was weakened.