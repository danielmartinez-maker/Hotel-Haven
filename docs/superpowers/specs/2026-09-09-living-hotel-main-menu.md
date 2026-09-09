# Hotel Haven Living Hotel Main Menu — Design Authority

**Document ID:** HH-UI-MAIN-001  
**Version:** v0.1  
**Status:** Approved / implementation-ready  
**Target:** Windows 11 x64  
**Language:** C++20  
**Rendering:** Direct3D 11 world + Direct2D 1.1 / DirectWrite overlay

This repository document records the approved implementation authority supplied by the user on 2026-09-09. The source design is `HH_UI_MAIN_001_LIVING_HOTEL_MAIN_MENU_V0_1(1).md`. Where temporary demo code conflicts with this document, this design governs the main-menu implementation.

## Product contract

Hotel Haven launches into a continuously rendered 2.5D hotel rather than a static title screen. The living backdrop is presentation-only: opening or idling on the main menu must not advance authoritative simulation state, guest needs, finances, reservations, staffing, time, inventory, or any saved gameplay variable.

Visual hierarchy: **Hotel first → Hotel Haven identity → Continue → secondary navigation → property details.**

## Main composition

At reference resolution 1920×1080, render:

- full-screen living hotel scene;
- left navigation beginning near x=72 and y=300, usable width about 360 px;
- brand at top-left near 72/72;
- latest-property summary card at right, width about 320 px, 64 px right margin, vertically centered around 48–55% of screen height;
- independent bottom-left build/version text.

Support 16:9, 16:10, 21:9, 32:9 and 4:3 down to 1280×720. On ultrawide, keep a centered 1920-wide UI safe zone while the world expands horizontally. Do not horizontally stretch UI elements.

## Exact navigation order

1. CONTINUE
2. NEW HOTEL
3. LOAD HOTEL
4. SCENARIOS
5. SANDBOX
6. SETTINGS
7. CREDITS
8. QUIT

There is a visual separator between SANDBOX and SETTINGS. If any valid save exists, CONTINUE gets initial focus. Otherwise CONTINUE remains visible but disabled and navigation skips it; NEW HOTEL receives initial focus. Keyboard/controller navigation wraps and skips disabled entries. Mouse hover transfers focus; keyboard/controller resumes from the current highlighted item.

## Commands

The frontend emits commands rather than constructing gameplay systems:

```cpp
enum class MainMenuCommand {
    None,
    ContinueLatest,
    StartNewHotel,
    OpenLoadHotel,
    OpenScenarios,
    OpenSandbox,
    OpenSettings,
    OpenCredits,
    ExitApplication
};
```

QUIT opens `Exit Hotel Haven?` with `[ EXIT ] [ CANCEL ]`; Escape/controller B cancels.

## Menu states and motion

Items support idle, hover, keyboard focus, controller focus, pressed and disabled. Focus/hover uses translucent brass, a thin border and about 8–12 px text shift over 150–220 ms. Press may compress toward 98% over 60–90 ms. Disabled is 30% opacity and never activates.

Reduced Motion disables idle camera drift, zoom breathing, large hover camera offsets and large item movement. Selection still uses opacity, border, color and crossfades.

## Theme

- overlay charcoal `#151412`, alpha 72–82%;
- primary text warm ivory `#F2ECE1`;
- secondary text `#B8B0A4`;
- accent brass `#B9975B`;
- selected background `rgba(185,151,91,0.34)`;
- disabled `rgba(242,236,225,0.30)`.

Display font: Cormorant Garamond, fallback Georgia then Times New Roman. Interface font: Inter, fallback Segoe UI then Arial.

## Snapshot/provider boundary

The menu must not parse authoritative save files. It consumes a read-only adapter owned by the future save/application layer:

```cpp
class IMenuHotelProvider {
public:
    virtual ~IMenuHotelProvider() = default;
    virtual std::optional<MenuHotelSnapshot> latestHotel() = 0;
};
```

Presentation DTOs:

```cpp
struct MenuPropertySummary {
    std::string hotelName;
    std::string city;
    std::string country;
    int starRating;
    std::uint32_t currentDay;
    std::optional<float> occupancy;
    std::optional<float> guestSatisfaction;
    std::optional<std::int64_t> cashMinorUnits;
    std::string currencyCode;
    std::optional<std::uint32_t> roomCount;
    std::uint64_t lastPlayedUtc;
};

struct MenuHotelSnapshot {
    MenuPropertySummary summary;
    MenuScenePresentation scene;
    MenuCameraAnchor camera;
};
```

Occupancy and satisfaction are 0.0–1.0. Stars clamp visually to 0–5. Missing property values render `—`. The menu formats values but performs no economic calculations.

Preferred background is the latest valid read-only presentation snapshot. Fallback is the canonical `menu.scene.showcase_hotel`. If that also fails, retain a neutral dark background and full menu operation. Valid metadata may still be shown even when the presentation snapshot is unavailable. Corrupt saves must never enable Continue, but Load Hotel remains reachable.

## Camera and living presentation

Reuse `hh::renderer::OrthoCamera` and existing 2.5D rendering policy. Base pitch is 55°. Idle drift is ±1–2° yaw, 1–3% zoom breathing, and small target interpolation. No continuous rotation.

Selection presets are relative to the base anchor:

- Continue: subtle push toward hotel center;
- New Hotel: slight pullback;
- Load Hotel: small horizontal orbit;
- Scenarios: opposite horizontal orbit;
- Sandbox: slight elevated pullback;
- Settings: camera motion almost stops;
- Credits: slow widen;
- Quit: no camera movement.

Transition target is 400–650 ms, interruptible/interpolated rather than queued.

Ambient presentation may show 6–16 visual-only actors using authored spline/waypoint sequences. They must not instantiate gameplay needs, psychology, reservations, pathfinding simulation, staff tasks, payroll, inventory or logistics. Major events launch roughly every 5–15 seconds. Production may randomize; tests require a deterministic seed.

## Rendering architecture

D3D11 remains authoritative for the world. A dedicated Direct2D/DirectWrite overlay renders after the world frame:

```text
Begin frame
Render Hotel Haven world through D3D11
Render menu presentation actors
Render darkening/gradient overlay
Render Direct2D UI geometry
Render DirectWrite text
Present swap chain
```

The frontend is a new top-level module with platform-agnostic model/controller/state and Windows-specific D2D/DirectWrite plus demo glue. Direct2D device-dependent resources must follow swap-chain/device lifecycle and be recreatable without resetting menu model state.

## Input/accessibility/localization

Required mouse: hover, left-click, wheel where relevant. Keyboard: Up/Down, optional W/S, Enter, Space, Escape. Controller architecture: D-pad/left stick, A activate, B back.

Required accessibility: UI scaling presets 90/100/110/125/150%, reduced motion, keyboard-only operation, visible focus, readable disabled state, subtitle-ready audio event architecture. No information depends only on color. Layout must tolerate about +40% text expansion.

## Failure handling

Optional presentation failures never block New Hotel, Load Hotel, Settings or Quit. Font failure falls back to system fonts. Audio failure continues silently. Save-summary failure disables Continue but preserves Load Hotel. Individual asset failure substitutes/omits the presentation object. Log snapshot load result, showcase fallback, font fallback, asset failure, save metadata failure, invalid command, Direct2D device recreation and DirectWrite initialization failure; do not log every frame/ambient event at normal level.

## Performance and startup targets

- UI interactive ≤2.0 s after graphics initialization;
- fade-in 400–700 ms;
- 60 FPS minimum target / 16.67 ms frame budget;
- UI rendering <1.5 ms GPU;
- menu logic <0.25 ms CPU average;
- input-to-highlight <50 ms;
- no blocking save scan on render thread;
- menu-specific incremental memory ≤384 MB;
- optional unfocused frame rate may drop to 15 FPS.

Streaming priority: fonts/primitives → menu model → save summary → coarse geometry → lighting → furniture → ambient actors → decorative details.

## Testing / acceptance authority

Pure tests cover item ordering, wrapping, disabled Continue, selection persistence, command generation, property formatting, partial metadata and reduced-motion behavior. Integration covers D3D11+D2D coexistence, UI-after-world, resize, high DPI, fullscreen/windowed, device recreation and font fallback. Save integration covers no saves, one/many valid saves, corrupt latest, missing snapshot, missing optional stats and incompatible snapshot version.

Required stress targets: 10,000 navigation events without crash/queue growth/focus corruption/animation accumulation; resize/fullscreen stress; 60-minute idle leak observation; rapid Settings/Credits/Quit open-close stress. Deterministic visual regression targets include no-save, valid-save, Continue, New Hotel, property card, quit modal, reduced motion, 21:9 and 1280×720.

The v0.1 menu is accepted when it launches directly into the Living Hotel menu, keeps the hotel world visible, respects save/Continue rules, exposes all eight actions, supports mouse+keyboard and controller abstraction, animates focus/camera correctly, honors Reduced Motion, overlays Settings without reloading the scene, confirms Quit, survives resize/device recreation, falls back safely, remains usable without living content, meets the target performance envelope, and passes automated/visual/stress validation.

## Out of scope

Do not implement New Hotel internals, save browser internals, scenario internals, sandbox internals, full Settings contents, gameplay simulation, guest AI, hotel construction, authoritative save parsing, hotel economics, multiplayer, mod browser or online services. The menu only emits/open surfaces for those systems.
