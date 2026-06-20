# Sol iso — canevas continus

Dépose ici les **canevas de paysage 5×5** (champs continus de 25 tuiles 256×128), par famille :

```
iso_tiles/canevas_monde/    ISO_CANEVAS_MONDE_V01..V25.png      (sol général + côte)
iso_tiles/cote_inversee/    ISO_COTE_INVERSEE_V01..V25.png      (côte, orientation opposée)
iso_tiles/estuaire/         ISO_ESTUAIRE_V01..V25.png           (embouchure / delta)
iso_tiles/canevas_falaises/ ISO_CANEVAS_FALAISES_V01..V25.png   (rupture de relief — barrière)
```

Modèle, règle de voisinage, invariants qualité, **falaises = inhabitable** (façon AoE) :
voir **`godot/ASSETS_ISO.md`** (§3, §3b).

Tant que ce dossier est vide, le sol reste en rendu **procédural** (repli automatique).
