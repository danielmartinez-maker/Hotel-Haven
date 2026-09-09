from __future__ import annotations

import json
from pathlib import Path

import numpy as np
import trimesh

from quality_enrichment import scene_quality_metrics
from v2_asset_common import PROFILE_ASSET_TYPES

SUPPORTED = {'.glb', '.fbx', '.obj'}


def _manifest_record(manifest, asset_id: str) -> dict:
    for family, _path, row in manifest.iter_rows():
        if row[0] == asset_id:
            return {
                'asset_id': row[0],
                'name': row[1],
                'subcategory': row[2],
                'material_family': row[3],
                'profile': row[4],
                'animation_set': row[5],
                'interaction_anchors': list(row[6]),
                'family': family,
            }
    raise KeyError(f'{asset_id} is not declared by {manifest.path.name}')


def _normalize_scene(source_model: Path) -> trimesh.Scene:
    scene = trimesh.load(source_model, force='scene')
    if not scene.geometry:
        raise ValueError(f'Meshy source contains no geometry: {source_model}')
    vertices = [np.asarray(g.vertices, dtype=float) for g in scene.geometry.values() if hasattr(g, 'vertices') and len(g.vertices)]
    if not vertices:
        raise ValueError(f'Meshy source contains no vertices: {source_model}')
    all_vertices = np.vstack(vertices)
    if not np.isfinite(all_vertices).all():
        raise ValueError(f'Meshy source contains non-finite vertices: {source_model}')

    bounds = np.asarray(scene.bounds, dtype=float)
    diagonal = float(np.linalg.norm(bounds[1] - bounds[0]))
    if diagonal <= 1e-6:
        raise ValueError(f'Meshy source has degenerate bounds: {source_model}')

    # Meshy authoring should already target metric scale. We correct obvious
    # centimeter/millimeter exports conservatively and otherwise preserve shape.
    scale = 1.0
    if diagonal > 80.0:
        scale = 0.01 if diagonal < 800.0 else 0.001
    elif diagonal < 0.02:
        scale = 100.0
    if scale != 1.0:
        scene.apply_scale(scale)

    bounds = np.asarray(scene.bounds, dtype=float)
    scene.apply_translation((0.0, 0.0, -float(bounds[0][2])))
    return scene


def normalize_meshy_asset(
    source_model: Path | str,
    target_asset_id: str,
    manifest,
    output_dir: Path | str,
    *,
    task_id: str,
    pbr_enabled: bool = True,
) -> tuple[Path, Path]:
    source = Path(source_model)
    suffix = source.suffix.lower()
    if suffix not in SUPPORTED:
        raise ValueError(f'unsupported Meshy source format: {suffix or "<none>"}')
    if not pbr_enabled:
        raise ValueError('Meshy source must be generated/exported with PBR enabled')

    record = _manifest_record(manifest, target_asset_id)
    scene = _normalize_scene(source)
    semantic_nodes = sorted(str(node) for node in scene.graph.nodes_geometry)
    if record['profile'] != 'P_SMALL_PROP' and (len(scene.geometry) < 2 or semantic_nodes == ['Body']):
        raise ValueError(
            f'{target_asset_id}: Meshy source is generic placeholder geometry; '
            'add semantic subcomponents before V2 normalization'
        )

    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)
    model_path = out / f'{target_asset_id}.glb'
    model_path.write_bytes(scene.export(file_type='glb'))
    metrics = scene_quality_metrics(model_path)

    profile = manifest.profiles[record['profile']]
    dependencies = []
    if record['profile'] == 'P_CHARACTER':
        dependencies.append('SK_HumanoidAdult')
    if record['animation_set']:
        dependencies.append(record['animation_set'])

    sidecar = {
        'schema': 1,
        'asset_id': target_asset_id,
        'asset_type': PROFILE_ASSET_TYPES[record['profile']],
        'source': str(source_model),
        'units': profile['units'],
        'lod_policy': profile['lod_policy'],
        'collision_policy': 'simple_proxy' if profile['collision_policy'] == 'none_or_simple_proxy' else profile['collision_policy'],
        'cutaway_policy': profile['cutaway_policy'],
        'material_slots': [record['material_family']],
        'interaction_anchors': record['interaction_anchors'],
        'dependencies': dependencies,
        'tags': ['meshy_authored', 'asset_quality_v2', record['family'], record['subcategory']],
        'quality_revision': 2,
        'quality_contract': 'HH_ASSET_QUALITY_V2',
        'quality_family': record['family'],
        'quality_profile': record['profile'],
        'semantic_nodes': metrics['semantic_nodes'],
        'face_count': metrics['face_count'],
        'geometry_count': metrics['geometry_count'],
        'bounds_m': metrics['bounds_m'],
        'floor_offset_m': metrics['floor_offset_m'],
        'diagonal_m': metrics['diagonal_m'],
        'authoring_provenance': {
            'provider': 'meshy',
            'task_id': task_id,
            'source_format': suffix.lstrip('.'),
            'pbr_enabled': True,
            'quality_review_state': 'NORMALIZED_FOR_V2_QA',
        },
    }
    sidecar_path = out / f'{target_asset_id}.asset.json'
    sidecar_path.write_text(json.dumps(sidecar, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return model_path, sidecar_path
