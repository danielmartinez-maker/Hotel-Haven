from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image, ImageDraw

BG = (244, 242, 237)
BORDER = (205, 201, 194)
TEXT = (45, 45, 45)
VISUAL_DISTINCT_GROUPS = {
    'wall_clocks': ('HH_A392', 'HH_A393'),
    'exterior_planters': ('HH_A440', 'HH_A441'),
    'hedges': ('HH_A442', 'HH_A443'),
}


def rot_z(a):
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]])


def rot_x(a):
    c, s = math.cos(a), math.sin(a)
    return np.array([[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]])


VIEW = rot_x(math.radians(35.264)) @ rot_z(math.radians(45.0))
LIGHT = np.array([0.35, -0.45, 0.82])
LIGHT = LIGHT / np.linalg.norm(LIGHT)


def material_rgb(geom):
    mat = getattr(getattr(geom, 'visual', None), 'material', None)
    value = getattr(mat, 'baseColorFactor', None)
    if value is None:
        return np.array([170.0, 160.0, 145.0])
    arr = np.asarray(value, dtype=float).reshape(-1)
    if arr.size < 3:
        return np.array([170.0, 160.0, 145.0])
    if arr.max() <= 1.01:
        arr = arr * 255.0
    return np.clip(arr[:3], 0, 255)


def render_asset(path: Path, size: int = 150):
    scene = trimesh.load(path, force='scene')
    triangles = []
    points = []
    for node in scene.graph.nodes_geometry:
        transform, gname = scene.graph.get(node)
        geom = scene.geometry[gname]
        verts = trimesh.transform_points(geom.vertices, transform)
        verts = (VIEW @ verts.T).T
        points.append(verts)
        face_verts = verts[geom.faces]
        normal = np.cross(face_verts[:, 1] - face_verts[:, 0], face_verts[:, 2] - face_verts[:, 0])
        length = np.linalg.norm(normal, axis=1, keepdims=True)
        length[length == 0] = 1
        normal = normal / length
        shade = .58 + .42 * np.clip(normal @ LIGHT, 0, 1)
        base = material_rgb(geom)
        for tri, sh in zip(face_verts, shade):
            triangles.append((float(tri[:, 1].mean()), tri, np.clip(base * sh, 25, 245)))

    if not points:
        return Image.new('RGB', (size, size), BG)

    all_points = np.vstack(points)
    projected = np.column_stack([all_points[:, 0], all_points[:, 2]])
    mn, mx = projected.min(axis=0), projected.max(axis=0)
    center = (mn + mx) / 2
    scale = (size * .72) / max(float((mx - mn).max()), 1e-6)

    def project(tri):
        q = np.column_stack([tri[:, 0], tri[:, 2]])
        q = (q - center) * scale
        q[:, 0] += size / 2
        q[:, 1] = size / 2 - q[:, 1]
        return [tuple(x) for x in q]

    image = Image.new('RGB', (size, size), BG)
    draw = ImageDraw.Draw(image)
    for _, tri, color in sorted(triangles, key=lambda item: item[0]):
        draw.polygon(project(tri), fill=tuple(color.astype(int)))
    return image


def image_content_ratio(image: Image.Image) -> float:
    arr = np.asarray(image.convert('RGB'), dtype=np.int16)
    bg = np.asarray(BG, dtype=np.int16)
    changed = np.any(np.abs(arr - bg) > 3, axis=2)
    return float(changed.mean())


def preview_fingerprint(image: Image.Image) -> tuple[int, ...]:
    gray = np.asarray(image.convert('L').resize((16, 16)), dtype=np.float32)
    threshold = float(gray.mean())
    return tuple((gray > threshold).astype(np.uint8).reshape(-1).tolist())


def _hamming(left: tuple[int, ...], right: tuple[int, ...]) -> int:
    return sum(a != b for a, b in zip(left, right))


def preview_quality_failures(rendered: dict[str, Image.Image], expected_count: int = 50) -> list[str]:
    failures = []
    if len(rendered) != expected_count:
        failures.append(f'preview batch contains {len(rendered)} renders; expected {expected_count}')

    for asset_id, image in sorted(rendered.items()):
        ratio = image_content_ratio(image)
        if ratio < 0.002:
            failures.append(f'{asset_id}: blank preview content ratio {ratio:.5f}')

    fingerprints = {asset_id: preview_fingerprint(image) for asset_id, image in rendered.items()}
    for group_name, asset_ids in VISUAL_DISTINCT_GROUPS.items():
        present = [asset_id for asset_id in asset_ids if asset_id in fingerprints]
        if len(present) < 2:
            continue
        for i, left_id in enumerate(present):
            for right_id in present[i + 1:]:
                distance = _hamming(fingerprints[left_id], fingerprints[right_id])
                if distance <= 1:
                    failures.append(
                        f'{group_name}: {left_id} and {right_id} have a duplicate preview signature '
                        f'(perceptual distance {distance})'
                    )
    return failures


def names_for_batch(manifest: Path):
    data = json.loads(manifest.read_text())
    return {row[0]: row[1] for group in data['groups'] for row in group['assets']}


def contact_sheet(
    batch_dir: Path,
    manifest: Path,
    out: Path,
    cell: int = 160,
    cols: int = 10,
    expected_count: int | None = None,
):
    paths = sorted(batch_dir.glob('HH_A*.glb'))
    names = names_for_batch(manifest)
    rows = math.ceil(len(paths) / cols) if paths else 1
    label_h = 36
    sheet = Image.new('RGB', (cols * cell, rows * (cell + label_h)), BG)
    draw = ImageDraw.Draw(sheet)
    rendered = {}

    for i, path in enumerate(paths):
        x = (i % cols) * cell
        y = (i // cols) * (cell + label_h)
        image = render_asset(path, cell)
        rendered[path.stem] = image
        sheet.paste(image, (x, y))
        draw.rectangle((x, y, x + cell - 1, y + cell + label_h - 1), outline=BORDER, width=1)
        draw.text((x + 5, y + cell + 2), path.stem, fill=TEXT)
        label = names.get(path.stem, '')[:24]
        draw.text((x + 5, y + cell + 17), label, fill=TEXT)

    failures = preview_quality_failures(
        rendered,
        expected_count=len(paths) if expected_count is None else expected_count,
    )
    if failures:
        raise RuntimeError('\n'.join(failures))

    out.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(out, optimize=True)
    ratios = [image_content_ratio(image) for image in rendered.values()]
    return {
        'asset_count': len(rendered),
        'min_content_ratio': round(min(ratios), 5) if ratios else 0.0,
        'max_content_ratio': round(max(ratios), 5) if ratios else 0.0,
        'status': 'PASS',
    }


def generate(repo_root: Path):
    preview_dir = repo_root / 'Art' / 'Validation' / 'Previews'
    results = {}
    for i in range(1, 11):
        batch = f'{i:02d}'
        results[batch] = contact_sheet(
            repo_root / 'Art' / 'Exports' / f'Batch{batch}',
            repo_root / 'GameData' / 'AssetDefinitions' / 'Manifest' / f'asset_batch_{batch}.json',
            preview_dir / f'Batch{batch}.png',
            expected_count=50,
        )
    report = {
        'schema': 1,
        'status': 'PASS',
        'batch_count': len(results),
        'expected_assets_per_batch': 50,
        'batches': results,
    }
    preview_dir.mkdir(parents=True, exist_ok=True)
    (preview_dir / 'preview_qc.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('generated 10 batch preview sheets; preview QA PASSED')
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('repo_root', nargs='?', default='.')
    args = parser.parse_args()
    generate(Path(args.repo_root).resolve())


if __name__ == '__main__':
    main()
