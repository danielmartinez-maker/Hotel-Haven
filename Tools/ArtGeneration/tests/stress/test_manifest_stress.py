from .stress_support import SplitMix64, load_stress_config


def test_splitmix64_is_replayable():
    a = SplitMix64(0x123456789ABCDEF0)
    b = SplitMix64(0x123456789ABCDEF0)
    assert [a.bounded(37) for _ in range(1000)] == [b.bounded(37) for _ in range(1000)]


def test_default_scale_is_pr(monkeypatch):
    monkeypatch.delenv('HH_STRESS_SCALE', raising=False)
    monkeypatch.delenv('HH_STRESS_SEED', raising=False)
    monkeypatch.delenv('HH_STRESS_SCENARIO', raising=False)
    config = load_stress_config()
    assert config.scale == 'pr'
    assert config.seed == 0x5EED5EED5EED5EED
    assert config.scenario == ''


def test_bounded_respects_range():
    rng = SplitMix64(123)
    assert all(0 <= rng.bounded(7) < 7 for _ in range(10_000))
