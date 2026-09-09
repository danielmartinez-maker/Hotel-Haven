# Hotel Haven

Native C++20 hotel construction and management game targeting Windows 11 x64, with a portable deterministic simulation and an orthographic Direct3D 11 client.

This branch integrates the previously separate renderer and asset pipeline with gameplay. See [implementation status](docs/IMPLEMENTATION_STATUS.md) for exact coverage and remaining HMG requirements.

## Build and play on Windows

Install Visual Studio 2022 with Desktop development with C++, Windows SDK, and CMake 3.25 or newer. From a developer PowerShell at this repository:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
.\build\Release\hotel_haven.exe
```

The game starts paused at 14:00 in a furnished starter property, one simulated hour before the arrival peak. Read the Guide panel, inspect rooms, then select 1×. Use Rooms to set rates and request service; Staff to hire and schedule workers; Guests to inspect each traveler’s archetype, traits, budget and personal queue tolerance; Supplies to reorder consumables; Finance to inspect operating results and reviews. Check-in, checkout, room turnover and repairs all require an eligible on-shift employee to travel to and finish the work.

The Integrated Game workflow builds both Linux and Windows, runs the tests, smoke-tests the Windows client, and packages a `Hotel-Haven-Windows` ZIP artifact. Extract the complete ZIP and run `hotel_haven.exe`; keep its `data` and `shaders` directories beside it.

## Controls

| Input | Action |
|---|---|
| Left click | Use selected construction tool or inspect a room |
| WASD | Pan |
| Q / E | Rotate by 90 degrees |
| Mouse wheel | Zoom |
| Page Up / Page Down | Change active floor |
| C | Toggle translucent neighboring floors |
| Space | Pause / resume |
| F5 / F9 | Save / load campaign |
| Escape | Return to inspection tool |
| Bottom toolbar | Pause, 1×, 3×, 8×, 20×; floors; walls; diagnostic overlays |

Standard room placement uses a 6×6 footprint. Its door is the third tile of the bottom edge. Connect that door to the single main entrance with passable corridor or lobby tiles. Stairs connect matching coordinates on adjacent floors. Construction is charged only after full validation and cannot exceed available cash. Commands that violate construction, service or occupancy rules report their exact reason in the status line.

Room-status colors: green ready, blue occupied, purple reserved, amber cleaning, orange dirty, red unavailable. Cleanliness and condition overlays run from red (0) to green (100); the Rooms inspector gives exact values.

Save location: `%LOCALAPPDATA%\HotelHaven\campaign.hhsave`. Saving writes a temporary file before replacing the previous save. Loading pauses the simulation. The current v10 writer uses integer currency, persists guest profiles, check-in diagnostics and ten-point reviews, validates references on load, retains full reservation/review history plus a bounded recent-task history, and migrates saves from formats v2 through v9.

## Portable simulation and automated campaigns

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./build/hotel_haven_headless --days 14 --restock --data game/data/balance.json
./build/hotel_haven_headless --days 3 --save campaign.hhsave
./build/hotel_haven_headless --load campaign.hhsave --days 7
```

The campaign runner prints CSV with cash, revenue, payroll, occupancy, reputation, completed stays, linen and blocked tasks. It does not render a window on Linux.

## Source layout

- `game/include/hh/game/Simulation.h`: command and read-only snapshot interface.
- `game/src`: authoritative simulation and serialization.
- `game/data`: external balance definitions.
- `game/app`: native client, procedural world presentation, management panels, campaign runner.
- `game/tests`: behavioral and determinism checks.
- `renderer`: existing orthographic camera, floor visibility, cutaway and instanced D3D11 renderer.
- `Tools/ContentPipeline`: existing HMG-070 asset metadata, cooking and validation tooling.
- `docs/specifications`: original HMG design documents.

The renderer consumes snapshots and cannot change gameplay state. Construction and management controls issue commands through the simulation API.
