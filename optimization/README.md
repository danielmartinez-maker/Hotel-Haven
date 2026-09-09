# Hotel Haven Optimization Boundary

This module provides the deterministic native interface between HMG-040 staff scheduling and optional optimization backends.

## Runtime boundary

`hh_optimization` is portable C++20 and has no CUDA, Python, cuOpt, or WSL dependency. It owns canonical `OptimizerSnapshot` data, stable snapshot fingerprinting, deterministic fallback task dispatch, rolling-horizon MILP request serialization, proposed task/service-station assignments, and deterministic plan validation before HMG-040 commits anything.

The optional NVIDIA worker lives in `Tools/CuOptWorker` and is development tooling. The game must treat its output as a proposal.

## Service stations

Continuous pools such as reception are represented as `ServiceStation` demand by time bucket plus eligible `StationCandidate` employees. The MILP can allocate a worker to a station for a bucket, but it does not choose the next guest in the queue; HMG-040/HMG-050 queue discipline remains authoritative.

## Determinism

For an accepted solver plan:

1. snapshot state is canonicalized by stable 64-bit IDs;
2. the plan must match the optimization epoch and snapshot fingerprint;
3. task and station assignments are revalidated against readiness, eligibility, availability, and employee non-overlap;
4. the simulation records the accepted plan as an ordered authoritative event;
5. replay consumes the recorded event rather than invoking cuOpt again.

If the optional solver is unavailable or its plan fails validation, use `DeterministicFallbackOptimizer`.

## Build

```bash
cmake -S optimization -B optimization/build
cmake --build optimization/build --config Release
ctest --test-dir optimization/build -C Release --output-on-failure
```
