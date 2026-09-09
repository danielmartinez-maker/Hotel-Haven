# Hotel Haven Balance Lab

The Balance Lab analyzes deterministic headless Hotel Haven runs across five objectives: operating cost, guest dissatisfaction (`100 - guest_satisfaction`), fatigue load, excess waiting minutes, and negative GOP (`-gop_minor`). All are minimized.

It does not replace the simulation. Each input JSON is a completed simulation experiment and therefore already includes downstream effects of queues, service failures, guest memories, reviews, reputation, demand, and revenue.

## Input

```json
{
  "run_id": "staffing-a-001",
  "operating_cost_minor": 31000000,
  "guest_satisfaction": 88.5,
  "fatigue_load": 920.0,
  "excess_wait_minutes": 412.0,
  "gop_minor": 11000000
}
```

## Frontier

```bash
python balance_lab.py frontier --input-dir Build/BalanceRuns --output-dir Build/BalanceFrontier
```

Outputs `pareto_frontier.json`, `pareto_frontier.csv`, `payoff_table.json`, and `knee_candidate.json`. The knee candidate is a deterministic compromise suggestion, not an automatic balance decision.

## Epsilon-constraint sweeps

```bash
python balance_lab.py epsilon-grid --input-dir Build/BalanceRuns --output Build/BalanceFrontier/epsilon_grid.json --primary-objective operating_cost --steps 4
```

This emits deterministic bounds for repeated single-objective experiments, following the NVIDIA epsilon-constraint workflow.

## Tests

```bash
python -m unittest -v
```
