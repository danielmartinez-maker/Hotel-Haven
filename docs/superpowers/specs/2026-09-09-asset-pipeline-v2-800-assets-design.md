# Hotel Haven Asset Pipeline V2 — 800-Asset Quality Expansion Design

## Purpose

Expand Hotel Haven's production asset library from 500 to 800 gameplay-facing assets while materially raising the quality bar of the content pipeline. The change must preserve the existing Architectural Diorama Realism art direction, existing cooked `.hasset` runtime boundary, deterministic content rules, and gameplay-authority separation.

This is an architectural production-pipeline expansion. It adds six new 50-asset batches, removes hard-coded assumptions that the library is exactly 500 assets / 10 batches, replaces narrow asset-ID quality checks with profile/family-driven contracts, and strengthens semantic, geometric, preview, material, variant, animation, and runtime validation.

## Baseline

Current `main` already contains:

- 500 gameplay-facing assets in 10 batches of 50;
- `hotel_haven_asset_manifest_v1.json` and sharded batch manifests;
- Architectural Diorama Realism V1 style contract;
- Python procedural art generation and asset-family factories;
- semantic-geometry validation;
- placement/pivot validation;
- animation dependency generation/linking;
- nonblank preview rendering and selected perceptual variant checks;
- native C++20 asset metadata/catalog/dependency/cooker infrastructure;
- deterministic `.hasset` cooking;
- cooked-runtime renderer integration.

V2 builds on current `main`. The previous 500-asset milestone remains an immutable V1 compatibility record.

## Goals

1. Ship exactly 800 gameplay-facing assets, preserving `HH_A001` through `HH_A500` unchanged as logical IDs and adding `HH_A501` through `HH_A800`.
2. Preserve the unified art direction while raising geometric, semantic, material, silhouette, and presentation quality.
3. Make quality rules declarative and scalable instead of primarily hard-coded per asset ID.
4. Make manifest totals, batch enumeration, preview generation, generation summaries, release audits, and CI derive their expected scope from manifest data rather than numeric literals.
5. Keep runtime consumption limited to cooked assets; Python, Blender, CUDA, Omniverse, OpenUSD, and other DCC/tooling dependencies remain offline-only and optional.
6. Keep simulation, room, economy, placement, reachability, task, and interaction authority in their owning game systems.
7. Preserve deterministic generation and deterministic cook outputs.
8. Make future expansion beyond 800 assets possible without another core-pipeline rewrite.

## Non-Goals

- No OpenUSD runtime migration.
- No CUDA dependency in `hh_game`, `renderer`, or shipping executables.
- No replacement of `.hasset` as the runtime content container.
- No redesign of Hotel Haven's visual identity.
- No procedural-generation authority over gameplay dimensions, prices, room rules, staff rules, or simulation behavior.
- No attempt to produce film-quality hero assets that violate management-camera readability or performance budgets.
- No change to existing asset IDs `HH_A001`–`HH_A500` unless a separately approved compatibility fix is required.

## Branch and Integration Strategy

Implementation branch:

`feature/asset-pipeline-v2-800-assets`

Base:

`main`

The branch must remain rebased/merged against current cooked-runtime behavior as needed. The V2 work should not fork a parallel runtime pipeline.

## Library Expansion

### Batch 11 — Building Systems & Circulation (`HH_A501`–`HH_A550`)

Target content:

- passenger elevator cab variants;
- service elevator cab variants;
- elevator doors and landing indicators;
- standard stairs, secure/service stairs, landings, rails and balustrades;
- corridor wall modules, corners, trims and service doors;
- utility/service-core architecture;
- fire doors and emergency-exit assemblies;
- mechanical/electrical service panels represented as art assets only;
- architectural dividers and partitions;
- lobby-to-corridor transition modules.

Primary profiles: `P_ARCH_STATIC`, `P_ARCH_ANIMATED`, `P_INTERACTIVE_ANIMATED`.

Quality emphasis: modular fit, floor alignment, cutaway readability, repeated-instance efficiency, transition-piece differentiation.

### Batch 12 — Guest Rooms & Bathrooms (`HH_A551`–`HH_A600`)

Target content:

- bed frame/headboard families;
- nightstands, lamps, desks and desk chairs;
- wardrobes, luggage benches and minibars;
- televisions, wall art and mirrors;
- bathroom vanities, sinks, toilets, tubs and shower assemblies;
- towel rails and amenity trays;
- accessible-room furniture/fixture variants;
- premium/luxury-room furnishing variants;
- connecting-room and suite-support pieces.

Primary profiles: `P_FURNITURE_STATIC`, `P_SMALL_PROP`, `P_INTERACTIVE_PREFAB`.

Quality emphasis: strong furniture-family silhouettes, believable hospitality proportions, restrained surface detail, clear premium/standard/accessibility differentiation.

### Batch 13 — Lobby, Public Space & Business (`HH_A601`–`HH_A650`)

Target content:

- reception desks and modules;
- concierge desk and bell stand variants;
- lounge seating families;
- coffee/side tables;
- public-area lamps and decorative dividers;
- business-center desks, printers and workstations;
- coworking seating/meeting pods;
- retail display fixtures;
- luggage holding furniture;
- queue-management posts and public-space wayfinding supports.

Primary profiles: `P_FURNITURE_STATIC`, `P_INTERACTIVE_PREFAB`, `P_SMALL_PROP`.

Quality emphasis: high first-impression quality, strong front-of-house role readability, variant differentiation without palette-only swaps.

### Batch 14 — F&B, Bar & Banqueting (`HH_A651`–`HH_A700`)

Target content:

- restaurant table/chair families;
- host stands and service stations;
- buffet tables, chafing stations and breakfast-service modules;
- bar counters, back bars, stools and bottle displays;
- coffee-service and café fixtures;
- banquet tables and stacking chairs;
- room-service staging pieces;
- tabletop service props;
- banquet/event serving equipment;
- compact kitchen/pass-through support props where visually useful.

Primary profiles: `P_FURNITURE_STATIC`, `P_SERVICE_PROP_ANIMATED`, `P_SMALL_PROP`, `P_INTERACTIVE_PREFAB`.

Quality emphasis: density control, readable service function, grouped-instance reuse, believable F&B equipment proportions.

### Batch 15 — Housekeeping, Engineering & Logistics (`HH_A701`–`HH_A750`)

Target content:

- housekeeping-cart variants;
- linen carts and laundry bins;
- shelving/racking families;
- cleaning equipment and supply caddies;
- maintenance tool carts and workbenches;
- waste/recycling bins and transport carts;
- back-of-house storage containers;
- receiving/delivery carts;
- laundry-room equipment shells;
- engineering/utility-room visual equipment;
- staff lockers and back-office support furniture.

Primary profiles: `P_SERVICE_PROP_ANIMATED`, `P_FURNITURE_STATIC`, `P_SMALL_PROP`, `P_INTERACTIVE_PREFAB`.

Quality emphasis: function-first silhouettes, moving-part semantics, interaction anchors, floor contact, service-role readability.

### Batch 16 — Amenities, Exterior & Human Variety (`HH_A751`–`HH_A800`)

Target content:

- spa treatment and relaxation variants;
- gym equipment variants;
- pool furniture and poolside service pieces;
- conference/event accessories;
- porte-cochère/drop-off pieces;
- exterior seating and planters;
- accessibility equipment and mobility-support props;
- luggage variants;
- additional guest archetype variants;
- additional staff-role presentation variants.

Primary profiles: mixed; character assets use `P_CHARACTER`.

Quality emphasis: visual variety, role clarity, character accessory differentiation, animation dependency completeness, exterior/weather-compatible material response.

## Manifest V2

Add:

`GameData/AssetDefinitions/hotel_haven_asset_manifest_v2.json`

V2 must:

- declare `asset_count: 800`;
- preserve all V1 profiles unless a V2 extension is explicitly needed;
- list batches 1 through 16;
- point to 16 sharded batch manifests;
- retain V1 as a historical compatibility artifact;
- include a `quality_contract` reference;
- keep gameplay-authority disclaimers explicit.

Add:

- `asset_batch_11.json`
- `asset_batch_12.json`
- `asset_batch_13.json`
- `asset_batch_14.json`
- `asset_batch_15.json`
- `asset_batch_16.json`

Each batch contains exactly 50 gameplay-facing assets.

## Quality Contract V2

Add:

`GameData/AssetDefinitions/asset_quality_contract_v2.json`

This becomes the machine-readable production-quality policy for generated gameplay assets.

### Contract layers

Quality is evaluated in layers rather than as one opaque score.

#### 1. Structural validity

Block release on:

- missing export/sidecar pair;
- duplicate logical ID;
- missing source reference;
- invalid dependency;
- dependency cycle;
- malformed GLB/mesh payload;
- NaN/Inf geometry;
- empty mesh for a mesh asset;
- unsupported runtime type;
- invalid units;
- invalid cutaway/collision/LOD policy.

#### 2. Geometry quality

Per profile/family define:

- face-count ceiling;
- minimum useful geometric complexity where applicable;
- degenerate-triangle tolerance;
- bounding-box sanity ranges;
- floor-contact tolerance;
- pivot-center tolerance;
- character proportion constraints;
- modular architectural edge/alignment tolerances;
- required semantic subcomponents/nodes.

A generic one-node `Body` placeholder is release-blocking for all production gameplay assets unless the profile explicitly permits single-body geometry and the family contract confirms the silhouette is semantically sufficient.

#### 3. Material quality

Validate:

- material family is allowed by the style contract;
- sampled base colors remain within approved hospitality palettes/accent ranges;
- metallic/roughness values fall inside family-appropriate ranges where represented in generated material data;
- no accidental pure-white fallback across a production asset;
- no neon/high-saturation outliers unless a named contract exception exists;
- material-slot count remains within profile budget;
- visually distinct materials are not created solely to bypass variant checks.

#### 4. Silhouette/readability quality

At the canonical management camera, validate diagnostic metrics for:

- frame occupancy;
- visible-area ratio;
- silhouette complexity;
- gross aspect ratio;
- minimum readable feature size;
- foreground/cutaway compatibility.

Hard failure applies to clearly invalid framing, blank renders, invisible geometry, or unusable scale. More subjective readability metrics remain reported diagnostics unless a deterministic threshold is well justified.

#### 5. Semantic quality

Move away from a small global dictionary of hand-picked asset IDs.

The quality contract should support family/profile rules such as:

- elevator assets require door/cab/indicator semantics as applicable;
- chairs require seat/back semantics where the generator uses decomposed geometry;
- desks require work-surface semantics;
- service carts require chassis/storage/handle semantics;
- bathroom fixtures require role-specific semantic nodes;
- humanoids require core articulated hierarchy plus role/accessory semantics;
- moving equipment requires named moving-part semantics.

Individual asset-ID exceptions remain supported for genuinely unique assets, but they are not the default mechanism.

#### 6. Variant differentiation

Variant groups are declared in data.

Validation compares:

- geometry signatures;
- bounding-box proportions;
- semantic-node sets;
- rendered perceptual fingerprints;
- optional silhouette fingerprints.

Required variants must not be material-only or name-only duplicates unless the manifest explicitly classifies them as color/material variants.

#### 7. Interaction completeness

For assets with gameplay-facing interaction anchors:

- sidecar anchors must exactly match manifest-authoritative art bindings;
- required anchor classes must be present;
- generated exports must expose matching locator/socket semantics when the asset format supports them;
- missing interaction bindings fail release.

Art metadata does not determine whether gameplay may execute an interaction.

#### 8. Animation quality

For animated assets:

- all required animation sets resolve;
- no deferred dependency links at release;
- required bones/nodes exist;
- clip/set IDs remain deterministic;
- loop/non-loop metadata follows the animation-set contract;
- mechanical animation ranges do not detach components or create invalid transforms;
- character variants preserve compatible skeleton contracts.

## Quality Metrics and Release Reporting

Generate a machine-readable quality report plus Markdown release audit.

Per asset, report where applicable:

- face count;
- material-slot count;
- bounding-box dimensions;
- floor offset;
- pivot offset;
- semantic completeness;
- interaction-anchor count;
- animation dependency count;
- canonical-preview occupancy;
- nonblank preview status;
- silhouette/perceptual signature;
- variant-distance results;
- warnings/failures by quality gate.

The release decision is based on explicit gate failures, not a vague aggregate score.

A summary quality index may be emitted for trend analysis only and must never allow a BLOCKER/CRITICAL gate to pass through averaging.

## Generator Architecture

### Current issue

`generate_all_assets.py` currently has numeric assumptions tied to batches 1–10 and 500 gameplay assets. This must be removed.

### V2 behavior

Create a manifest-driven batch registry.

Generation flow:

1. Load active master manifest.
2. Enumerate batch entries from manifest data.
3. Resolve a generator adapter for each batch/family.
4. Generate exports and sidecars.
5. Apply shared normalization.
6. Generate/link mechanical and humanoid animation dependencies.
7. Run floor-contact hardening where appropriate.
8. Run structural/geometry/material/semantic/variant QC.
9. Render canonical previews.
10. Run preview QC.
11. Validate the complete generated dependency tree.
12. Emit generation summary and release audit.

Expected totals are derived from the active manifest and actual animation-generator outputs.

### Factory strategy

Do not add six independent monolithic generators containing repeated primitives.

Extend/create reusable factories around semantic families:

- `architecture_factory.py`
- `guestroom_factory.py`
- `public_space_factory.py`
- `food_beverage_factory.py`
- `service_asset_factory.py` extensions
- `amenity_decor_factory.py` extensions
- `character_factory.py` extensions

Batch entrypoints remain thin orchestration modules.

Factories must produce meaningfully differentiated geometry and expose semantic nodes needed by quality validation.

## Preview Pipeline V2

Extend `render_asset_previews.py` to be manifest-driven.

Required outputs:

- one canonical preview per gameplay-facing asset;
- one preview board per production batch;
- `preview_qc.json` covering all 16 batches;
- perceptual/silhouette signatures used by variant QC;
- deterministic camera/framing rules;
- nonblank validation;
- minimum and maximum frame-occupancy validation;
- explicit failure records for clipped or effectively invisible assets.

Preview generation is a production validation surface, not authoritative runtime rendering.

## Native Content Pipeline Compatibility

The C++20 content subsystem remains authoritative for metadata/catalog/dependency/fingerprint/cook/container behavior.

V2 must preserve:

- `.hasset` deterministic binary envelope;
- source/sidecar/dependency/importer/cooker/compression/platform fingerprint inputs;
- atomic cook replacement;
- source assets never loaded by shipping executables;
- deterministic topological cook order;
- release severity levels;
- runtime/source separation.

Where new quality metadata is not required by runtime, it stays outside the shipping payload or is stripped during cook.

## Runtime and Renderer Boundary

The renderer consumes cooked assets through the existing runtime registry and GLB/cooked mesh path already integrated on `main`.

No renderer API should need to know that the authoring library grew from 500 to 800 assets.

Runtime tests must verify that representative assets from batches 11–16 can be cooked, registered, loaded, and submitted through the existing render path.

## NVIDIA / OpenUSD Tooling Position

The NVIDIA skills catalog identifies `omniverse-cad-to-simready` and `omniverse-usd-performance-tuning` as relevant workflows for source-asset conformance, validation, instancing, and large-scene optimization.

Hotel Haven V2 will use those ideas only at the offline tooling/design level unless separately approved.

Allowed optional future adapter:

`Art source / DCC -> OpenUSD validation/optimization workspace -> Hotel Haven export -> .asset.json -> .hasset cook`

Constraints:

- OpenUSD is not runtime authority.
- Omniverse is not required to build or run the game.
- CUDA is not required to build or run the game.
- CI must have a complete non-NVIDIA path.
- The canonical Hotel Haven runtime artifact remains `.hasset`.
- Repeated assets should preserve instancing/reuse semantics in authoring/export workflows where doing so reduces memory and production duplication.

No NVIDIA skill installation is required for the core V2 implementation. Installing those optional skills is a separate user-approved action.

## CI / RED→GREEN Strategy

Extend `content-pipeline-ci.yml` and relevant renderer tests.

### RED tests must be added first for

- active manifest contains 800 assets;
- manifest contains exactly 16 batches;
- each production batch contains exactly 50 gameplay assets;
- generation code has no 500/10-batch fixed assertions controlling scope;
- generic placeholder geometry fails under V2 family contracts;
- missing semantic family nodes fail;
- invalid floor contact fails;
- invalid pivot fails;
- excessive profile face count fails;
- disallowed material response fails;
- blank preview fails;
- under/over-framed preview fails;
- duplicate required variants fail geometric/perceptual differentiation;
- missing interaction anchors fail;
- missing animation dependencies fail;
- batches 11–16 generate deterministic sidecars/exports;
- representative new assets cook deterministically;
- representative new cooked assets load through runtime registry;
- repeated generation produces stable quality signatures and deterministic metadata.

### GREEN completion criteria

- all Python art-generation tests pass;
- all native content-pipeline tests pass;
- all renderer/runtime asset tests pass;
- 800 / 800 gameplay assets validate;
- 16 / 16 batches generate and preview successfully;
- zero unresolved BLOCKER/CRITICAL/MAJOR release diagnostics in shipping configuration;
- zero deferred animation dependencies;
- zero missing generated dependencies;
- all required interaction anchors normalize correctly;
- semantic quality gate passes;
- variant differentiation gate passes;
- preview QC passes;
- deterministic cook tests pass;
- generated release audit records final counts and hashes.

## Performance and Scale Budgets

V2 quality improvement must not translate into unconstrained mesh growth.

Rules:

- budgets are profile/family-specific rather than one global polygon cap;
- repeated hotel furniture and architectural modules favor reusable geometry/material families;
- decorative microdetail that is invisible from the canonical management camera is rejected or moved into material response;
- character complexity remains bounded by the existing renderer/animation constraints unless separately profiled and approved;
- new assets must not require runtime source-format parsing;
- preview/render stress tests should include dense rooms/public areas containing many repeated instances from V2 batches.

## Compatibility and Migration

- V1 manifests and IDs remain readable.
- V2 is the active production manifest after migration.
- existing 500 assets retain IDs and gameplay bindings.
- generated output directories may be regenerated from clean state; generated outputs are not hand-authored authority.
- cook-state invalidation occurs naturally when manifest/sidecar/source/cooker inputs change.
- gameplay code must not require a bulk migration solely because the catalog expanded.

## Error Handling

Generation/validation failures must identify:

- asset ID;
- batch;
- family/profile;
- gate name;
- deterministic failure reason;
- relevant measured value versus contract limit where applicable.

The pipeline should fail the affected validation run rather than silently substitute generic production geometry.

Optional authoring adapters may fail independently without blocking the standard non-NVIDIA generation/cook path.

## Implementation Boundaries

Expected implementation areas:

- `GameData/AssetDefinitions/`
- `GameData/AssetDefinitions/Manifest/`
- `Tools/ArtGeneration/`
- `Tools/ArtGeneration/tests/`
- `Tools/ContentPipeline/` only where metadata/release behavior genuinely needs extension;
- `.github/workflows/content-pipeline-ci.yml`;
- selected renderer/runtime asset tests;
- `Art/Validation/` generated/release records.

Avoid unrelated simulation, UI, economy, or renderer refactors.

## Acceptance Criteria

The project is complete only when all of the following are true:

1. `HH_A001`–`HH_A800` exist as exactly 800 unique gameplay-facing logical asset IDs.
2. Batches 1–16 each contain exactly 50 gameplay assets.
3. V2 active manifest is fully manifest-driven throughout generation, preview, validation and auditing.
4. Quality policy is data-driven by profile/family with only justified asset-specific exceptions.
5. Generic fallback geometry cannot pass production release accidentally.
6. Required variants are demonstrably distinct beyond display names.
7. Every generated gameplay asset passes structural, geometry, material, semantic, placement, dependency, preview and applicable animation/interaction gates.
8. Existing `.hasset` deterministic cooking and runtime loading remain intact.
9. Representative new assets render through the existing cooked runtime path.
10. Full RED→GREEN verification is recorded in CI and a final V2 release audit.
11. No shipping target gains a Python, Blender, CUDA, Omniverse, or OpenUSD dependency.
12. The pipeline can expand to additional manifest-declared batches without changing core hard-coded batch-count logic.

## Design Decision Summary

Hotel Haven Asset Pipeline V2 will scale the library from 500 to 800 assets by adding six production batches while preserving the current runtime content architecture. The core improvement is a shift from selected hard-coded quality checks to manifest/profile/family-driven production contracts with deterministic preview and variant validation. The quality upgrade and catalog expansion are implemented together so all 300 new assets enter through the stricter system rather than creating another legacy tier.
