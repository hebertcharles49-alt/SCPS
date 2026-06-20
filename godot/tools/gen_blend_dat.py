#!/usr/bin/env python3
"""Forge blend.dat — banc de MASQUES DE FONDU variés pour le sol iso (shader iso_blend.gdshader).

But : un masque de fondu UNIQUE répété sur chaque tuile rend la grille « évidente ». On bake ici N
variantes (rotation ISO + flips + variance d'échelle + bruit de bord, bornées au losange) ; le
renderer en pioche une par cellule → fondu sans répétition. Format compact, gzip-compressé.

Format .dat : "SBD1" (4o) + N (u8) + TW (u16 LE) + TH (u16 LE) + gzip( N × TW×TH octets L8 ).
L'atlas est VERTICAL (256 × N·128) une fois décompressé → lu direct par le shader.

Usage : python3 gen_blend_dat.py [chemin/iso_tiles]   (défaut : ../project/assets/scps/pack/iso_tiles)
"""
from PIL import Image, ImageFilter, ImageChops, ImageDraw
import gzip, struct, os, sys, random

BASE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(__file__), "..", "project", "assets", "scps", "pack", "iso_tiles")
SRC = os.path.join(BASE, "aoe2_src", "blends", "landland.png")   # le masque de fondu de référence
DAT = os.path.join(BASE, "blend.dat")
TW, TH, N, SEED = 256, 128, 16, 7

base = Image.open(SRC).convert("L").resize((TW, TH))
diam = Image.new("L", (TW, TH), 0)
ImageDraw.Draw(diam).polygon([(TW//2, 0), (TW-1, TH//2), (TW//2, TH-1), (0, TH//2)], fill=255)

def iso_rot(img, deg):                       # rotation en espace ISO (désquish y×2 → rotate → resquish)
    return img.resize((TW, TW)).rotate(deg, resample=Image.BICUBIC, fillcolor=0).resize((TW, TH))

random.seed(SEED)
variants = []
for i in range(N):
    v = iso_rot(base, random.uniform(0, 360))
    if random.random() < 0.5: v = v.transpose(Image.FLIP_LEFT_RIGHT)
    if random.random() < 0.5: v = v.transpose(Image.FLIP_TOP_BOTTOM)
    s = random.uniform(0.90, 1.10)           # légère variance d'échelle
    vi = v.resize((max(1, int(TW*s)), max(1, int(TH*s))))
    canv = Image.new("L", (TW, TH), 0)
    canv.paste(vi, ((TW - vi.width)//2, (TH - vi.height)//2))
    v = canv
    noise = Image.effect_noise((TW//8, TH//8), 80).resize((TW, TH)).filter(ImageFilter.GaussianBlur(2))
    v = ImageChops.multiply(v, noise.point(lambda p: min(255, int(210 + 0.32*p))))  # bruit de bord
    v = ImageChops.multiply(v, diam)         # borné au losange
    variants.append(v.tobytes())

payload = b"".join(variants)
blob = struct.pack("<4sBHH", b"SBD1", N, TW, TH) + gzip.compress(payload, 9)
open(DAT, "wb").write(blob)
print(f"{DAT}: {N} variantes, {len(blob)} octets (brut {len(payload)} → gzip)")
