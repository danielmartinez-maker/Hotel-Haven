#!/usr/bin/env python3
"""Deterministic OpenUSD authoring/assembly generator for Hotel Haven.

This tool creates authoring stages only. Hotel Haven's shipping runtime continues
to load HMG-070 cooked assets rather than USD source stages.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

INSTANCE_MODES = {"scenegraph", "point_instancer", "unique"}
INSTANCING_THRESHOLD = 100


def _identifier(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9_]", "_", value)
    if not cleaned:
        cleaned = "Unnamed"
    if cleaned[0].isdigit():
        cleaned = "_" + cleaned
    return cleaned


def _number(value: Any, label: str) -> float:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{label} must be numeric")
    result = float(value)
    if not math.isfinite(result):
        raise ValueError(f"{label} must be finite")
    return result


def _validate_transform(transform: Any, instance_id: str) -> tuple[float, ...]:
    if not isinstance(transform, list) or len(transform) != 9:
        raise ValueError(f"{instance_id}: transform must contain 9 numeric values")
    values = tuple(_number(value, f"{instance_id}: transform") for value in transform)
    if values[6] <= 0 or values[7] <= 0 or values[8] <= 0:
        raise ValueError(f"{instance_id}: scale components must be > 0")
    return values


def validate_manifest(manifest: dict[str, Any]) -> None:
    if manifest.get("schema") != 1:
        raise ValueError("unsupported manifest schema")
    hotel_id = manifest.get("hotel_id")
    if not isinstance(hotel_id, str) or not hotel_id.strip():
        raise ValueError("hotel_id must be a non-empty string")
    assets = manifest.get("assets")
    floors = manifest.get("floors")
    if not isinstance(assets, list) or not isinstance(floors, list):
        raise ValueError("assets and floors must be arrays")

    asset_map: dict[str, dict[str, Any]] = {}
    for asset in assets:
        if not isinstance(asset, dict):
            raise ValueError("asset must be an object")
        asset_id = asset.get("asset_id")
        if not isinstance(asset_id, str) or not asset_id:
            raise ValueError("asset_id must be a non-empty string")
        if asset_id in asset_map:
            raise ValueError(f"duplicate asset_id {asset_id}")
        asset_map[asset_id] = asset
        if not isinstance(asset.get("usd_path"), str) or not asset["usd_path"]:
            raise ValueError(f"{asset_id}: usd_path must be a non-empty string")
        if not isinstance(asset.get("runtime_asset_id"), str) or not asset["runtime_asset_id"]:
            raise ValueError(f"{asset_id}: runtime_asset_id must be a non-empty string")
        mode = asset.get("instance_mode")
        if mode not in INSTANCE_MODES:
            raise ValueError(f"{asset_id}: instance_mode must be one of {sorted(INSTANCE_MODES)}")
        expected = asset.get("expected_instances", 0)
        if isinstance(expected, bool) or not isinstance(expected, int) or expected < 0:
            raise ValueError(f"{asset_id}: expected_instances must be a non-negative integer")
        if expected > INSTANCING_THRESHOLD and mode == "unique" and not asset.get("instancing_exempt_reason"):
            raise ValueError(f"{asset_id}: assets expected more than 100 times must be instance-friendly")

    floor_ids: set[int] = set()
    instance_ids: set[str] = set()
    actual_counts: Counter[str] = Counter()
    for floor in floors:
        if not isinstance(floor, dict):
            raise ValueError("floor must be an object")
        floor_id = floor.get("floor_id")
        if isinstance(floor_id, bool) or not isinstance(floor_id, int):
            raise ValueError("floor_id must be an integer")
        if floor_id in floor_ids:
            raise ValueError(f"duplicate floor_id {floor_id}")
        floor_ids.add(floor_id)
        instances = floor.get("instances")
        if not isinstance(instances, list):
            raise ValueError(f"floor {floor_id}: instances must be an array")
        for instance in instances:
            if not isinstance(instance, dict):
                raise ValueError(f"floor {floor_id}: instance must be an object")
            instance_id = instance.get("instance_id")
            if not isinstance(instance_id, str) or not instance_id:
                raise ValueError(f"floor {floor_id}: instance_id must be a non-empty string")
            if instance_id in instance_ids:
                raise ValueError(f"duplicate instance_id {instance_id}")
            instance_ids.add(instance_id)
            asset_id = instance.get("asset_id")
            if asset_id not in asset_map:
                raise ValueError(f"{instance_id}: unknown asset {asset_id}")
            _validate_transform(instance.get("transform"), instance_id)
            actual_counts[asset_id] += 1

    for asset_id, count in actual_counts.items():
        asset = asset_map[asset_id]
        if count > INSTANCING_THRESHOLD and asset["instance_mode"] == "unique" and not asset.get("instancing_exempt_reason"):
            raise ValueError(f"{asset_id}: actual placement count more than 100 requires instancing")


def _format_number(value: float) -> str:
    if abs(value - round(value)) < 1e-9:
        return str(int(round(value)))
    return format(value, ".9g")


def _vec3(values: tuple[float, float, float]) -> str:
    return "(" + ", ".join(_format_number(value) for value in values) + ")"


def _quaternion_xyz(rx: float, ry: float, rz: float) -> tuple[float, float, float, float]:
    x = math.radians(rx) * 0.5
    y = math.radians(ry) * 0.5
    z = math.radians(rz) * 0.5
    cx, sx = math.cos(x), math.sin(x)
    cy, sy = math.cos(y), math.sin(y)
    cz, sz = math.cos(z), math.sin(z)
    w = cx * cy * cz + sx * sy * sz
    qx = sx * cy * cz - cx * sy * sz
    qy = cx * sy * cz + sx * cy * sz
    qz = cx * cy * sz - sx * sy * cz
    return (w, qx, qy, qz)


def _atomic_write(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp = path.with_suffix(path.suffix + ".tmp")
    temp.write_text(text, encoding="utf-8", newline="\n")
    os.replace(temp, path)


def _asset_reference_path(asset: dict[str, Any]) -> str:
    return "../" + asset["usd_path"].replace("\\", "/").lstrip("/")


def _scenegraph_instance(instance: dict[str, Any], asset: dict[str, Any], indent: str = "        ") -> str:
    tx, ty, tz, rx, ry, rz, sx, sy, sz = _validate_transform(instance["transform"], instance["instance_id"])
    name = "I_" + _identifier(instance["instance_id"])
    instanceable = "true" if asset["instance_mode"] == "scenegraph" else "false"
    return (
        f'{indent}def Xform "{name}" (\n'
        f'{indent}    prepend references = @{_asset_reference_path(asset)}@</Asset>\n'
        f'{indent}    instanceable = {instanceable}\n'
        f'{indent})\n'
        f'{indent}{{\n'
        f'{indent}    double3 xformOp:translate = {_vec3((tx, ty, tz))}\n'
        f'{indent}    double3 xformOp:rotateXYZ = {_vec3((rx, ry, rz))}\n'
        f'{indent}    double3 xformOp:scale = {_vec3((sx, sy, sz))}\n'
        f'{indent}    uniform token[] xformOpOrder = ["xformOp:translate", "xformOp:rotateXYZ", "xformOp:scale"]\n'
        f'{indent}}}\n'
    )


def _point_instancer(asset_id: str, asset: dict[str, Any], instances: list[dict[str, Any]], indent: str = "        ") -> str:
    name = "PI_" + _identifier(asset_id)
    ordered = sorted(instances, key=lambda item: item["instance_id"])
    positions: list[str] = []
    orientations: list[str] = []
    scales: list[str] = []
    for instance in ordered:
        tx, ty, tz, rx, ry, rz, sx, sy, sz = _validate_transform(instance["transform"], instance["instance_id"])
        positions.append(_vec3((tx, ty, tz)))
        w, qx, qy, qz = _quaternion_xyz(rx, ry, rz)
        orientations.append("(" + ", ".join(_format_number(v) for v in (w, qx, qy, qz)) + ")")
        scales.append(_vec3((sx, sy, sz)))
    proto_path = f"</Floor/PointInstancers/{name}/Prototypes/P0>"
    return (
        f'{indent}def PointInstancer "{name}"\n'
        f'{indent}{{\n'
        f'{indent}    rel prototypes = [{proto_path}]\n'
        f'{indent}    int[] protoIndices = [{", ".join("0" for _ in ordered)}]\n'
        f'{indent}    point3f[] positions = [{", ".join(positions)}]\n'
        f'{indent}    quatf[] orientations = [{", ".join(orientations)}]\n'
        f'{indent}    float3[] scales = [{", ".join(scales)}]\n'
        f'{indent}    def Scope "Prototypes"\n'
        f'{indent}    {{\n'
        f'{indent}        def Xform "P0" (\n'
        f'{indent}            prepend references = @{_asset_reference_path(asset)}@</Asset>\n'
        f'{indent}        )\n'
        f'{indent}        {{\n'
        f'{indent}        }}\n'
        f'{indent}    }}\n'
        f'{indent}}}\n'
    )


def _hotel_stage(floors: list[dict[str, Any]]) -> str:
    lines = ["#usda 1.0", "(", '    defaultPrim = "Hotel"', "    metersPerUnit = 1", '    upAxis = "Y"', ")", "", 'def Xform "Hotel"', "{", '    def Scope "Floors"', "    {"]
    for floor in sorted(floors, key=lambda item: item["floor_id"]):
        floor_id = int(floor["floor_id"])
        label = f"F_{floor_id:02d}" if floor_id >= 0 else f"B_{abs(floor_id):02d}"
        lines.extend([f'        def Xform "{label}" (', f"            prepend payload = @floors/{label}.usda@</Floor>", "        )", "        {", "        }"])
    lines.extend(["    }", "}", ""])
    return "\n".join(lines)


def _floor_stage(floor: dict[str, Any], asset_map: dict[str, dict[str, Any]]) -> str:
    floor_id = int(floor["floor_id"])
    instances = sorted(floor["instances"], key=lambda item: item["instance_id"])
    scenegraph = [item for item in instances if asset_map[item["asset_id"]]["instance_mode"] != "point_instancer"]
    point_groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for item in instances:
        if asset_map[item["asset_id"]]["instance_mode"] == "point_instancer":
            point_groups[item["asset_id"]].append(item)
    lines = ["#usda 1.0", "(", '    defaultPrim = "Floor"', "    metersPerUnit = 1", '    upAxis = "Y"', ")", "", 'def Xform "Floor"', "{", f"    custom int hotelHaven:floorId = {floor_id}", '    def Scope "ScenegraphInstances"', "    {"]
    for item in scenegraph:
        lines.append(_scenegraph_instance(item, asset_map[item["asset_id"]], indent="        ").rstrip("\n"))
    lines.extend(["    }", '    def Scope "PointInstancers"', "    {"])
    for asset_id in sorted(point_groups):
        lines.append(_point_instancer(asset_id, asset_map[asset_id], point_groups[asset_id], indent="        ").rstrip("\n"))
    lines.extend(["    }", "}", ""])
    return "\n".join(lines)


def _cook_manifest(manifest: dict[str, Any]) -> dict[str, Any]:
    used = {instance["asset_id"] for floor in manifest["floors"] for instance in floor["instances"]}
    asset_map = {asset["asset_id"]: asset for asset in manifest["assets"]}
    floors = []
    for floor in sorted(manifest["floors"], key=lambda item: item["floor_id"]):
        floor_id = int(floor["floor_id"])
        label = f"F_{floor_id:02d}" if floor_id >= 0 else f"B_{abs(floor_id):02d}"
        floors.append(f"floors/{label}.usda")
    return {
        "schema": 1,
        "hotel_id": manifest["hotel_id"],
        "authoring_stage": "hotel.usda",
        "runtime_policy": "cook_to_hasset",
        "floors": floors,
        "assets": [{"asset_id": asset_id, "usd_path": asset_map[asset_id]["usd_path"], "runtime_asset_id": asset_map[asset_id]["runtime_asset_id"], "instance_mode": asset_map[asset_id]["instance_mode"]} for asset_id in sorted(used)],
    }


def generate(manifest: dict[str, Any], output_dir: Path) -> None:
    validate_manifest(manifest)
    output_dir = Path(output_dir)
    asset_map = {asset["asset_id"]: asset for asset in manifest["assets"]}
    _atomic_write(output_dir / "hotel.usda", _hotel_stage(manifest["floors"]))
    for floor in sorted(manifest["floors"], key=lambda item: item["floor_id"]):
        floor_id = int(floor["floor_id"])
        label = f"F_{floor_id:02d}" if floor_id >= 0 else f"B_{abs(floor_id):02d}"
        _atomic_write(output_dir / "floors" / f"{label}.usda", _floor_stage(floor, asset_map))
    _atomic_write(output_dir / "hotel_haven_usd_cook_manifest.json", json.dumps(_cook_manifest(manifest), indent=2, sort_keys=True) + "\n")


def _load_manifest(path: Path) -> dict[str, Any]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        raise ValueError("manifest root must be an object")
    return payload


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Hotel Haven deterministic OpenUSD scene assembly")
    subparsers = parser.add_subparsers(dest="command", required=True)
    validate_parser = subparsers.add_parser("validate")
    validate_parser.add_argument("manifest")
    generate_parser = subparsers.add_parser("generate")
    generate_parser.add_argument("manifest")
    generate_parser.add_argument("--output-dir", required=True)
    args = parser.parse_args(argv)
    manifest = _load_manifest(Path(args.manifest))
    validate_manifest(manifest)
    if args.command == "generate":
        generate(manifest, Path(args.output_dir))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
