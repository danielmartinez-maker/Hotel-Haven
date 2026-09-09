# Hotel Haven OpenUSD Pipeline

This tool creates deterministic OpenUSD **authoring** stages for large Hotel Haven properties. It does not change the HMG-070 runtime boundary: source/authoring USD is transformed/cooked into Hotel Haven runtime assets before shipping.

## Design

- `hotel.usda` is a shallow root stage.
- Each hotel floor is a payload boundary under `floors/`.
- Reusable furniture/room components use referenced scenegraph instances with `instanceable = true`.
- Very high-count simple repeated props can use `UsdGeomPointInstancer`.
- A generated `hotel_haven_usd_cook_manifest.json` maps authoring references back to stable Hotel Haven runtime asset IDs.
- 1 stage unit = 1 meter and Y is up.

## Manifest

```json
{
  "schema": 1,
  "hotel_id": "property_001",
  "assets": [
    {
      "asset_id": "asset.prop.bed.king.modern_01",
      "usd_path": "Assets/bed_king_modern_01.usda",
      "runtime_asset_id": "asset.prop.bed.king.modern_01",
      "instance_mode": "scenegraph",
      "expected_instances": 250
    }
  ],
  "floors": [
    {
      "floor_id": 0,
      "instances": [
        {
          "instance_id": "room_001_bed",
          "asset_id": "asset.prop.bed.king.modern_01",
          "transform": [1, 0, 1, 0, 0, 0, 1, 1, 1]
        }
      ]
    }
  ]
}
```

Transform order is translation XYZ, rotation XYZ in degrees, scale XYZ.

## Commands

```bash
python hh_usd.py validate hotel_scene.json
python hh_usd.py generate hotel_scene.json --output-dir Art/Generated/OpenUSD/property_001
python -m unittest -v
```

Assets expected or observed more than 100 times cannot use `instance_mode: unique` unless an explicit `instancing_exempt_reason` exists, matching HMG-070's batchability rule.

On NVIDIA/Omniverse development machines, run `usdchecker` and the `omniverse-usd-performance-tuning` workflow against generated stages as an additional performance/structural gate. Baseline CI intentionally remains independent of Omniverse installation.
