# Renderer Foundation C Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a standalone Win32 + Direct3D 11 orthographic 2.5D renderer foundation for Hotel Haven with camera, floor visibility, wall cutaway/blueprint modes, focus occlusion, instanced box rendering, picking, tests, and Windows CI.

**Architecture:** Keep all renderer implementation under `renderer/` so the simulation foundation can be developed independently. Pure CPU presentation policy composes renderer-facing scene data into opaque/translucent/wireframe batches; a D3D11 backend draws those batches using one unit cube plus dynamic instance data.

**Tech Stack:** C++20, Win32, Direct3D 11, DirectXMath, HLSL Shader Model 5.0, CMake 3.25+, CTest, GitHub Actions `windows-latest`.

**Spec:** `docs/superpowers/specs/2026-09-05-renderer-foundation-c-design.md`

## Global Constraints

- Platform target: Windows 11 x64.
- Projection: orthographic.
- Default camera pitch: exactly 55 degrees; accepted pitch range 45–65 degrees.
- Default camera rotation input: 90-degree yaw increments; camera API may accept arbitrary yaw for optional smooth rotation.
- Wall render states: `FullHeight`, `Cutaway`, `Blueprint`.
- Active floor is full detail; normal mode hides all other floors; context mode shows only immediately adjacent floors as translucent shells.
- Rendering state is presentation-only and must never become authoritative simulation state.
- No Guest AI, Construction, Economics, Staff, Logistics, entity-ID, simulation-clock, or scheduler implementation in this branch.
- No root `CMakeLists.txt` changes.

---

### Task 1: Build/test harness and RED policy tests

**Files:**
- Create: `renderer/CMakeLists.txt`
- Create: `renderer/cmake/CompilerWarnings.cmake`
- Create: `renderer/tests/TestFramework.h`
- Create: `renderer/tests/TestMain.cpp`
- Create: `renderer/tests/CameraTests.cpp`
- Create: `renderer/tests/FloorVisibilityTests.cpp`
- Create: `renderer/tests/OcclusionTests.cpp`
- Create: `renderer/tests/SceneComposerTests.cpp`
- Create: `.github/workflows/renderer-ci.yml`

**Interfaces:**
- Tests consume the APIs defined in Tasks 2–3.
- CMake initially references those not-yet-created production files so the first CI run must fail for missing renderer implementation.

- [ ] **Step 1: Create a no-dependency test framework**

`TestFramework.h` defines `TEST_CASE(name)`, `EXPECT_TRUE`, `EXPECT_FALSE`, `EXPECT_EQ`, `EXPECT_NEAR`, a static registry, and an exception-based failure type. `TestMain.cpp` executes every registered test, prints pass/fail lines, and returns nonzero if any test fails.

- [ ] **Step 2: Write failing camera tests**

Tests assert:

```cpp
hh::renderer::OrthoCamera camera;
EXPECT_NEAR(camera.pitchDegrees(), 55.0f, 0.0001f);
camera.setPitchDegrees(10.0f);
EXPECT_NEAR(camera.pitchDegrees(), 45.0f, 0.0001f);
camera.setPitchDegrees(90.0f);
EXPECT_NEAR(camera.pitchDegrees(), 65.0f, 0.0001f);
camera.setYawDegrees(361.0f);
EXPECT_NEAR(camera.yawDegrees(), 1.0f, 0.0001f);
EXPECT_NEAR(hh::renderer::OrthoCamera::snapYaw90(136.0f), 180.0f, 0.0001f);
```

- [ ] **Step 3: Write failing floor visibility tests**

Normal mode expects only the active floor to return `Full`. Context mode expects `active-1` and `active+1` to return `TranslucentShell` and all others `Hidden`.

- [ ] **Step 4: Write failing occlusion tests**

Test a segment from `(0,5,-10)` to `(0,1,0)` against an AABB centered in front of the target and assert hit; move the AABB to x=10 and assert miss.

- [ ] **Step 5: Write failing scene-composer tests**

Create items across floors and assert hidden floors are excluded, adjacent context floors become translucent, blueprint walls go to the wireframe batch, and a full-height wall intersecting the focus segment is marked cutaway while an off-axis wall is not.

- [ ] **Step 6: Add Windows CI**

Workflow triggers on pull requests and pushes affecting `renderer/**` or the workflow. Commands:

```powershell
cmake -S renderer -B renderer/build -A x64
cmake --build renderer/build --config Release
ctest --test-dir renderer/build -C Release --output-on-failure
```

- [ ] **Step 7: Commit RED state and verify CI fails because production headers/sources are missing**

Commit message: `test: define renderer foundation behavior`

---

### Task 2: Camera, floor visibility, and occlusion GREEN

**Files:**
- Create: `renderer/include/hh/renderer/MathTypes.h`
- Create: `renderer/include/hh/renderer/Camera.h`
- Create: `renderer/src/Camera.cpp`
- Create: `renderer/include/hh/renderer/FloorVisibility.h`
- Create: `renderer/src/FloorVisibility.cpp`
- Create: `renderer/include/hh/renderer/Occlusion.h`
- Create: `renderer/src/Occlusion.cpp`

**Interfaces:**
- Produces `Vec3`, `Color`, `Aabb`, `OrthoCamera`, `FloorVisibilityPolicy`, `segmentIntersectsAabb`.
- Task 3 consumes all of them.

- [ ] **Step 1: Implement renderer POD math types**

`Vec3`, `Color`, and `Aabb` are aggregate structs of floats only. Add `Vec3` `+`, `-`, and scalar multiplication helpers required by occlusion and camera positioning.

- [ ] **Step 2: Implement `OrthoCamera`**

Exact API:

```cpp
class OrthoCamera {
public:
    OrthoCamera() noexcept;
    void setTarget(Vec3 target) noexcept;
    Vec3 target() const noexcept;
    void setPitchDegrees(float value) noexcept;
    float pitchDegrees() const noexcept;
    void setYawDegrees(float value) noexcept;
    float yawDegrees() const noexcept;
    static float snapYaw90(float value) noexcept;
    void rotateSnapped(int quarterTurns) noexcept;
    void setOrthoHeight(float value) noexcept;
    float orthoHeight() const noexcept;
    void setAspectRatio(float value) noexcept;
    float aspectRatio() const noexcept;
    DirectX::XMMATRIX viewMatrix() const noexcept;
    DirectX::XMMATRIX projectionMatrix() const noexcept;
    DirectX::XMMATRIX viewProjectionMatrix() const noexcept;
    Vec3 worldPosition() const noexcept;
};
```

Clamp pitch with `std::clamp`, normalize yaw to `[0,360)`, and implement `snapYaw90` with nearest-quarter-turn rounding. `viewMatrix` uses a camera direction derived from pitch/yaw and `XMMatrixLookAtLH`. `projectionMatrix` uses `XMMatrixOrthographicLH(orthoHeight*aspect, orthoHeight, 0.1f, 2000.0f)`.

- [ ] **Step 3: Implement floor visibility policy**

Exact enums/API:

```cpp
enum class FloorContextMode { Normal, AdjacentContext };
enum class FloorRenderVisibility { Hidden, Full, TranslucentShell };
FloorRenderVisibility floorVisibility(int itemFloor, int activeFloor, FloorContextMode mode) noexcept;
```

- [ ] **Step 4: Implement segment/AABB intersection**

Use the slab method over the finite segment parameter range `[0,1]`. Parallel-axis handling: if direction magnitude on an axis is below `1e-6f`, return false when the origin coordinate lies outside that slab.

- [ ] **Step 5: Run tests and require camera/floor/occlusion tests to pass**

Command: `ctest --test-dir renderer/build -C Release --output-on-failure`.

- [ ] **Step 6: Commit**

Commit message: `feat: add renderer camera and visibility policies`

---

### Task 3: Pure scene composition GREEN

**Files:**
- Create: `renderer/include/hh/renderer/RenderScene.h`
- Create: `renderer/include/hh/renderer/SceneComposer.h`
- Create: `renderer/src/SceneComposer.cpp`

**Interfaces:**
- Produces renderer-facing scene DTOs and `SceneComposer::compose`.
- Task 4 consumes `ComposedScene`.

- [ ] **Step 1: Define render DTOs**

Exact enums/structs:

```cpp
enum class RenderCategory { Floor, Wall, Object, Selection };
enum class WallRenderMode { FullHeight, Cutaway, Blueprint };

struct BoxRenderItem {
    Vec3 center{};
    Vec3 size{1.0f,1.0f,1.0f};
    Color color{1.0f,1.0f,1.0f,1.0f};
    int floorId{};
    RenderCategory category{RenderCategory::Object};
    Aabb bounds{};
};

struct RenderScene {
    std::vector<BoxRenderItem> items;
    std::optional<Vec3> focusTarget;
    int activeFloor{};
};

struct ComposedBox {
    BoxRenderItem item;
    bool cutaway{};
};

struct ComposedScene {
    std::vector<ComposedBox> opaque;
    std::vector<ComposedBox> translucent;
    std::vector<ComposedBox> wireframe;
};
```

- [ ] **Step 2: Implement `SceneComposer::compose`**

Exact signature:

```cpp
ComposedScene compose(const RenderScene& scene,
                      FloorContextMode contextMode,
                      WallRenderMode wallMode,
                      Vec3 cameraWorldPosition) const;
```

For each item: apply floor visibility first; skip `Hidden`. For wall items, `Blueprint` selects wireframe, `Cutaway` sets `cutaway=true`, and `FullHeight` sets `cutaway=true` only if a focus target exists and `segmentIntersectsAabb(cameraWorldPosition, focusTarget, item.bounds)` returns true. `TranslucentShell` multiplies alpha by exactly `0.25f` and goes to `translucent`. Blueprint walls go to `wireframe`; otherwise full-floor items go to `opaque`.

- [ ] **Step 3: Run all pure tests and verify GREEN**

- [ ] **Step 4: Commit**

Commit message: `feat: compose multi-floor renderer scene`

---

### Task 4: D3D11 instanced backend and shader smoke test

**Files:**
- Create: `renderer/src/d3d11/D3D11Renderer.h`
- Create: `renderer/src/d3d11/D3D11Renderer.cpp`
- Create: `renderer/shaders/InstancedBox.hlsl`
- Create: `renderer/tests/D3D11SmokeTests.cpp`

**Interfaces:**
- Consumes `ComposedScene` and `OrthoCamera`.
- Produces `D3D11Renderer::initialize(HWND,uint32_t,uint32_t)`, `resize`, `render`, and `shutdown`.

- [ ] **Step 1: Add RED backend smoke test**

Create a WARP `ID3D11Device` with `D3D11CreateDevice` and compile `VSMain`/`PSMain` from the production shader path using `D3DCompileFromFile`. Assert device and shader blobs are non-null.

- [ ] **Step 2: Create production HLSL**

Input layout semantics: `POSITION`, `WORLD0`, `WORLD1`, `WORLD2`, `WORLD3`, `COLOR`. Camera constant buffer contains `float4x4 viewProjection`. Vertex shader computes `mul(mul(float4(position,1), world), viewProjection)` and forwards color; pixel shader returns color.

- [ ] **Step 3: Implement D3D11 resources**

Create device/swap chain, RTV, 24-bit-depth/8-bit-stencil DSV, viewport, unit cube VB/IB, dynamic instance buffer sized for 16,384 instances, camera constant buffer, shaders/input layout, solid/wireframe rasterizer states, opaque/alpha blend states, and depth states.

- [ ] **Step 4: Implement batched draw path**

Convert each `ComposedBox` into an instance world transform. Demo cutaway rendering multiplies wall Y scale by `0.35f` and shifts center down so the base stays fixed; this factor is renderer-demo presentation behavior, not a Construction rule. Draw opaque, translucent, and wireframe batches separately with `DrawIndexedInstanced`.

- [ ] **Step 5: Implement resize and Present error reporting**

Skip zero dimensions. Recreate only RTV/DSV/viewport. Return a diagnostic on device removed/reset.

- [ ] **Step 6: Run CTest including WARP/shader smoke test**

- [ ] **Step 7: Commit**

Commit message: `feat: add Direct3D 11 instanced renderer`

---

### Task 5: Win32 interactive renderer demo

**Files:**
- Create: `renderer/src/win32/Win32Window.h`
- Create: `renderer/src/win32/Win32Window.cpp`
- Create: `renderer/app/RendererDemo.cpp`

**Interfaces:**
- Consumes `OrthoCamera`, `RenderScene`, `SceneComposer`, and `D3D11Renderer`.
- Produces `hotel_haven_renderer_demo.exe`.

- [ ] **Step 1: Implement RAII Win32 window**

Register one window class, create a resizable overlapped window, expose `HWND`, client width/height, close state, resize notification, mouse wheel delta, left-click coordinates, and key-down edge events.

- [ ] **Step 2: Build procedural three-floor demo scene**

Use renderer-demo constants only: 12x10 floor footprint, 3.2 world-unit floor separation, 2.8 wall height, 0.15 wall thickness. Add floor slabs, perimeter/interior wall boxes, a few object boxes, and a selection marker. Every item gets a correct AABB.

- [ ] **Step 3: Implement controls**

`W/A/S/D` pan target; `Q/E` call `rotateSnapped(-1/+1)`; wheel scales ortho height and clamps only in the demo to `[8.0f, 120.0f]`; Page Up/Down clamp active demo floor to `[0,2]`; `C` toggles context; `1/2/3` choose wall modes; Esc closes.

- [ ] **Step 4: Implement active-floor click picking**

Use inverse view-projection unprojection from cursor NDC to near/far world points and intersect with the horizontal plane `y = activeFloor * 3.2f`. On hit, set `RenderScene.focusTarget` and move the selection marker.

- [ ] **Step 5: Render loop and resize**

Pump messages, apply input, recompute aspect, compose scene using `camera.worldPosition()`, render, and update the window title with active floor/context/wall mode.

- [ ] **Step 6: Build Release and run all CTest tests**

- [ ] **Step 7: Commit**

Commit message: `feat: add interactive 2.5D renderer demo`

---

### Task 6: Documentation, verification, and PR

**Files:**
- Create: `renderer/README.md`

- [ ] **Step 1: Document prerequisites/build/run/controls**

Include Visual Studio 2022 Desktop development with C++, Windows 11 SDK, CMake commands, executable path, controls, module boundary, and statement that demo dimensions/colors are non-authoritative.

- [ ] **Step 2: Verify branch diff contains no gameplay/simulation implementation**

Compare against `main`; only docs, workflow, and `renderer/` files are allowed.

- [ ] **Step 3: Verify CI on final head**

Require renderer build/test workflow green before claiming completion.

- [ ] **Step 4: Open a draft or ready PR targeting `main`**

Title: `Renderer foundation C: native 2.5D D3D11 slice`.

Body must identify HMG-000 coverage, non-goals, test command, controls, and integration boundary with the parallel simulation branch.

- [ ] **Step 5: Commit documentation if needed**

Commit message: `docs: document renderer foundation C`
