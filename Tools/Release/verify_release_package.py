#!/usr/bin/env python3
"""Validate the Hotel Haven Windows release ZIP shipping contract."""

from __future__ import annotations

import argparse
import sys
import zipfile
from collections import Counter
from pathlib import Path, PurePosixPath


REPO_ROOT = Path(__file__).resolve().parents[2]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from Tools.ContentPipeline.scripts.verify_runtime_asset_set import expected_runtime_ids


REQUIRED_RUNTIME_FILES = (
    "hotel_haven.exe",
    "hotel_haven_headless.exe",
    "shaders/InstancedBox.hlsl",
    "shaders/Mesh.hlsl",
    "data/balance.json",
    "README.md",
    "IMPLEMENTATION_STATUS.md",
)
DEVELOPMENT_SUFFIXES = (".pdb", ".obj", ".ilk", ".lib", ".exp")
DEVELOPMENT_NAMES = {"CMakeCache.txt"}


def _normalized_name(name: str) -> str | None:
    if not name or "\\" in name or name.startswith("/"):
        return None
    path = PurePosixPath(name)
    if any(part in ("", ".", "..") or ":" in part for part in path.parts):
        return None
    return path.as_posix().rstrip("/")


def _package_root(names: set[str]) -> str | None:
    clients = sorted(name for name in names if name == "hotel_haven.exe" or name.endswith("/hotel_haven.exe"))
    if len(clients) != 1:
        return None
    client = PurePosixPath(clients[0])
    return "" if len(client.parts) == 1 else PurePosixPath(*client.parts[:-1]).as_posix()


def _under_root(root: str, relative: str) -> str:
    return relative if not root else f"{root}/{relative}"


def validate_archive(
    archive_path: Path | str, exports_root: Path | str
) -> list[str]:
    archive_path = Path(archive_path)
    exports_root = Path(exports_root)
    errors: list[str] = []
    try:
        runtime_ids = expected_runtime_ids(exports_root)
    except RuntimeError as exc:
        return [f"unable to resolve expected runtime asset set: {exc}"]
    try:
        with zipfile.ZipFile(archive_path, "r") as archive:
            raw_names = [info.filename for info in archive.infolist() if not info.is_dir()]
    except (OSError, zipfile.BadZipFile) as exc:
        return [f"unable to read release package: {exc}"]

    unsafe = sorted(name for name in raw_names if _normalized_name(name) is None)
    if unsafe:
        errors.append("unsafe archive path(s): " + ", ".join(unsafe))

    safe_names = [normalized for name in raw_names if (normalized := _normalized_name(name)) is not None]
    duplicate_names = sorted(name for name, count in Counter(safe_names).items() if count > 1)
    if duplicate_names:
        errors.append("duplicate archive entry/entries: " + ", ".join(duplicate_names))

    names = set(safe_names)
    root = _package_root(names)
    if root is None:
        errors.append("expected exactly one hotel_haven.exe package root")
        return errors

    for relative in REQUIRED_RUNTIME_FILES:
        expected = _under_root(root, relative)
        if expected not in names:
            errors.append(f"missing required runtime file: {relative}")

    expected_assets = {
        _under_root(root, f"data/assets/{asset_id}.hasset")
        for asset_id in runtime_ids
    }
    asset_prefix = _under_root(root, "data/assets/")
    packaged_assets = {
        name
        for name in names
        if name.startswith(asset_prefix) and name.endswith(".hasset")
    }
    if packaged_assets != expected_assets:
        missing = sorted(expected_assets - packaged_assets)
        unexpected = sorted(packaged_assets - expected_assets)
        errors.append(
            "packaged runtime asset set diverges from source mesh metadata; "
            f"found {len(packaged_assets)}, expected {len(expected_assets)} "
            f"(missing {len(missing)}, unexpected {len(unexpected)})"
        )
        if missing:
            errors.append("missing runtime asset(s): " + ", ".join(missing[:25]))
        if unexpected:
            errors.append(
                "unexpected runtime asset(s): " + ", ".join(unexpected[:25])
            )

    root_prefix = f"{root}/" if root else ""
    for name in sorted(names):
        if root_prefix and not name.startswith(root_prefix):
            errors.append(f"file outside package root: {name}")
            continue
        relative = name[len(root_prefix) :] if root_prefix else name
        basename = PurePosixPath(relative).name
        if basename in DEVELOPMENT_NAMES or relative.lower().endswith(DEVELOPMENT_SUFFIXES):
            errors.append(f"development-only artifact present: {relative}")
        if "CMakeFiles" in PurePosixPath(relative).parts:
            errors.append(f"development-only artifact present: {relative}")

    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--exports-root",
        type=Path,
        required=True,
        help="Generated asset export root containing canonical .asset.json sidecars",
    )
    parser.add_argument("archive", type=Path, help="CPack ZIP to validate")
    args = parser.parse_args(argv)

    errors = validate_archive(args.archive, args.exports_root)
    if errors:
        print("Hotel Haven release package integrity: FAILED", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("Hotel Haven release package integrity: PASS")
    print("Verified required runtime files and exact source-metadata runtime asset set.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
