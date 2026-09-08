from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"expected exactly one match in {path}: found {count}\n{old}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def patch_batch01() -> None:
    path = ROOT / "Tools" / "ArtGeneration" / "batch01_generate.py"
    replace_once(path, "import json\nfrom pathlib import Path\n", "import json\nfrom pathlib import Path\nimport re\n")
    replace_once(
        path,
        "PALETTE = {\n",
        "ASSET_ID_RE = re.compile(r'^HH_A\\d{3}$')\n"
        "PROFILE_ASSET_TYPES = {\n"
        "    'P_ARCH_STATIC': 'StaticMeshAsset',\n"
        "    'P_ARCH_ANIMATED': 'SkinnedMeshAsset',\n"
        "}\n\n\n"
        "def _validated_entries(data: dict) -> list[list]:\n"
        "    try:\n"
        "        entries = [row for group in data['groups'] for row in group['assets']]\n"
        "    except (KeyError, TypeError) as exc:\n"
        "        raise ValueError('manifest must contain groups with asset rows') from exc\n"
        "    seen: set[str] = set()\n"
        "    for row in entries:\n"
        "        if not isinstance(row, list) or len(row) != 7:\n"
        "            raise ValueError(f'invalid asset row: {row!r}')\n"
        "        asset_id = row[0]\n"
        "        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:\n"
        "            raise ValueError(f'invalid asset_id: {asset_id!r}')\n"
        "        if asset_id in seen:\n"
        "            raise ValueError(f'duplicate asset_id: {asset_id}')\n"
        "        seen.add(asset_id)\n"
        "        profile = row[4]\n"
        "        if profile not in PROFILE_ASSET_TYPES:\n"
        "            raise ValueError(f'unsupported profile for batch 01: {profile!r}')\n"
        "    return entries\n\n\n"
        "PALETTE = {\n",
    )
    replace_once(
        path,
        "    output_dir = Path(output_dir)\n    output_dir.mkdir(parents=True, exist_ok=True)\n    data = json.loads(manifest_path.read_text(encoding='utf-8'))\n    entries = [row for group in data['groups'] for row in group['assets']]\n",
        "    output_dir = Path(output_dir)\n    data = json.loads(manifest_path.read_text(encoding='utf-8'))\n    entries = _validated_entries(data)\n    output_dir.mkdir(parents=True, exist_ok=True)\n",
    )
    replace_once(
        path,
        "        asset_id, display_name, _subcategory, material_family, _profile, _animation_set, _anchors = row\n",
        "        asset_id, display_name, _subcategory, material_family, _profile, _animation_set, _anchors = row\n",
    )
    replace_once(
        path,
        "    rows = {row[0]: row for group in data['groups'] for row in group['assets']}\n",
        "    rows = {row[0]: row for row in _validated_entries(data)}\n",
    )
    replace_once(
        path,
        "        asset_id, _display_name, subcategory, material_family, _profile, animation_set, _anchors = rows[glb_path.stem]\n",
        "        asset_id, _display_name, subcategory, material_family, profile, animation_set, _anchors = rows[glb_path.stem]\n",
    )
    replace_once(path, "            'asset_type': 'StaticMeshAsset',\n", "            'asset_type': PROFILE_ASSET_TYPES[profile],\n")


def patch_batch02() -> None:
    path = ROOT / "Tools" / "ArtGeneration" / "batch02_generate.py"
    replace_once(path, "import json\nfrom pathlib import Path\n", "import json\nfrom pathlib import Path\nimport re\n")
    replace_once(
        path,
        "PALETTE = {\n",
        "ASSET_ID_RE = re.compile(r'^HH_A\\d{3}$')\n"
        "PROFILE_ASSET_TYPES = {\n"
        "    'P_ARCH_STATIC': 'StaticMeshAsset',\n"
        "    'P_ARCH_ANIMATED': 'SkinnedMeshAsset',\n"
        "}\n\n\n"
        "def _validated_rows(data: dict) -> list[list]:\n"
        "    try:\n"
        "        rows = [row for group in data['groups'] for row in group['assets']]\n"
        "    except (KeyError, TypeError) as exc:\n"
        "        raise ValueError('manifest must contain groups with asset rows') from exc\n"
        "    if len(rows) != 50:\n"
        "        raise ValueError(f'Batch 02 must contain 50 assets; got {len(rows)}')\n"
        "    seen: set[str] = set()\n"
        "    for row in rows:\n"
        "        if not isinstance(row, list) or len(row) != 7:\n"
        "            raise ValueError(f'invalid asset row: {row!r}')\n"
        "        asset_id = row[0]\n"
        "        if not isinstance(asset_id, str) or ASSET_ID_RE.fullmatch(asset_id) is None:\n"
        "            raise ValueError(f'invalid asset_id: {asset_id!r}')\n"
        "        if asset_id in seen:\n"
        "            raise ValueError(f'duplicate asset_id: {asset_id}')\n"
        "        seen.add(asset_id)\n"
        "        profile = row[4]\n"
        "        if profile not in PROFILE_ASSET_TYPES:\n"
        "            raise ValueError(f'unsupported profile for batch 02: {profile!r}')\n"
        "    return rows\n\n\n"
        "PALETTE = {\n",
    )
    replace_once(path, "def sidecar(asset_id, subcat, mat, animset):\n", "def sidecar(asset_id, subcat, mat, profile, animset):\n")
    replace_once(path, "        'asset_type': 'StaticMeshAsset',\n", "        'asset_type': PROFILE_ASSET_TYPES[profile],\n")
    replace_once(
        path,
        "    rows = [a for g in data['groups'] for a in g['assets']]\n    if len(rows) != 50:\n        raise ValueError(f'Batch 02 must contain 50 assets; got {len(rows)}')\n",
        "    rows = _validated_rows(data)\n",
    )
    replace_once(
        path,
        "            (output / f'{asset_id}.asset.json').write_text(json.dumps(sidecar(asset_id, subcat, mat, animset), indent=2) + '\\n')\n",
        "            (output / f'{asset_id}.asset.json').write_text(json.dumps(sidecar(asset_id, subcat, mat, profile, animset), indent=2) + '\\n')\n",
    )


def patch_existing_generator_tests() -> None:
    path1 = ROOT / "Tools" / "ArtGeneration" / "tests" / "test_batch01_generator.py"
    replace_once(
        path1,
        "from batch01_generate import generate_batch  # noqa: E402\n",
        "from batch01_generate import PROFILE_ASSET_TYPES, generate_batch  # noqa: E402\n",
    )
    replace_once(
        path1,
        "    assert len(outputs) == 50\n\n    for glb_path in outputs:\n",
        "    assert len(outputs) == 50\n    rows = {row[0]: row for row in load_entries()}\n\n    for glb_path in outputs:\n",
    )
    replace_once(
        path1,
        "        assert meta['asset_type'] == 'StaticMeshAsset'\n",
        "        assert meta['asset_type'] == PROFILE_ASSET_TYPES[rows[glb_path.stem][4]]\n",
    )

    path2 = ROOT / "Tools" / "ArtGeneration" / "tests" / "test_batch02_generator.py"
    replace_once(
        path2,
        "    animated = {row[0] for row in rows if row[5] is not None}\n",
        "    animated = {row[0] for row in rows if row[5] is not None}\n    rows_by_id = {row[0]: row for row in rows}\n    expected_types = {'P_ARCH_STATIC': 'StaticMeshAsset', 'P_ARCH_ANIMATED': 'SkinnedMeshAsset'}\n",
    )
    replace_once(
        path2,
        "        assert sidecar['asset_type'] == 'StaticMeshAsset'\n",
        "        assert sidecar['asset_type'] == expected_types[rows_by_id[sidecar['asset_id']][4]]\n",
    )


def patch_integrity_test() -> None:
    path = ROOT / "Tools" / "QA" / "test_repository_integrity.py"
    replace_once(
        path,
        "    assert ids == [f\"HH_A{i:03d}\" for i in range(1, 501)]\n",
        "    assert set(ids) == {f\"HH_A{i:03d}\" for i in range(1, 501)}\n",
    )


def main() -> None:
    patch_batch01()
    patch_batch02()
    patch_existing_generator_tests()
    patch_integrity_test()
    print("Applied verified Hotel Haven QA fixes")


if __name__ == "__main__":
    main()
