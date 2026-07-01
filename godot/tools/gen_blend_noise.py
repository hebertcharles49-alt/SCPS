#!/usr/bin/env python3
"""Forge blend_noise.png — un BRUIT fbm SEAMLESS (tileable), mon propre « stamp » de fondu.

Le shader iso_blend l'échantillonne en ESPACE ISO CONTINU (repeat) pour onduler le bord des
transitions de terrain de façon organique MAIS cohérente d'une tuile à l'autre. Valeur-bruit
multi-octave à lattice WRAP (périodes qui divisent la taille → couture invisible), smootherstep.
Rien d'externe, 100 % généré. Usage : python3 gen_blend_noise.py [chemin/iso_tiles]
"""
from PIL import Image
import random, os, sys
BASE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(__file__), "..", "project", "assets", "scps", "pack", "iso_tiles")
SIZE, OCTAVES, SEED = 256, [(4, 1), (8, 2), (16, 3), (32, 4), (64, 5)], 1337

def smoother(t): return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)

def lattice_noise(size, period, seed):
    random.seed(seed)
    g = [[random.random() for _ in range(period)] for _ in range(period)]
    out = [[0.0] * size for _ in range(size)]
    sc = period / float(size)
    for y in range(size):
        fy = y * sc; iy = int(fy) % period; iy1 = (iy + 1) % period; ty = smoother(fy - int(fy))
        for x in range(size):
            fx = x * sc; ix = int(fx) % period; ix1 = (ix + 1) % period; tx = smoother(fx - int(fx))
            a, b = g[iy][ix], g[iy][ix1]; c, d = g[iy1][ix], g[iy1][ix1]
            out[y][x] = (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty
    return out

acc = [[0.0] * SIZE for _ in range(SIZE)]; amp = 1.0; tot = 0.0
for period, sd in OCTAVES:
    n = lattice_noise(SIZE, period, SEED + sd)
    for y in range(SIZE):
        row = acc[y]; nr = n[y]
        for x in range(SIZE): row[x] += amp * nr[x]
    tot += amp; amp *= 0.5
lo = min(min(r) for r in acc); hi = max(max(r) for r in acc); rng = (hi - lo) or 1.0
img = Image.new("L", (SIZE, SIZE)); px = img.load()
for y in range(SIZE):
    for x in range(SIZE): px[x, y] = int(255 * (acc[y][x] - lo) / rng)
out = os.path.join(BASE, "blend_noise.png")
img.save(out)
print(out, f"{SIZE}x{SIZE} seamless fbm ({len(OCTAVES)} octaves)")
