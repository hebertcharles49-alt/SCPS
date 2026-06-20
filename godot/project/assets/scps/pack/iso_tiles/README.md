# Sol iso — palette de tuiles + blend

- `super_biomes_01.png` — **palette** de tuiles de terrain découpées (atlas 10×10 de 256×128).
  Le renderer **PIOCHE** des tuiles de terre par cellule (variation) et les **fond (alpha)** sur le
  blend procédural ; l'eau reste au procédural ; les falaises sont assombries (barrière). On
  n'utilise **jamais** la disposition de la planche comme un stamp — ce sont des palettes-exemple.
- Familles de feature (à venir) : `canevas_monde` / `cote_inversee` / `estuaire` /
  `canevas_falaises` (overlays côte / embouchure / rupture de relief).

Modèle complet, **falaises = inhabitable** (AoE), anti-répétition : **`godot/ASSETS_ISO.md`** (§3, §3b).

Dossier sans palette ⇒ sol **procédural** seul (repli automatique).
