from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"expected one match in {path}, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def patch_batch01() -> None:
    path = ROOT / "Tools" / "ArtGeneration" / "batch01_generate.py"
    old = """    seen: set[str] = set()\n    for row in entries:\n        if not isinstance(row, list) or len(row) != 7:\n            raise ValueError(f'invalid asset row: {row!r}')\n        asset_id = row[0]\n        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:\n            raise ValueError(f'invalid asset_id: {asset_id!r}')\n        if asset_id in seen:\n            raise ValueError(f'duplicate asset_id: {asset_id}')\n        seen.add(asset_id)\n        profile = row[4]\n        if profile not in PROFILE_ASSET_TYPES:\n            raise ValueError(f'unsupported profile for batch 01: {profile!r}')\n    return entries\n"""
    new = """    if len(entries) != 50:\n        raise ValueError(f'Batch 01 must contain 50 assets; got {len(entries)}')\n    seen: set[str] = set()\n    for row in entries:\n        if not isinstance(row, list) or len(row) != 7:\n            raise ValueError(f'invalid asset row: {row!r}')\n        asset_id = row[0]\n        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:\n            raise ValueError(f'invalid asset_id: {asset_id!r}')\n        if asset_id in seen:\n            raise ValueError(f'duplicate asset_id: {asset_id}')\n        seen.add(asset_id)\n        material_family = row[3]\n        if material_family not in PALETTE:\n            raise ValueError(f'unknown material family: {material_family!r}')\n        profile = row[4]\n        if profile not in PROFILE_ASSET_TYPES:\n            raise ValueError(f'unsupported profile for batch 01: {profile!r}')\n        animation_set = row[5]\n        if profile == 'P_ARCH_ANIMATED':\n            if not isinstance(animation_set, str) or not animation_set:\n                raise ValueError(f'animated profile requires animation_set: {asset_id}')\n        elif animation_set is not None:\n            raise ValueError(f'static profile must not define animation_set: {asset_id}')\n    return entries\n"""
    replace_once(path, old, new)


def patch_batch02() -> None:
    path = ROOT / "Tools" / "ArtGeneration" / "batch02_generate.py"
    old = """    seen: set[str] = set()\n    for row in rows:\n        if not isinstance(row, list) or len(row) != 7:\n            raise ValueError(f'invalid asset row: {row!r}')\n        asset_id = row[0]\n        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:\n            raise ValueError(f'invalid asset_id: {asset_id!r}')\n        if asset_id in seen:\n            raise ValueError(f'duplicate asset_id: {asset_id}')\n        seen.add(asset_id)\n        profile = row[4]\n        if profile not in PROFILE_ASSET_TYPES:\n            raise ValueError(f'unsupported profile for batch 02: {profile!r}')\n    return rows\n"""
    new = """    seen: set[str] = set()\n    for row in rows:\n        if not isinstance(row, list) or len(row) != 7:\n            raise ValueError(f'invalid asset row: {row!r}')\n        asset_id = row[0]\n        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:\n            raise ValueError(f'invalid asset_id: {asset_id!r}')\n        if asset_id in seen:\n            raise ValueError(f'duplicate asset_id: {asset_id}')\n        seen.add(asset_id)\n        material_family = row[3]\n        if material_family not in PALETTE:\n            raise ValueError(f'unknown material family: {material_family!r}')\n        profile = row[4]\n        if profile not in PROFILE_ASSET_TYPES:\n            raise ValueError(f'unsupported profile for batch 02: {profile!r}')\n        animation_set = row[5]\n        if profile == 'P_ARCH_ANIMATED':\n            if not isinstance(animation_set, str) or not animation_set:\n                raise ValueError(f'animated profile requires animation_set: {asset_id}')\n        elif animation_set is not None:\n            raise ValueError(f'static profile must not define animation_set: {asset_id}')\n    return rows\n"""
    replace_once(path, old, new)


def main() -> None:
    patch_batch01()
    patch_batch02()
    print("Applied generator manifest validation hardening")


if __name__ == "__main__":
    main()
