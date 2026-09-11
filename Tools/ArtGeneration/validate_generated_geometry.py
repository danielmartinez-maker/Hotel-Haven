from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import trimesh

from semantic_asset_quality import semantic_contract_failures, variant_signature_failures

EXPECTED_MOVING = {
    'ANSET_MECH_DOOR': ('MOV_DoorLeaf',),
    'ANSET_MECH_SLIDING_DOOR': ('MOV_SlidingPanel', 'MOV_PocketPanel'),
    'ANSET_MECH_REVOLVING_DOOR': ('MOV_RevolvingLeaf',),
    'ANSET_MECH_ELEVATOR': ('MOV_ElevatorDoor',),
    'ANSET_MECH_OVERHEAD_DOOR': ('MOV_DockPanel',),
    'ANSET_MECH_CURTAIN': ('MOV_CurtainPanel',),
    'ANSET_SERVICE_CART': ('MOV_Wheel',),
}
CHARACTER_NODES = {
    'Hips', 'Spine', 'Chest', 'Head',
    'UpperArm_L', 'LowerArm_L', 'Hand_L',
    'UpperArm_R', 'LowerArm_R', 'Hand_R',
    'UpperLeg_L', 'LowerLeg_L', 'Foot_L',
    'UpperLeg_R', 'LowerLeg_R', 'Foot_R',
}
PROFILE_FACE_BUDGETS = {
    'P_ARCH_STATIC': 5000,
    'P_ARCH_ANIMATED': 5000,
    'P_FURNITURE_STATIC': 4200,
    'P_SERVICE_PROP_ANIMATED': 4200,
    'P_INTERACTIVE_PREFAB': 4200,
    'P_INTERACTIVE_ANIMATED': 4200,
    'P_SMALL_PROP': 3000,
    'P_CHARACTER': 2500,
}

# Placement authority ultimately lives in the owning gameplay systems. These
# pieces use normal art-production profiles but are intentionally mounted on
# walls, ceilings, counters, or other secondary surfaces, so floor contact is
# not an appropriate release criterion for them.
CONTEXTUAL_PLACEMENT_OVERRIDES = {
    'HH_A153': 'wall-mounted television',
    'HH_A165': 'hanging bathrobe',
    'HH_A175': 'wall towel shelf',
    'HH_A177': 'wall sconce',
    'HH_A179': 'wall-mounted hair dryer',
    'HH_A204': 'ceiling chandelier',
    'HH_A221': 'front-desk computer placed on counter',
    'HH_A238': 'wall/lobby clock',
    'HH_A371': 'presentation screen',
    'HH_A372': 'conference whiteboard',
}


def manifest_rows(manifest_dir: Path):
    out = {}
    for path in sorted(manifest_dir.glob('asset_batch_*.json')):
        data = json.loads(path.read_text())
        for group in data['groups']:
            for row in group['assets']:
                out[row[0]] = {
                    'name': row[1],
                    'profile': row[4],
                    'animation_set': row[5],
                    'interaction_anchors': list(row[6]),
                }
    return out


def profile_contracts(repo_root: Path) -> dict[str, dict]:
    path = repo_root / 'GameData' / 'AssetDefinitions' / 'hotel_haven_asset_manifest_v1.json'
    return json.loads(path.read_text(encoding='utf-8'))['profiles']


def material_rgb(geom):
    mat = getattr(getattr(geom, 'visual', None), 'material', None)
    rgba = getattr(mat, 'baseColorFactor', None)
    if rgba is None:
        return None
    arr = np.asarray(rgba).reshape(-1)
    if arr.size < 3:
        return None
    if arr.dtype.kind == 'f' and float(np.nanmax(arr)) <= 1.0:
        arr = arr * 255.0
    return tuple(int(x) for x in np.clip(np.rint(arr[:3]), 0, 255))


def animation_dependency_failures(
    asset_id: str,
    meta: dict,
    sidecar: dict,
    available_animation_sets: set[str],
) -> list[str]:
    animation_set = meta.get('animation_set')
    if not animation_set:
        return []
    failures = []
    dependencies = sidecar.get('dependencies', [])
    if animation_set not in dependencies:
        failures.append(f'{asset_id}: sidecar missing animation dependency {animation_set}')
    if animation_set not in available_animation_sets:
        failures.append(
            f'{asset_id}: animation dependency {animation_set} has no generated animation-set asset'
        )
    return failures


def profile_contract_failures(asset_id: str, meta: dict, sidecar: dict, contract: dict) -> list[str]:
    failures = []
    for field in ('units', 'lod_policy', 'cutaway_policy'):
        expected = contract.get(field)
        actual = sidecar.get(field)
        if actual != expected:
            failures.append(f'{asset_id}: {field} {actual!r} does not match profile contract {expected!r}')

    collision_contract = contract.get('collision_policy')
    actual_collision = sidecar.get('collision_policy')
    if collision_contract == 'none_or_simple_proxy':
        if actual_collision not in {'none', 'simple_proxy'}:
            failures.append(
                f'{asset_id}: collision_policy {actual_collision!r} is not allowed by profile contract {collision_contract!r}'
            )
    elif actual_collision != collision_contract:
        failures.append(
            f'{asset_id}: collision_policy {actual_collision!r} does not match profile contract {collision_contract!r}'
        )

    expected_anchors = list(meta.get('interaction_anchors', []))
    actual_anchors = sidecar.get('interaction_anchors')
    if actual_anchors != expected_anchors:
        failures.append(
            f'{asset_id}: interaction_anchors {actual_anchors!r} do not match manifest {expected_anchors!r}'
        )
    return failures


def placement_failures(asset_id: str, pivot_profile: str, bounds_min, bounds_max) -> list[str]:
    if asset_id in CONTEXTUAL_PLACEMENT_OVERRIDES:
        return []
    if pivot_profile in {'contextual', 'contextual_architecture'}:
        return []
    if pivot_profile not in {'floor_contact_center', 'feet_midpoint'}:
        return []

    minimum = np.asarray(bounds_min, dtype=float)
    maximum = np.asarray(bounds_max, dtype=float)
    if minimum.shape != (3,) or maximum.shape != (3,):
        return [f'{asset_id}: placement bounds must contain three coordinates']

    failures = []
    floor_z = float(minimum[2])
    floor_tolerance = 0.12
    if floor_z > floor_tolerance:
        failures.append(f'{asset_id}: floating {floor_z:.3f}m above placement floor')
    elif floor_z < -floor_tolerance:
        failures.append(f'{asset_id}: sunk {-floor_z:.3f}m below placement floor')

    horizontal_extent = maximum[:2] - minimum[:2]
    center_xy = (minimum[:2] + maximum[:2]) * 0.5
    centering_tolerance = max(0.25, 0.55 * float(np.max(horizontal_extent)))
    center_distance = float(np.linalg.norm(center_xy))
    if center_distance > centering_tolerance:
        failures.append(
            f'{asset_id}: off-center placement pivot by {center_distance:.3f}m '
            f'(tolerance {centering_tolerance:.3f}m)'
        )
    return failures


def release_audit_markdown(summary: dict, report: dict, batch_statuses: dict[str, str]) -> str:
    gameplay = summary.get('gameplay_asset_count', 0)
    linked = summary.get('animation_links', 0)
    expected = summary.get('expected_animation_links', 0)
    deferred = summary.get('animation_links_deferred', 0)
    qc_failures = report.get('failure_count', 0)
    qc_status = report.get('status', 'UNKNOWN')
    contract_failures = report.get('profile_contract_failure_count', 0)
    placement_failure_count = report.get('placement_failure_count', 0)
    semantic_failure_count = report.get('semantic_failure_count', 0)
    contract_status = 'PASS' if contract_failures == 0 else 'FAIL'
    placement_status = 'PASS' if placement_failure_count == 0 else 'FAIL'
    semantic_status = 'PASS' if semantic_failure_count == 0 else 'FAIL'
    anchor_bindings = report.get('interaction_anchor_bindings', 0)
    expected_anchor_bindings = report.get('expected_interaction_anchor_bindings', 0)

    unresolved = qc_failures + deferred
    target_assets = int(summary.get('declared_gameplay_asset_count', gameplay))
    if gameplay != target_assets:
        unresolved += 1
    if linked != expected:
        unresolved += 1

    lines = [
        f'# Hotel Haven {target_assets}-Asset Library Release Audit V1',
        '',
        '## Release gate',
        '',
        f'- Gameplay-facing assets: **{gameplay} / {target_assets}**',
        f'- Generated asset records (gameplay + animation support): **{summary.get("generated_asset_records", 0)}**',
        f'- Animation dependencies linked: **{linked} / {expected}**',
        f'- Deferred animation dependencies: **{deferred}**',
        f'- Interaction anchors normalized: **{anchor_bindings} / {expected_anchor_bindings}**',
        f'- Profile contract conformance: **{contract_status}** with **{contract_failures}** failures',
        f'- Placement/pivot QC: **{placement_status}** with **{placement_failure_count}** failures',
        f'- Semantic identity QC: **{semantic_status}** with **{semantic_failure_count}** failures',
        f'- Contextual placement exceptions: **{report.get("contextual_placement_override_count", 0)}** documented assets',
        f'- Floor-support hardening applied: **{summary.get("floor_support_assets_hardened", 0)}** generated assets',
        f'- Mechanical animation coverage: **{summary.get("mechanical_clips", 0)} clips / {summary.get("mechanical_sets", 0)} sets**',
        f'- Humanoid animation coverage: **{summary.get("humanoid_clips", 0)} clips / {summary.get("humanoid_sets", 0)} sets / {summary.get("humanoid_skeletons", 0)} skeletons**',
        f'- Geometry QC: **{qc_status}** with **{qc_failures}** failures',
        f'- Maximum generated face count: **{report.get("max_faces", 0)}**',
        f'- Material palette: **{report.get("unique_material_colors", 0)}** unique sampled RGB colors; non-white ratio **{report.get("nonwhite_material_ratio", 0)}**',
        f'- Unresolved BLOCKER/CRITICAL/MAJOR issues: **{unresolved}**',
        '',
        '## Profile face budgets',
        '',
        '| Profile | Maximum faces |',
        '| --- | ---: |',
    ]
    for profile, budget in sorted(report.get('profile_face_budgets', {}).items()):
        lines.append(f'| {profile} | {budget} |')
    lines.extend(['', '## Batch status', '', '| Batch | Status |', '| --- | --- |'])
    for batch in range(1, int(summary.get('declared_batch_count', len(batch_statuses))) + 1):
        lines.append(f'| Batch {batch:02d} | {batch_statuses.get(f"{batch:02d}", "UNKNOWN")} |')
    lines.extend([
        '',
        '## Operational note',
        '',
        'Generated GLB binaries and preview boards remain CI artifacts by design; the repository stores deterministic generators, manifests, validation policy, and this audit record.',
        '',
    ])
    return '\n'.join(lines)


def validate(repo_root: Path) -> dict:
    manifest_dir = repo_root / 'GameData' / 'AssetDefinitions' / 'Manifest'
    manifests = manifest_rows(manifest_dir)
    contracts = profile_contracts(repo_root)
    exports = repo_root / 'Art' / 'Exports'
    failures = []
    contract_failure_details = []
    placement_failure_details = []
    semantic_failure_details = []
    stats = []
    signature_by_asset = {}
    material_samples = 0
    nonwhite_materials = 0
    unique_colors = set()
    expected_anchor_bindings = sum(len(meta.get('interaction_anchors', [])) for meta in manifests.values())
    anchor_bindings = 0
    available_animation_sets = {
        json.loads(path.read_text())['asset_id']
        for path in exports.rglob('ANSET_*.animset.json.asset.json')
    }
    paths = sorted(exports.glob('Batch*/*.glb'))
    expected_asset_count = len(manifests)
    if len(paths) != expected_asset_count:
        failures.append(f'expected {expected_asset_count} GLBs, found {len(paths)}')

    for path in paths:
        asset_id = path.stem
        meta = manifests.get(asset_id)
        if not meta:
            failures.append(f'{asset_id}: missing manifest row')
            continue
        contract = contracts.get(meta['profile'])
        if contract is None:
            failures.append(f'{asset_id}: missing profile contract {meta["profile"]}')
            continue

        sidecar_path = path.with_name(path.name + '.asset.json')
        if not sidecar_path.is_file():
            failures.append(f'{asset_id}: missing normalized sidecar {sidecar_path.name}')
            continue
        try:
            sidecar = json.loads(sidecar_path.read_text())
        except Exception as exc:
            failures.append(f'{asset_id}: sidecar load failed: {exc}')
            continue

        failures.extend(animation_dependency_failures(asset_id, meta, sidecar, available_animation_sets))
        contract_failures = profile_contract_failures(asset_id, meta, sidecar, contract)
        contract_failure_details.extend(contract_failures)
        failures.extend(contract_failures)
        actual_anchors = sidecar.get('interaction_anchors', [])
        if isinstance(actual_anchors, list):
            actual_anchor_set = set(actual_anchors)
            anchor_bindings += sum(
                1 for anchor in meta.get('interaction_anchors', []) if anchor in actual_anchor_set
            )

        try:
            scene = trimesh.load(path, force='scene')
        except Exception as exc:
            failures.append(f'{asset_id}: load failed: {exc}')
            continue
        geoms = list(scene.geometry.values())
        if not geoms:
            failures.append(f'{asset_id}: empty scene')
            continue
        verts = [g.vertices for g in geoms if hasattr(g, 'vertices') and len(g.vertices)]
        if not verts:
            failures.append(f'{asset_id}: no vertices')
            continue
        vertices = np.vstack(verts)
        if not np.isfinite(vertices).all():
            failures.append(f'{asset_id}: non-finite vertices')

        try:
            bounds = np.asarray(scene.bounds, dtype=float)
        except Exception:
            bounds = np.vstack((vertices.min(axis=0), vertices.max(axis=0)))
        if bounds.shape != (2, 3) or not np.isfinite(bounds).all():
            bounds = np.vstack((vertices.min(axis=0), vertices.max(axis=0)))
        bounds_min, bounds_max = bounds
        extents = bounds_max - bounds_min
        diagonal = float(np.linalg.norm(extents))

        asset_placement_failures = placement_failures(
            asset_id,
            contract.get('pivot_profile', 'contextual'),
            bounds_min,
            bounds_max,
        )
        placement_failure_details.extend(asset_placement_failures)
        failures.extend(asset_placement_failures)

        faces = 0
        degenerate = 0
        asset_colors = set()
        for geom in geoms:
            if hasattr(geom, 'faces'):
                faces += len(geom.faces)
            if hasattr(geom, 'area_faces'):
                degenerate += int(np.sum(np.asarray(geom.area_faces) < 1e-10))
            rgb = material_rgb(geom)
            if rgb is not None:
                material_samples += 1
                asset_colors.add(rgb)
                unique_colors.add(rgb)
                if rgb != (255, 255, 255):
                    nonwhite_materials += 1

        if faces <= 0:
            failures.append(f'{asset_id}: no faces')
        if degenerate:
            failures.append(f'{asset_id}: {degenerate} degenerate faces')
        if diagonal < 0.05:
            failures.append(f'{asset_id}: implausibly small diagonal {diagonal:.4f}m')
        if diagonal > 8.0:
            failures.append(f'{asset_id}: implausibly large diagonal {diagonal:.3f}m')
        budget = PROFILE_FACE_BUDGETS.get(meta['profile'], 5000)
        if faces > budget:
            failures.append(f'{asset_id}: face count {faces} exceeds {meta["profile"]} budget {budget}')

        nodes = set(map(str, scene.graph.nodes_geometry))
        semantic_failures = semantic_contract_failures(asset_id, nodes)
        semantic_failure_details.extend(semantic_failures)
        failures.extend(semantic_failures)

        animation_set = meta['animation_set']
        if meta['profile'] == 'P_CHARACTER':
            missing_nodes = sorted(CHARACTER_NODES - nodes)
            if missing_nodes:
                failures.append(f'{asset_id}: missing character nodes {missing_nodes}')
        elif animation_set in EXPECTED_MOVING:
            prefixes = EXPECTED_MOVING[animation_set]
            if not any(any(node.startswith(prefix) for prefix in prefixes) for node in nodes):
                failures.append(f'{asset_id}: {animation_set} missing one of {prefixes}')

        signature_by_asset[asset_id] = (
            len(geoms),
            faces,
            tuple(round(float(x), 3) for x in extents),
            tuple(sorted(asset_colors)),
        )
        stats.append({
            'asset_id': asset_id,
            'profile': meta['profile'],
            'faces': faces,
            'geometries': len(geoms),
            'diagonal_m': round(diagonal, 5),
            'extents_m': [round(float(x), 5) for x in extents],
            'bounds_min_m': [round(float(x), 5) for x in bounds_min],
            'bounds_max_m': [round(float(x), 5) for x in bounds_max],
            'material_colors': len(asset_colors),
        })

    variant_failures = variant_signature_failures(signature_by_asset)
    semantic_failure_details.extend(variant_failures)
    failures.extend(variant_failures)

    if set(manifests) != set(item['asset_id'] for item in stats):
        missing = sorted(set(manifests) - set(item['asset_id'] for item in stats))
        if missing:
            failures.append(f'missing generated assets: {missing[:20]}')

    colored_ratio = (nonwhite_materials / material_samples) if material_samples else 0.0
    if material_samples == 0:
        failures.append('no PBR material samples found in generated GLBs')
    if colored_ratio < 0.60:
        failures.append(f'PBR palette collapsed toward white: nonwhite ratio={colored_ratio:.3f}')
    if len(unique_colors) < 12:
        failures.append(f'PBR palette has only {len(unique_colors)} unique RGB colors; expected at least 12')

    contextual_overrides = sorted(set(manifests) & set(CONTEXTUAL_PLACEMENT_OVERRIDES))
    report = {
        'schema': 1,
        'status': 'PASS' if not failures else 'FAIL',
        'asset_count': len(stats),
        'failure_count': len(failures),
        'failures': failures,
        'profile_contract_failure_count': len(contract_failure_details),
        'profile_contract_failures': contract_failure_details,
        'placement_failure_count': len(placement_failure_details),
        'placement_failures': placement_failure_details,
        'semantic_failure_count': len(semantic_failure_details),
        'semantic_failures': semantic_failure_details,
        'contextual_placement_override_count': len(contextual_overrides),
        'contextual_placement_overrides': {
            asset_id: CONTEXTUAL_PLACEMENT_OVERRIDES[asset_id]
            for asset_id in contextual_overrides
        },
        'interaction_anchor_bindings': anchor_bindings,
        'expected_interaction_anchor_bindings': expected_anchor_bindings,
        'max_faces': max((item['faces'] for item in stats), default=0),
        'max_diagonal_m': max((item['diagonal_m'] for item in stats), default=0),
        'material_samples': material_samples,
        'nonwhite_material_ratio': round(colored_ratio, 5),
        'unique_material_colors': len(unique_colors),
        'profile_face_budgets': PROFILE_FACE_BUDGETS,
        'animation_set_assets': len(available_animation_sets),
        'assets': stats,
    }
    validation_dir = repo_root / 'Art' / 'Validation'
    validation_dir.mkdir(parents=True, exist_ok=True)
    (validation_dir / 'generated_geometry_qc.json').write_text(
        json.dumps(report, indent=2, sort_keys=True) + '\n'
    )
    summary_path = validation_dir / 'full_generation_summary.json'
    if summary_path.is_file():
        summary = json.loads(summary_path.read_text())
        counts = summary.get('batch_counts', {})
        batch_statuses = {
            f'{i:02d}': (
                'PRODUCTION_GENERATOR_VALIDATED'
                if counts.get(f'{i:02d}') == 50
                else f'INCOMPLETE_{counts.get(f"{i:02d}", 0)}_OF_50'
            )
            for i in range(1, int(summary.get('declared_batch_count', len(counts))) + 1)
        }
        (validation_dir / 'library_release_audit_v1.md').write_text(
            release_audit_markdown(summary, report, batch_statuses)
        )
    if failures:
        raise RuntimeError('\n'.join(failures))
    print(
        f"geometry QA PASSED: {len(stats)} assets; max_faces={report['max_faces']}; "
        f"colors={report['unique_material_colors']}; nonwhite={report['nonwhite_material_ratio']}; "
        f"anchors={anchor_bindings}/{expected_anchor_bindings}; contracts=PASS; placement=PASS; semantic=PASS"
    )
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('repo_root', nargs='?', default='.')
    args = parser.parse_args()
    validate(Path(args.repo_root).resolve())


if __name__ == '__main__':
    main()
