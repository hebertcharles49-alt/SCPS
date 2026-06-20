#!/usr/bin/env python3
"""Cuit des copies de tuiles à luminance NORMALISÉE (homogénéise le contraste DANS les pixels).
Raison : le modulate canvas de Godot CLAMP COLOR à [0,1] → impossible de RELEVER les sombres au
runtime (>1 clampé). On relève donc forêt/eau et on calme sable/neige vers une cible commune, cuit
sur disque. Terre cible ~138, eau ~98, force 0.72 (garde de la variation). 100 % nos textures.
"""
from PIL import Image
import os, sys
SRC = sys.argv[1] if len(sys.argv) > 1 else "godot/project/assets/scps/pack/iso_tiles/aoe2_src/textures"
DST = sys.argv[2] if len(sys.argv) > 2 else "godot/project/assets/scps/pack/iso_tiles/norm"
os.makedirs(DST, exist_ok=True)
NAMES = ["uwtr","wt5","wtr","wt2","wt4","sha","sh2","sh3","snd","bch","grs","gr2","gr3","fc1",
         "fc2","fc3","gr6","gr5","gr4","ds2","ds3","des","ds4","for","fo2","rm1","rm2","rc1",
         "rc2","rck","rc3","sno","snf","ice"]
WATER = {"uwtr","wt5","wtr","wt2","wt4","sha","sh2","sh3"}
STR = 0.72
def load(n):
    for c in ("color","COLOR"):
        p = os.path.join(SRC, f"g_{n}_00_{c}.png")
        if os.path.exists(p): return Image.open(p).convert("RGBA")
    return None
done = 0
for n in NAMES:
    im = load(n)
    if im is None:
        print("MANQUE", n); continue
    px = im.load(); W,H = im.size
    s = 0.0; cnt = 0
    for y in range(0,H,2):
        for x in range(0,W,2):
            r,g,b,a = px[x,y]
            if a > 40: s += 0.299*r+0.587*g+0.114*b; cnt += 1
    mean = s/max(cnt,1)
    tgt = 98.0 if n in WATER else 138.0
    f = 1.0 + STR*(tgt/max(mean,1.0) - 1.0)
    f = max(0.55, min(2.4, f))
    for y in range(H):
        for x in range(W):
            r,g,b,a = px[x,y]
            if a == 0: continue
            px[x,y] = (min(255,int(r*f)), min(255,int(g*f)), min(255,int(b*f)), a)
    im.save(os.path.join(DST, f"{n}.png")); done += 1
print(f"cuit {done}/{len(NAMES)} → {DST}")
