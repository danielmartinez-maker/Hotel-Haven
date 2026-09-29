from __future__ import annotations

import argparse
import importlib
import json
import shutil
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


def generator_source(batch: int, family: str | None = None) -> str:
    if family is not None:
        return 'Tools/ArtGeneration/generator_registry.py'
    return f'Tools/ArtGeneration/batch{batch:02d}_generate.py'


def resolve_batch_generator(entry):
    if entry.generator_family is None:
        return importlib.import_module(f'batch{entry.batch:02d}_generate')
    from generator_registry import GENERATOR_FAMILIES
    builder = GENERATOR_FAMILIES.get(entry.generator_family)
    if builder is None:
        raise ValueError(f'unknown generator family: {entry.generator_family}')
    return builder


def run_batch_generator(module, batch: int, manifest: Path, out: Path, family: str | None = None):
    if hasattr(module, 'generate_package'):
        return module.generate_package(manifest, out, generator_source(batch, family))
    if hasattr(module, 'generate'):
        return module.generate(manifest, out, True)
    raise RuntimeError(f'batch {batch:02d} generator exposes neither generate_package nor generate')


def normalize_sidecars(exports_root: Path) -> int:
    renamed = 0
    for sidecar in sorted(exports_root.rglob('*.asset.json')):
        if not sidecar.exists():
            continue
        if sidecar.name.endswith(('.glb.asset.json', '.anim.json.asset.json', '.animset.json.asset.json', '.skeleton.json.asset.json')):
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
            if target.exists():
                previous = json.loads(target.read_text())
                current = json.loads(sidecar.read_text())
                if any(previous.get(key) != current.get(key) for key in ('asset_id', 'source')) or not set(current.get('dependencies', [])).issubset(previous.get('dependencies', [])):
                    raise RuntimeError(f'conflicting sidecars for {target}')
                sidecar.unlink()
                continue
            sidecar.replace(target)
            renamed += 1
    return renamed


def gameplay_manifest_metadata(root: Path) -> tuple[dict[str, dict], dict[str, dict]]:
    active = load_active_manifest(root)
    profiles = active.profiles
    assets: dict[str, dict] = {}
    for _family, _path, row in active.iter_rows():
        if row[0] in assets:
            raise RuntimeError(f'duplicate canonical gameplay asset: {row[0]}')
        assets[row[0]] = {'profile': row[4], 'interaction_anchors': list(row[6])}
    return assets, profiles


def normalize_gameplay_sidecar_data(sidecar: dict, meta: dict, contract: dict) -> dict:
    normalized = dict(sidecar)
    # The manifest profile is authoritative for runtime representation. Legacy
    # generators predate PrefabAsset and some animation-profile refinements, so
    # preserving their historical asset_type here makes the cooked package
    # disagree with the profile contract even when every other field is
    # normalized correctly.
    normalized['asset_type'] = contract['asset_type']
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


def normalize_gameplay_sidecars(root: Path, exports_root: Path) -> tuple[int, int]:
    assets, profiles = gameplay_manifest_metadata(root)
    normalized_count = 0
    anchor_bindings = 0
    for sidecar_path in sorted(exports_root.glob('Batch*/*.glb.asset.json')):
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
    return sum(bool(row[5]) for _, _, row in manifest.iter_rows())


def validate_generated_tree(root: Path, exports: Path) -> dict:
    records = {}
    dependencies = []
    paired = 0
    sources = 0
    for sidecar in sorted(exports.rglob('*.asset.json')):
        data = json.loads(sidecar.read_text())
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

    root = Path(repo_root)
    active = load_active_manifest(root)
    manifest_dir = root / 'GameData' / 'AssetDefinitions' / 'Manifest'
    exports = root / 'Art' / 'Exports'
    exports.mkdir(parents=True, exist_ok=True)
    # Rebuild only declared generated outputs. Stale sidecars from a previous
    # run must never masquerade as another canonical asset or dependency.
    for entry in active.batch_entries:
        shutil.rmtree(exports / f'Batch{entry.batch:02d}', ignore_errors=True)
    for generated_dir in ('Animations', 'AnimationsHumanoid'):
        shutil.rmtree(exports / generated_dir, ignore_errors=True)
    batch_counts = {}

    for entry in active.batch_entries:
        batch = entry.batch
        module = resolve_batch_generator(entry)
        manifest = active.batch_path(batch)
        out = exports / f'Batch{batch:02d}'
        paths = run_batch_generator(module, batch, manifest, out, entry.generator_family)
        if len(paths) != entry.asset_count:
            raise RuntimeError(f'batch {batch} generated {len(paths)} assets, expected {entry.asset_count}')
        batch_counts[f'{batch:02d}'] = len(paths)

    floor_support_assets = harden_floor_supports(exports)
    mech_clips, mech_sets = generate_mechanical(exports / 'Animations')
    hum_skeletons, hum_clips, hum_sets = generate_humanoid(
        root / 'GameData' / 'AssetDefinitions' / 'animation_sets_v1.json',
        exports / 'AnimationsHumanoid',
    )
    linked, deferred = link(manifest_dir, exports)
    normalized = normalize_sidecars(exports)
    normalized_gameplay, interaction_anchor_bindings = normalize_gameplay_sidecars(root, exports)
    # Some legacy batch exporters write a second short sidecar during their
    # post-export metadata pass. Reconcile it against the normalized payload.
    normalize_sidecars(exports)
    tree = validate_generated_tree(root, exports)
    expected_links = expected_animation_bindings(active)

    summary = {
        'schema': 1,
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
    expected_records = active.asset_count + mech_clips + mech_sets + hum_skeletons + hum_clips + hum_sets
    if summary['gameplay_asset_count'] != active.asset_count:
        raise RuntimeError(f"expected {active.asset_count} gameplay assets, got {summary['gameplay_asset_count']}")
    if normalized_gameplay != active.asset_count:
        raise RuntimeError(f'expected {active.asset_count} normalized gameplay sidecars, got {normalized_gameplay}')
    if tree['generated_asset_records'] != expected_records:
        raise RuntimeError(f"expected {expected_records} generated records, got {tree['generated_asset_records']}")
    if linked != expected_links or deferred != 0:
        raise RuntimeError(
            f'animation dependency linking incomplete: linked={linked} expected={expected_links} deferred={deferred}'
        )
    (root / 'Art' / 'Validation' / 'full_generation_summary.json').write_text(
        json.dumps(summary, indent=2, sort_keys=True) + '\n'
    )
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('repo_root', nargs='?', default='.')
    args = parser.parse_args()
    print(json.dumps(generate_all(Path(args.repo_root).resolve()), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
