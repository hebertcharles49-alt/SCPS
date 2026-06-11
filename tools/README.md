# tools/ — le calibrateur (Arc J)

La grille de paramètres remplace la main. Trois étages :

- **J1** — `scps/scps_tune.{h,c}` + `scps_tune_list.h` : les constantes de calibrage
  RUNTIME sont surchargeables par l'env `SCPS_TUNE="NOM=VAL,…"`. Sans env, défauts
  compilés, sortie byte-identique. `./chronicle --tunables` les liste.
- **J2** — `SCPS_CSV=chemin ./chronicle …` : une ligne `SUMMARY` par balayage,
  colonnes stables (cf. l'en-tête de `chronicle.c`). stdout inchangé.
- **J3** — `tools/calibrate.py` : lance la grille en parallèle (processus), même
  graines pour chaque point, score les bandes, désigne le meilleur point.

## Usage

```
tools/calibrate.py \
  --param NOM:min:max:pas [--param NOM2:…]   # 1 ou 2 → grille 1D/2D
  --target metric:lo:hi [--target …]          # bornes vides = non bornées
  --sims 3 --years 150 --seeds 7,23,42 --jobs 4
  [--yes] [--force] [--dump out.csv]
```

`./chronicle --tunables` donne les noms disponibles ; les colonnes-métriques
disponibles : `flux_or_med tresor_med ipm_final ipm_pic acc360 acc540 acc960
ratio_poursuite batailles top_event_share n_stab n_destab acharnement hegemon_cracked`.

## Trois exemples réels

**1. La chasse au flux d'or (Arc I)** — `ENTRETIEN_DIV` × marge d'import sur les bandes :
```
tools/calibrate.py \
  --param ENTRETIEN_DIV:200:600:100 \
  --param MANUF_UPKEEP_DAY:0.03:0.07:0.02 \
  --target flux_or_med:-5:20 --target tresor_med::12000 \
  --sims 3 --years 150 --seeds 7,23,42 --jobs 4
```

**2. Le ratio de la curée (H4)** — plafond de poursuite × létalité du choc :
```
tools/calibrate.py \
  --param CUREE_CAP:0.15:0.30:0.05 \
  --param BT_CHOC_MORTS:0.006:0.012:0.002 \
  --target ratio_poursuite:2:5 \
  --sims 3 --years 150 --seeds 7,23,42
```

**3. Le palier 540 ne doit pas re-casser (B3)** — en bougeant le robinet :
```
tools/calibrate.py \
  --param ENTRETIEN_DIV:200:600:200 \
  --target flux_or_med:-5:20 --target acc540:6:15 \
  --sims 3 --years 150 --seeds 7,23,42
```

`make calibrate-smoke` : la grille minimale (2 points × 1 sim × 20 ans) de bout en bout.

## Détails

- **Cache** : `tools/results.csv` (clé : mtime binaire · tune · graine · sims · années).
  Relancer ne recalcule que le neuf. `--force` refait tout.
- **Estimation** avant lancement (un run d'étalonnage 1×5 ans) ; confirmation si > 30 min
  (`--yes` pour les scripts).
- **Déterminisme** : même point, même graines → métriques identiques.
- Membrane : rien de tout ceci ne touche le viewer.
