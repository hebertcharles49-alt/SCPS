# pack/map — atlas de carte (la PLACE est gardée)

Les intégrations magenta (qui bavaient) sont retirées. `uikit.gd` charge en
priorité un **PNG à alpha** (aucun keying) ; dépose simplement le fichier ici et
il s'allume — aucun code à changer.

| fichier attendu | grille | cellule | format |
|---|---|---|---|
| `settlements.png` | 6 colonnes (tier 0-5) × 6 lignes (groupe : montagne·rivière·estuaire·rural·marché·fortifié) | 96 px | PNG, **fond transparent** |
| `dressing.png` | 16 colonnes (ids MAPD) | 32 px | PNG, **fond transparent** |

En l'absence de ces PNG : les villes retombent sur des marqueurs (disque teinté
au pays, visibles en zoom fort), les arbres ne sont pas dessinés. Aucune bave.

> Repli legacy : un `.bmp` à fond magenta (`FF00FF`) est encore accepté et
> détouré (`_key_magenta`), mais le PNG à alpha est la voie propre.
