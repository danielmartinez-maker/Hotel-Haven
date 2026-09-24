# Full-Game NVIDIA Integration Design

## Goal

Integrate PR #5's reusable optimization/tooling foundation into `feature/full-game-integration` without changing simulation authority or introducing CUDA/Python into `hh_game`.

## Branch / foundation

- Integration branch: `feature/full-game-nvidia-integration`, based on `feature/full-game-integration`.
- PR #5 (`feature/nvidia-optimization-openusd`) remains unchanged and reusable.
- Port its `optimization/`, cuOpt worker, Balance Lab, OpenUSD tooling, NVIDIA scripts, and their portable CI/tests into the integration branch.

## Build architecture

- Root `CMakeLists.txt` adds `add_subdirectory(optimization)` before `add_subdirectory(game)`.
- `game/CMakeLists.txt` links `hh_game` **PRIVATE** to `hh_optimization` (and the existing `hh_assets`).
- `hh_game` never imports Python, CUDA, cuOpt runtime libraries, WSL2 APIs, or OpenUSD runtime dependencies.
- `Tools/CuOptWorker`, `Tools/BalanceLab`, `Tools/OpenUsdPipeline`, and `Tools/NVIDIA` remain development/offline tooling.

## Staff assignment seam

`Simulation::Impl::staffAndTasks()` retains authoritative responsibility for:

- shift activation/off-duty state;
- resource availability and blocked/ready transitions;
- task generation and task lifecycle;
- pathfinding and authoritative travel costs;
- task execution, fatigue, wages, inventory consumption, room state and completion effects.

Only worker selection is refactored.

### Flow

1. Refresh staff shift state and task resource blocked/ready state exactly as today.
2. Assign immediate guest-facing `CheckIn` / `CheckOut` work through the existing native greedy assignment path so critical service does not depend on optimization.
3. `buildOptimizerSnapshot()` converts the remaining authoritative state to `hh::optimization::OptimizerSnapshot`:
   - employees are available only when on shift and currently unassigned;
   - every active task is represented with its current Ready/Blocked/Traveling/Working/Completed state;
   - candidates are eligible only when role, shift, current assignment and authoritative path reachability permit it;
   - travel cost is derived from simulation pathfinding, including the supply-closet leg for unclaimed turnover work;
   - work estimates remain advisory; real completion stays in the simulation.
4. Native `DeterministicFallbackOptimizer` proposes assignments synchronously in-process.
5. `validatePlan()` revalidates epoch, fingerprint, readiness, eligibility, availability, durations and overlap.
6. The simulation rechecks current task/person state at commit time and applies assignments through the same task/person transitions previously used by the greedy block.
7. If optimization is disabled, a plan is invalid/stale, or native validation fails, the existing native greedy assignment path runs instead.

## Optional cuOpt boundary

- cuOpt remains optional and out-of-process through the existing worker/tooling from PR #5.
- The simulation frame/tick loop never launches, waits for, polls synchronously on, or depends on WSL2/cuOpt.
- A pure plan-resolution helper accepts an already-available external plan, validates it, and otherwise returns a validated native plan. This keeps the future asynchronous handoff testable without adding an external-runtime dependency to the game.

## Runtime control

`Simulation` exposes a dependency-free `setStaffOptimizerEnabled(bool)` switch. Native optimization is enabled by default on this integration branch; disabling it selects the legacy native assignment path. The switch is execution configuration, not authoritative save state.

## Deterministic tests

Add a dedicated optimizer integration test target proving:

1. identical seed/input with native optimization produces byte-identical saves;
2. disabled optimizer and native optimizer both preserve authority invariants;
3. blocked tasks never receive employees;
4. one employee never overlaps active tasks;
5. assignments respect role and shift eligibility;
6. poor layout still causes longer waits, lower satisfaction and lower operating profit with native optimization enabled;
7. stale/invalid external plans resolve to native fallback without mutating the authoritative snapshot.

Existing simulation tests remain unchanged and continue to cover save/load, construction, economics, staff/task execution and the existing poor-layout acceptance behavior.

## Headless Balance Lab output

Extend `hotel_haven_headless` with `--balance-output FILE` and `--run-id ID`.

At the end of a real simulation run it writes deterministic JSON containing at least:

- `run_id`;
- `seed` and elapsed simulation time;
- `operating_cost_minor = payroll + supplies + utilities`;
- `guest_satisfaction` from actual guest/review simulation outcomes;
- `fatigue_load` from simulated employee fatigue;
- `excess_wait_minutes` from actual guest queue waiting;
- `gop_minor = revenue - operating_cost_minor`;
- supporting throughput/reputation fields.

The JSON schema matches Balance Lab's existing loader so `Tools/BalanceLab/balance_lab.py frontier --input-dir ...` can consume headless outputs directly.
