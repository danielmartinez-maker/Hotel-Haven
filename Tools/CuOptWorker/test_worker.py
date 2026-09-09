import copy
import json
import unittest

import hotel_haven_cuopt_worker as worker


def request_fixture():
    return {
        "schema": 1,
        "optimization_epoch": 7,
        "simulation_second": 1000,
        "snapshot_fingerprint": 12345,
        "bucket_minutes": 5,
        "horizon_buckets": 4,
        "lexicographic_objectives": ["critical_service", "guest_wait_lateness", "labor_travel_cost", "fatigue_zone_switch"],
        "employees": [
            {"id": 2, "available": True, "fatigue": 30, "regular_wage_minor_per_hour": 2000, "overtime_wage_minor_per_hour": 3000, "overtime": False, "needs_break": False, "available_from_bucket": 0, "available_until_bucket": 4},
            {"id": 1, "available": True, "fatigue": 20, "regular_wage_minor_per_hour": 2100, "overtime_wage_minor_per_hour": 3150, "overtime": False, "needs_break": False, "available_from_bucket": 1, "available_until_bucket": 4},
        ],
        "service_stations": [{"id": 30, "guest_impact_weight": 5, "minimum_coverage": 1, "demand_seconds_by_bucket": [300, 600, 300, 0]}],
        "station_candidates": [
            {"employee_id": 1, "station_id": 30, "service_capacity_seconds_per_bucket": 300, "zone_penalty_seconds": 0},
            {"employee_id": 2, "station_id": 30, "service_capacity_seconds_per_bucket": 300, "zone_penalty_seconds": 0},
        ],
        "tasks": [
            {"id": 11, "state": "Ready", "priority": 90, "deadline_bucket": 3, "estimated_work_seconds": 300, "guest_impact": 40, "revenue_impact": 20},
            {"id": 10, "state": "Blocked", "priority": 150, "deadline_bucket": 1, "estimated_work_seconds": 300, "guest_impact": 100, "revenue_impact": 100},
        ],
        "candidates": [
            {"employee_id": 2, "task_id": 11, "travel_seconds": 0, "effective_work_seconds": 300, "duration_buckets": 1, "fatigue_penalty_seconds": 0, "zone_penalty_seconds": 0, "task_switch_penalty_seconds": 0, "skill_bonus_seconds": 0},
            {"employee_id": 1, "task_id": 11, "travel_seconds": 0, "effective_work_seconds": 600, "duration_buckets": 2, "fatigue_penalty_seconds": 0, "zone_penalty_seconds": 0, "task_switch_penalty_seconds": 0, "skill_bonus_seconds": 0},
        ],
    }


class WorkerTests(unittest.TestCase):
    def test_validate_request_rejects_unknown_employee_reference(self):
        req = request_fixture()
        req["candidates"].append(copy.deepcopy(req["candidates"][0]))
        req["candidates"][-1]["employee_id"] = 999
        with self.assertRaisesRegex(ValueError, "unknown employee"):
            worker.validate_request(req)

    def test_decision_slots_respect_state_horizon_and_employee_availability(self):
        req = request_fixture()
        worker.validate_request(req)
        self.assertEqual(worker.build_decision_slots(req), [
            {"employee_id": 1, "task_id": 11, "start_bucket": 1, "duration_buckets": 2},
            {"employee_id": 1, "task_id": 11, "start_bucket": 2, "duration_buckets": 2},
            {"employee_id": 2, "task_id": 11, "start_bucket": 0, "duration_buckets": 1},
            {"employee_id": 2, "task_id": 11, "start_bucket": 1, "duration_buckets": 1},
            {"employee_id": 2, "task_id": 11, "start_bucket": 2, "duration_buckets": 1},
            {"employee_id": 2, "task_id": 11, "start_bucket": 3, "duration_buckets": 1},
        ])

    def test_station_slots_respect_employee_availability(self):
        req = request_fixture()
        worker.validate_request(req)
        self.assertEqual(worker.build_station_slots(req), [
            {"employee_id": 1, "station_id": 30, "bucket": 1},
            {"employee_id": 1, "station_id": 30, "bucket": 2},
            {"employee_id": 1, "station_id": 30, "bucket": 3},
            {"employee_id": 2, "station_id": 30, "bucket": 0},
            {"employee_id": 2, "station_id": 30, "bucket": 1},
            {"employee_id": 2, "station_id": 30, "bucket": 2},
            {"employee_id": 2, "station_id": 30, "bucket": 3},
        ])

    def test_normalize_response_sorts_station_coverage(self):
        req = request_fixture()
        response = worker.normalize_success_response(req, assignments=[], station_coverage=[
            {"station_id": 30, "employee_id": 2, "bucket": 2},
            {"station_id": 30, "employee_id": 1, "bucket": 1},
        ], objective_values={"critical_service": 0.0})
        self.assertEqual(response["station_coverage"], [
            {"station_id": 30, "employee_id": 1, "bucket": 1},
            {"station_id": 30, "employee_id": 2, "bucket": 2},
        ])

    def test_normalize_response_is_stable_and_preserves_snapshot_identity(self):
        req = request_fixture()
        response = worker.normalize_success_response(req, assignments=[
            {"task_id": 20, "employee_id": 2, "start_bucket": 2, "duration_buckets": 1},
            {"task_id": 11, "employee_id": 2, "start_bucket": 0, "duration_buckets": 1},
        ], objective_values={"labor_travel_cost": 7.25, "critical_service": 0.0})
        self.assertEqual(response["optimization_epoch"], 7)
        self.assertEqual(response["snapshot_fingerprint"], 12345)
        self.assertEqual([a["task_id"] for a in response["assignments"]], [11, 20])
        self.assertEqual(list(response["objective_values"]), ["critical_service", "labor_travel_cost"])
        self.assertEqual(json.dumps(response, sort_keys=True), json.dumps(response, sort_keys=True))


if __name__ == "__main__":
    unittest.main()
