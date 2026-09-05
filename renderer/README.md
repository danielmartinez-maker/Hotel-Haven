# Hotel Haven Renderer Foundation C

Native Windows 2.5D presentation foundation for Hotel Haven. This module is intentionally isolated from gameplay and simulation authority so the simulation kernel can be implemented on a parallel branch and integrated later through renderer-facing scene data.

## Scope

Implemented renderer responsibilities:

- orthographic 2.5D camera;
- default camera pitch of 55 degrees with the HMG-000 45–65 degree range;
- arbitrary yaw plus deterministic 90-degree snapped rotation controls;
- active-floor visibility and optional translucent adjacent-floor context;
- wall presentation modes: full height, cutaway, and blueprint/wireframe;
- focus-based foreground-wall cutaway using camera-to-focus AABB occlusion;
- CPU scene composition into opaque, translucent, and wireframe batches;
- Direct3D 11 instanced box rendering;
- resize-safe swap-chain render targets and depth buffer;
- WARP-device and production-HLSL smoke test;
- Win32 interactive demonstration application;
- click-to-focus active-floor picking.

This module does **not** implement construction validity, rooms, authoritative world geometry, simulation timing, entities, guests, staff, economics, reservations, tasks, logistics, or any other gameplay authority.

## Requirements

- Windows 11 x64 target;
- Visual Studio 2022 or newer with **Desktop development with C++**;
- Windows SDK with Direct3D 11 / D3DCompiler headers and libraries;
- CMake 3.25 or newer.

No third-party runtime or test dependency is required.

## Configure, build, and test

From the repository root:

```powershell
cmake -S renderer -B renderer/build -A x64
cmake --build renderer/build --config Release
ctest --test-dir renderer/build -C Release --output-on-failure
```

The Release demo executable is generated under the configuration output directory, normally:

```text
renderer/build/Release/hotel_haven_renderer_demo.exe
```

CMake copies `InstancedBox.hlsl` into a `shaders/` directory next to the executable.

## Demo controls

| Input | Action |
|---|---|
| `W` / `A` / `S` / `D` | Pan camera target on the horizontal plane |
| `Q` / `E` | Rotate yaw -90 / +90 degrees |
| Mouse wheel | Zoom |
| `Page Up` / `Page Down` | Select demo floor 0–2 |
| `C` | Toggle adjacent-floor context |
| `1` | Full-height walls |
| `2` | Cutaway walls |
| `3` | Blueprint/wireframe walls |
| Left click | Set focus on the active-floor plane; foreground walls cut away when they occlude it |
| `Esc` | Exit |

## Architecture

`hh_renderer_core` contains platform-independent presentation policy and scene composition. `hh_renderer_d3d11` owns the Direct3D 11 GPU backend. `hh_renderer_win32` owns the native window and input event surface. `hotel_haven_renderer_demo` composes those modules into an inspectable procedural scene.

The renderer consumes `RenderScene` / `BoxRenderItem` presentation DTOs. Future Construction code should translate authoritative physical geometry into those DTOs; the renderer must not infer room validity or mutate gameplay state.

### GPU batching

The D3D11 backend stores one unit cube vertex/index mesh and streams per-instance transforms/colors into a dynamic instance buffer. Opaque, translucent, and wireframe groups are issued as separate instanced batches instead of one draw call per hotel object. One upload/draw chunk supports up to 16,384 instances; larger groups are automatically chunked.

## Non-authoritative demo constants

The procedural demo exists only to exercise renderer behavior before Construction is connected. These values are presentation fixtures, not Hotel Haven construction rules:

- 3 floors;
- 12 × 10 world-unit floor slabs;
- 3.2 world-unit floor separation;
- 2.8 world-unit wall height;
- 0.15 world-unit wall thickness;
- demo zoom clamp: orthographic height 8–120;
- cutaway wall display factor: 0.35 of supplied wall height;
- demo colors and object placements.

Authoritative tile scale, building validity, wall geometry, object footprints, and floor contents remain owned by the Construction subsystem when that subsystem is integrated.

## Tests

The native test executable covers:

- exact 55-degree camera default;
- camera pitch clamping to 45–65 degrees;
- yaw normalization and 90-degree snapping;
- normal and adjacent-context floor visibility;
- finite segment/AABB occlusion hit and miss behavior;
- hidden-floor exclusion;
- context-floor translucency;
- blueprint wall routing;
- focus-based selective cutaway;
- Direct3D 11 WARP device creation;
- production vertex/pixel shader compilation.

GitHub Actions runs the same configure, Release build, and CTest commands on Windows for renderer changes.

## Parallel-development boundary

Renderer Foundation C lives under `renderer/` and does not add a repository-root `CMakeLists.txt`. The simulation-foundation branch can therefore define its own root/project structure without importing renderer internals. Integration should occur after both branches land by adding the renderer subdirectory and writing an explicit adapter from authoritative world/construction data to `RenderScene`.
