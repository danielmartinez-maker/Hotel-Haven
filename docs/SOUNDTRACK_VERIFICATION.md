# Soundtrack verification record

Local verification performed for the initial soundtrack integration:

- C++20 catalog compilation with `-Wall -Wextra -Wpedantic`;
- catalog ordering, missing-file fallback, volume, and wraparound tests;
- Win32 soundtrack player translation-unit syntax check against the consumed WinMM/Win32 API surface;
- JSON manifest parse validation;
- exact size and SHA-256 verification for all four supplied MP3 files;
- ZIP integrity validation for the complete implementation overlay.

A real Windows CI build remains the authoritative check for the production WinMM link and the complete Hotel Haven executable.
