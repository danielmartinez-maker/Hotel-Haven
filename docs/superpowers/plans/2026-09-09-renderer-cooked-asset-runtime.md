# Renderer Cooked Asset Runtime Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the actual Hotel Haven Windows client and renderer consume the existing 500-asset production library through cooked `.hasset` files, with cached GLB mesh resources and asset instancing replacing procedural hotel geometry.

**Architecture:** Keep simulation authority unchanged. Merge the integrated game, hardened asset library, and renderer visibility baselines; split the content pipeline’s runtime-safe `.hasset`/JSON layer; decode the generated GLB subset into renderer CPU resources; resolve stable `HH_A###` IDs once into compact renderer handles; extend scene composition/visibility to production asset instances; cache/upload immutable D3D11 meshes once and instance repeated assets; finally replace `game/app/WorldView.cpp`’s procedural hotel objects with production asset handles while retaining procedural selection/diagnostic overlays.

**Tech Stack:** C++20, CMake 3.25+, Win32, Direct3D 11, DirectXMath, HLSL Shader Model 5.0, existing HMG-070 `.hasset` v1, existing Python/trimesh art-generation pipeline, GitHub Actions Windows CI.

**Spec:** `docs/superpowers/specs/2026-09-09-renderer-cooked-asset-runtime-design.md`

## Global Constraints

- Shipping/runtime code loads cooked `.hasset` files only; it never resolves `Art/Exports` as a fallback.
- `.hasset` v1 binary format remains byte-compatible.
- Stable `HH_A###` string IDs remain the cross-system identity; renderer `AssetHandle` values are process-local and never saved.
- The authoritative 500-asset manifests on the asset-library branch are reused, not copied into a second catalog.
- Asset-space conversion is centralized as `(x,y,z) -> (x,z,y)` with triangle winding reversed exactly once.
- Current generated gameplay GLBs require GLB 2.0, indexed triangle primitives, `POSITION`, scenes/nodes, node matrix/TRS, multiple meshes/materials, and PBR base-color/metallic/roughness factors.
- Unsupported textures, morphs, sparse accessors, compressed mesh extensions, and true glTF skin/animation channels fail explicitly.
- Current 50 character assets render in authored bind/static pose; no fake skeletal animation is introduced.
- Existing renderer floor visibility, blueprint, cutaway, focus protection, orthographic frustum culling, and transparent ordering remain presentation-only.
- Existing simulation, construction legality, room/economy/staff/service logic must not depend on asset IDs.
- Repeated opaque instances of the same asset/submesh must use instanced draws; transparent order-dependent instances may fall back to per-instance indexed draws for correctness.
- No per-frame `.hasset` disk I/O, GLB parsing, or immutable mesh-buffer recreation.

---

### Task 1: Integrate the three authoritative branch baselines

**Files:**
- Preserve from `feature/full-game-integration`: root `CMakeLists.txt`, `game/**`, integrated Windows client/game workflow, and its D3D11 client-specific changes.
- Import from `feature/500-asset-library`: `GameData/AssetDefinitions/**`, `Tools/ArtGeneration/**`, hardened `Tools/ContentPipeline/**`, `Art/Reference/**`, `Art/Validation/**`, and asset-pipeline workflow content.
- Import from `feature/renderer-visibility-pass`: `renderer/include/hh/renderer/Visibility.h`, `renderer/src/Visibility.cpp`, `renderer/tests/VisibilityTests.cpp`, plus its `renderer/CMakeLists.txt`, `renderer/README.md`, and demo visibility wiring.
- Preserve current branch: `docs/superpowers/specs/2026-09-09-renderer-cooked-asset-runtime-design.md` and this plan.

**Interfaces:**
- Consumes: commit `6b975ef6854a6bafc39517dfd7585d4f7b7ea1d7` (integrated game), `a79811b6d897d24203e6e49993d52d409c3bff59` (500 assets), `3fd4c5e2e4087bcb90e235cad4bdd4e60a1aec9d` (visibility).
- Produces: one branch tree containing the game client, 500-asset generation/cooking source, and visibility-enabled renderer.

- [ ] **Step 1: Create a synthetic integration commit rather than overwriting whole subsystems blindly**

Use the full-game tree as the base. Overlay asset-library subtrees for `GameData/AssetDefinitions`, `Tools/ArtGeneration`, `Art/Reference`, `Art/Validation`, and `Tools/ContentPipeline`. Overlay only the renderer-visibility files listed above so the full-game branch’s client-specific D3D changes remain intact. Preserve the current spec/plan blobs.

- [ ] **Step 2: Verify the integrated tree contains each required baseline**

Check these exact paths exist on `feature/renderer-cooked-asset-runtime`:

```text
game/app/WorldView.cpp
GameData/AssetDefinitions/hotel_haven_asset_manifest_v1.json
GameData/AssetDefinitions/Manifest/asset_batch_10.json
Tools/ArtGeneration/generate_all_assets.py
Tools/ContentPipeline/include/hh/assets/Hasset.h
renderer/include/hh/renderer/Visibility.h
renderer/src/Visibility.cpp
```

- [ ] **Step 3: Run baseline CI/build before feature code**

Run on Windows CI:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Expected: the integrated baseline builds and existing tests pass before asset-runtime implementation starts.

- [ ] **Step 4: Commit**

```bash
git commit -m "chore: integrate game renderer and asset baselines"
```

---

### Task 2: Split the runtime-safe asset format library

**Files:**
- Modify: `Tools/ContentPipeline/CMakeLists.txt`
- Existing sources reused unchanged initially: `src/Types.cpp`, `src/Json.cpp`, `src/Hasset.cpp`
- Test: existing `Tools/ContentPipeline/tests/HassetTests.cpp`, `MetadataTests.cpp`, and the complete content-pipeline test executable.

**Interfaces:**
- Produces: CMake target `hh_asset_format` exposing `hh/assets/Types.h`, `Json.h`, and `Hasset.h`.
- Produces: existing `hh_assets` target linking `PUBLIC hh_asset_format` and containing authoring/cook/catalog code.

- [ ] **Step 1: Write a build-level RED assertion**

Add a tiny renderer-side compile test source `renderer/tests/AssetFormatLinkTests.cpp`:

```cpp
#include "TestFramework.h"
#include "hh/assets/Hasset.h"

TEST_CASE("renderer links runtime hasset format without cooker") {
    hh::assets::HassetDocument document;
    document.asset_id = "HH_A001";
    const auto bytes = hh::assets::serialize_hasset(document);
    const auto roundTrip = hh::assets::parse_hasset(bytes);
    EXPECT_EQ(roundTrip.asset_id, std::string("HH_A001"));
}
```

Link `hh_renderer_tests` against the planned `hh_asset_format` target. Before the target exists, configure/build must fail with a missing target/include relationship.

- [ ] **Step 2: Run RED**

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

Expected: FAIL because `hh_asset_format` does not exist yet.

- [ ] **Step 3: Implement the target split**

Change `Tools/ContentPipeline/CMakeLists.txt` to the following target structure:

```cmake
add_library(hh_asset_format STATIC
    src/Types.cpp
    src/Json.cpp
    src/Hasset.cpp
)
target_include_directories(hh_asset_format PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
hh_enable_strict_warnings(hh_asset_format)

add_library(hh_assets STATIC
    src/Metadata.cpp
    src/Catalog.cpp
    src/DependencyGraph.cpp
    src/Hash.cpp
    src/Fingerprint.cpp
    src/Cooker.cpp
    src/Exporter.cpp
    src/Cli.cpp
)
target_include_directories(hh_assets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hh_assets PUBLIC hh_asset_format)
hh_enable_strict_warnings(hh_assets)
```

Do not change `.hasset` serialization bytes or public header names.

- [ ] **Step 4: Run GREEN**

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all existing content-pipeline and renderer tests pass.

- [ ] **Step 5: Commit**

```bash
git commit -m "refactor(assets): split runtime asset format target"
```

---

### Task 3: Add a strict CPU GLB decoder for the generated library

**Files:**
- Create: `renderer/include/hh/renderer/assets/MeshAsset.h`
- Create: `renderer/include/hh/renderer/assets/GlbLoader.h`
- Create: `renderer/src/assets/GlbLoader.cpp`
- Create: `renderer/tests/GlbLoaderTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**
- Consumes: `hh_asset_format` JSON parser and `hh::assets::AssetType`.
- Produces:

```cpp
namespace hh::renderer {
struct MeshVertex { Vec3 position{}; };
struct MeshMaterial {
    Color baseColor{1,1,1,1};
    float metallic{};
    float roughness{1};
    bool transparent{};
};
struct MeshSubmesh {
    std::uint32_t firstIndex{};
    std::uint32_t indexCount{};
    std::uint32_t materialIndex{};
};
struct CpuMeshAsset {
    std::string assetId;
    hh::assets::AssetType assetType{};
    Aabb localBounds{};
    std::vector<MeshVertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<MeshSubmesh> submeshes;
    std::vector<MeshMaterial> materials;
};

CpuMeshAsset decodeGlb(std::string_view assetId,
                       hh::assets::AssetType type,
                       std::span<const std::byte> bytes);
}
```

- [ ] **Step 1: Write RED tests using deterministic in-memory GLB fixtures**

The test helper must construct a valid GLB 2.0 with padded JSON/BIN chunks containing one triangle. Test these exact behaviors:

```cpp
TEST_CASE("glb loader converts asset Z-up to renderer Y-up") {
    const auto mesh = decodeGlb("HH_TEST", hh::assets::AssetType::StaticMeshAsset,
                                makeTriangleGlb({{0,0,0},{0,1,0},{0,0,1}}));
    EXPECT_NEAR(mesh.vertices[1].position.z, 1.0f, 0.0001f);
    EXPECT_NEAR(mesh.vertices[2].position.y, 1.0f, 0.0001f);
}

TEST_CASE("glb loader reverses winding after basis swap") {
    const auto mesh = decodeGlb("HH_TEST", hh::assets::AssetType::StaticMeshAsset,
                                makeIndexedTriangleGlb({0,1,2}));
    EXPECT_EQ(mesh.indices[0], 0u);
    EXPECT_EQ(mesh.indices[1], 2u);
    EXPECT_EQ(mesh.indices[2], 1u);
}

TEST_CASE("glb loader preserves transparent base color") {
    const auto mesh = decodeGlb("HH_TEST", hh::assets::AssetType::StaticMeshAsset,
                                makeMaterialGlb({0.34f,0.55f,0.64f,0.42f}));
    EXPECT_NEAR(mesh.materials[0].baseColor.a, 0.42f, 0.0001f);
    EXPECT_TRUE(mesh.materials[0].transparent);
}
```

Also add malformed accessor/index and node-matrix tests.

- [ ] **Step 2: Run RED**

Expected: compile fails because `GlbLoader.h`/`decodeGlb` do not exist.

- [ ] **Step 3: Implement GLB container validation**

Validate magic `0x46546C67`, version `2`, declared byte length, JSON chunk type `0x4E4F534A`, BIN chunk type `0x004E4942`, 4-byte chunk alignment, and all JSON/BIN ranges before reading data.

- [ ] **Step 4: Implement accessors/primitives/materials**

Support current-library component types for positions/indices and primitive mode `4` only. Flatten node hierarchy by applying each node’s matrix or TRS to primitive positions before the axis conversion. Append all primitives into one `CpuMeshAsset`; preserve submesh boundaries and material indices.

Material rule:

```cpp
material.transparent = alphaMode == "BLEND" || material.baseColor.a < 0.999f;
```

- [ ] **Step 5: Compute finite local bounds from decoded renderer-space positions**

Reject empty/non-finite geometry. Bounds are min/max over all decoded positions after node transform and axis conversion.

- [ ] **Step 6: Run GREEN**

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all GLB loader tests pass.

- [ ] **Step 7: Commit**

```bash
git commit -m "feat(renderer): decode cooked glb mesh payloads"
```

---

### Task 4: Add `RuntimeAssetRegistry` and stable renderer handles

**Files:**
- Create: `renderer/include/hh/renderer/assets/RuntimeAssetRegistry.h`
- Create: `renderer/src/assets/RuntimeAssetRegistry.cpp`
- Create: `renderer/tests/RuntimeAssetRegistryTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**

```cpp
struct AssetHandle {
    std::uint32_t value{};
    friend bool operator==(AssetHandle, AssetHandle) = default;
};

class RuntimeAssetRegistry {
public:
    explicit RuntimeAssetRegistry(std::filesystem::path assetRoot);
    AssetHandle loadRequired(std::string_view assetId);
    std::optional<AssetHandle> tryLoad(std::string_view assetId, std::string& error);
    void preload(std::span<const std::string_view> assetIds);
    const CpuMeshAsset& resource(AssetHandle handle) const;
    const Aabb& localBounds(AssetHandle handle) const;
    std::size_t loadedCount() const noexcept;
private:
    std::filesystem::path assetRoot_;
    std::unordered_map<std::string, AssetHandle> handles_;
    std::vector<CpuMeshAsset> resources_;
};
```

- [ ] **Step 1: Write RED registry tests**

Create temporary `.hasset` files in the test using the existing serializer and the in-memory GLB fixture. Verify:

```cpp
const auto first = registry.loadRequired("HH_A001");
const auto second = registry.loadRequired("HH_A001");
EXPECT_EQ(first, second);
EXPECT_EQ(registry.loadedCount(), static_cast<std::size_t>(1));
```

Also verify missing path diagnostic includes `HH_MISSING.hasset`, envelope-ID mismatch throws/fails explicitly, non-GLB payload is rejected, and bounds are finite/non-empty.

- [ ] **Step 2: Run RED**

Expected: compile failure because `RuntimeAssetRegistry` does not exist.

- [ ] **Step 3: Implement lazy load/cache**

For `loadRequired("HH_A001")`, read exactly:

```text
<assetRoot>/HH_A001.hasset
```

Parse using `hh::assets::parse_hasset`, require `document.asset_id == requestedId`, and call `decodeGlb` on `document.payload`. Insert one `CpuMeshAsset` into `resources_` and return its index as the handle. Never inspect `Art/Exports`.

- [ ] **Step 4: Run GREEN and commit**

```powershell
ctest --test-dir build -C Release --output-on-failure
git commit -m "feat(renderer): add cooked runtime asset registry"
```

---

### Task 5: Extend scene composition and visibility to mesh asset instances

**Files:**
- Modify: `renderer/include/hh/renderer/RenderScene.h`
- Modify: `renderer/src/SceneComposer.cpp`
- Modify: `renderer/src/Visibility.cpp`
- Modify: `renderer/tests/SceneComposerTests.cpp`
- Modify: `renderer/tests/VisibilityTests.cpp`

**Interfaces:**

Add:

```cpp
struct AssetTransform {
    Vec3 translation{};
    Vec3 rotationDegrees{};
    Vec3 scale{1,1,1};
};
struct AssetRenderItem {
    AssetHandle asset{};
    AssetTransform transform{};
    Color tint{1,1,1,1};
    int floorId{};
    RenderCategory category{RenderCategory::Object};
    Aabb bounds{};
};
struct ComposedAsset {
    AssetRenderItem item;
    bool cutaway{};
};
```

Extend `RenderScene` with `std::vector<AssetRenderItem> assetItems;` and `ComposedScene` with `opaqueAssets`, `translucentAssets`, `wireframeAssets`.

- [ ] **Step 1: Write RED scene-policy tests**

Verify hidden-floor assets are excluded, context-floor tint alpha is multiplied by `0.25`, blueprint wall assets route to `wireframeAssets`, cutaway marks only wall-category assets, and non-wall asset categories remain full-height.

- [ ] **Step 2: Write RED visibility tests**

Add one off-camera `AssetRenderItem` and one visible item with explicit world AABBs. Verify only the visible asset remains. Add near/far translucent assets and verify stable back-to-front ordering by transformed bound center/view depth.

- [ ] **Step 3: Implement composition symmetrically with boxes**

Do not infer wall semantics from `AssetHandle`. Use only `item.category`.

- [ ] **Step 4: Implement visibility symmetrically with boxes**

Reuse the existing clip-space AABB test; apply it to asset world bounds. Preserve existing box behavior unchanged.

- [ ] **Step 5: Run GREEN and commit**

```powershell
ctest --test-dir build -C Release --output-on-failure
git commit -m "feat(renderer): compose and cull asset instances"
```

---

### Task 6: Add D3D11 GPU mesh caching and instanced asset draws

**Files:**
- Modify: `renderer/src/d3d11/D3D11Renderer.h`
- Modify: `renderer/src/d3d11/D3D11Renderer.cpp`
- Create: `renderer/shaders/InstancedMesh.hlsl`
- Modify: `renderer/tests/D3D11SmokeTests.cpp`
- Modify: `renderer/CMakeLists.txt`

**Interfaces:**
- Change renderer entry point to:

```cpp
RendererResult render(const ComposedScene& scene,
                      const OrthoCamera& camera,
                      const RuntimeAssetRegistry& assets);
```

- Add GPU cache keyed by `AssetHandle.value`:

```cpp
struct GpuMeshResource {
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    std::vector<MeshSubmesh> submeshes;
    std::vector<MeshMaterial> materials;
    Aabb localBounds{};
};
std::unordered_map<std::uint32_t, GpuMeshResource> meshCache_;
```

- [ ] **Step 1: Write RED WARP cache tests**

Add a test-only observable such as `std::size_t cachedMeshCount() const noexcept`. Create a WARP-backed renderer/device fixture, load one CPU mesh resource, request the same asset twice, and assert the cache count remains `1`.

- [ ] **Step 2: Run RED**

Expected: compile failure because asset rendering/cache API is missing.

- [ ] **Step 3: Implement immutable GPU resource creation**

Create one immutable vertex buffer from `CpuMeshAsset::vertices` and one immutable `R32_UINT` index buffer from `indices`. Upload only on cache miss.

- [ ] **Step 4: Implement asset instance world matrices**

Compose scale, XYZ Euler rotation, and translation in renderer world space. For `cutaway == true`, apply the same `0.35f` Y-height factor as box walls while preserving the mesh’s transformed bottom plane: compute local base from `resource.localBounds.min.y`, scale around that base, then apply the instance transform.

- [ ] **Step 5: Implement opaque instanced draw grouping**

Group visible opaque asset instances by `(AssetHandle, submesh index)`. For each group, stream existing `InstanceData` world matrices/tints and call `DrawIndexedInstanced` once per compatible group/chunk. Multiply instance tint by the submesh material base color before upload.

- [ ] **Step 6: Implement transparent draw ordering**

For any material with `transparent == true`, or any context/translucent asset item, use alpha blending with depth read/no write. Build an order-dependent draw list using each instance’s world-bound center transformed to view space and sort far-to-near. Transparent instances may use individual indexed draws rather than cross-depth instancing.

- [ ] **Step 7: Add flat presentation lighting shader**

`InstancedMesh.hlsl` must pass world position from VS to PS and compute a face normal from derivatives:

```hlsl
float3 n = normalize(cross(ddx(input.worldPos), ddy(input.worldPos)));
float3 lightDir = normalize(float3(-0.45, 0.80, -0.35));
float diffuse = 0.35 + 0.65 * saturate(dot(n, lightDir));
return float4(input.color.rgb * diffuse, input.color.a);
```

Flip the normal when needed using `SV_IsFrontFace` so backface-disabled/wireframe behavior does not invert lighting unexpectedly.

- [ ] **Step 8: Preserve box/debug draw path**

Existing unit-cube drawing remains for selections, previews, and explicit fallback diagnostics.

- [ ] **Step 9: Run GREEN and commit**

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
git commit -m "feat(renderer): render cached cooked mesh assets"
```

---

### Task 7: Resolve production handles once and replace procedural game presentation

**Files:**
- Modify: `game/app/WorldView.h`
- Modify: `game/app/WorldView.cpp`
- Modify: `game/app/WorldViewTests.cpp`
- Modify: `game/app/Client.h`
- Modify: `game/app/GameMain.cpp`
- Modify: root `CMakeLists.txt` and/or `game/CMakeLists.txt` as required for runtime asset linkage/copying.

**Interfaces:**

Add a startup-resolved table:

```cpp
struct WorldAssetHandles {
    AssetHandle floor{ }, wall{ }, door{ }, entrance{ }, stairs{ };
    AssetHandle receptionDesk{ }, supplyCabinet{ };
    AssetHandle bed{ }, nightstand{ }, desk{ }, deskChair{ };
    AssetHandle vanity{ }, toilet{ }, showerGlass{ };
    AssetHandle plant{ };
    AssetHandle guestMale{ }, guestFemale{ };
    AssetHandle receptionistMale{ }, receptionistFemale{ };
    AssetHandle housekeeperMale{ }, housekeeperFemale{ };
    AssetHandle maintenanceMale{ }, maintenanceFemale{ };
};

WorldAssetHandles resolveWorldAssets(RuntimeAssetRegistry& registry);
RenderScene worldScene(const hh::game::SimulationView&,
                       const WorldViewOptions&,
                       const WorldAssetHandles&);
```

Use these exact existing production IDs for the first integrated client mapping:

```text
floor                HH_A071  Carpet Tile Warm Beige
wall                 HH_A004  Interior Wall Cream
door                 HH_A012  Standard Guest Room Door
entrance             HH_A057  Lobby Automatic Door
stairs               HH_A030  Straight Stair
receptionDesk        HH_A186  Reception Desk Single
supplyCabinet        HH_A141  Standard Wardrobe
bed                  HH_A113  Queen Guest Bed
nightstand           HH_A121  Standard Nightstand
desk                 HH_A126  Guest Room Desk Compact
deskChair            HH_A129  Desk Chair Upholstered
vanity               HH_A166  Bathroom Vanity Single
toilet               HH_A169  Bathroom Toilet Standard
showerGlass           HH_A171  Bathroom Shower Glass
plant                HH_A440  Planter Exterior Large
guestMale            HH_A451  Guest Businessman
guestFemale          HH_A452  Guest Businesswoman
receptionistMale     HH_A481  Receptionist Man
receptionistFemale   HH_A482  Receptionist Woman
housekeeperMale      HH_A487  Housekeeper Man
housekeeperFemale    HH_A488  Housekeeper Woman
maintenanceMale      HH_A495  Maintenance Technician Man
maintenanceFemale    HH_A496  Maintenance Technician Woman
```

- [ ] **Step 1: Write RED WorldView tests**

Given a tiny snapshot containing one wall tile, one front desk, one room with one bed/bath, and one person of each role, assert `worldScene(...).assetItems` contains the corresponding handles and that production furnishing/person geometry is no longer emitted as `BoxRenderItem` objects.

Keep selection outlines, hover previews, room status/cleanliness/condition overlay rugs, and explicit debug/fallback markers as boxes.

- [ ] **Step 2: Run RED**

Expected: tests fail because `WorldAssetHandles` and asset-backed `worldScene` do not exist.

- [ ] **Step 3: Implement an `asset(...)` helper in `WorldView.cpp`**

The helper accepts handle, floor, world X/Z, local Y offset, rotation/scale, tint, and category. It computes world bounds from the registry-derived local bounds before scene composition. Do not resolve asset IDs inside this helper or frame loop.

- [ ] **Step 4: Replace procedural normal-world geometry**

Use production assets for floor tiles, walls, doors, entrance, stairs, front desk, supply cabinet, room beds/nightstands/desks/chairs/bath fixtures/plants, and people. Choose guest/staff gender variant deterministically from entity ID parity; do not add gameplay state.

- [ ] **Step 5: Initialize runtime assets in the Windows client**

At client startup, locate `data/assets` beside `hotel_haven.exe`, construct `RuntimeAssetRegistry`, call `resolveWorldAssets` once, and pass the table into every `worldScene` call. Required startup asset failure must abort with the asset ID/path diagnostic.

- [ ] **Step 6: Pass registry to renderer**

Change client render invocation to:

```cpp
const auto scene = worldScene(snapshot, viewOptions, worldAssets);
const auto composed = composer.compose(scene, contextMode, wallMode, camera.worldPosition());
const auto visible = prepareVisibleScene(composed, camera);
renderer.render(visible, camera, runtimeAssets);
```

- [ ] **Step 7: Run GREEN and commit**

```powershell
ctest --test-dir build -C Release --output-on-failure
git commit -m "feat(game): present hotel world with production assets"
```

---

### Task 8: Build, cook, package, and verify all 500 assets end-to-end

**Files:**
- Create: `renderer/tests/AllAssetsIntegration.cpp` or a dedicated integration executable under `renderer/tests/integration/`
- Create/Modify: `.github/workflows/asset-renderer-integration.yml`
- Modify: root packaging commands in `CMakeLists.txt` / `.github/workflows/game.yml`
- Modify: `README.md`, `renderer/README.md`, `docs/IMPLEMENTATION_STATUS.md`

**Interfaces:**
- Consumes authoritative master manifest and all ten batch manifests.
- Produces `Build/CookedAssets/*.hasset` during CI and packaged `data/assets/*.hasset` beside the game binary.

- [ ] **Step 1: Add a full-library RED integration test**

The executable reads `GameData/AssetDefinitions/hotel_haven_asset_manifest_v1.json`, enumerates exactly 500 gameplay IDs from the ten referenced batch files, loads each from an injected cooked root through `RuntimeAssetRegistry`, and reports counters:

```text
gameplay_ids=500
loaded=500
static=450
skinned=50
invalid_bounds=0
```

Before generation/cooking/package wiring exists, this integration job must fail because cooked files are absent.

- [ ] **Step 2: Run the existing art generator in CI**

```powershell
python Tools/ArtGeneration/generate_all_assets.py .
```

Require the existing generator/quality tests to pass before cooking.

- [ ] **Step 3: Build content pipeline and cook all generated assets**

Use the existing `asset` CLI against `Art/Exports` and write to `Build/CookedAssets`. Do not add a runtime bypass around the cooker.

- [ ] **Step 4: Run all-500 CPU integration test**

Expected exact counts: `500 loaded`, `450 StaticMeshAsset`, `50 SkinnedMeshAsset`, `0 invalid bounds`.

- [ ] **Step 5: Add representative WARP GPU smoke coverage**

Preload and create GPU buffers for at least:

```text
HH_A004 wall
HH_A113 bed
HH_A186 reception desk
HH_A291 housekeeping cart
HH_A440 planter
HH_A451 guest character
```

Expected: no D3D11 resource-creation failure and one cached GPU mesh per unique handle.

- [ ] **Step 6: Package cooked assets with Windows game**

The `Hotel-Haven-Windows` artifact must contain:

```text
hotel_haven.exe
data/balance.json
data/assets/HH_A001.hasset
...
data/assets/HH_A500.hasset
shaders/InstancedBox.hlsl
shaders/InstancedMesh.hlsl
```

- [ ] **Step 7: Final verification**

Run fresh Windows CI commands:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

Then require the dedicated asset-renderer integration workflow and Windows game packaging/smoke workflow to conclude `success`.

- [ ] **Step 8: Update implementation status accurately**

State that the commercial asset library is now called by the renderer/game for static/bind-pose presentation. Do **not** claim skeletal animation, textures, full PBR, hot reload, or all HMG-071–079 runtime features.

- [ ] **Step 9: Commit**

```bash
git commit -m "test: verify all production assets in renderer runtime"
```

---

## Plan Self-Review

- Spec coverage: runtime-only `.hasset` loading, GLB subset, coordinate conversion, material alpha compatibility, static/bind-pose assets, handle/cache ownership, floor/cutaway/blueprint/visibility behavior, demo/game consumption, all-500 integration, WARP smoke, and packaging are each assigned to explicit tasks.
- User-intent coverage: Task 7 targets the actual `game/app/WorldView.cpp` procedural presentation adapter, not only the renderer demo, so the in-game renderer calls the production assets.
- Placeholder scan: no `TODO`, `TBD`, generic “add tests”, or undefined follow-up work is used as an implementation instruction.
- Type consistency: `AssetHandle`, `CpuMeshAsset`, `RuntimeAssetRegistry`, `AssetRenderItem`, `WorldAssetHandles`, and the new `D3D11Renderer::render(..., RuntimeAssetRegistry&)` signature are introduced before downstream tasks consume them.
