#!/usr/bin/env python3
"""Validate the shipping client's cooked world-asset binding contract."""
from __future__ import annotations

import json
import re
import sys
from collections import Counter
from pathlib import Path

ASSET_ID_RE = re.compile(r"^HH_A\d{3}$")
CPP_ASSET_ID_RE = re.compile(r'"(HH_A\d{3})"')


def load_json(path: Path) -> dict:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, json.JSONDecodeError) as exc:
        raise AssertionError(f"cannot read valid JSON {path}: {exc}") from exc


def repo_root_from_script(script_path: Path) -> Path:
    return script_path.resolve().parents[3]


def catalog_asset_types(repo_root: Path) -> dict[str, str]:
    defs = repo_root / "GameData" / "AssetDefinitions"
    master = load_json(defs / "hotel_haven_asset_manifest_v1.json")
    profiles = master.get("profiles", {})
    if not isinstance(profiles, dict):
        raise AssertionError("master profiles must be an object")

    result: dict[str, str] = {}
    for batch_spec in master.get("batches", []):
        batch = load_json(repo_root / batch_spec["path"])
        for group in batch.get("groups", []):
            for row in group.get("assets", []):
                if not isinstance(row, list) or len(row) < 5:
                    continue
                asset_id = row[0]
                profile_id = row[4]
                profile = profiles.get(profile_id)
                if not isinstance(profile, dict):
                    raise AssertionError(
                        f"{asset_id}: unknown profile {profile_id!r}"
                    )
                result[asset_id] = profile.get("asset_type")
    return result


def validate(repo_root: Path) -> list[str]:
    errors: list[str] = []
    defs = repo_root / "GameData" / "AssetDefinitions"
    contract_path = defs / "runtime_world_bindings_v1.json"

    try:
        contract = load_json(contract_path)
        catalog = catalog_asset_types(repo_root)
    except AssertionError as exc:
        return [str(exc)]

    if contract.get("schema") != 1:
        errors.append("runtime world binding schema must be 1")
    if contract.get("source") != "game/app/WorldAssetBindings.cpp":
        errors.append("runtime world binding source must point to WorldAssetBindings.cpp")

    groups = contract.get("groups")
    if not isinstance(groups, dict) or not groups:
        errors.append("runtime world binding groups must be a non-empty object")
        groups = {}

    ids: list[str] = []
    for group_name, group_ids in groups.items():
        if not isinstance(group_ids, list):
            errors.append(f"group {group_name} must be an array")
            continue
        for asset_id in group_ids:
            if not isinstance(asset_id, str) or not ASSET_ID_RE.fullmatch(asset_id):
                errors.append(f"group {group_name} has invalid asset id {asset_id!r}")
                continue
            ids.append(asset_id)

    declared_count = contract.get("asset_count")
    if declared_count != len(ids):
        errors.append(
            f"runtime binding asset_count is {declared_count!r}, flattened groups contain {len(ids)}"
        )

    duplicates = sorted(
        asset_id for asset_id, count in Counter(ids).items() if count > 1
    )
    if duplicates:
        errors.append(f"runtime binding contract has duplicate ids: {duplicates}")

    supported_types = contract.get("runtime_supported_asset_types")
    if not isinstance(supported_types, list) or not supported_types:
        errors.append("runtime_supported_asset_types must be a non-empty array")
        supported_types = []
    supported_type_set = set(supported_types)

    for asset_id in ids:
        asset_type = catalog.get(asset_id)
        if asset_type is None:
            errors.append(f"{asset_id}: missing from canonical 500-asset catalog")
        elif asset_type not in supported_type_set:
            errors.append(
                f"{asset_id}: catalog type {asset_type!r} is not runtime-renderable"
            )

    expected_groups = {
        "guest_characters": [f"HH_A{i:03d}" for i in range(451, 481)],
        "receptionist_characters": ["HH_A481", "HH_A482"],
        "housekeeper_characters": ["HH_A487", "HH_A488"],
        "maintenance_characters": ["HH_A495", "HH_A496"],
    }
    for group_name, expected in expected_groups.items():
        actual = groups.get(group_name)
        if actual != expected:
            errors.append(
                f"{group_name} must exactly match {expected[0]}-{expected[-1]}"
                if len(expected) > 2
                else f"{group_name} must exactly match {expected}"
            )

    source_path = repo_root / str(contract.get("source", ""))
    try:
        source = source_path.read_text(encoding="utf-8")
    except OSError as exc:
        errors.append(f"cannot read runtime binding source {source_path}: {exc}")
        return errors

    cpp_ids = CPP_ASSET_ID_RE.findall(source)
    cpp_duplicates = sorted(
        asset_id for asset_id, count in Counter(cpp_ids).items() if count > 1
    )
    if cpp_duplicates:
        errors.append(
            f"WorldAssetBindings.cpp repeats runtime asset ids: {cpp_duplicates}"
        )

    contract_set = set(ids)
    cpp_set = set(cpp_ids)
    missing_in_cpp = sorted(contract_set - cpp_set)
    extra_in_cpp = sorted(cpp_set - contract_set)
    if missing_in_cpp:
        errors.append(
            f"runtime binding manifest ids missing from C++ source: {missing_in_cpp}"
        )
    if extra_in_cpp:
        errors.append(
            f"C++ runtime asset ids missing from binding manifest: {extra_in_cpp}"
        )

    return errors


def main() -> int:
    repo_root = repo_root_from_script(Path(__file__))
    errors = validate(repo_root)
    if errors:
        print(
            f"Hotel Haven runtime world binding validation FAILED ({len(errors)} issue(s)):"
        )
        for error in errors:
            print(f" - {error}")
        return 1

    contract = load_json(
        repo_root / "GameData" / "AssetDefinitions" / "runtime_world_bindings_v1.json"
    )
    print("Hotel Haven runtime world binding validation PASSED")
    print(f" - {contract['asset_count']} unique renderer-compatible startup assets")
    print(" - C++ binding IDs exactly match the canonical runtime binding manifest")
    print(" - guest/staff character variant ranges are complete")
    return 0


if __name__ == "__main__":
    sys.exit(main())
