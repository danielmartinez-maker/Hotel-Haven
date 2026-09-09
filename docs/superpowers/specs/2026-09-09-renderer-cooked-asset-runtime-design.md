# Renderer Cooked Asset Runtime Integration — Design

## Status

Approved architectural direction: **consume Hotel Haven production assets through cooked `.hasset` runtime envelopes, decode their embedded GLB payloads, cache renderer-owned mesh resources, and render instances by stable asset ID/renderer handle**.

This design extends Renderer Foundation C and the renderer visibility pass. It deliberately preserves the existing HMG-070 authority boundary: authoring sources and `Art/Exports` are build-time inputs only; the shipping renderer consumes cooked assets.

## Goal

Replace the renderer demo/client's dependence on procedural unit-cube hotel geometry with a runtime asset path that can use the already-produced Hotel Haven asset library.

The completed integration must:

1. resolve stable gameplay-facing asset IDs such as `HH_A001`;
2. load `Build/CookedAssets/<asset-id>.hasset` (or the packaged equivalent under `data/assets`);
3. validate and unwrap the existing `.hasset` v1 envelope;
4. decode the embedded GLB mesh/material payload;
5. build/cache D3D11 GPU mesh resources;
6. render placed instances through the existing orthographic camera, floor visibility, cutaway/blueprint, culling, translucent sorting, and batching pipeline;
7. retain procedural boxes only for debug primitives, construction previews, selections, explicit fallback diagnostics, and tests.

## Repository integration baseline

Implementation branch: `feature/renderer-cooked-asset-runtime`.

The branch must combine the authoritative work from:

- `feature/renderer-visibility-pass` — orthographic frustum culling and stable translucent sorting;
- `feature/500-asset-library` — the 500 gameplay asset definitions, art generators, hardened generated geometry, animation metadata, and content-pipeline verification.

The asset-library branch remains authoritative for art-generation/content-pipeline files. The renderer branch remains authoritative for renderer files. Integration must not fork or duplicate the 500-asset manifest.

## Verified production-asset characteristics

The current generated release artifact contains:

- 500 gameplay GLB files;
- 450 `StaticMeshAsset` records;
- 50 `SkinnedMeshAsset` records;
- no embedded images or textures in the gameplay GLBs;
- no glTF skins or glTF animation channels in the current gameplay GLBs;
- triangle-list primitives only;
- `POSITION` vertex attributes only;
- indexed geometry;
- PBR material `baseColorFactor`, `metallicFactor`, and `roughnessFactor` values;
- a small number of authored node matrices/hierarchies;
- generated geometry authored in Hotel Haven art space as X=width, Y=depth, Z=up, in meters.

The loader may therefore implement a strict supported GLB subset for the current library, while rejecting unsupported payload features with explicit diagnostics rather than silently rendering corrupt geometry.

## Authority boundaries

### Content pipeline owns

- stable asset IDs;
- `.hasset` envelope serialization and fingerprints;
- dependency lists and asset types;
- production asset metadata;
- generated/exported GLB payloads;
- validation and cooking.

### Renderer runtime owns

- locating packaged cooked files;
- parsing runtime-safe `.hasset` data;
- decoding supported GLB mesh/material data;
- renderer-space coordinate conversion;
- GPU buffer/resource lifetime;
- renderer-local handles and caches;
- render batching and presentation state.

### Renderer runtime does not own

- room validity;
- object placement legality;
- interaction semantics;
- guest/staff behavior;
- construction cost;
- asset production metadata changes;
- gameplay animation state;
- simulation authority.

## Runtime packaging contract

Authoring/runtime layout:

```text
Art/Exports/                       # generated build-time interchange; never shipping runtime lookup
Build/CookedAssets/                # local cook output
  HH_A001.hasset
  ...
  HH_A500.hasset

data/assets/                       # packaged runtime copy beside game executable
  HH_A001.hasset
  ...
  HH_A500.hasset
```

`RuntimeAssetRegistry` receives an asset-root path. The demo defaults to `data/assets` beside the executable. Tests may inject a temporary cooked root.

No renderer code may resolve `Art/Exports` as a runtime fallback.

## Runtime asset-format library

The existing content-pipeline implementation currently puts JSON parsing, `.hasset` parsing, cooking, CLI, and authoring operations into one `hh_assets` target. Integration will split the runtime-safe format layer without changing the file format:

- `hh_asset_format`: `Types`, `Json`, and `Hasset` parsing/serialization primitives;
- `hh_assets`: existing catalog/dependency/fingerprint/cooker/exporter/CLI layer, linking `hh_asset_format`.

Renderer asset loading links only the runtime-safe `hh_asset_format` target. It must not link cooker/exporter/CLI code into the shipping renderer.

The `.hasset` v1 binary contract remains unchanged.

## Renderer-facing asset contract

### AssetHandle

A renderer-local compact handle returned after an asset ID is resolved. Handles are stable for the lifetime of a `RuntimeAssetRegistry` but are not persisted in saves and are not gameplay IDs.

```cpp
struct AssetHandle {
    std::uint32_t value{};
};
```

Stable string IDs remain the cross-system identity. The registry owns the ID-to-handle mapping.

### AssetTransform

Presentation-only placement transform:

- world translation;
- Euler rotation in degrees;
- non-uniform scale, default `(1,1,1)`.

Production assets are authored in meters. Game/construction code provides placement; renderer code does not infer tile dimensions.

### AssetRenderItem

Presentation DTO containing:

- `AssetHandle asset`;
- `AssetTransform transform`;
- `Color tint` (default white, alpha 1);
- `int floorId`;
- `RenderCategory category`;
- world-space `Aabb bounds`.

World bounds are derived from the registry's decoded local mesh bounds plus the presentation transform before scene composition/culling. Gameplay does not author mesh bounds.

### RenderScene

`RenderScene` is extended to carry both:

- existing `BoxRenderItem` debug/procedural primitives;
- `AssetRenderItem` production asset instances.

This avoids breaking selection/construction-preview tooling while moving normal hotel presentation to production meshes.

## GLB decoding

### Supported container

GLB 2.0 only for the first runtime slice.

The loader validates:

- GLB magic/version/declared length;
- exactly one JSON chunk;
- one BIN chunk when referenced;
- buffer, bufferView, accessor bounds;
- supported component types and primitive modes;
- index ranges;
- node hierarchy references;
- material references.

Malformed input produces a renderer asset error containing the asset ID and failing invariant.

### Reused JSON parser

GLB JSON parsing uses the runtime-safe `hh_asset_format` JSON parser. The renderer does not introduce a second JSON implementation and does not vendor a network-fetched glTF dependency.

### Current-library subset

Required immediately:

- scenes/nodes;
- node hierarchy;
- node `matrix` and standard TRS transforms;
- meshes;
- one or more triangle primitives;
- `POSITION` attributes;
- indexed triangles;
- PBR base color/metallic/roughness factors;
- multiple materials/submeshes.

Unsupported textures, morph targets, skinning, compressed geometry extensions, sparse accessors, or non-triangle primitives fail explicitly until implemented.

## Coordinate conversion

The generated Hotel Haven assets use X=width, Y=depth, Z=up. Renderer world space uses X=horizontal, Y=up, Z=depth and Direct3D left-handed view/projection matrices.

Importer conversion is therefore centralized and applied exactly once:

```text
asset (x, y, z) -> renderer (x, z, y)
```

Because this basis swap changes handedness, triangle winding is reversed once during import so back-face culling remains correct. Node transforms and local bounds are converted using the same basis transform.

No game/simulation code performs this conversion.

## CPU mesh resource

Decoded asset resource:

```text
CpuMeshAsset
  assetId
  assetType
  localBounds
  vertices[]              # renderer-space positions
  indices[]
  submeshes[]
    firstIndex
    indexCount
    materialIndex
  materials[]
    baseColor
    metallic
    roughness
    transparent
```

The current generated GLBs omit vertex normals. The first renderer slice does not mutate the art files to synthesize normal attributes.

## Material and first-pass lighting behavior

The first integrated shader uses authored PBR base color as the primary material color and computes a flat geometric face normal in the pixel shader from world-position derivatives. This provides readable 2.5D depth without requiring generated normal streams.

First-pass lighting:

- fixed presentation-only ambient term;
- fixed presentation-only directional term;
- base color and instance tint;
- alpha blending for transparent material/item presentation.

`metallic` and `roughness` are retained in the runtime material record so the material contract does not need to change when the renderer gains a fuller PBR model. They are not authoritative gameplay values.

### Glass compatibility

The current generated GLBs contain translucent base-color alpha values while exporting `alphaMode` as opaque. For this production library, the runtime loader classifies a material as transparent when either:

- glTF `alphaMode == BLEND`; or
- `baseColorFactor.a < 0.999`.

This is an explicit compatibility rule for the existing generated Hotel Haven material library, not a general replacement for glTF semantics.

## Static and skinned asset behavior

### StaticMeshAsset

Fully renderable in this slice.

### SkinnedMeshAsset

The current 50 character GLBs contain hierarchical bind-pose geometry but no glTF skin/animation channels. They are loaded and rendered in their authored static/bind pose so every one of the 500 gameplay GLBs is visually callable by the renderer.

The asset type is preserved. No fake skeletal animation is introduced. Later animation integration may attach skeleton/animation-set resources without changing stable asset IDs or the basic asset-instance scene contract.

## RuntimeAssetRegistry

Responsibilities:

1. resolve stable asset ID to cooked path;
2. read and parse `.hasset`;
3. validate envelope asset ID/type;
4. decode GLB payload on first load;
5. cache CPU decoded resources;
6. expose local bounds;
7. assign renderer-local `AssetHandle`;
8. return explicit load diagnostics;
9. never mutate gameplay state.

Loading is lazy by default. A `preload(span<asset-id>)` API supports known starter-hotel sets and tests.

Duplicate loads of the same ID return the same handle/resource.

## D3D11 GPU resource cache

`D3D11Renderer` gains a mesh-resource cache keyed by `AssetHandle`.

Each cached asset owns:

- immutable vertex buffer;
- immutable index buffer;
- submesh/material metadata;
- local bounds;
- last-known registry generation if needed for explicit reload later.

GPU creation occurs on first render/preload requiring a device. No per-frame vertex/index buffer recreation is allowed.

## Asset draw batching

Frame flow:

```text
simulation/construction snapshot
  -> presentation adapter resolves AssetHandles
  -> RenderScene (asset items + debug boxes)
  -> SceneComposer floor/wall policy
  -> visibility/frustum stage
  -> asset draw grouping
  -> D3D11 GPU cache lookup/upload-once
  -> opaque asset submeshes
  -> transparent asset submeshes back-to-front
  -> wireframe/blueprint asset passes
  -> debug/selection box passes
  -> Present
```

Instances are grouped by:

- asset handle;
- presentation pass (opaque/translucent/wireframe);
- compatible material/submesh state.

Repeated placements of the same hotel asset use instanced draws. The renderer must not issue one draw call per repeated chair/bed/wall module when the instances are compatible.

## Floor, cutaway, blueprint, and visibility behavior

Existing renderer policies apply to asset items as well as boxes:

- hidden-floor asset items are excluded;
- adjacent context floors reduce instance alpha;
- wall-category asset items participate in full/cutaway/blueprint modes;
- blueprint uses the mesh wireframe rasterizer path;
- cutaway applies a presentation-only vertical scale/clip strategy to wall-category instances until authored cutaway meshes exist;
- selection/debug overlays remain presentation-only;
- frustum culling uses transformed production-mesh AABBs;
- translucent instance order uses camera view depth.

The renderer does not infer which asset is a wall from the asset ID. The presentation adapter supplies `RenderCategory` from authoritative construction/world semantics.

## Missing/invalid assets

There are two behaviors:

### Required/preloaded production asset

Load failure is fatal for the demo/integration test and reports:

- asset ID;
- cooked path;
- envelope/GLB error.

### Non-required runtime instance

The renderer records a diagnostic and may draw an explicit magenta/debug fallback box using the item's world bounds. It must never silently substitute a different production asset.

## Demo conversion

`RendererDemo` stops using procedural boxes for the visible hotel furnishings/architecture that have direct production counterparts.

The first asset-backed demo scene includes representative production assets from multiple families, including at minimum:

- floor/wall architectural module;
- guest-room door/window;
- bed/furniture item;
- front-desk/public-area object;
- service/logistics prop;
- decorative/amenity prop;
- one staff/guest character in bind pose.

Procedural boxes remain only for selection markers and explicit debug geometry.

## Build/cook integration

The repository CI path must prove the complete chain rather than checking only isolated parsers:

1. run the existing art-generation pipeline;
2. validate generated exports;
3. build the content-pipeline runtime/tool targets;
4. cook gameplay assets to a temporary `Build/CookedAssets` root;
5. build renderer asset-runtime tests and demo;
6. run native tests;
7. load/parse every gameplay `.hasset` referenced by `HH_A001` through `HH_A500`;
8. verify all 500 produce a valid renderer mesh/bind-pose resource and finite non-empty bounds;
9. run D3D11 WARP smoke coverage for GPU buffer creation on representative assets.

The full 500-asset generation/cook chain may live in a dedicated integration workflow so the fast renderer unit workflow remains responsive.

## Tests

### Runtime format/registry

- valid `.hasset` resolves by stable ID;
- envelope asset-ID mismatch fails;
- unsupported/non-GLB payload fails explicitly;
- duplicate resolve returns the same handle;
- missing required asset fails with ID/path diagnostic;
- local bounds are finite and non-empty.

### GLB loader

- parses known generated static asset fixture;
- parses multi-node/multi-material asset;
- applies node matrices;
- converts Z-up art coordinates to Y-up renderer coordinates;
- reverses triangle winding correctly;
- rejects out-of-range accessors/indices;
- preserves base color alpha/material values;
- classifies current glass material as transparent.

### Scene/visibility

- asset items obey floor visibility;
- asset world bounds cull off-camera instances;
- translucent asset items sort stably back-to-front;
- blueprint wall assets route to wireframe;
- cutaway affects only wall-category asset items.

### D3D11 smoke

- WARP device creates representative static asset vertex/index buffers;
- repeated same-handle items share one GPU mesh resource;
- representative asset submeshes issue valid indexed-instanced draws in a smoke frame where feasible.

### Full library integration

- exactly 500 gameplay IDs are enumerated from the authoritative manifest;
- every ID has a cooked file after generation/cook;
- 450 static and 50 skinned gameplay assets load;
- all 500 current GLB payloads produce drawable geometry;
- current unsupported GLB-feature count remains zero for the gameplay library.

## Performance constraints

- no GLB parsing on every frame;
- no `.hasset` disk I/O on every frame;
- no repeated immutable GPU mesh uploads after cache creation;
- repeated identical assets use instancing;
- visibility culling occurs before instance-buffer upload;
- stable IDs are resolved to compact handles outside the inner draw loop;
- resource-cache misses and failures are observable diagnostics.

## Non-goals for this slice

- skeletal animation playback;
- animation blending/state machines;
- textures/normal maps;
- morph targets;
- mesh compression extensions;
- streaming/eviction budgets;
- hot reload;
- final PBR/IBL/shadow renderer;
- authored cutaway LOD meshes;
- gameplay asset placement decisions.

These can extend the resource types and GPU material path later without changing the stable-ID/cooked-asset runtime boundary.

## Acceptance criteria

1. Renderer/runtime never loads `Art/Exports` directly.
2. Stable `HH_A###` IDs resolve through packaged `.hasset` files.
3. `.hasset` v1 remains backward-compatible and authoritative.
4. Existing 500-asset manifests are reused, not duplicated.
5. All 500 gameplay GLBs can be loaded into a drawable static/bind-pose mesh resource.
6. The renderer demo visibly uses production assets instead of procedural hotel geometry for normal scene content.
7. Repeated production assets are cached and instanced.
8. Asset mesh bounds participate in the existing frustum-culling stage.
9. Production asset items participate in floor context, cutaway, blueprint, and translucent sorting behavior.
10. Windows native tests and D3D11 WARP asset smoke tests pass.
11. Full asset integration CI verifies generation -> validation -> cook -> runtime load for all 500 gameplay assets.
12. No simulation/construction/economy/AI authority is added to the renderer.
