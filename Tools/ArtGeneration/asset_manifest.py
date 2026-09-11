from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator


@dataclass(frozen=True)
class BatchEntry:
    batch: int
    path: str
    asset_count: int


class AssetManifest:
    """Repository-scoped gameplay asset manifest.

    The master manifest is authoritative for batch enumeration. Directory
    contents are deliberately ignored so stale or experimental batch shards
    cannot silently enter generation, validation, or release counts.
    """

    def __init__(self, repo_root: Path | str, manifest_path: Path | str):
        self.repo_root = Path(repo_root).resolve()
        path = Path(manifest_path)
        self.path = path if path.is_absolute() else (self.repo_root / path)
        self.path = self.path.resolve()
        self.data = json.loads(self.path.read_text(encoding='utf-8'))

        raw_batches = self.data.get('batches', [])
        self._batches = tuple(
            BatchEntry(int(entry['batch']), str(entry['path']), int(entry['asset_count']))
            for entry in raw_batches
        )
        numbers = tuple(entry.batch for entry in self._batches)
        if len(numbers) != len(set(numbers)):
            raise ValueError(f'duplicate batch numbers in {self.path}')
        if numbers != tuple(sorted(numbers)):
            raise ValueError(f'batches must be sorted in {self.path}')

    @property
    def asset_count(self) -> int:
        return int(self.data['asset_count'])

    @property
    def batch_numbers(self) -> tuple[int, ...]:
        return tuple(entry.batch for entry in self._batches)

    @property
    def batch_entries(self) -> tuple[BatchEntry, ...]:
        return self._batches

    @property
    def profiles(self) -> dict[str, dict]:
        return dict(self.data['profiles'])

    @property
    def quality_contract_path(self) -> Path | None:
        value = self.data.get('quality_contract')
        return (self.repo_root / value).resolve() if value else None

    def batch_path(self, batch: int) -> Path:
        for entry in self._batches:
            if entry.batch == batch:
                return (self.repo_root / entry.path).resolve()
        raise KeyError(f'batch {batch} is not declared by {self.path.name}')

    def iter_batch_paths(self) -> Iterator[Path]:
        for entry in self._batches:
            yield (self.repo_root / entry.path).resolve()

    def iter_rows(self) -> Iterator[tuple[str, str, list]]:
        """Yield (family, batch_path, row) tuples from declared shards only."""
        for path in self.iter_batch_paths():
            data = json.loads(path.read_text(encoding='utf-8'))
            for group in data['groups']:
                family = str(group['family'])
                for row in group['assets']:
                    yield family, str(path), row


def load_active_manifest(repo_root: Path | str) -> AssetManifest:
    root = Path(repo_root).resolve()
    definitions = root / 'GameData' / 'AssetDefinitions'
    for filename in ('hotel_haven_asset_manifest_v2.json', 'hotel_haven_asset_manifest_v1.json'):
        candidate = definitions / filename
        if candidate.is_file():
            return AssetManifest(root, candidate)
    raise FileNotFoundError(f'no Hotel Haven gameplay asset manifest found below {definitions}')
