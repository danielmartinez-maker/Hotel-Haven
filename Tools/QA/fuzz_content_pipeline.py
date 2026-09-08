from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import random
import string
import subprocess
import tempfile


SANITIZER_MARKERS = (
    "AddressSanitizer",
    "UndefinedBehaviorSanitizer",
    "runtime error:",
    "LeakSanitizer",
    "heap-use-after-free",
    "stack-buffer-overflow",
    "heap-buffer-overflow",
)


def _random_scalar(rng: random.Random):
    choice = rng.randrange(6)
    if choice == 0:
        return None
    if choice == 1:
        return bool(rng.randrange(2))
    if choice == 2:
        return rng.randint(-(2**63), 2**63 - 1)
    if choice == 3:
        return rng.random() * 1e20
    if choice == 4:
        return "".join(rng.choice(string.printable) for _ in range(rng.randrange(0, 256)))
    return [rng.randint(-1000, 1000) for _ in range(rng.randrange(0, 32))]


def _case_payloads(count: int, seed: int) -> list[str]:
    rng = random.Random(seed)
    cases = [
        "",
        "{",
        "[]",
        "null",
        "true",
        "0",
        '"string"',
        "{" + ('"x":' * 32) + "0" + ("}" * 32),
        json.dumps({"schema": 1, "asset_id": "../escape", "asset_type": "StaticMeshAsset"}),
        json.dumps({"schema": 1, "asset_id": "A" * 100_000}),
        json.dumps({"schema": 2**63, "asset_id": "HH_A001"}),
    ]
    while len(cases) < count:
        mode = rng.randrange(5)
        if mode == 0:
            cases.append("".join(rng.choice(string.printable) for _ in range(rng.randrange(1, 1024))))
        elif mode == 1:
            obj = {"k" + str(i): _random_scalar(rng) for i in range(rng.randrange(0, 24))}
            cases.append(json.dumps(obj))
        elif mode == 2:
            obj = {
                "schema": _random_scalar(rng),
                "asset_id": _random_scalar(rng),
                "asset_type": _random_scalar(rng),
                "source": _random_scalar(rng),
                "units": _random_scalar(rng),
                "lod_policy": _random_scalar(rng),
                "collision_policy": _random_scalar(rng),
                "material_slots": _random_scalar(rng),
                "tags": _random_scalar(rng),
                "dependencies": _random_scalar(rng),
            }
            cases.append(json.dumps(obj))
        elif mode == 3:
            depth = rng.randrange(1, 64)
            value = _random_scalar(rng)
            for i in range(depth):
                value = {f"level_{i}": value}
            cases.append(json.dumps(value))
        else:
            cases.append(json.dumps([_random_scalar(rng) for _ in range(rng.randrange(0, 128))]))
    return cases[:count]


def run(asset_exe: Path, count: int, seed: int, timeout: float) -> None:
    asset_exe = asset_exe.resolve()
    if not asset_exe.exists():
        raise FileNotFoundError(asset_exe)

    cases = _case_payloads(count, seed)
    env = os.environ.copy()
    env.setdefault("ASAN_OPTIONS", "abort_on_error=1:detect_leaks=1:strict_string_checks=1")
    env.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")

    with tempfile.TemporaryDirectory(prefix="hotel_haven_fuzz_") as temp:
        temp_dir = Path(temp)
        for index, payload in enumerate(cases):
            path = temp_dir / f"case_{index:04d}.asset.json"
            path.write_text(payload, encoding="utf-8", errors="strict")
            try:
                result = subprocess.run(
                    [str(asset_exe), "validate", str(path)],
                    capture_output=True,
                    text=True,
                    errors="replace",
                    timeout=timeout,
                    env=env,
                )
            except subprocess.TimeoutExpired as exc:
                raise AssertionError(f"case {index} timed out") from exc

            combined = result.stdout + "\n" + result.stderr
            if result.returncode < 0:
                raise AssertionError(f"case {index} terminated by signal {-result.returncode}\n{combined}")
            marker = next((m for m in SANITIZER_MARKERS if m in combined), None)
            if marker is not None:
                raise AssertionError(f"case {index} triggered {marker}\n{combined}")

    print(f"fuzzed {len(cases)} malformed/hostile metadata documents with seed {seed}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("asset_exe", type=Path)
    parser.add_argument("--cases", type=int, default=300)
    parser.add_argument("--seed", type=int, default=0x48415645)
    parser.add_argument("--timeout", type=float, default=3.0)
    args = parser.parse_args()
    run(args.asset_exe, args.cases, args.seed, args.timeout)


if __name__ == "__main__":
    main()
