# Hotel Haven Runtime Asset Library Release Audit V1

## Release gate

- Gameplay-facing assets: **2000 / 2000**
- Generated asset records (gameplay + animation support): **2097**
- Animation dependencies linked: **268 / 268**
- Deferred animation dependencies: **0**
- Interaction anchors normalized: **354 / 354**
- Profile contract conformance: **PASS** with **0** failures
- Placement/pivot QC: **PASS** with **0** failures
- Semantic identity QC: **PASS** with **0** failures
- Contextual placement exceptions: **20** documented assets
- Floor-support hardening applied: **1** generated assets
- Mechanical animation coverage: **28 clips / 9 sets**
- Humanoid animation coverage: **47 clips / 11 sets / 2 skeletons**
- Geometry QC: **PASS** with **0** failures
- Maximum generated face count: **3968**
- Material palette: **92** unique sampled RGB colors; non-white ratio **1.0**
- Unresolved BLOCKER/CRITICAL/MAJOR issues: **0**

## Profile face budgets

| Profile | Maximum faces |
| --- | ---: |
| P_ARCH_ANIMATED | 5000 |
| P_ARCH_STATIC | 5000 |
| P_CHARACTER | 2500 |
| P_FURNITURE_STATIC | 4200 |
| P_INTERACTIVE_ANIMATED | 4200 |
| P_INTERACTIVE_PREFAB | 4200 |
| P_SERVICE_PROP_ANIMATED | 4200 |
| P_SMALL_PROP | 3000 |

## Batch status

| Batch | Status |
| --- | --- |
| Batch 01 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 02 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 03 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 04 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 05 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 06 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 07 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 08 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 09 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 10 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 11 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 12 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 13 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 14 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 15 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 16 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 17 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 18 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 19 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 20 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 21 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 22 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 23 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 24 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 25 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 26 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 27 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 28 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 29 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 30 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 31 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 32 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 33 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 34 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 35 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 36 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 37 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 38 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 39 | PRODUCTION_GENERATOR_VALIDATED |
| Batch 40 | PRODUCTION_GENERATOR_VALIDATED |

## Operational note

Generated GLB binaries and preview boards remain CI artifacts by design; the repository stores deterministic generators, manifests, validation policy, and this audit record.
