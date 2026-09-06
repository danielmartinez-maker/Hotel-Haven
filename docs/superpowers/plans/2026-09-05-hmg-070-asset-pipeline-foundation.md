# HMG-070 Asset Pipeline Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a deterministic, validated, dependency-aware native C++20 asset production foundation and `asset` CLI implementing HMG-070 for Hotel Haven.

**Architecture:** `Tools/ContentPipeline` is an independent Windows CMake project. Focused core modules parse/validate sidecars, catalog assets, analyze dependencies, compute SHA-256 content fingerprints, serialize deterministic `.hasset` envelopes, cook incrementally, and expose the HMG-070 CLI. Runtime and gameplay authority remain outside this subsystem.

**Tech Stack:** C++20, CMake 3.25+, CTest, Win32 BCrypt SHA-256, `std::filesystem`, GitHub Actions Windows runner.

**Spec:** `docs/superpowers/specs/2026-09-05-hmg-070-asset-pipeline-foundation-design.md`

## Global Constraints

- Target Windows 11 x64.
- 1 engine unit = 1 meter; 1 gameplay tile = 1 m × 1 m.
- Blender is the primary 3D source DCC; `.glb` is primary 3D interchange.
- Shipping/runtime code never loads files from `Art/Source` or `Art/Reference`.
- Identical source + sidecar + transitive dependencies + importer version + cooker version + compression settings + platform target must produce identical fingerprints and deterministic `.hasset` bytes.
- Timestamp-only invalidation is forbidden.
- Gameplay/economy authority must not be introduced into art metadata.
- Ordinary dependency cycles fail validation.
- `RELEASE_READY` permits zero BLOCKER, zero CRITICAL, zero known MAJOR diagnostics.
- Strict warnings are errors: MSVC `/W4 /WX /permissive- /EHsc`.

---

## File Structure

Create:

```text
Art/Source/.gitkeep
Art/Reference/.gitkeep
Art/Exports/.gitkeep
Art/Generated/.gitkeep
Art/Validation/.gitkeep
GameData/AssetDefinitions/.gitkeep
Tools/ContentPipeline/CMakeLists.txt
Tools/ContentPipeline/README.md
Tools/ContentPipeline/cmake/CompilerWarnings.cmake
Tools/ContentPipeline/include/hh/assets/Types.h
Tools/ContentPipeline/include/hh/assets/Json.h
Tools/ContentPipeline/include/hh/assets/Metadata.h
Tools/ContentPipeline/include/hh/assets/Catalog.h
Tools/ContentPipeline/include/hh/assets/DependencyGraph.h
Tools/ContentPipeline/include/hh/assets/Hash.h
Tools/ContentPipeline/include/hh/assets/Fingerprint.h
Tools/ContentPipeline/include/hh/assets/Hasset.h
Tools/ContentPipeline/include/hh/assets/Cooker.h
Tools/ContentPipeline/include/hh/assets/Exporter.h
Tools/ContentPipeline/include/hh/assets/Cli.h
Tools/ContentPipeline/src/Json.cpp
Tools/ContentPipeline/src/Metadata.cpp
Tools/ContentPipeline/src/Catalog.cpp
Tools/ContentPipeline/src/DependencyGraph.cpp
Tools/ContentPipeline/src/Hash.cpp
Tools/ContentPipeline/src/Fingerprint.cpp
Tools/ContentPipeline/src/Hasset.cpp
Tools/ContentPipeline/src/Cooker.cpp
Tools/ContentPipeline/src/Exporter.cpp
Tools/ContentPipeline/src/Cli.cpp
Tools/ContentPipeline/app/AssetMain.cpp
Tools/ContentPipeline/tests/Test.h
Tools/ContentPipeline/tests/TestMain.cpp
Tools/ContentPipeline/tests/MetadataTests.cpp
Tools/ContentPipeline/tests/DependencyGraphTests.cpp
Tools/ContentPipeline/tests/FingerprintTests.cpp
Tools/ContentPipeline/tests/HassetTests.cpp
Tools/ContentPipeline/tests/CookerTests.cpp
Tools/ContentPipeline/tests/CliTests.cpp
.github/workflows/content-pipeline-ci.yml
```

Modify/create root `.gitignore` to ignore `/Build/CookedAssets/` and `/Tools/ContentPipeline/build/`.

---

### Task 1: Project scaffold, diagnostics, asset types, lifecycle

**Files:**
- Create: `Tools/ContentPipeline/CMakeLists.txt`
- Create: `Tools/ContentPipeline/cmake/CompilerWarnings.cmake`
- Create: `Tools/ContentPipeline/include/hh/assets/Types.h`
- Create: `Tools/ContentPipeline/tests/Test.h`
- Create: `Tools/ContentPipeline/tests/TestMain.cpp`
- Create: `Tools/ContentPipeline/tests/MetadataTests.cpp`

**Interfaces:**
- Produces `enum class AssetType`, `Severity`, `LifecycleState`, `CutawayPolicy`.
- Produces `struct Diagnostic { Severity severity; std::string code; std::string message; };`.
- Produces `bool is_release_blocking(Severity)` and `bool can_transition(LifecycleState from, LifecycleState to, bool review_rejected = false)`.

- [ ] **Step 1: Write failing lifecycle/type tests**

Add assertions that all 13 runtime type names round-trip, sequential lifecycle transitions succeed, `REVIEW -> TECH_ART` succeeds only with `review_rejected=true`, `REVIEW -> RELEASE_READY` fails, and BLOCKER/CRITICAL/MAJOR are release-blocking while MINOR is not.

- [ ] **Step 2: Configure and verify failure**

Run:

```powershell
cmake -S Tools/ContentPipeline -B Tools/ContentPipeline/build -A x64
cmake --build Tools/ContentPipeline/build --config Release
```

Expected: build fails because `Types.h` contracts are not implemented.

- [ ] **Step 3: Implement minimal type/lifecycle layer**

Implement exact string mappings and lifecycle transition rules in `Types.h`; keep functions small and constexpr where practical.

- [ ] **Step 4: Run tests**

```powershell
ctest --test-dir Tools/ContentPipeline/build -C Release --output-on-failure
```

Expected: PASS.

- [ ] **Step 5: Commit**

```text
feat: add content pipeline type and lifecycle foundation
```

---

### Task 2: JSON parser and HMG-070 sidecar validation

**Files:**
- Create: `include/hh/assets/Json.h`
- Create: `src/Json.cpp`
- Create: `include/hh/assets/Metadata.h`
- Create: `src/Metadata.cpp`
- Extend: `tests/MetadataTests.cpp`

**Interfaces:**
- `JsonValue parse_json(std::string_view text);`
- `AssetMetadata load_metadata(const std::filesystem::path& sidecar);`
- `std::vector<Diagnostic> validate_metadata(const AssetMetadata& metadata);`
- `std::string canonicalize_metadata(const AssetMetadata& metadata);`

`AssetMetadata` fields: `schema`, `asset_id`, `asset_type`, `source`, `units`, `lod_policy`, `collision_policy`, `material_slots`, `tags`, `dependencies`, plus optional `cutaway_policy`, `pivot_exception_reason`, `source_revision`, `metadata_revision`, `cooker_schema`, `lifecycle_state`, `content_owner`, `technical_reviewer`, `art_reviewer`, `dependent_feature_owner`, and `milestone`.

- [ ] **Step 1: Write failing parser/metadata tests**

Cover valid sidecar, malformed JSON, missing every required field, schema other than `1`, invalid asset type, 3D asset with non-`meters` units, legal/illegal cutaway policy, JSON string escaping, arrays, integers, booleans/null tolerance for unknown optional values, and deterministic canonical ordering.

- [ ] **Step 2: Run tests and confirm RED**

```powershell
cmake --build Tools/ContentPipeline/build --config Release
ctest --test-dir Tools/ContentPipeline/build -C Release --output-on-failure
```

Expected: metadata tests fail.

- [ ] **Step 3: Implement internal JSON parser**

Implement recursive-descent parsing for object, array, string with JSON escapes and `\uXXXX`, integer/double number, `true`, `false`, `null`. Reject trailing non-whitespace and malformed UTF-8 escape sequences deterministically.

- [ ] **Step 4: Implement metadata mapping/validation**

Reject unsupported schema, missing required fields, wrong field types, invalid asset type/cutaway policy, empty `asset_id`, and 3D types whose `units != "meters"`. Do not impose a new asset-ID grammar.

- [ ] **Step 5: Run tests and commit**

Expected: PASS.

```text
feat: validate HMG-070 asset sidecars
```

---

### Task 3: Catalog and dependency graph

**Files:**
- Create: `include/hh/assets/Catalog.h`
- Create: `src/Catalog.cpp`
- Create: `include/hh/assets/DependencyGraph.h`
- Create: `src/DependencyGraph.cpp`
- Create: `tests/DependencyGraphTests.cpp`

**Interfaces:**
- `AssetCatalog AssetCatalog::scan(const std::filesystem::path& exports_root);`
- `const AssetRecord& AssetCatalog::by_id(std::string_view id) const;`
- `const AssetRecord& AssetCatalog::resolve(std::string_view id_or_path) const;`
- `DependencyGraph DependencyGraph::build(const AssetCatalog& catalog);`
- `std::vector<std::string> dependencies_of(std::string_view id, bool transitive) const;`
- `std::vector<std::string> dependents_of(std::string_view id, bool transitive) const;`
- `std::vector<std::string> topological_order() const;`

- [ ] **Step 1: Write failing catalog/graph tests**

Create temp fixtures for two valid assets, duplicate IDs, missing dependency, `A -> B -> C` traversal, and `A -> B -> A` cycle. Assert traversal and topological outputs are lexically deterministic for equal-order nodes.

- [ ] **Step 2: Run tests and confirm RED**

- [ ] **Step 3: Implement catalog scanning**

Recursively consider only filenames ending `.asset.json`. Resolve sibling export path by removing `.asset.json`; store repository-relative source/export/sidecar paths where possible. Duplicate IDs throw a validation error carrying both paths.

- [ ] **Step 4: Implement graph validation/traversal**

Use deterministic adjacency ordering and Kahn/DFS cycle detection. Missing dependency and cycle diagnostics are BLOCKER.

- [ ] **Step 5: Run tests and commit**

```text
feat: add asset catalog and dependency graph
```

---

### Task 4: SHA-256 and deterministic cook fingerprint

**Files:**
- Create: `include/hh/assets/Hash.h`
- Create: `src/Hash.cpp`
- Create: `include/hh/assets/Fingerprint.h`
- Create: `src/Fingerprint.cpp`
- Create: `tests/FingerprintTests.cpp`

**Interfaces:**
- `std::array<std::byte, 32> sha256(std::span<const std::byte> bytes);`
- `std::string sha256_hex(std::span<const std::byte> bytes);`
- `std::string hash_file(const std::filesystem::path& path);`
- `struct FingerprintSettings { std::string importer_version; std::string cooker_version; std::string compression_settings; std::string platform_target; };`
- `std::string compute_fingerprint(const AssetRecord&, const AssetCatalog&, const DependencyGraph&, const FingerprintSettings&);`

- [ ] **Step 1: Write failing hashing/fingerprint tests**

Assert SHA-256 known vector for `abc` equals `ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad`; touching timestamps does not change the fingerprint; changing source bytes, sidecar bytes, a transitive dependency source, importer version, cooker version, compression settings, or platform target changes it.

- [ ] **Step 2: Run tests and confirm RED**

- [ ] **Step 3: Implement BCrypt SHA-256**

Use Windows CNG `BCryptOpenAlgorithmProvider(BCRYPT_SHA256_ALGORITHM)`, `BCryptCreateHash`, `BCryptHashData`, and `BCryptFinishHash`; link `bcrypt`.

- [ ] **Step 4: Implement canonical fingerprint stream**

Hash length-prefixed UTF-8 fields in fixed order. Sort transitive dependency IDs lexically before adding each dependency ID + source hash + sidecar hash. Never include timestamps or absolute paths.

- [ ] **Step 5: Run tests and commit**

```text
feat: add deterministic asset fingerprints
```

---

### Task 5: Deterministic `.hasset` binary container

**Files:**
- Create: `include/hh/assets/Hasset.h`
- Create: `src/Hasset.cpp`
- Create: `tests/HassetTests.cpp`

**Interfaces:**
- `struct HassetDocument { AssetType type; std::string asset_id; std::string fingerprint; std::vector<std::string> dependencies; std::string source_path; std::string sidecar_path; std::vector<std::byte> payload; };`
- `std::vector<std::byte> serialize_hasset(const HassetDocument&);`
- `HassetDocument parse_hasset(std::span<const std::byte>);`

Binary format version 1:

```text
magic[8] = "HHASSET\0"
u32 version = 1
u32 asset_type
u32 asset_id_len + bytes
u32 fingerprint_len + bytes
u32 dependency_count
  repeated: u32 dep_len + dep_bytes
u32 source_path_len + bytes
u32 sidecar_path_len + bytes
u64 payload_len + payload_bytes
```

All integer fields are little-endian. Dependencies serialize in lexical order. Paths are repository-relative with `/` separators.

- [ ] **Step 1: Write failing round-trip/determinism/corruption tests**

Assert repeated serialization is byte-identical; parsing round-trips; bad magic/version, impossible lengths, and truncation fail; absolute source/sidecar paths are rejected by serializer.

- [ ] **Step 2: Run tests and confirm RED**

- [ ] **Step 3: Implement serializer/parser with checked bounds**

Every read checks remaining bytes before advancing. Reject payload lengths larger than remaining input.

- [ ] **Step 4: Run tests and commit**

```text
feat: add deterministic hasset container
```

---

### Task 6: Cooker, cook state, incremental invalidation, atomic publication

**Files:**
- Create: `include/hh/assets/Cooker.h`
- Create: `src/Cooker.cpp`
- Create: `tests/CookerTests.cpp`

**Interfaces:**
- `struct CookOptions { std::filesystem::path repository_root; std::filesystem::path cooked_root; FingerprintSettings fingerprint; };`
- `struct CookResult { std::string asset_id; bool cooked; std::filesystem::path output; std::string fingerprint; };`
- `CookResult cook_one(const AssetCatalog&, const DependencyGraph&, std::string_view asset_id, const CookOptions&);`
- `std::vector<CookResult> cook_all(...);`
- `std::vector<CookResult> cook_changed(...);`

Cook-state file: `Build/CookedAssets/.cook-state.json`, containing schema `1` and mapping `asset_id -> fingerprint` emitted with lexical key ordering.

- [ ] **Step 1: Write failing cooker tests**

Cover first cook, unchanged no-op, changed source recook, dependency change recooking transitive dependent, removed/missing input failure, repeated byte-identical cook, deterministic topological output ordering, and preservation of previous `.hasset` when a forced serialization/input failure occurs.

- [ ] **Step 2: Run tests and confirm RED**

- [ ] **Step 3: Implement generic payload and cook-state logic**

Generic payload is exactly the export/interchange file bytes. Fingerprint and header carry traceability. Cook to `<cooked_root>/<asset_id>.hasset` with asset ID characters preserved except `/` and `\\` rejected because HMG-070 IDs are logical identifiers, not paths.

- [ ] **Step 4: Implement atomic publication**

Write `<output>.tmp`, close/flush, then replace final output only after successful serialization. Delete temp file on failure.

- [ ] **Step 5: Run tests and commit**

```text
feat: add incremental deterministic asset cooker
```

---

### Task 7: Export adapter and complete `asset` CLI

**Files:**
- Create: `include/hh/assets/Exporter.h`
- Create: `src/Exporter.cpp`
- Create: `include/hh/assets/Cli.h`
- Create: `src/Cli.cpp`
- Create: `app/AssetMain.cpp`
- Create: `tests/CliTests.cpp`

**Interfaces:**
- `struct ExportResult { bool success; int exit_code; std::string message; };`
- `ExportResult export_asset(const AssetRecord&, const std::filesystem::path& repository_root);`
- `int run_asset_cli(std::span<const std::string_view> args, std::ostream& out, std::ostream& err);`

- [ ] **Step 1: Write failing CLI tests**

Cover exact commands:

```text
validate <asset-id|path>
export <asset-id|path>
cook <asset-id|path>
cook --changed
cook --all
inspect <asset-id>
deps <asset-id>
audit --milestone <name>
```

Assert malformed invocations return non-zero; `inspect` and `deps` output lexical deterministic records; validation/cook failures return non-zero; milestone audit fails if matching assets have release-blocking diagnostics or lack HMG-070 ownership/review fields.

- [ ] **Step 2: Run tests and confirm RED**

- [ ] **Step 3: Implement export orchestration boundary**

Read `HOTEL_HAVEN_BLENDER`. If absent/nonexistent, return a deterministic configuration error. If present, validate `.blend` source existence and return a clear unsupported-type-policy error until a later HMG-071–079 exporter adapter supplies exact Blender arguments. Do not invent type-specific export switches.

- [ ] **Step 4: Implement CLI commands**

The CLI discovers repository root from current directory by walking upward until `Art` and `Tools/ContentPipeline` are found. `validate` runs metadata/catalog/graph validation. `audit --milestone` scans matching sidecars and requires `content_owner`, `technical_reviewer`, `art_reviewer`, `dependent_feature_owner`, `milestone`, lifecycle state `RELEASE_READY`, and no release-blocking diagnostics.

- [ ] **Step 5: Run tests and commit**

```text
feat: add HMG-070 asset command line interface
```

---

### Task 8: Repository layout, documentation, and Windows CI gate

**Files:**
- Create: canonical `.gitkeep` files under `Art/*` and `GameData/AssetDefinitions`
- Create/modify: `.gitignore`
- Create: `Tools/ContentPipeline/README.md`
- Create: `.github/workflows/content-pipeline-ci.yml`
- Modify: `Tools/ContentPipeline/CMakeLists.txt` if integration changes are needed

**Interfaces:**
- Produces documented local commands:

```powershell
cmake -S Tools/ContentPipeline -B Tools/ContentPipeline/build -A x64
cmake --build Tools/ContentPipeline/build --config Release
ctest --test-dir Tools/ContentPipeline/build -C Release --output-on-failure
Tools/ContentPipeline/build/Release/asset.exe validate Art/Exports
```

- [ ] **Step 1: Add canonical tracked layout and ignore generated output**

Root `.gitignore` entries:

```text
/Build/CookedAssets/
/Tools/ContentPipeline/build/
```

- [ ] **Step 2: Add README**

Document authority boundary, directory contract, command reference, environment variable `HOTEL_HAVEN_BLENDER`, deterministic fingerprint inputs, `.hasset` v1 envelope, validation severities, and explicitly deferred HMG-071–079 type-specific rules.

- [ ] **Step 3: Add GitHub Actions workflow**

Use `windows-latest`, `actions/checkout@v4`, CMake configure/build/CTest Release commands, then run `asset.exe validate Art/Exports` so CI and local validation share code.

- [ ] **Step 4: Run full verification**

```powershell
cmake -S Tools/ContentPipeline -B Tools/ContentPipeline/build -A x64
cmake --build Tools/ContentPipeline/build --config Release
ctest --test-dir Tools/ContentPipeline/build -C Release --output-on-failure
Tools/ContentPipeline/build/Release/asset.exe validate Art/Exports
```

Expected: all tests pass and empty/valid canonical exports tree validates successfully.

- [ ] **Step 5: Commit**

```text
ci: enforce HMG-070 content pipeline validation
```

---

## Final Verification

1. Re-run the full Release configure/build/CTest/CLI validation from Task 8.
2. Confirm `git diff main...feature/hmg-070-asset-pipeline-foundation` contains no generated `Build/CookedAssets` or `Tools/ContentPipeline/build` output.
3. Confirm no runtime/cooker path reads `Art/Source` as payload; source files are hashed for provenance only.
4. Confirm repeated cook fixture outputs are byte-identical.
5. Confirm all HMG-070 CLI verbs exist and malformed/error paths are covered by tests.
6. Confirm type-specific asset budget policy remains deferred rather than invented.
