# Hotel Haven 500-Asset Library Release Audit V1

## Release gate

- Gameplay-facing assets: **500 / 500**
- Generated asset records (gameplay + animation support): **591**
- Animation dependencies linked: **87 / 87**
- Deferred animation dependencies: **0**
- Mechanical animation coverage: **24 clips / 7 sets**
- Humanoid animation coverage: **47 clips / 11 sets / 2 skeletons**
- Geometry QC: **PASS** with **0** failures
- Maximum generated face count: **3024**
- Material palette: **83** unique sampled RGB colors; non-white ratio **1.0**
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

## Operational note

Generated GLB binaries and preview boards remain CI artifacts by design; the repository stores deterministic generators, manifests, validation policy, and this audit record.
