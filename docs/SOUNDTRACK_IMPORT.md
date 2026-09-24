# Soundtrack asset import

The source branch contains the soundtrack runtime, catalog, metadata, tests, and packaging integration.
Place the following project-owner-supplied files in `game/data/audio/` before a release build:

| File | Bytes | SHA-256 |
|---|---:|---|
| `Morning_in_the_Atrium.mp3` | 4,323,801 | `af78af388491faf33449efad632dfc6d09f53406d3ccf05cbad91dace933e69d` |
| `Sunlight_on_Marble.mp3` | 4,330,697 | `cc0018a839e3fa9b0f0775b128a9774d8cb5d13802682bda4317b80347199460` |
| `The_Concierge_Desk.mp3` | 4,342,609 | `89b2308da2b26d58f7186516c592810e83660ec86944707f0885aa94057939b7` |
| `First_Light_on_Marble.mp3` | 4,377,091 | `06031b117eb1b1af6bd6c7f0cf72d955d07c176f38c06df97e32f577a3e51823` |

CMake registers `hh_soundtrack_assets` when all four files are present. That test verifies exact byte sizes, hashes, and manifest references. When the files are absent, configuration remains valid and the player exits silently without affecting the simulation.
