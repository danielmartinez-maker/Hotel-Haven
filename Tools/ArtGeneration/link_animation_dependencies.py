from __future__ import annotations

import argparse
import json
from pathlib import Path


def manifest_animation_bindings(manifest_dir: Path) -> dict[str, str]:
    bindings: dict[str, str] = {}
    for manifest_path in sorted(manifest_dir.glob('asset_batch_*.json')):
        data = json.loads(manifest_path.read_text())
        for group in data['groups']:
            for row in group['assets']:
                asset_id, _name, _subcategory, _material, _profile, animation_set, _anchors = row
                if animation_set:
                    bindings[asset_id] = animation_set
    return bindings


def available_animation_sets(exports_root: Path) -> set[str]:
    result = set()
    for path in exports_root.rglob('ANSET_*.animset.asset.json'):
        result.add(json.loads(path.read_text())['asset_id'])
    return result


def link(manifest_dir: Path, exports_root: Path) -> tuple[int, int]:
    bindings = manifest_animation_bindings(manifest_dir)
    available = available_animation_sets(exports_root)
    linked = 0
    deferred = 0
    for sidecar_path in exports_root.rglob('HH_A*.asset.json'):
        data = json.loads(sidecar_path.read_text())
        animation_set = bindings.get(data['asset_id'])
        if not animation_set:
            continue
        if animation_set not in available:
            deferred += 1
            continue
        deps = list(data.get('dependencies', []))
        if animation_set not in deps:
            deps.append(animation_set)
            deps.sort()
            data['dependencies'] = deps
            sidecar_path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')
        linked += 1
    print(f'linked={linked} deferred={deferred}')
    return linked, deferred


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('manifest_dir', type=Path)
    ap.add_argument('exports_root', type=Path)
    args = ap.parse_args()
    link(args.manifest_dir, args.exports_root)


if __name__ == '__main__':
    main()
