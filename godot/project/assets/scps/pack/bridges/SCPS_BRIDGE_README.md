# SCPS Modular Iso Bridge

The bridge was authored on a pure-black source canvas, then sliced into final
PNG sprites with alpha. No magenta or black background remains in the six
gameplay sprites.

Each orientation has three pieces:

`start`, then `span` repeated zero or more times, then `end`.

- `EW`: advance the next tile by `[256, 0]` pixels.
- `NS`: advance the next tile by `[0, 128]` pixels.
- Every sprite is `384x384` with a logical `256x128` terrain footprint.
- Place `[192, 192]` in the sprite over `[128, 64]`, the centre of the
  corresponding route tile. Equivalently, draw the sprite from the route
  tile's top-left offset `[-64, -128]`.

`SCPS_BRIDGE_MODULAR_INDEX.json` contains the same placement data for code.
`REVIEW_BRIDGE_EW.png` and `REVIEW_BRIDGE_NS.png` verify the repeated-span
assembly and are not gameplay sprites.
