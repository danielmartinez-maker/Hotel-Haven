# Soundtrack verification record

Verification performed for the initial soundtrack integration:

- C++20 catalog compilation with `-Wall -Wextra -Wpedantic`;
- catalog ordering, missing-file fallback, volume, and wraparound tests;
- Win32 soundtrack player compilation/link verification against WinMM on Windows CI;
- JSON manifest parse validation;
- exact size and SHA-256 verification for all four supplied MP3 files;
- ZIP integrity validation for the complete implementation overlay and playable Windows package;
- binary-bearing repository import verified by the repository asset verifier before commit;
- binary-bearing branch commit: `7ed73abae1c30821fb7bf14dc6772a6e04f4a14b`.

The four soundtrack MP3 files are now stored under `game/data/audio/` on `feature/hotel-soundtrack-v1`. A fresh connector-authored commit intentionally follows the binary import so normal GitHub Actions verification runs against the binary-bearing branch head.

A real Windows CI build remains the authoritative check for the production WinMM link and the complete Hotel Haven executable.
