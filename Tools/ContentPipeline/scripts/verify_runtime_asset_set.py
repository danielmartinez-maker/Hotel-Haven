#!/usr/bin/env python3
"""Verify the renderer runtime package matches source mesh metadata exactly."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

RUNTIME_TYPES = {"StaticMeshAsset", "SkinnedMeshAsset"}


def expected_runtime_ids(exports_root: Path) -> set[str]:
    expected: set[str] = set()
    for sidecar in sorted(exports_root.rglob("*.asset.json")):
        try:
            metadata = json.loads(sidecar.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise RuntimeError(f"cannot read {sidecar}: {exc}") from exc
        if metadata.get("asset_type") not in RUNTIME_TYPES:
            continue
        asset_id = metadata.get("asset_id")
        if not isinstance(asset_id, str) or not asset_id:
            raise RuntimeError(f"{sidecar}: runtime mesh has invalid asset_id")
        if asset_id in expected:
            raise RuntimeError(f"duplicate runtime asset_id {asset_id}")
        expected.add(asset_id)
    if not expected:
        raise RuntimeError(f"no runtime mesh assets found under {exports_root}")
    return expected


def cooked_runtime_ids(cooked_root: Path) -> set[str]:
    ids = {path.stem for path in cooked_root.glob("*.hasset") if path.is_file()}
    if not ids:
        raise RuntimeError(f"no cooked .hasset files found under {cooked_root}")
    return ids


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("exports_root", type=Path)
    parser.add_argument("cooked_root", type=Path)
    args = parser.parse_args()

    expected = expected_runtime_ids(args.exports_root)
    actual = cooked_runtime_ids(args.cooked_root)
    missing = sorted(expected - actual)
    extra = sorted(actual - expected)
    if missing or extra:
        if missing:
            print(f"missing runtime assets ({len(missing)}): {', '.join(missing[:25])}")
        if extra:
            print(f"unexpected runtime assets ({len(extra)}): {', '.join(extra[:25])}")
        return 1
    print(f"Verified exact runtime mesh asset set: {len(actual)} cooked assets")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
