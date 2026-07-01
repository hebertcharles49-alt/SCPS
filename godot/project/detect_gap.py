#!/usr/bin/env python3
# Détecte le « jaune entre deux zones bleues » : la rivière (bleu, au centre) DOIT être connectée
# à la mer (bleu, au bord) ; sinon il y a une coupure de terre/sable → FAIL. Pur PIL + BFS.
import sys
from PIL import Image
from collections import deque

def main(path):
    im = Image.open(path).convert('RGB'); W, H = im.size; px = im.load(); n = W*H
    blue = bytearray(n)
    for y in range(H):
        base = y*W
        for x in range(W):
            r, g, b = px[x, y]
            if b > 42 and b > r + 8 and b > g + 2:          # eau = sarcelle SOMBRE dominante (calibré sur le rendu)
                blue[base+x] = 1
    # flood depuis le BORD (la mer touche le bord) → tout le bleu connecté à la mer
    seen = bytearray(n); dq = deque()
    def push(i):
        if blue[i] and not seen[i]: seen[i] = 1; dq.append(i)
    for x in range(W): push(x); push((H-1)*W+x)
    for y in range(H): push(y*W); push(y*W+W-1)
    while dq:
        i = dq.popleft(); x = i % W; y = i // W
        if x+1 < W: push(i+1)
        if x   > 0: push(i-1)
        if y+1 < H: push(i+W)
        if y   > 0: push(i-W)
    # bleu CENTRAL (la rivière focalisée) non relié au bord = coupé de la mer
    cx0, cx1 = int(W*0.18), int(W*0.82); cy0, cy1 = int(H*0.18), int(H*0.82)
    central = 0; disc = 0
    for y in range(cy0, cy1):
        base = y*W
        for x in range(cx0, cx1):
            i = base+x
            if blue[i]:
                central += 1
                if not seen[i]: disc += 1
    tb = sum(blue); cb = sum(seen)
    print(f"[detect] central_blue={central} disconnected={disc} total_blue={tb} connected={cb} ({cb/max(1,tb)*100:.0f}%)")
    print("[detect] " + ("FAIL: rivière COUPÉE de la mer (jaune entre deux bleus)" if disc > 40
          else "PASS: rivière reliée à la mer"))
    # VISU : bleu relié à la mer = VERT ; bleu coupé = ROUGE ; reste = original assombri
    viz = im.copy(); vp = viz.load()
    for y in range(H):
        base = y*W
        for x in range(W):
            i = base+x
            if blue[i]:
                vp[x, y] = (0, 230, 0) if seen[i] else (255, 0, 0)
    viz.save(path.rsplit('.', 1)[0] + "_viz.png")

main(sys.argv[1])
