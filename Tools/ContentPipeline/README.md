# Hotel Haven HMG-070 Content Pipeline Foundation

Native C++20 implementation of the HMG-070 Asset Production Pipeline Foundation.

## Authority boundary

This toolchain owns asset metadata parsing, validation, dependency analysis, deterministic fingerprints, `.hasset` v1 containers, incremental cooking, milestone audits, and the `asset` CLI. It does not own hotel simulation, economy, room validity, balance, rendering policy, or type-specific art budgets.

The shipping game must consume cooked assets only. `Art/Source` and `Art/Reference` are authoring inputs and are never runtime payload locations. The cooker may hash source files for reproducibility/provenance, but cooked payload bytes come from deterministic interchange files under `Art/Exports`.

## Canonical repository layout

```text
Art/
  Source/
  Reference/
  Exports/
  Generated/
  Validation/
GameData/
  AssetDefinitions/
Tools/
  ContentPipeline/
Build/
  CookedAssets/       # generated and ignored by git
```

Every exported asset uses `<export_basename>.asset.json`. Schema 1 requires:

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

Supported logical runtime types are `StaticMeshAsset`, `SkinnedMeshAsset`, `SkeletonAsset`, `AnimationClipAsset`, `AnimationSetAsset`, `MaterialAsset`, `TextureAsset`, `PrefabAsset`, `UIAtlasAsset`, `FontAsset`, `VFXAsset`, `AudioClipAsset`, and `AudioBankAsset`.

Legal cutaway policies are `normal`, `fade_when_foreground`, `hide_upper_section`, and `never_cut`.

## Build and test

Windows 11 x64 with Visual Studio 2022 and CMake 3.25+:

```powershell
cmake -S Tools/ContentPipeline -B Tools/ContentPipeline/build -A x64
cmake --build Tools/ContentPipeline/build --config Release
ctest --test-dir Tools/ContentPipeline/build -C Release --output-on-failure
```

The project is intentionally portable enough for non-Windows developer test builds, but Windows 11 x64 is the shipping target.

## CLI

From the repository root:

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

`validate` also accepts the canonical `Art/Exports` directory for repository/CI validation.

`export` implements the HMG-070 orchestration boundary. Set `HOTEL_HAVEN_BLENDER` to the Blender executable. HMG-070 does not define type-specific Blender export switches, so the command reports that an HMG-071–079 exporter adapter is required rather than silently inventing transformation policy.

## Deterministic fingerprints

The SHA-256 fingerprint includes:

- logical asset ID;
- source file content hash;
- sidecar content hash;
- export/interchange content hash;
- every transitive dependency ID plus source/sidecar/export hashes;
- importer version;
- cooker version;
- compression settings;
- platform target.

Filesystem timestamps are never fingerprint inputs.

## `.hasset` v1

The deterministic runtime envelope contains:

```text
magic[8] = "HHASSET\0"
u32 version = 1
u32 asset_type
u32 asset_id_len + bytes
u32 fingerprint_len + bytes
u32 dependency_count
  repeated: u32 dependency_len + bytes
u32 source_path_len + bytes
u32 sidecar_path_len + bytes
u64 payload_len + payload_bytes
```

Integers are little-endian. Dependencies are serialized in lexical order. Provenance paths must be repository-relative. The payload is the exported interchange file; editable source bytes are not embedded.

Cook publication uses a temporary file and atomic replacement. A failed recook therefore leaves the previous valid `.hasset` in place.

## Lifecycle and release audit

HMG-070 lifecycle:

```text
REQUESTED -> CONCEPT -> BLOCKOUT -> PRODUCTION -> TECH_ART -> REVIEW -> APPROVED -> COOKED -> IN_GAME_VERIFIED -> RELEASE_READY
```

Review rejection may return to the relevant upstream concept/blockout/production/tech-art state when explicitly marked as rejection.

Validation severity is `BLOCKER`, `CRITICAL`, `MAJOR`, or `MINOR`. `RELEASE_READY` allows zero BLOCKER, zero CRITICAL, and zero known MAJOR issues.

`audit --milestone` requires matching assets to have `content_owner`, `technical_reviewer`, `art_reviewer`, `dependent_feature_owner`, matching `milestone`, and lifecycle `RELEASE_READY` in addition to passing foundation validation.

## Deferred type-specific rules

HMG-070 does not define machine-readable polygon budgets, texture resolutions/formats, LOD thresholds, authored collision shapes, socket schemas, rig conventions, animation compression profiles, VFX budgets, or audio codec/bank layouts. Those validators/processors belong to HMG-071 through HMG-079 and plug into this foundation without changing stable IDs, dependency semantics, fingerprinting, or the `.hasset` envelope.
