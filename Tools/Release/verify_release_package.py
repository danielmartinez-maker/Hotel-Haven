#!/usr/bin/env python3
"""Validate the Hotel Haven Windows release ZIP shipping contract."""

from __future__ import annotations

import argparse
import sys
import zipfile
from collections import Counter
from pathlib import Path, PurePosixPath


REQUIRED_RUNTIME_FILES = (
    "hotel_haven.exe",
    "hotel_haven_headless.exe",
    "shaders/InstancedBox.hlsl",
    "shaders/Mesh.hlsl",
    "data/balance.json",
    "README.md",
    "IMPLEMENTATION_STATUS.md",
)
EXPECTED_ASSETS = tuple(f"data/assets/HH_A{index:03d}.hasset" for index in range(1, 501))
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


def validate_archive(archive_path: Path | str) -> list[str]:
    archive_path = Path(archive_path)
    errors: list[str] = []
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

    expected_assets = {_under_root(root, relative) for relative in EXPECTED_ASSETS}
    asset_prefix = _under_root(root, "data/assets/")
    packaged_assets = {
        name
        for name in names
        if name.startswith(asset_prefix) and name.endswith(".hasset")
    }
    if packaged_assets != expected_assets:
        missing_count = len(expected_assets - packaged_assets)
        extra_count = len(packaged_assets - expected_assets)
        errors.append(
            "expected exact 500-asset runtime set; "
            f"found {len(packaged_assets)} (missing {missing_count}, unexpected {extra_count})"
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
    parser.add_argument("archive", type=Path, help="CPack ZIP to validate")
    args = parser.parse_args(argv)

    errors = validate_archive(args.archive)
    if errors:
        print("Hotel Haven release package integrity: FAILED", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("Hotel Haven release package integrity: PASS")
    print("Verified required runtime files and exact 500 cooked assets.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
