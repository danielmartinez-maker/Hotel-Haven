# HMG-070 Asset Pipeline Foundation Design

## Purpose

Implement the HMG-070 Asset Production Pipeline Foundation as a standalone native C++20 content subsystem for Hotel Haven. The subsystem owns asset metadata validation, cataloging, dependency analysis, deterministic fingerprints, generic cooking/container infrastructure, lifecycle/release validation, and the command-line surface required by HMG-070. It must remain separate from gameplay authority and from renderer implementation details.

## Authority

This design implements `HMG_070_ASSET_PRODUCTION_PIPELINE_FOUNDATION_V0_1.md`. HMG-070 remains authoritative when this design and the source specification differ.

The pipeline must preserve the HMG-070 rules that source assets are never loaded directly by the shipping executable; every runtime asset must be reproducible from versioned source, metadata, importer/cooker versions, and deterministic settings; timestamp-only invalidation is forbidden; and art metadata cannot silently become gameplay/economy authority.

## Repository Layout

```text
/Art
  /Source
  /Reference
  /Exports
  /Generated
  /Validation
/GameData
  /AssetDefinitions
/Tools
  /ContentPipeline
    /include/hh/assets
    /src
    /app
    /tests
    /fixtures
/Build
  /CookedAssets
```

`Build/CookedAssets` is generated output and must never be hand-edited. Runtime code may consume cooked asset containers only.

## Build and Platform

- Windows 11 x64 target.
- C++20.
- CMake 3.25 or newer.
- CTest-based tests.
- No third-party runtime dependency is required for HMG-070 foundation behavior.
- The content-pipeline CMake project remains isolated under `Tools/ContentPipeline` so it can build independently of `renderer/`.

## Components

### AssetMetadata

`AssetMetadata` parses `<export_basename>.asset.json` and validates schema `1`.

Required semantic fields:

- `schema`
- `asset_id`
- `asset_type`
- `source`
- `units`
- `lod_policy`
- `collision_policy`
- `material_slots`
- `tags`
- `dependencies`

Supported logical runtime types are exactly:

- `StaticMeshAsset`
- `SkinnedMeshAsset`
- `SkeletonAsset`
- `AnimationClipAsset`
- `AnimationSetAsset`
- `MaterialAsset`
- `TextureAsset`
- `PrefabAsset`
- `UIAtlasAsset`
- `FontAsset`
- `VFXAsset`
- `AudioClipAsset`
- `AudioBankAsset`

Unknown required schema fields or unsupported schema versions fail import. For 3D asset classes, `units` must be `meters`. The foundation does not invent an asset-ID naming grammar that HMG-070 does not define.

Optional foundation metadata supported by HMG-070 includes `cutaway_policy`, `pivot_exception_reason`, ownership/review fields, milestone, source revision, metadata revision, and cooker schema.

### AssetLifecycle

The exact lifecycle is:

`REQUESTED -> CONCEPT -> BLOCKOUT -> PRODUCTION -> TECH_ART -> REVIEW -> APPROVED -> COOKED -> IN_GAME_VERIFIED -> RELEASE_READY`

Rejected review returns to the relevant upstream state. The foundation validates legal forward transitions and explicit review rejection transitions; it does not infer creative approval.

### AssetCatalog

The catalog scans sidecars under `Art/Exports`, indexes `asset_id -> metadata/export/source`, and rejects duplicate logical IDs. Asset lookup accepts a logical ID or a repository-relative sidecar/export path.

### DependencyGraph

The graph stores logical asset dependencies from sidecars and provides:

- direct dependencies;
- transitive dependencies;
- transitive dependents;
- deterministic topological ordering;
- cycle detection;
- missing-dependency detection.

Ordinary circular dependencies fail validation. HMG-070's exception for explicitly supported bundles is left as an extension point because the foundation spec does not define bundle-resolution metadata.

### Deterministic Fingerprint

Each cooked cache key includes, at minimum:

- source file content hash;
- sidecar content hash;
- transitive source dependency hashes;
- importer version;
- cooker version;
- compression settings;
- platform target.

SHA-256 is used for deterministic fingerprints. Timestamps are never part of the key.

### HassetContainer

`.hasset` is implemented as a deterministic binary envelope with:

- fixed magic/version header;
- logical asset type;
- stable asset ID;
- deterministic fingerprint;
- dependency table;
- traceability metadata;
- payload byte count and payload bytes.

The writer emits fields in deterministic order and never embeds absolute workstation paths. The reader validates magic, version, declared lengths, and truncation before returning data.

Type-specific mesh/texture/animation/audio serialization is not invented by HMG-070; HMG-071 through HMG-079 can provide those processors later without changing the container contract.

### Cooker

The generic cooker:

1. resolves the catalog entry;
2. validates metadata and dependency graph;
3. computes the deterministic fingerprint;
4. compares it to recorded cook state;
5. builds a deterministic generic payload from the exported interchange file plus normalized traceability data;
6. writes a temporary file under `Build/CookedAssets`;
7. atomically replaces the final `.hasset` only after successful serialization;
8. records cook state keyed by logical asset ID.

A failed cook must not destroy the previous valid output.

`cook --changed` recooks changed assets plus transitive dependents. `cook --all` follows deterministic topological order.

### Export Boundary

`asset export` is implemented as orchestration, not invented DCC policy. It validates asset identity/source metadata, locates the configured Blender executable or reports a deterministic configuration error, and delegates to an exporter adapter interface. Type-specific Blender flags and transformation policy remain owned by HMG-071 through HMG-079.

### Validation and Severity

Validation diagnostics use:

- `BLOCKER`
- `CRITICAL`
- `MAJOR`
- `MINOR`

`RELEASE_READY` requires zero BLOCKER, zero CRITICAL, and zero known MAJOR diagnostics in the shipping configuration.

Foundation validation covers schema/type correctness, duplicate IDs, missing dependencies, cycles, legal cutaway policy values, required ownership/review/milestone fields when release auditing is requested, and runtime/source separation. Geometry-specific scale, pivot, LOD, UV, texture, collision, socket, and animation-budget checks are extension points for HMG-071 through HMG-079 because HMG-070 does not define their machine-readable export representation.

### Cutaway Policy

Legal values are exactly:

- `normal`
- `fade_when_foreground`
- `hide_upper_section`
- `never_cut`

The pipeline validates metadata only. Renderer behavior remains renderer authority.

## CLI

The executable is named `asset` and supports:

```text
asset validate <asset-id|path>
asset export <asset-id|path>
asset cook <asset-id|path>
asset cook --changed
asset cook --all
asset inspect <asset-id>
asset deps <asset-id>
asset audit --milestone <name>
```

Commands return non-zero on BLOCKER/CRITICAL validation failures, malformed invocation, missing assets, cook failure, or release-audit failure.

`inspect` prints normalized metadata, fingerprint, output path, and direct dependencies. `deps` prints direct and transitive dependency/dependent information deterministically.

## CI

A Windows GitHub Actions workflow configures, builds, and runs the content-pipeline tests in Release. It also runs repository asset validation. CI uses the same executable and validation code as local tooling.

## Testing

Tests cover:

- required metadata fields and schema version;
- all logical runtime types;
- 3D `meters` enforcement;
- duplicate IDs;
- missing dependencies;
- cycle detection;
- transitive dependency/dependent traversal;
- deterministic topological order;
- lifecycle legal/illegal transitions;
- cutaway-policy validation;
- severity/release gate;
- timestamp independence;
- source/sidecar/dependency/importer/cooker/compression/platform fingerprint invalidation;
- byte-identical repeated cooks;
- atomic-output preservation after failed cook;
- malformed/truncated `.hasset` rejection;
- runtime-source path prohibition;
- CLI success/failure behavior.

## Non-Goals

HMG-070 foundation does not define or implement game balance, economy values, authoritative room dimensions, mesh polygon budgets, texture resolutions/compression formats, character rigs, animation clip semantics, VFX simulation rules, audio codecs/bank layouts, or Blender type-specific export switches. Those belong to later authoritative specs and plug into this foundation through validators/processors/adapters.
