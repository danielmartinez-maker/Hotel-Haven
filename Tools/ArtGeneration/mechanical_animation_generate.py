from __future__ import annotations

import argparse
import json
from pathlib import Path


def channel(target: str, transform: str, keyframes, *, index_sign=None):
    data = {
        'target_pattern': target,
        'space': 'local',
        'transform': transform,
        'keyframes': [{'t': float(t), 'value': value} for t, value in keyframes],
    }
    if index_sign is not None:
        data['index_sign'] = index_sign
    return data


def clip(duration, loop, *channels, events=None):
    return {
        'duration_seconds': float(duration),
        'loop': bool(loop),
        'channels': list(channels),
        'events': list(events or []),
    }


def build_sets():
    return {
        'ANSET_MECH_DOOR': {
            'skeleton_id': 'SK_MechanicalSimple',
            'clips': {
                'AN_DOOR_CLOSED': clip(0.0, False, channel('MOV_DoorLeaf*','rotation_z_degrees',[(0,0.0)], index_sign='alternate')),
                'AN_DOOR_OPEN': clip(0.55, False, channel('MOV_DoorLeaf*','rotation_z_degrees',[(0,0.0),(1,88.0)], index_sign='alternate'), events=[{'time':0.55,'name':'open_complete'}]),
                'AN_DOOR_OPEN_HOLD': clip(0.0, True, channel('MOV_DoorLeaf*','rotation_z_degrees',[(0,88.0)], index_sign='alternate')),
                'AN_DOOR_CLOSE': clip(0.50, False, channel('MOV_DoorLeaf*','rotation_z_degrees',[(0,88.0),(1,0.0)], index_sign='alternate'), events=[{'time':0.50,'name':'close_complete'}]),
            },
        },
        'ANSET_MECH_SLIDING_DOOR': {
            'skeleton_id': 'SK_MechanicalSimple',
            'clips': {
                'AN_SLIDE_CLOSED': clip(0.0, False, channel('MOV_SlidingPanel*','translation_x_m',[(0,0.0)], index_sign='alternate')),
                'AN_SLIDE_OPEN': clip(0.65, False, channel('MOV_SlidingPanel*','translation_x_m',[(0,0.0),(1,0.48)], index_sign='alternate'), events=[{'time':0.65,'name':'open_complete'}]),
                'AN_SLIDE_OPEN_HOLD': clip(0.0, True, channel('MOV_SlidingPanel*','translation_x_m',[(0,0.48)], index_sign='alternate')),
                'AN_SLIDE_CLOSE': clip(0.60, False, channel('MOV_SlidingPanel*','translation_x_m',[(0,0.48),(1,0.0)], index_sign='alternate'), events=[{'time':0.60,'name':'close_complete'}]),
            },
        },
        'ANSET_MECH_REVOLVING_DOOR': {
            'skeleton_id': 'SK_MechanicalSimple',
            'clips': {
                'AN_REVOLVE_IDLE': clip(0.0, True, channel('MOV_RevolvingLeaf*','rotation_z_degrees',[(0,0.0)])),
                'AN_REVOLVE': clip(2.80, True, channel('MOV_RevolvingLeaf*','rotation_z_degrees',[(0,0.0),(1,360.0)])),
            },
        },
        'ANSET_MECH_ELEVATOR': {
            'skeleton_id': 'SK_MechanicalSimple',
            'clips': {
                'AN_ELEVATOR_CLOSED': clip(0.0, False, channel('MOV_ElevatorDoor*','translation_x_m',[(0,0.0)], index_sign='alternate')),
                'AN_ELEVATOR_OPEN': clip(0.85, False, channel('MOV_ElevatorDoor*','translation_x_m',[(0,0.0),(1,0.38)], index_sign='alternate'), events=[{'time':0.85,'name':'open_complete'}]),
                'AN_ELEVATOR_HOLD': clip(0.0, True, channel('MOV_ElevatorDoor*','translation_x_m',[(0,0.38)], index_sign='alternate')),
                'AN_ELEVATOR_CLOSE': clip(0.80, False, channel('MOV_ElevatorDoor*','translation_x_m',[(0,0.38),(1,0.0)], index_sign='alternate'), events=[{'time':0.80,'name':'close_complete'}]),
            },
        },
        'ANSET_MECH_CURTAIN': {
            'skeleton_id': 'SK_MechanicalSimple',
            'clips': {
                'AN_CURTAIN_CLOSED': clip(0.0, False, channel('MOV_CurtainPanel*','translation_x_m',[(0,0.0)], index_sign='alternate')),
                'AN_CURTAIN_OPEN': clip(1.10, False, channel('MOV_CurtainPanel*','translation_x_m',[(0,0.0),(1,0.44)], index_sign='alternate'), events=[{'time':1.10,'name':'open_complete'}]),
                'AN_CURTAIN_OPEN_HOLD': clip(0.0, True, channel('MOV_CurtainPanel*','translation_x_m',[(0,0.44)], index_sign='alternate')),
                'AN_CURTAIN_CLOSE': clip(1.00, False, channel('MOV_CurtainPanel*','translation_x_m',[(0,0.44),(1,0.0)], index_sign='alternate'), events=[{'time':1.00,'name':'close_complete'}]),
            },
        },
        'ANSET_SERVICE_CART': {
            'skeleton_id': 'SK_ServiceProp',
            'clips': {
                'AN_CART_IDLE': clip(0.0, True, channel('MOV_Wheel*','rotation_x_degrees',[(0,0.0)])),
                'AN_CART_ROLL': clip(0.70, True, channel('MOV_Wheel*','rotation_x_degrees',[(0,0.0),(1,360.0)])),
            },
        },
    }


def sidecar(asset_id: str, asset_type: str, deps):
    return {
        'schema': 1,
        'asset_id': asset_id,
        'asset_type': asset_type,
        'source': 'Tools/ArtGeneration/mechanical_animation_generate.py',
        'units': 'meters',
        'lod_policy': 'animation_shared',
        'collision_policy': 'none',
        'material_slots': [],
        'tags': ['hotel-haven','animation','shared-mechanical'],
        'dependencies': list(deps),
        'source_revision': 1,
        'metadata_revision': 1,
        'cooker_schema': 1,
        'lifecycle_state': 'PRODUCTION',
    }


def generate(output: Path):
    sets = build_sets()
    output.mkdir(parents=True, exist_ok=True)
    clip_ids = []
    for set_id, spec in sets.items():
        for clip_id, clip_spec in spec['clips'].items():
            payload = {
                'schema': 1,
                'animation_clip_id': clip_id,
                'animation_set_id': set_id,
                'skeleton_id': spec['skeleton_id'],
                **clip_spec,
            }
            (output / f'{clip_id}.anim.json').write_text(json.dumps(payload, indent=2) + '\n')
            (output / f'{clip_id}.anim.asset.json').write_text(json.dumps(sidecar(clip_id, 'AnimationClipAsset', []), indent=2) + '\n')
            clip_ids.append(clip_id)
        set_payload = {
            'schema': 1,
            'animation_set_id': set_id,
            'skeleton_id': spec['skeleton_id'],
            'clips': list(spec['clips'].keys()),
        }
        (output / f'{set_id}.animset.json').write_text(json.dumps(set_payload, indent=2) + '\n')
        (output / f'{set_id}.animset.asset.json').write_text(json.dumps(sidecar(set_id, 'AnimationSetAsset', spec['clips'].keys()), indent=2) + '\n')
    print(f'generated {len(clip_ids)} clips and {len(sets)} animation sets')
    return len(clip_ids), len(sets)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    generate(args.output)


if __name__ == '__main__':
    main()
