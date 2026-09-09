from __future__ import annotations

import importlib.util
import json
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[3]
ART = ROOT / 'Tools' / 'ArtGeneration'
sys.path.insert(0, str(ART))


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


batch05 = load_module('batch05_completion', ART / 'batch05_generate.py')
batch06 = load_module('batch06_completion', ART / 'batch06_generate.py')
batch07 = load_module('batch07_completion', ART / 'batch07_generate.py')
factory = load_module('service_factory_completion', ART / 'service_asset_factory.py')


def node_names(scene):
    return {str(node) for node in scene.graph.nodes_geometry}


def test_batch05_and_service_factory_preserve_fractional_pbr_colors():
    for material in (
        batch05.material('MAT_WOOD_WARM'),
        factory.material('MAT_SERVICE_PAINT'),
    ):
        rgba = np.asarray(material.baseColorFactor)
        assert rgba.shape == (4,)
        assert not np.all(rgba[:3] == 255), rgba
        assert np.all(rgba[:3] < 240), rgba


def test_animated_batches_write_direct_animation_dependencies(tmp_path):
    direct = batch05.sidecar(
        'HH_A211', 'general', 'MAT_BRASS_POLISHED',
        'P_INTERACTIVE_ANIMATED', 'ANSET_SERVICE_CART'
    )
    assert direct['dependencies'] == ['ANSET_SERVICE_CART']

    for batch, generator, asset_id, name in (
        (6, batch06.generate_package, 'HH_A291', 'Housekeeping Cart Standard'),
        (7, batch07.generate_package, 'HH_A320', 'Utility Cart Flatbed'),
    ):
        manifest = {
            'schema': 1,
            'batch': batch,
            'asset_count': 1,
            'groups': [{
                'family': 'test',
                'assets': [[
                    asset_id, name, 'general', 'MAT_SERVICE_PAINT',
                    'P_SERVICE_PROP_ANIMATED', 'ANSET_SERVICE_CART', ['INT_PUSH_01']
                ]],
            }],
        }
        manifest_path = tmp_path / f'batch_{batch}.json'
        manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
        [glb] = generator(manifest_path, tmp_path / f'b{batch}')
        sidecar = json.loads(glb.with_suffix('.asset.json').read_text(encoding='utf-8'))
        assert sidecar['dependencies'] == ['ANSET_SERVICE_CART']


def test_batch05_kiosks_have_role_specific_readable_hardware():
    self_check = node_names(batch05.kiosk('Self Check In Kiosk'))
    information = node_names(batch05.kiosk('Lobby Information Kiosk'))
    assert {'CardReader', 'ReceiptSlot'} <= self_check
    assert {'MapPanel', 'InfoBeacon'} <= information
    assert self_check != information


def test_batch06_host_and_kitchen_assets_have_role_specific_silhouettes():
    host = node_names(factory.build_asset('Restaurant Host Stand', 'MAT_WOOD_WARM'))
    fryer = node_names(factory.build_asset('Kitchen Fryer', 'MAT_STAINLESS'))
    oven = node_names(factory.build_asset('Kitchen Oven', 'MAT_STAINLESS'))
    dishwasher = node_names(factory.build_asset('Dishwasher Commercial', 'MAT_STAINLESS'))

    assert {'HostTop', 'ReservationScreen'} <= host
    assert {'FryerBasket_0', 'FryerBasket_1'} <= fryer
    assert 'OvenHandle' in oven
    assert 'DishwasherHandle' in dishwasher
    assert oven != dishwasher


def test_batch07_laundry_and_gym_assets_are_functionally_distinct():
    washer = node_names(factory.build_asset('Laundry Washer Commercial', 'MAT_SERVICE_PAINT'))
    dryer = node_names(factory.build_asset('Laundry Dryer Commercial', 'MAT_SERVICE_PAINT'))
    bike = node_names(factory.build_asset('Stationary Bike', 'MAT_BLACKENED_STEEL'))
    elliptical = node_names(factory.build_asset('Elliptical Trainer', 'MAT_BLACKENED_STEEL'))

    assert 'DetergentDrawer' in washer
    assert 'VentGrille' in dryer
    assert washer != dryer
    assert {'Pedal_Left', 'Pedal_Right', 'Handlebar'} <= bike
    assert {'Pedal_Left', 'Pedal_Right'} <= elliptical


def test_batch07_service_cart_wheel_contract_matches_vehicle_type():
    cart = node_names(factory.build_asset('Utility Cart Flatbed', 'MAT_SERVICE_PAINT'))
    hand_truck = node_names(factory.build_asset('Hand Truck', 'MAT_STAINLESS'))
    assert sum(name.startswith('MOV_Wheel_') for name in cart) == 4
    assert sum(name.startswith('MOV_Wheel_') for name in hand_truck) == 2
