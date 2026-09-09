# Hotel Haven Frontend — Living Hotel Main Menu v0.1

This module implements HH-UI-MAIN-001, the Living Hotel main menu for Hotel Haven.

## Architecture boundary

The frontend is presentation-only. `MainMenuModel` owns UI state, `MainMenuController` converts input into menu/application actions, `MainMenuView` owns formatting/layout, `MenuTransitionDirector` and `MenuSceneController` own presentation motion, and `D2DUiRenderer` draws the Direct2D/DirectWrite overlay.

The menu does not parse authoritative save files and never advances Hotel Haven simulation state. Save/application code is expected to implement `IMenuHotelProvider` and supply a read-only `MenuHotelSnapshot`/`MenuPropertySummary`. The canonical `ShowcaseHotelScene` is a presentation fallback when no usable hotel snapshot exists.

The world/UI frame boundary is explicit:

```text
D3D11Renderer::renderWorld(...)
D2DUiRenderer::draw(...)
D3D11Renderer::present()
```

`D3D11Renderer::render(...)` remains available for renderer clients that do not need an overlay.

## Demo

Target: `hotel_haven_menu_demo.exe`

Default launch represents a first-run/no-save state. Optional fixtures:

```text
--valid-save       Enables Continue and displays the Beaumont property plaque.
--reduced-motion   Starts with Reduced Motion enabled.
```

Controls:

```text
Up / W             Previous enabled menu item
Down / S           Next enabled menu item
Enter / Space      Activate / confirm / toggle the Settings option
Escape             Cancel the active modal or overlay
Mouse              Hover, activate, toggle Settings, and use quit buttons
D-pad / Left stick Navigate enabled menu items
Gamepad A           Activate / confirm / toggle the Settings option
Gamepad B           Cancel the active modal or overlay
```

Settings and Credits are lightweight overlays over the still-running Living Hotel scene. Settings currently exposes the v0.1 Reduced Motion option; toggling it freezes ambient presentation actors and disables menu camera motion without unloading the scene. Credits also overlays without reinitializing the renderer or backdrop.

Continue, New Hotel, Load Hotel, Scenarios, and Sandbox intentionally emit frontend/application commands or demo surfaces. Their full workflow internals are outside HH-UI-MAIN-001 v0.1.

## Controller input boundary

`MenuGamepadMapper` is platform-agnostic and converts a normalized `MenuGamepadState` into edge-triggered menu actions. Held D-pad/stick/A/B input therefore does not accumulate repeated menu actions. `XInputMenuGamepad` is the Windows adapter used by the demo and polls controller 0 through the Windows XInput API.

## Failure behavior

The UI layer fails closed rather than taking down navigation. Expected production integration behavior is:

- missing/incompatible latest presentation snapshot → canonical showcase hotel;
- missing showcase assets → neutral dark backdrop with full UI;
- font lookup failure → Cormorant Garamond → Georgia → Times New Roman and Inter → Segoe UI → Arial fallback chains;
- optional property fields → em dash;
- invalid save summary → Continue disabled while Load Hotel remains reachable;
- Direct2D target loss → recreate device-dependent target resources while preserving model state;
- audio failure → silent menu;
- individual presentation-asset failure → omit/substitute that object.

## Automated validation

`hh_frontend_tests` covers menu order, focus wrapping, disabled Continue behavior, command generation, Settings/Credits overlay state, property formatting, partial metadata, responsive layout, deterministic ambient scheduling and showcase actor motion, controller edge mapping, interruptible transitions, reduced-motion camera invariants, rapid quit-modal cycling, 10,000 navigation events, Direct2D safe-failure behavior, and font fallback contracts.

The Windows CI workflow builds the frontend and renderer in Release mode and runs both test suites with strict warnings enabled. The renderer suite also validates the D3D11 WARP/shader path and the render-world/present interop seam.

## Interactive validation still required

Two acceptance items require a live Windows graphics session and must not be represented as headless CI results:

1. the 60-minute menu idle soak while observing process memory, D3D/D2D resource counts, handle count, and frame time for persistent growth;
2. deterministic golden screenshot capture/comparison at the required reference states and resolutions, including 1920×1080, 1280×720, and 21:9.

The deterministic showcase scene, fixed presentation seed support, reduced-motion fixture, and resolution-independent layout are provided so those workstation checks can be performed reproducibly.
