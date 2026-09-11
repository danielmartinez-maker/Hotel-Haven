from __future__ import annotations

import json
from pathlib import Path

import numpy as np
import trimesh


def scene_quality_metrics(asset_path: Path | str) -> dict:
    path = Path(asset_path)
    scene = trimesh.load(path, force='scene')
    geoms = list(scene.geometry.values())
    face_count = sum(len(getattr(geom, 'faces', ())) for geom in geoms)
    try:
        bounds = np.asarray(scene.bounds, dtype=float)
    except Exception:
        vertices = [np.asarray(g.vertices) for g in geoms if hasattr(g, 'vertices') and len(g.vertices)]
        bounds = np.vstack((np.vstack(vertices).min(axis=0), np.vstack(vertices).max(axis=0)))
    if bounds.shape != (2, 3):
        raise ValueError(f'invalid bounds for {path}')
    semantic_nodes = sorted(str(node) for node in scene.graph.nodes_geometry)
    return {
        'face_count': int(face_count),
        'geometry_count': len(geoms),
        'semantic_nodes': semantic_nodes,
        'bounds_m': [[round(float(v), 6) for v in row] for row in bounds],
        'floor_offset_m': round(float(bounds[0][2]), 6),
        'diagonal_m': round(float(np.linalg.norm(bounds[1] - bounds[0])), 6),
    }


def enrich_asset_sidecar(asset_path: Path | str, sidecar_path: Path | str, meta: dict) -> dict:
    asset_path = Path(asset_path)
    sidecar_path = Path(sidecar_path)
    sidecar = json.loads(sidecar_path.read_text(encoding='utf-8'))
    metrics = scene_quality_metrics(asset_path)
    provenance = dict(sidecar.get('authoring_provenance') or {})
    provenance.setdefault('provider', 'hotel_haven_procedural')
    provenance.setdefault('source', sidecar.get('source'))
    provenance['quality_review_state'] = 'GENERATED_FOR_V2_QA'

    sidecar.update({
        'quality_revision': 2,
        'quality_contract': 'HH_ASSET_QUALITY_V2',
        'quality_profile': meta['profile'],
        'quality_family': meta['family'],
        'quality_material_family': meta.get('material_family'),
        'material_response_contract': 'soft_simplified_pbr',
        'semantic_nodes': metrics['semantic_nodes'],
        'face_count': metrics['face_count'],
        'geometry_count': metrics['geometry_count'],
        'bounds_m': metrics['bounds_m'],
        'floor_offset_m': metrics['floor_offset_m'],
        'diagonal_m': metrics['diagonal_m'],
        'authoring_provenance': provenance,
    })
    sidecar_path.write_text(json.dumps(sidecar, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return sidecar


def enrich_gameplay_library(repo_root: Path | str, exports_root: Path | str, manifest) -> dict:
    root = Path(repo_root)
    exports = Path(exports_root)
    enriched = 0
    max_faces = 0
    body_only = []
    for entry in manifest.batch_entries:
        data = json.loads(manifest.batch_path(entry.batch).read_text(encoding='utf-8'))
        for group in data['groups']:
            family = str(group['family'])
            for row in group['assets']:
                asset_id = row[0]
                meta = {
                    'profile': row[4],
                    'family': family,
                    'material_family': row[3],
                }
                asset = exports / f'Batch{entry.batch:02d}' / f'{asset_id}.glb'
                sidecar = asset.with_name(asset.name + '.asset.json')
                if not asset.is_file() or not sidecar.is_file():
                    raise FileNotFoundError(f'missing generated asset pair for {asset_id}')
                result = enrich_asset_sidecar(asset, sidecar, meta)
                enriched += 1
                max_faces = max(max_faces, int(result['face_count']))
                if result['semantic_nodes'] == ['Body']:
                    body_only.append(asset_id)
    return {
        'quality_enriched_assets': enriched,
        'quality_max_faces': max_faces,
        'quality_body_only_assets': body_only,
        'quality_body_only_asset_count': len(body_only),
    }
