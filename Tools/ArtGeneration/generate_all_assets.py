from __future__ import annotations

import argparse
import importlib
import json
from pathlib import Path

from asset_manifest import AssetManifest, load_active_manifest


def install_trimesh_color_guard():
    import numpy as np
    import trimesh

    cls = trimesh.visual.material.PBRMaterial
    if getattr(cls, '_hotel_haven_color_guard', False):
        return
    original = cls.__init__

    def guarded(self, *args, **kwargs):
        color = kwargs.get('baseColorFactor')
        if color is not None:
            arr = np.asarray(color)
            if arr.size and arr.dtype.kind == 'f' and float(np.nanmax(arr)) > 1.0:
                kwargs['baseColorFactor'] = np.clip(np.rint(arr), 0, 255).astype(np.uint8)
        original(self, *args, **kwargs)

    cls.__init__ = guarded
    cls._hotel_haven_color_guard = True


def generator_source(batch: int) -> str:
    return f'Tools/ArtGeneration/batch{batch:02d}_generate.py'


def run_batch_generator(module, batch: int, manifest: Path, out: Path):
    if hasattr(module, 'generate_package'):
        return module.generate_package(manifest, out, generator_source(batch))
    if hasattr(module, 'generate'):
        return module.generate(manifest, out, True)
    raise RuntimeError(f'batch {batch:02d} generator exposes neither generate_package nor generate')


def normalize_sidecars(exports_root: Path) -> int:
    renamed = 0
    for sidecar in sorted(exports_root.rglob('*.asset.json')):
        if not sidecar.exists():
            continue
        base = sidecar.name[:-len('.asset.json')]
        candidates = [
            p
            for p in sidecar.parent.iterdir()
            if p.is_file() and p.name.startswith(base + '.') and not p.name.endswith('.asset.json')
        ]
        if len(candidates) != 1:
            raise RuntimeError(
                f'expected exactly one export for sidecar {sidecar}, found {[p.name for p in candidates]}'
            )
        target = sidecar.parent / (candidates[0].name + '.asset.json')
        if target != sidecar:
            sidecar.replace(target)
            renamed += 1
    return renamed


def gameplay_manifest_metadata(
    root: Path,
    manifest: AssetManifest | None = None,
) -> tuple[dict[str, dict], dict[str, dict]]:
    active = manifest or load_active_manifest(root)
    profiles = active.profiles
    assets: dict[str, dict] = {}
    for family, _manifest_path, row in active.iter_rows():
        assets[row[0]] = {
            'profile': row[4],
            'family': family,
            'material_family': row[3],
            'animation_set': row[5],
            'interaction_anchors': list(row[6]),
        }
    return assets, profiles


def normalize_gameplay_sidecar_data(sidecar: dict, meta: dict, contract: dict) -> dict:
    normalized = dict(sidecar)
    normalized['units'] = contract['units']
    normalized['lod_policy'] = contract['lod_policy']
    normalized['cutaway_policy'] = contract['cutaway_policy']

    collision_contract = contract['collision_policy']
    if collision_contract == 'none_or_simple_proxy':
        current = normalized.get('collision_policy')
        normalized['collision_policy'] = current if current in {'none', 'simple_proxy'} else 'simple_proxy'
    else:
        normalized['collision_policy'] = collision_contract

    anchors = list(meta.get('interaction_anchors', []))
    normalized['interaction_anchors'] = anchors
    tags = [
        tag
        for tag in normalized.get('tags', [])
        if not str(tag).startswith('interaction_anchor:')
    ]
    tags.extend(f'interaction_anchor:{anchor}' for anchor in anchors)
    normalized['tags'] = tags
    return normalized


def normalize_gameplay_sidecars(
    root: Path,
    exports_root: Path,
    manifest: AssetManifest | None = None,
) -> tuple[int, int]:
    assets, profiles = gameplay_manifest_metadata(root, manifest)
    normalized_count = 0
    anchor_bindings = 0
    for sidecar_path in sorted(exports_root.glob('Batch*/*.asset.json')):
        data = json.loads(sidecar_path.read_text(encoding='utf-8'))
        asset_id = data.get('asset_id')
        meta = assets.get(asset_id)
        if meta is None:
            continue
        contract = profiles[meta['profile']]
        normalized = normalize_gameplay_sidecar_data(data, meta, contract)
        sidecar_path.write_text(
            json.dumps(normalized, indent=2, sort_keys=True) + '\n',
            encoding='utf-8',
        )
        normalized_count += 1
        anchor_bindings += len(meta['interaction_anchors'])
    return normalized_count, anchor_bindings


def expected_animation_bindings(manifest: AssetManifest) -> int:
    return sum(1 for _family, _path, row in manifest.iter_rows() if row[5])


def validate_generated_tree(root: Path, exports: Path) -> dict:
    records = {}
    dependencies = []
    paired = 0
    sources = 0
    for sidecar in sorted(exports.rglob('*.asset.json')):
        data = json.loads(sidecar.read_text(encoding='utf-8'))
        asset_id = data['asset_id']
        if asset_id in records:
            raise RuntimeError(f'duplicate generated asset_id: {asset_id}')
        export = sidecar.with_name(sidecar.name[:-len('.asset.json')])
        if not export.is_file():
            raise RuntimeError(f'missing export paired with {sidecar}')
        paired += 1
        source = root / Path(data['source'])
        if not source.is_file():
            raise RuntimeError(f'missing source for {asset_id}: {data["source"]}')
        sources += 1
        records[asset_id] = sidecar
        dependencies.extend((asset_id, dep) for dep in data.get('dependencies', []))
    missing = [(owner, dep) for owner, dep in dependencies if dep not in records]
    if missing:
        raise RuntimeError(f'missing generated dependencies: {missing[:10]}')
    return {
        'generated_asset_records': len(records),
        'paired_exports': paired,
        'resolved_sources': sources,
        'resolved_dependencies': len(dependencies),
    }


def generate_all(repo_root: Path | str):
    install_trimesh_color_guard()
    from mechanical_animation_generate import generate as generate_mechanical
    from humanoid_animation_generate import generate as generate_humanoid
    from link_animation_dependencies import link
    from floor_contact_hardening import harden_floor_supports

    root = Path(repo_root).resolve()
    manifest = load_active_manifest(root)
    manifest_dir = root / 'GameData' / 'AssetDefinitions' / 'Manifest'
    exports = root / 'Art' / 'Exports'
    exports.mkdir(parents=True, exist_ok=True)
    batch_counts = {}

    for entry in manifest.batch_entries:
        batch = entry.batch
        module = importlib.import_module(f'batch{batch:02d}_generate')
        batch_manifest = manifest.batch_path(batch)
        out = exports / f'Batch{batch:02d}'
        paths = run_batch_generator(module, batch, batch_manifest, out)
        batch_counts[f'{batch:02d}'] = len(paths)

    floor_support_assets = harden_floor_supports(exports)
    mech_clips, mech_sets = generate_mechanical(exports / 'Animations')
    hum_skeletons, hum_clips, hum_sets = generate_humanoid(
        root / 'GameData' / 'AssetDefinitions' / 'animation_sets_v1.json',
        exports / 'AnimationsHumanoid',
    )
    linked, deferred = link(manifest_dir, exports)
    normalized = normalize_sidecars(exports)
    normalized_gameplay, interaction_anchor_bindings = normalize_gameplay_sidecars(root, exports, manifest)
    tree = validate_generated_tree(root, exports)
    expected_links = expected_animation_bindings(manifest)

    summary = {
        'schema': 2 if manifest.data.get('schema') == 2 else 1,
        'active_manifest': manifest.path.name,
        'declared_batch_count': len(manifest.batch_entries),
        'declared_gameplay_asset_count': manifest.asset_count,
        'batch_counts': batch_counts,
        'gameplay_asset_count': sum(batch_counts.values()),
        'floor_support_assets_hardened': len(floor_support_assets),
        'floor_support_asset_ids': floor_support_assets,
        'mechanical_clips': mech_clips,
        'mechanical_sets': mech_sets,
        'humanoid_skeletons': hum_skeletons,
        'humanoid_clips': hum_clips,
        'humanoid_sets': hum_sets,
        'animation_links': linked,
        'expected_animation_links': expected_links,
        'animation_links_deferred': deferred,
        'normalized_sidecars': normalized,
        'normalized_gameplay_sidecars': normalized_gameplay,
        'interaction_anchor_bindings': interaction_anchor_bindings,
        **tree,
    }
    expected_records = manifest.asset_count + mech_clips + mech_sets + hum_skeletons + hum_clips + hum_sets
    if summary['gameplay_asset_count'] != manifest.asset_count:
        raise RuntimeError(
            f"expected {manifest.asset_count} gameplay assets, got {summary['gameplay_asset_count']}"
        )
    if normalized_gameplay != manifest.asset_count:
        raise RuntimeError(
            f'expected {manifest.asset_count} normalized gameplay sidecars, got {normalized_gameplay}'
        )
    if tree['generated_asset_records'] != expected_records:
        raise RuntimeError(
            f"expected {expected_records} generated records, got {tree['generated_asset_records']}"
        )
    if linked != expected_links or deferred != 0:
        raise RuntimeError(
            f'animation dependency linking incomplete: linked={linked} expected={expected_links} deferred={deferred}'
        )
    (root / 'Art' / 'Validation' / 'full_generation_summary.json').write_text(
        json.dumps(summary, indent=2, sort_keys=True) + '\n', encoding='utf-8'
    )
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('repo_root', nargs='?', default='.')
    args = parser.parse_args()
    print(json.dumps(generate_all(Path(args.repo_root).resolve()), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
