# Renderer Foundation C — Design

## Status

Approved implementation direction: **Approach C — renderer-first**.

This design implements only the visual/runtime presentation requirements that can be derived from HMG-000 without inventing Guest AI, Construction, Economics, Staff, Logistics, or simulation-kernel behavior.

## Goal

Create a standalone Windows 11 x64 native C++ 2.5D renderer foundation for Hotel Haven that can display a procedural multi-floor hotel scene, provide the HMG-000 orthographic camera and floor/cutaway presentation behavior, and expose clean renderer-facing interfaces for later simulation/construction integration.

## Non-goals

- No authoritative simulation clock or scheduler.
- No entity registry or gameplay IDs.
- No room-validation logic.
- No guest/staff movement or AI.
- No booking/economics/logistics behavior.
- No final art direction or production assets.
- No root-level engine integration. This slice remains under `renderer/` so another branch can build the simulation foundation independently.

## Technology

- Language: C++20.
- Platform: Windows 11 x64.
- Windowing: raw Win32.
- Graphics: Direct3D 11.
- Math: DirectXMath.
- Shader language: HLSL Shader Model 5.0, compiled at runtime through `D3DCompileFromFile`.
- Build: CMake 3.25+ with Visual Studio 2022.
- Tests: small in-repository native test executable registered with CTest; no third-party test dependency.
- CI: GitHub Actions on `windows-latest`.

## Isolation Contract

All source, tests, shaders, and build metadata for this slice live under `renderer/`. The branch does not create or modify a root `CMakeLists.txt` and does not define simulation/gameplay classes. Future integration can add `add_subdirectory(renderer)` from the root without changing renderer internals.

## Module Layout

```text
renderer/
├── CMakeLists.txt
├── README.md
├── cmake/
│   └── CompilerWarnings.cmake
├── include/hh/renderer/
│   ├── Camera.h
│   ├── FloorVisibility.h
│   ├── MathTypes.h
│   ├── Occlusion.h
│   ├── RenderScene.h
│   └── SceneComposer.h
├── src/
│   ├── Camera.cpp
│   ├── FloorVisibility.cpp
│   ├── Occlusion.cpp
│   ├── SceneComposer.cpp
│   ├── d3d11/
│   │   ├── D3D11Renderer.h
│   │   └── D3D11Renderer.cpp
│   └── win32/
│       ├── Win32Window.h
│       └── Win32Window.cpp
├── app/
│   └── RendererDemo.cpp
├── shaders/
│   └── InstancedBox.hlsl
└── tests/
    ├── TestMain.cpp
    ├── CameraTests.cpp
    ├── FloorVisibilityTests.cpp
    ├── OcclusionTests.cpp
    └── SceneComposerTests.cpp
```

A repository-level workflow file `.github/workflows/renderer-ci.yml` is allowed because CI cannot live under the module. It only configures/builds/tests `renderer/` and therefore does not couple to the simulation branch.

## Renderer-facing data contract

The renderer does not consume gameplay objects. It consumes immutable per-frame presentation data.

### Math types

`Vec3 { float x, y, z; }`, `Color { float r, g, b, a; }`, and `Aabb { Vec3 min, max; }` are small POD renderer types. They deliberately do not encode hotel semantics.

### Render item

`BoxRenderItem` contains:

- `Vec3 center`
- `Vec3 size`
- `Color color`
- `int floorId`
- `RenderCategory category` where category is `Floor`, `Wall`, `Object`, or `Selection`
- `Aabb bounds`

Construction later converts its authoritative geometry into these presentation items.

### Scene input

`RenderScene` contains a vector of `BoxRenderItem`, an optional `focusTarget`, and the active floor. The renderer never mutates the scene.

## Camera

`OrthoCamera` owns presentation-only camera state:

- target/focus point in renderer world coordinates;
- pitch in degrees;
- yaw in degrees;
- orthographic view height;
- aspect ratio.

HMG-000 behavior:

- projection is orthographic;
- default pitch is 55 degrees;
- pitch is clamped to 45–65 degrees;
- yaw accepts arbitrary values for smooth rotation;
- default input rotates by 90-degree snapped increments;
- zoom modifies orthographic view height;
- camera logic never changes authoritative world state.

The renderer library does not assign authoritative min/max zoom values because HMG-000 does not specify them. The demo app uses presentation-only limits documented as demo constants.

## Floor visibility

`FloorVisibilityPolicy` implements two modes:

### Normal

- active floor: fully visible;
- all lower floors: hidden;
- all higher floors: hidden.

### Context

- active floor: fully visible;
- immediately adjacent floors `active-1` and `active+1`: translucent shell;
- every other floor: hidden.

The policy returns `Hidden`, `Full`, or `TranslucentShell` and contains no gameplay state.

## Wall visibility and selected-focus protection

The global wall presentation mode is one of the exact HMG-000 states:

1. `FullHeight`
2. `Cutaway`
3. `Blueprint`

For `FullHeight`, selected-focus protection may override an individual foreground wall to `Cutaway` when that wall's AABB intersects the segment from the camera position to the selected focus target before the target. This is a generic renderer occlusion test and does not require room semantics.

For `Cutaway`, all visible wall items are shortened to the renderer-provided cutaway height factor.

For `Blueprint`, wall items are drawn using a wireframe rasterizer state.

The demo scene chooses its own full wall dimensions and cutaway factor; those values are explicitly non-authoritative and will be replaced by Construction-provided geometry later.

## Scene composition

`SceneComposer` transforms `RenderScene` into three GPU batches:

- opaque boxes;
- translucent boxes;
- wireframe boxes.

Rules:

- hidden-floor items are excluded;
- context-shell items are placed in the translucent batch with reduced alpha;
- wall mode is applied after floor visibility;
- foreground focus occlusion can downgrade a full-height wall to cutaway;
- non-wall items are unaffected by wall mode;
- selection geometry is presentation-only.

This pure CPU step is unit-tested independently of Direct3D.

## Direct3D 11 backend

`D3D11Renderer` owns:

- D3D11 device and immediate context;
- DXGI swap chain;
- render-target view;
- depth-stencil texture/view;
- viewport;
- unit-cube vertex/index buffers;
- dynamic per-instance buffer;
- camera constant buffer;
- vertex/pixel shaders and input layout;
- solid and wireframe rasterizer states;
- opaque and alpha-blend states.

Each box uses instancing. A single unit cube is scaled/translated by a per-instance world matrix. This avoids one draw call per hotel object and provides the correct architectural path toward HMG-000's 10,000+ placed-object target.

Opaque, translucent, and wireframe batches are issued separately. The renderer accepts resize events and recreates only size-dependent render targets. A zero-size/minimized client area does not attempt a resize or draw.

## Shader contract

`InstancedBox.hlsl` provides `VSMain` and `PSMain`.

Vertex input:

- per-vertex `POSITION`;
- four per-instance world-matrix rows;
- per-instance color.

Constant buffer:

- camera view-projection matrix.

The pixel shader outputs the interpolated instance color. No lighting model is introduced because HMG-000 does not define one.

## Demo application

`RendererDemo` creates a procedural, deliberately non-authoritative hotel-shaped scene with three floors so renderer behavior is inspectable before Construction exists.

Controls:

- `W/A/S/D`: pan camera target on the horizontal plane;
- `Q/E`: rotate yaw by -90/+90 degrees;
- mouse wheel: zoom within demo-only presentation limits;
- `Page Up/Page Down`: change active demo floor;
- `C`: toggle adjacent-floor context mode;
- `1`: full-height walls;
- `2`: cutaway walls;
- `3`: blueprint walls;
- left mouse click: pick the active-floor plane and set the presentation focus target;
- `Esc`: exit.

The demo uses fixed procedural dimensions only to exercise rendering. They are not Construction rules.

## Picking

Screen picking unprojects a cursor position through the inverse view-projection matrix to form a ray and intersects that ray with the horizontal plane corresponding to the active demo floor. The resulting point changes only `RenderScene.focusTarget`.

## Error handling

Initialization functions return a `RendererResult` containing success/failure and an error string. HRESULT failures include the failing operation and hexadecimal HRESULT. Shader compiler diagnostics are included in errors. The demo writes fatal errors to stderr and a Win32 message box, then exits nonzero.

If `Present` reports `DXGI_ERROR_DEVICE_REMOVED` or `DXGI_ERROR_DEVICE_RESET`, the demo terminates with a diagnostic rather than silently continuing with undefined GPU state. Device recreation is outside this foundation slice.

## Testing

Pure tests cover:

- default camera pitch is exactly 55 degrees;
- pitch clamps to [45, 65];
- yaw normalizes and 90-degree snapping is deterministic;
- floor normal/context visibility rules;
- segment/AABB occlusion hit and miss cases;
- scene composition excludes hidden floors;
- context floors become translucent;
- blueprint walls become wireframe;
- focus-occluding full-height walls become cutaway while non-occluding walls remain full-height.

A Windows backend smoke test creates a D3D11 WARP device and compiles the production HLSL shader. It does not create an interactive swap chain in CI.

## Acceptance criteria

1. `cmake -S renderer -B renderer/build -A x64` succeeds with Visual Studio 2022 on Windows.
2. `cmake --build renderer/build --config Release` succeeds with warnings treated as errors for project code.
3. `ctest --test-dir renderer/build -C Release --output-on-failure` passes.
4. The demo opens a resizable native window and displays a multi-floor procedural 2.5D scene.
5. Camera is orthographic with 55-degree default pitch and 90-degree default yaw steps.
6. Floor selection, context mode, all three wall states, focus-based foreground cutaway, zoom, pan, resize, and click focus are visibly functional.
7. CPU scene composition is independent of Direct3D and has unit coverage.
8. Rendering uses instanced box batches rather than per-object draw calls.
9. No gameplay subsystem authority is implemented or duplicated.
