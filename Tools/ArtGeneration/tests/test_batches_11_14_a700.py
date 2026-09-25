from __future__ import annotations

import importlib
import json
import sys
from pathlib import Path

import trimesh

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / "Tools" / "ArtGeneration"
MANIFEST = ROOT / "GameData" / "AssetDefinitions" / "Manifest"
sys.path.insert(0, str(ART))


def rows(batch: int):
    data = json.loads(
        (MANIFEST / f"asset_batch_{batch:02d}.json").read_text(encoding="utf-8")
    )
    return [row for group in data["groups"] for row in group["assets"]]


def test_a700_extension_generator_modules_exist():
    for batch in range(11, 15):
        assert (ART / f"batch{batch:02d}_generate.py").is_file(), batch


def test_a700_extension_generates_50_semantic_assets_per_batch(tmp_path):
    for batch in range(11, 15):
        module = importlib.import_module(f"batch{batch:02d}_generate")
        out = tmp_path / f"Batch{batch:02d}"
        paths = module.generate_package(
            MANIFEST / f"asset_batch_{batch:02d}.json",
            out,
            f"Tools/ArtGeneration/batch{batch:02d}_generate.py",
        )
        assert len(paths) == 50
        assert {path.stem for path in paths} == {row[0] for row in rows(batch)}
        assert len(list(out.glob("*.asset.json"))) == 50

        for path in paths:
            scene = trimesh.load(path, force="scene")
            names = set(scene.graph.nodes_geometry)
            assert len(scene.geometry) >= 2, path.stem
            assert names != {"Body"}, path.stem
            assert any(
                token in name
                for name in names
                for token in (
                    "Primary", "Base", "Seat", "Top", "Frame", "Shell",
                    "Door", "Cab", "Panel", "Wheel", "Fixture",
                    "Functional", "Role", "Bed", "Counter", "Shelf",
                )
            ), (path.stem, sorted(names))

            sidecar = json.loads(
                path.with_suffix(".asset.json").read_text(encoding="utf-8")
            )
            assert sidecar["asset_id"] == path.stem
            assert sidecar["units"] == "meters"
            assert sidecar["quality_revision"] == 2
            assert sidecar["quality_contract"] == "HH_ASSET_QUALITY_V2"
            assert sidecar["material_slots"]


def test_a700_extension_preserves_profile_runtime_asset_types(tmp_path):
    expected = {
        "P_ARCH_STATIC": "StaticMeshAsset",
        "P_ARCH_ANIMATED": "SkinnedMeshAsset",
        "P_FURNITURE_STATIC": "StaticMeshAsset",
        "P_SERVICE_PROP_ANIMATED": "SkinnedMeshAsset",
        "P_INTERACTIVE_PREFAB": "PrefabAsset",
        "P_SMALL_PROP": "StaticMeshAsset",
    }
    for batch in range(11, 15):
        module = importlib.import_module(f"batch{batch:02d}_generate")
        out = tmp_path / f"types-{batch:02d}"
        module.generate_package(
            MANIFEST / f"asset_batch_{batch:02d}.json",
            out,
            f"Tools/ArtGeneration/batch{batch:02d}_generate.py",
        )
        for row in rows(batch):
            asset_id, _name, _subcat, _mat, profile, _animation, _anchors = row
            sidecar = json.loads(
                (out / f"{asset_id}.asset.json").read_text(encoding="utf-8")
            )
            assert sidecar["asset_type"] == expected[profile]


def test_live_a700_props_use_purpose_built_semantic_geometry(tmp_path):
    expectations = {
        12: {
            "HH_A559": {"Base", "FrameStem", "LampShade"},
            "HH_A570": {"Frame", "ArtPanel"},
            "HH_A571": {"Frame", "MirrorGlass"},
            "HH_A578": {"BaseTray", "FunctionalInset"},
            "HH_A579": {"FrameRiser", "FunctionalHead"},
            "HH_A580": {"FrameRail", "Mount_L", "Mount_R"},
            "HH_A582": {"BaseTray", "Bottle_0"},
        },
        13: {
            "HH_A610": {"SeatBase", "BaseLeg_0"},
            "HH_A614": {"Base", "SeatCushion", "TopPad"},
            "HH_A619": {"Base", "FrameStem", "LampShade"},
            "HH_A623": {"BasePlanter", "FrameSlat_0", "RolePlant_0"},
            "HH_A643": {"Base", "Frame", "Shelf_0"},
            "HH_A644": {"Base", "FramePedestal", "PanelScreen"},
        },
    }
    for batch, assets in expectations.items():
        module = importlib.import_module(f"batch{batch:02d}_generate")
        out = tmp_path / f"purpose-built-{batch:02d}"
        module.generate_package(
            MANIFEST / f"asset_batch_{batch:02d}.json",
            out,
            f"Tools/ArtGeneration/batch{batch:02d}_generate.py",
        )
        for asset_id, required_nodes in assets.items():
            scene = trimesh.load(out / f"{asset_id}.glb", force="scene")
            nodes = set(map(str, scene.graph.nodes_geometry))
            assert required_nodes <= nodes, (asset_id, sorted(required_nodes - nodes))
