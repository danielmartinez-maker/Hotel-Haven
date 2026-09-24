from pathlib import Path
import sys

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
sys.path.insert(0, str(ART))

from amenity_decor_factory import screen_or_board
from service_asset_factory import gym
from v2_asset_common import elevator_or_door, luggage_or_accessibility, sofa


def _floor_z(scene) -> float:
    return float(np.asarray(scene.bounds, dtype=float)[0, 2])


def test_elevator_doors_expose_required_moving_node():
    for name in (
        'Elevator Landing Doors Single',
        'Elevator Landing Doors Double',
        'Freight Elevator Gate',
    ):
        scene = elevator_or_door(name, 'MAT_STAINLESS')
        assert 'MOV_ElevatorDoor' in set(scene.graph.nodes_geometry)


def test_sofas_are_floor_supported():
    for name in ('Sofa Bed', 'Lounge Sofa Two Seat', 'Lounge Sofa Three Seat'):
        scene = sofa(name, 'MAT_UPHOLSTERY')
        assert abs(_floor_z(scene)) <= 1e-9


def test_stationary_bike_flywheel_reaches_floor():
    scene = gym('Gym Stationary Bike', 'MAT_ELECTRONICS')
    assert abs(_floor_z(scene)) <= 1e-9


def test_mobile_whiteboard_is_floor_standing():
    scene = screen_or_board('Conference Mobile Whiteboard', 'MAT_SIGNAGE')
    assert abs(_floor_z(scene)) <= 1e-9


def test_wheelchair_wheels_are_vertical_and_floor_supported():
    scene = luggage_or_accessibility('Accessibility Wheelchair', 'MAT_BLACKENED_STEEL')
    assert abs(_floor_z(scene)) <= 1e-9
    # The wheel radius, not its thin axle width, must drive vertical extent.
    bounds = np.asarray(scene.bounds, dtype=float)
    assert bounds[1, 2] - bounds[0, 2] >= 1.0
