# SCPS Full Terrain Tiles

This is a terrain-tile pack, not an overlay or a sprite pack.

- Every gameplay PNG is an opaque `256x256` RGB terrain tile.
- There is no alpha, magenta key, black padding, UI frame, or isometric shape.
- `water_light/`: 6 full shallow-water variations.
- `urban_pavement/`: 6 full urban paving variations.
- `lake/`: 47 neighbour states, 2 variations each. Each tile contains ground,
  shore and lake water as needed.
- `river/`, `road_mud/`, `road_gravel/`, `road_cobble/`: 15 cardinal states,
  2 variations each. Each tile contains its surrounding ground and the route.

`SCPS_FULL_TERRAIN_TILES.json` is the selector index. For cardinal routes,
set `n/e/s/w` when that adjacent cell continues the same river or road. Lake
tiles additionally use diagonal bits; the JSON lists their values.

`REVIEW_*.png` files are inspection maps only. They are not game tiles.
