# Living Hotel Main Menu Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the HH-UI-MAIN-001 v0.1 Living Hotel main menu as a new C++20 frontend module layered over the existing D3D11 renderer, including deterministic menu state/navigation, property presentation, camera/transition behavior, a Direct2D/DirectWrite overlay, a standalone Windows menu demo, and CI tests.

**Architecture:** Keep model/controller/formatting/transition logic platform-agnostic in `frontend/include` + `frontend/src`. Add a narrow D3D11 interop seam so the world can render without presenting, allowing Direct2D/DirectWrite to draw on the same swap-chain backbuffer before `Present`. The menu scene is read-only presentation data and never owns or ticks gameplay simulation.

**Tech Stack:** C++20, CMake 3.25+, Win32, Direct3D 11, DXGI, Direct2D 1.1, DirectWrite, WRL, existing `hh::renderer::OrthoCamera`, existing lightweight C++ test style.

**Spec:** `docs/superpowers/specs/2026-09-09-living-hotel-main-menu.md`

## Global Constraints

- Target Windows 11 x64 only.
- C++20; no new third-party runtime dependency.
- D3D11 owns world rendering; Direct2D/DirectWrite draws only the menu overlay.
- The menu must never parse authoritative save files or advance gameplay simulation.
- Main menu order is fixed: Continue, New Hotel, Load Hotel, Scenarios, Sandbox, Settings, Credits, Quit.
- Disabled Continue remains visible and is skipped by keyboard/controller focus.
- Reduced Motion disables idle camera drift, zoom breathing, and large camera/item movement.
- UI logical reference resolution is 1920×1080 with centered safe-zone behavior on ultrawide.
- Required minimum resolution is 1280×720.
- Missing optional content must degrade to a usable neutral menu.
- Existing renderer demo behavior must remain compatible.

---

### Task 1: Pure frontend model, command mapping, and navigation

**Files:**
- Create: `frontend/CMakeLists.txt`
- Create: `frontend/include/hh/frontend/MainMenuCommands.h`
- Create: `frontend/include/hh/frontend/MainMenuModel.h`
- Create: `frontend/include/hh/frontend/MainMenuController.h`
- Create: `frontend/src/MainMenuModel.cpp`
- Create: `frontend/src/MainMenuController.cpp`
- Create: `frontend/tests/TestFramework.h`
- Create: `frontend/tests/TestMain.cpp`
- Create: `frontend/tests/MainMenuModelTests.cpp`
- Create: `frontend/tests/MainMenuNavigationTests.cpp`

**Interfaces:**
- Produces `enum class MainMenuItem`, `enum class MainMenuCommand`, `enum class MainMenuModal`, `class MainMenuModel`, and `class MainMenuController`.
- `MainMenuController::navigate(int delta)`, `activate()`, `cancel()`, `hover(MainMenuItem)`, `setPressed(MainMenuItem, bool)` mutate model and return/queue only application commands.

- [ ] **Step 1: Write failing model/navigation tests**

```cpp
TEST_CASE("menu order is specification order") {
    const auto items = hh::frontend::MainMenuModel::orderedItems();
    EXPECT_EQ(items.size(), std::size_t{8});
    EXPECT_EQ(items[0], hh::frontend::MainMenuItem::Continue);
    EXPECT_EQ(items[7], hh::frontend::MainMenuItem::Quit);
}

TEST_CASE("no save disables continue and selects new hotel") {
    hh::frontend::MainMenuModel model(false);
    EXPECT_FALSE(model.isEnabled(hh::frontend::MainMenuItem::Continue));
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::NewHotel);
}

TEST_CASE("navigation wraps and skips disabled continue") {
    hh::frontend::MainMenuModel model(false);
    hh::frontend::MainMenuController controller(model);
    controller.navigate(-1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::Quit);
    controller.navigate(1);
    EXPECT_EQ(model.selected(), hh::frontend::MainMenuItem::NewHotel);
}

TEST_CASE("quit activation opens modal before emitting exit") {
    hh::frontend::MainMenuModel model(true);
    hh::frontend::MainMenuController controller(model);
    model.select(hh::frontend::MainMenuItem::Quit);
    EXPECT_EQ(controller.activate(), hh::frontend::MainMenuCommand::None);
    EXPECT_EQ(model.modal(), hh::frontend::MainMenuModal::QuitConfirm);
    EXPECT_EQ(controller.confirmQuit(), hh::frontend::MainMenuCommand::ExitApplication);
}
```

- [ ] **Step 2: Configure the frontend test target and run it to verify RED**

Run on Windows:

```powershell
cmake -S frontend -B build/frontend -G "Visual Studio 17 2022" -A x64
cmake --build build/frontend --config Debug
ctest --test-dir build/frontend -C Debug --output-on-failure
```

Expected: compile failure because frontend model/controller types do not yet exist.

- [ ] **Step 3: Implement the minimal model/controller**

Use fixed `std::array<MainMenuItem, 8>` order, enabled flags, selected item, modal state, and command mapping. `select()` ignores disabled items. `navigate()` wraps with modulo and checks at most eight candidates so it cannot loop indefinitely.

- [ ] **Step 4: Run tests to GREEN**

Expected: model/navigation tests pass.

- [ ] **Step 5: Commit**

```bash
git add frontend
git commit -m "feat: add main menu model and navigation"
```

---

### Task 2: Snapshot/provider contracts and property formatting

**Files:**
- Create: `frontend/include/hh/frontend/MenuHotelProvider.h`
- Create: `frontend/include/hh/frontend/MainMenuView.h`
- Create: `frontend/src/MainMenuView.cpp`
- Create: `frontend/tests/MenuHotelProviderTests.cpp`
- Modify: `frontend/CMakeLists.txt`

**Interfaces:**
- Produces `MenuPropertySummary`, `MenuScenePresentation`, `MenuCameraAnchor`, `MenuHotelSnapshot`, `IMenuHotelProvider`.
- Produces `PropertyCardText MainMenuView::formatProperty(const MenuPropertySummary&) const` and `LayoutMetrics MainMenuView::layout(float physicalWidth, float physicalHeight, float uiScale) const`.

- [ ] **Step 1: Write failing formatting/layout tests**

```cpp
TEST_CASE("property values clamp and missing values use em dash") {
    hh::frontend::MenuPropertySummary summary{};
    summary.hotelName = "The Beaumont";
    summary.city = "Paris";
    summary.country = "France";
    summary.starRating = 8;
    summary.currentDay = 184;
    summary.occupancy = 0.81f;
    summary.guestSatisfaction.reset();
    summary.cashMinorUnits = 128000000;
    summary.currencyCode = "EUR";
    summary.roomCount = 247;

    hh::frontend::MainMenuView view;
    const auto card = view.formatProperty(summary);
    EXPECT_EQ(card.visualStars, 5);
    EXPECT_EQ(card.occupancy, std::string{"81%"});
    EXPECT_EQ(card.satisfaction, std::string{"—"});
    EXPECT_EQ(card.rooms, std::string{"247"});
}

TEST_CASE("ultrawide keeps centered 1920 safe zone") {
    hh::frontend::MainMenuView view;
    const auto layout = view.layout(3840.0f, 1080.0f, 1.0f);
    EXPECT_NEAR(layout.safeZoneLeft, 960.0f, 0.01f);
    EXPECT_NEAR(layout.navigationLeft, 1032.0f, 0.01f);
}
```

- [ ] **Step 2: Run RED**

Expected: missing provider/view types.

- [ ] **Step 3: Implement DTOs, provider interface, formatter, and logical layout**

Currency formatting in v0.1 is presentation-only: support ISO `EUR`, `USD`, `GBP`, `MXN` symbols and otherwise show `CODE <major-value>`. Keep the conversion deterministic and do not perform economic calculations beyond converting minor units to display major units. Use em dash for missing optional values.

- [ ] **Step 4: Run GREEN**

Expected: formatting and layout tests pass.

- [ ] **Step 5: Commit**

```bash
git add frontend
git commit -m "feat: add menu property presentation contracts"
```

---

### Task 3: Transition director, camera response, and deterministic ambient scheduler

**Files:**
- Create: `frontend/include/hh/frontend/MenuTransitionDirector.h`
- Create: `frontend/include/hh/frontend/MenuSceneController.h`
- Create: `frontend/src/MenuTransitionDirector.cpp`
- Create: `frontend/src/MenuSceneController.cpp`
- Create: `frontend/tests/MenuTransitionTests.cpp`
- Create: `frontend/tests/MenuSceneControllerTests.cpp`
- Modify: `frontend/CMakeLists.txt`

**Interfaces:**
- `MenuTransitionDirector::retarget(MainMenuItem, bool reducedMotion)` interrupts current transition and interpolates toward a new target.
- `MenuTransitionDirector::update(float deltaSeconds)` returns a normalized/eased `MenuTransitionState`.
- `MenuSceneController::cameraPose(float elapsedSeconds, const MenuTransitionState&)` returns target/yaw/pitch/orthoHeight offsets relative to `MenuCameraAnchor`.
- `AmbientPresentationScheduler(std::uint32_t seed)` produces deterministic `AmbientEvent` launches in tests without gameplay entities.

- [ ] **Step 1: Write failing transition/reduced-motion tests**

```cpp
TEST_CASE("retargeting interrupts instead of queueing") {
    hh::frontend::MenuTransitionDirector director;
    director.retarget(hh::frontend::MainMenuItem::LoadHotel, false);
    director.update(0.20f);
    director.retarget(hh::frontend::MainMenuItem::Scenarios, false);
    EXPECT_EQ(director.targetItem(), hh::frontend::MainMenuItem::Scenarios);
    EXPECT_EQ(director.queuedTransitionCount(), std::size_t{0});
}

TEST_CASE("reduced motion zeros camera offsets") {
    hh::frontend::MenuSceneController scene;
    scene.setReducedMotion(true);
    const auto pose = scene.cameraPose(8.0f, {});
    EXPECT_NEAR(pose.yawOffsetDegrees, 0.0f, 0.0001f);
    EXPECT_NEAR(pose.zoomScale, 1.0f, 0.0001f);
}

TEST_CASE("ambient scheduler is deterministic for a fixed seed") {
    hh::frontend::AmbientPresentationScheduler a(42u);
    hh::frontend::AmbientPresentationScheduler b(42u);
    EXPECT_EQ(a.nextEvent(), b.nextEvent());
    EXPECT_EQ(a.nextEvent(), b.nextEvent());
}
```

- [ ] **Step 2: Run RED**

Expected: missing transition/scene controller types.

- [ ] **Step 3: Implement eased, interruptible state and presentation-only scheduling**

Use smoothstep easing and a 0.50 s default hover transition. Idle yaw is bounded to ±2°, breathing to ±3%. Reduced Motion returns the base anchor and bypasses those offsets. Ambient events are enum-only presentation sequences with a deterministic `std::mt19937` seed and randomized 5–15 s major-event interval.

- [ ] **Step 4: Add 10,000-navigation stress test**

```cpp
TEST_CASE("ten thousand navigation events preserve valid focus") {
    hh::frontend::MainMenuModel model(false);
    hh::frontend::MainMenuController controller(model);
    for (int i = 0; i < 10000; ++i) {
        controller.navigate((i % 3) == 0 ? -1 : 1);
        EXPECT_TRUE(model.isEnabled(model.selected()));
    }
}
```

- [ ] **Step 5: Run GREEN and commit**

```bash
git add frontend
git commit -m "feat: add menu transitions and living presentation state"
```

---

### Task 4: Split D3D11 world draw from Present and expose narrow interop handles

**Files:**
- Modify: `renderer/src/d3d11/D3D11Renderer.h`
- Modify: `renderer/src/d3d11/D3D11Renderer.cpp`
- Modify: `renderer/tests/D3D11SmokeTests.cpp`

**Interfaces:**
- Add `RendererResult renderWorld(const ComposedScene&, const OrthoCamera&)`.
- Add `RendererResult present()`.
- Keep `render(...)` as `renderWorld(...)` followed by `present()` for backward compatibility.
- Add read-only accessors `ID3D11Device* device() const noexcept` and `IDXGISwapChain* swapChain() const noexcept` solely for renderer-owned overlay interop.

- [ ] **Step 1: Add failing API/smoke tests**

```cpp
TEST_CASE("renderWorld and present reject use before initialization") {
    hh::renderer::D3D11Renderer renderer;
    hh::renderer::ComposedScene scene;
    hh::renderer::OrthoCamera camera;
    EXPECT_FALSE(static_cast<bool>(renderer.renderWorld(scene, camera)));
    EXPECT_FALSE(static_cast<bool>(renderer.present()));
    EXPECT_TRUE(renderer.device() == nullptr);
    EXPECT_TRUE(renderer.swapChain() == nullptr);
}
```

- [ ] **Step 2: Run renderer tests RED**

```powershell
cmake -S renderer -B build/renderer -G "Visual Studio 17 2022" -A x64
cmake --build build/renderer --config Debug
ctest --test-dir build/renderer -C Debug --output-on-failure
```

Expected: compile failure due missing methods.

- [ ] **Step 3: Extract existing draw path into `renderWorld`, move existing `Present(1,0)` block to `present`, preserve `render` wrapper**

Do not duplicate draw code. `renderWorld` must preserve all current render-target clearing, batching and error handling. `present` validates initialization and retains device-removed/device-reset reporting.

- [ ] **Step 4: Run renderer tests GREEN and commit**

```bash
git add renderer
git commit -m "refactor: separate world rendering from present"
```

---

### Task 5: Direct2D/DirectWrite overlay renderer and font fallback

**Files:**
- Create: `frontend/src/d2d/D2DUiRenderer.h`
- Create: `frontend/src/d2d/D2DUiRenderer.cpp`
- Create: `frontend/src/d2d/FontManager.h`
- Create: `frontend/src/d2d/FontManager.cpp`
- Create: `frontend/src/d2d/UiPrimitiveRenderer.h`
- Create: `frontend/src/d2d/UiPrimitiveRenderer.cpp`
- Create: `frontend/tests/D2DUiSmokeTests.cpp`
- Modify: `frontend/CMakeLists.txt`

**Interfaces:**
- `D2DUiRenderer::initialize(ID3D11Device*, IDXGISwapChain*)` creates D2D factory/device/context, DirectWrite factory, and swap-chain target bitmap.
- `D2DUiRenderer::resize(IDXGISwapChain*)` releases/recreates target bitmap after renderer resize.
- `D2DUiRenderer::draw(const MainMenuModel&, const MainMenuView&, const UiFrameState&)` renders overlay but never calls Present.
- `D2DUiRenderer::discardDeviceResources()` supports device recreation.
- `FontManager` attempts Cormorant Garamond/Inter first, then Georgia/Times New Roman and Segoe UI/Arial.

- [ ] **Step 1: Write smoke tests for invalid initialization and font fallback selection**

```cpp
TEST_CASE("d2d renderer fails safely without d3d interop") {
    hh::frontend::D2DUiRenderer ui;
    const auto result = ui.initialize(nullptr, nullptr);
    EXPECT_FALSE(static_cast<bool>(result));
}

TEST_CASE("font manager always returns fallback family") {
    hh::frontend::FontManager fonts;
    EXPECT_FALSE(fonts.displayFallbackChain().empty());
    EXPECT_FALSE(fonts.interfaceFallbackChain().empty());
}
```

- [ ] **Step 2: Run RED**

Expected: missing renderer/font classes.

- [ ] **Step 3: Implement D2D interop and drawing primitives**

Create D2D device from the D3D11 device's `IDXGIDevice`, then target the current swap-chain buffer via `IDXGISurface` and `ID2D1DeviceContext::CreateBitmapFromDxgiSurface`. Use premultiplied alpha. Draw charcoal left/right exposure panels, focus rectangle/border, menu text, brand text, version text, property plaque, stars/stat values and quit modal.

- [ ] **Step 4: Implement resize/device-resource recreation and graceful font failure**

On `D2DERR_RECREATE_TARGET`, release only device-dependent target resources and return a recoverable result; model state remains untouched. Font lookup falls back through the specified families.

- [ ] **Step 5: Run GREEN and commit**

```bash
git add frontend
git commit -m "feat: add Direct2D living menu overlay"
```

---

### Task 6: Standalone Living Hotel menu demo, fallback scene, input, resize, and CI

**Files:**
- Create: `frontend/app/MainMenuDemo.cpp`
- Create: `frontend/src/ShowcaseHotelScene.h`
- Create: `frontend/src/ShowcaseHotelScene.cpp`
- Create: `.github/workflows/frontend-ci.yml`
- Modify: `frontend/CMakeLists.txt`
- Modify: `renderer/src/win32/Win32Window.h`
- Modify: `renderer/src/win32/Win32Window.cpp`

**Interfaces:**
- Build target `hotel_haven_menu_demo.exe`.
- `ShowcaseHotelScene::build()` returns a renderer `RenderScene` representing a curated medium urban hotel using current primitive renderer capabilities.
- Win32 input exposes non-authoritative mouse position/movement and key presses required for hover/click/keyboard menu navigation without changing the renderer-core model.

- [ ] **Step 1: Write failing input behavior tests where pure, and compile demo target RED**

The demo must exercise: no-save mode, optional deterministic valid-save fixture via command-line flag, keyboard Up/Down/W/S, Enter/Space, Escape, mouse hover/click, resize, reduced-motion fixture flag, and quit confirmation.

- [ ] **Step 2: Implement showcase hotel presentation and menu loop**

Startup order: create Win32 window → initialize D3D11 renderer → initialize menu model immediately → query demo provider → initialize D2D overlay → progressively enable showcase detail. No gameplay simulation object is created. Apply camera pose from `MenuSceneController` to `hh::renderer::OrthoCamera` each frame.

Frame order must be:

```cpp
renderer.renderWorld(composed, camera);
uiRenderer.draw(model, view, frameState);
renderer.present();
```

Settings/Credits are lightweight overlay states that do not unload/reinitialize the living scene. Quit uses the confirmation modal.

- [ ] **Step 3: Add resize lifecycle**

On resize: release D2D target bitmap → call `renderer.resize()` → recreate D2D target bitmap → recompute logical UI layout. Keep selected item/model state unchanged.

- [ ] **Step 4: Add Windows CI**

Workflow runs on `windows-latest` and executes:

```powershell
cmake -S renderer -B build/renderer -G "Visual Studio 17 2022" -A x64
cmake --build build/renderer --config Release
ctest --test-dir build/renderer -C Release --output-on-failure
cmake -S frontend -B build/frontend -G "Visual Studio 17 2022" -A x64
cmake --build build/frontend --config Release
ctest --test-dir build/frontend -C Release --output-on-failure
```

- [ ] **Step 5: Run full Windows test matrix locally/CI and commit**

```bash
git add frontend renderer .github/workflows/frontend-ci.yml
git commit -m "feat: integrate living hotel main menu demo"
```

---

### Task 7: Acceptance hardening and verification

**Files:**
- Create: `frontend/tests/MainMenuStressTests.cpp`
- Create: `frontend/README.md`
- Modify: `frontend/CMakeLists.txt`

**Interfaces:**
- No new production API unless a failing acceptance test requires a narrowly scoped fix.

- [ ] **Step 1: Add deterministic acceptance tests**

Cover no-save initial focus, valid-save Continue, partial metadata, quit cancel, 10,000 navigation events, rapid modal open/close, repeated layout recomputation for 1280×720 / 1920×1080 / 2560×1080 / 3840×1080 / 1600×1200, and reduced-motion camera invariants.

- [ ] **Step 2: Build Release with strict warnings and run both test suites**

```powershell
cmake -S renderer -B build/renderer -G "Visual Studio 17 2022" -A x64
cmake --build build/renderer --config Release
ctest --test-dir build/renderer -C Release --output-on-failure
cmake -S frontend -B build/frontend -G "Visual Studio 17 2022" -A x64
cmake --build build/frontend --config Release
ctest --test-dir build/frontend -C Release --output-on-failure
```

Expected: zero failed tests and zero compile warnings promoted by existing strict-warning policy.

- [ ] **Step 3: Document demo controls, architecture boundary, failure fallbacks, and remaining validation requiring an interactive Windows/GPU session**

The README must explicitly distinguish automated CI coverage from the 60-minute interactive leak soak and pixel-reference capture, which require a live Windows graphics session rather than being fabricated in headless tooling.

- [ ] **Step 4: Final commit**

```bash
git add frontend
git commit -m "test: harden living hotel main menu acceptance"
```

## Plan self-review

- Spec coverage: model/navigation, provider boundary, property card formatting, responsive layout, reduced motion, camera presets, ambient presentation scheduling, D3D11→D2D frame order, device/resize lifecycle, font fallback, no-save/corrupt-safe behavior, menu command boundary, input, quit modal, performance-friendly nonblocking architecture, stress navigation, CI, and failure fallbacks are assigned to tasks.
- Explicitly deferred by spec/out-of-scope: New Hotel internals, save browser internals, scenario internals, sandbox internals, full Settings, gameplay simulation, guest AI, construction/economics, authoritative save parsing, multiplayer, mods, online services.
- Interactive-only acceptance: 60-minute GPU/resource leak soak and golden screenshot capture cannot be truthfully completed in a headless non-Windows connector session; the implementation must provide deterministic hooks and documentation so they can be run on a Windows workstation/CI agent with graphics capture.
- Placeholder scan: no implementation task depends on TBD/TODO rules.
- Type consistency: `MainMenuModel`/`Controller`, provider DTOs, `MainMenuView`, `MenuTransitionDirector`, `MenuSceneController`, renderer `renderWorld/present`, and `D2DUiRenderer` have single consistent names across tasks.
