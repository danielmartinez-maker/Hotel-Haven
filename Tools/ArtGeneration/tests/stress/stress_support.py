from __future__ import annotations

import os
from dataclasses import dataclass

_MASK64 = 0xFFFFFFFFFFFFFFFF
_VALID_SCALES = {'pr', 'extended', 'exhaustive'}


@dataclass(frozen=True)
class StressConfig:
    scale: str
    seed: int
    scenario: str


class SplitMix64:
    def __init__(self, seed: int):
        if not 0 <= seed <= _MASK64:
            raise ValueError('seed must fit uint64')
        self._state = seed

    def next_u64(self) -> int:
        self._state = (self._state + 0x9E3779B97F4A7C15) & _MASK64
        z = self._state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & _MASK64
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & _MASK64
        return (z ^ (z >> 31)) & _MASK64

    def bounded(self, upper: int) -> int:
        if not 0 < upper <= (1 << 64):
            raise ValueError('upper must be in [1, 2**64]')
        if upper == (1 << 64):
            return self.next_u64()
        threshold = (1 << 64) % upper
        while True:
            value = self.next_u64()
            if value >= threshold:
                return value % upper


def _parse_seed(text: str) -> int:
    if not text or text.startswith('-'):
        raise ValueError('HH_STRESS_SEED must be an unsigned integer')
    value = int(text, 0)
    if not 0 <= value <= _MASK64:
        raise ValueError('HH_STRESS_SEED must fit uint64')
    return value


def load_stress_config(default_seed: int = 0x5EED5EED5EED5EED) -> StressConfig:
    scale = os.environ.get('HH_STRESS_SCALE', '') or 'pr'
    if scale not in _VALID_SCALES:
        raise ValueError('HH_STRESS_SCALE must be pr, extended, or exhaustive')
    seed_text = os.environ.get('HH_STRESS_SEED', '')
    seed = _parse_seed(seed_text) if seed_text else default_seed
    if not 0 <= seed <= _MASK64:
        raise ValueError('default seed must fit uint64')
    return StressConfig(
        scale=scale,
        seed=seed,
        scenario=os.environ.get('HH_STRESS_SCENARIO', ''),
    )


def iterations(config: StressConfig, pr: int, extended: int, exhaustive: int) -> int:
    if min(pr, extended, exhaustive) < 0:
        raise ValueError('iteration budgets must be non-negative')
    return {'pr': pr, 'extended': extended, 'exhaustive': exhaustive}[config.scale]
