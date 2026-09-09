import json
import tempfile
import unittest
from pathlib import Path

import balance_lab


RUNS = [
    {"run_id": "cheap", "operating_cost_minor": 100, "guest_satisfaction": 70, "fatigue_load": 80, "excess_wait_minutes": 40, "gop_minor": 20},
    {"run_id": "balanced", "operating_cost_minor": 130, "guest_satisfaction": 90, "fatigue_load": 35, "excess_wait_minutes": 12, "gop_minor": 45},
    {"run_id": "luxury", "operating_cost_minor": 190, "guest_satisfaction": 97, "fatigue_load": 20, "excess_wait_minutes": 3, "gop_minor": 35},
    {"run_id": "dominated", "operating_cost_minor": 200, "guest_satisfaction": 85, "fatigue_load": 90, "excess_wait_minutes": 50, "gop_minor": 10},
]


class BalanceLabTests(unittest.TestCase):
    def test_objective_vector_and_frontier_drop_dominated_run(self):
        points = [balance_lab.to_point(run) for run in RUNS]
        frontier = balance_lab.pareto_frontier(points)
        self.assertEqual([point.run_id for point in frontier], ["cheap", "balanced", "luxury"])
        balanced = next(point for point in frontier if point.run_id == "balanced")
        self.assertEqual(balanced.values["guest_dissatisfaction"], 10.0)
        self.assertEqual(balanced.values["negative_gop"], -45.0)

    def test_payoff_table_and_knee_candidate_are_deterministic(self):
        frontier = balance_lab.pareto_frontier([balance_lab.to_point(run) for run in RUNS])
        payoff = balance_lab.payoff_table(frontier)
        self.assertEqual(payoff["operating_cost"]["run_id"], "cheap")
        self.assertEqual(payoff["guest_dissatisfaction"]["run_id"], "luxury")
        self.assertEqual(payoff["negative_gop"]["run_id"], "balanced")
        self.assertEqual(balance_lab.select_knee_candidate(frontier)["run_id"], "balanced")

    def test_epsilon_grid_is_stable_and_excludes_primary_objective(self):
        frontier = balance_lab.pareto_frontier([balance_lab.to_point(run) for run in RUNS])
        grid = balance_lab.generate_epsilon_grid(frontier, primary_objective="operating_cost", steps=2)
        self.assertEqual(len(grid), 16)
        self.assertEqual(grid[0]["primary_objective"], "operating_cost")
        self.assertNotIn("operating_cost", grid[0]["epsilon_bounds"])
        self.assertEqual(sorted(grid[0]["epsilon_bounds"]), sorted(balance_lab.OBJECTIVES[1:]))

    def test_frontier_cli_writes_deterministic_json_and_csv(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            input_dir = root / "runs"
            output_dir = root / "out"
            input_dir.mkdir()
            for run in reversed(RUNS):
                (input_dir / f"{run['run_id']}.json").write_text(json.dumps(run), encoding="utf-8")
            rc = balance_lab.main(["frontier", "--input-dir", str(input_dir), "--output-dir", str(output_dir)])
            self.assertEqual(rc, 0)
            frontier = json.loads((output_dir / "pareto_frontier.json").read_text(encoding="utf-8"))
            self.assertEqual([row["run_id"] for row in frontier], ["cheap", "balanced", "luxury"])
            csv_text = (output_dir / "pareto_frontier.csv").read_text(encoding="utf-8")
            self.assertTrue(csv_text.startswith("run_id,operating_cost,guest_dissatisfaction,fatigue_load,excess_wait,negative_gop\n"))


if __name__ == "__main__":
    unittest.main()
