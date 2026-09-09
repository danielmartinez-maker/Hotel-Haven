# Hotel Haven Frontend — Living Hotel Main Menu v0.1

This module implements HH-UI-MAIN-001, the Living Hotel main menu for Hotel Haven.

## Architecture boundary

The frontend is presentation-only. `MainMenuModel` owns UI state, `MainMenuController` converts input into `MainMenuCommand` values, `MainMenuView` owns formatting/layout, `MenuTransitionDirector` and `MenuSceneController` own presentation motion, and `D2DUiRenderer` draws the Direct2D/DirectWrite overlay.

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
--reduced-motion   Disables idle/selection camera movement.
```

Controls:

```text
Up / W             Previous enabled menu item
Down / S           Next enabled menu item
Enter / Space      Activate selected item
Mouse              Hover and activate menu items
Escape             Cancel active modal
Y                   Confirm the quit fixture while the quit modal is open
```

The New Hotel, Load Hotel, Scenarios, Sandbox, Settings, Credits, and Continue actions intentionally emit frontend/application commands or demo fixtures. Their full workflows are outside HH-UI-MAIN-001 v0.1.

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

`hh_frontend_tests` covers menu order, focus wrapping, disabled Continue behavior, command generation, property formatting, partial metadata, responsive layout, deterministic ambient scheduling, interruptible transitions, reduced-motion camera invariants, rapid quit-modal cycling, 10,000 navigation events, Direct2D safe-failure behavior, and font fallback contracts.

The Windows CI workflow builds the frontend and renderer in Release mode and runs both test suites with strict warnings enabled. The renderer suite also validates the D3D11 WARP/shader path and the render-world/present interop seam.

## Interactive validation still required

Two acceptance items require a live Windows graphics session and must not be represented as headless CI results:

1. the 60-minute menu idle soak while observing process memory, D3D/D2D resource counts, handle count, and frame time for persistent growth;
2. deterministic golden screenshot capture/comparison at the required reference states and resolutions, including 1920×1080, 1280×720, and 21:9.

The deterministic showcase scene, fixed presentation seed support, reduced-motion fixture, and resolution-independent layout are provided so those workstation checks can be performed reproducibly.
