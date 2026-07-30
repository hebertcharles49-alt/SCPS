# SYNTHÈSE DE SESSION — handoff roulant (2026-07-31 — GRANDS FLEUVES · CHRONICLE PER-PROVINCE · VAGUE COLONISATION)

## VAGUE COLONISATION (commit suivant 44a40e1) — « le stock drive la demande »
- Décision joueur : baisse FORTE des prix (pop min 500→300 · convoi 250→150 · vivres
  0.35→0.25 · WPC IA 8→4, tunables) + grenier-commerce (FOOD_STOCK_MONTHS=6 : le déficit
  de réserve vivrière devient demande de marché, motif tools ; le grenier plein débloque
  la colonisation — econ_colony_food_ok PARTAGÉ drain/façade).
- Fix : l'UI mentait (façade 800/0.5 vs moteur 500/0.35) ; le tally ASSIETTE grain ne
  versait rien dans demand[] (pénurie invisible du commerce).
- Mesuré 60 ans : prov colonisées 31→51 (g7) · 35→46 (g9), pop monde ↑, 0 colonie-suicide.
- Gates : kill-switch prouvé (golden IDENTIQUE aux anciennes valeurs) · golden re-baseliné ·
  determinism · savetest · 40/40 bancs (scps_api_demo 243/243 — l'assertion colonize
  débloquée + 6 aval ; events_demo : fixture mtth → boucle jusqu'au-tir bornée).
- DÉCISION COLONISER RÉSOLUE (l'entrée « décision en attente » ci-dessous est CLOSE).

## ÉTAT COURANT (arbre NON commité — vague en cours de gate)
- **GRANDS FLEUVES (scps_world.c)** : le D8 brut mourait au fond de chaque cuvette →
  troncs ~50 cellules, fragmentés à chaque lac. Fix racine : routage du drainage sur
  surface REMPLIE (priority-flood+epsilon Barnes, tas binaire déterministe, popseq
  inversé = ordre topologique exact de l'accumulation) + exemption du Nil (atténuation
  aride pondérée par le flux : un grand fleuve TRAVERSE le désert) + traversée des lacs
  au tracé (« se jette dans un lac » n'est plus une embouchure — Rhône/Léman) + bruit
  epsilon par cellule (hash d'index : sans lui, les fleuves de plaine étaient des
  DIAGONALES parfaites — vu au shot, corrigé, re-validé au shot).
- **Résultat mesuré** : troncs max 291/177/308/209 (graines 7/9/42/209) vs 46-63 avant.
  Cible joueur 150-300 ATTEINTE. Rendu : delta ramifié connecté à la mer, méandres.
- **Tunables registre J** : RIVER_FILL=1 · RIVER_ARID_NIL=1. Kill-switch PROUVÉ :
  SCPS_TUNE="RIVER_FILL=0,RIVER_ARID_NIL=0" → anciens fleuves + golden IDENTIQUE.
- **iso_ground.gd** : la gravure saute les segments eau-eau (le lac fait le lien).
- **CHRONICLE ENRICHI** : par sim, ligne `FLEUVES : N tracés · tronc max X c. · Y ≥100 ·
  Z ≥150` + dump `PROV <pid> ville="…" pays=<cid> pop=<n>` par province colonisée
  (nom = toponyme posé à la colonisation, jamais re-tiré) + `PROV total N colonisée(s)`.
  sweep_analyze.py agrège (fleuves_n/tronc_max/fleuves_100/fleuves_150/prov_total).
- **Bug moteur trouvé par la vague** : endgame_pick_fauteur ne sautait pas POLITY_WILD
  → un hameau libre devenait fauteur de cataclysme (foyer non colonisé, refresh mort).
  Fix 1 ligne (scps_endgame.c). Banc ronces re-vert.

## GATES (état au moment du handoff)
- golden : re-baseliné 1× (avant jitter epsilon) — À REFAIRE après l'agent bancs
  (le jitter a re-changé les mondes) : make golden-update + determinism + smoke + savetest.
- savetest : ✓ (avant jitter — à re-passer).
- make test : 34/40 verts ; 6 bancs en RECALIBRAGE par agent sonnet (statecraft, agency,
  missions, warhost, events, scps_api — fixtures calés sur l'ancien monde ; kill-switch
  les prouve verts). L'agent est prévenu du re-changement jitter.

## GATES FINAUX (moteur figé, jitter inclus)
- golden re-baseliné ✓ (décision documentée, go joueur) · determinism ✓ · savetest ✓ ·
  kill-switch ✓ (ancien monde byte-identique, tronc 46 c.).
- run_tests full : 39/40 VERTS. 11 fixtures recalibrés par agent (statecraft, agency,
  missions + campaign, ai, structural, audit_eco — recherche dynamique du candidat au
  lieu d'index en dur ; warhost/events déjà robustes). Seul rouge : scps_api_demo
  1/237 — « une cible LÉGALE existe (scps_can_colonize) », voir DÉCISION ci-dessous.
- GIGASWEEP : ANNULÉ (la nuit est passée — décision joueur 2026-07-31).

## DÉCISION JOUEUR EN ATTENTE — le verbe COLONISER post-fleuves
Les capitales-joueur du nouveau monde plafonnent souvent à food_sat ~40-47 % ; le verbe
joueur exige ≥50 % (scps_can_colonize, seuil en dur scps_api.c:3892, le drain revalide).
Cartographie 4 graines : 9 ✗ · 7 ✗ · 3 ✗ · 42 ✓. La colonisation IA vit partout (dump
PROV). Options : abaisser le seuil · booster le food du spawn curated · statu quo.

## RESTES (hors vague)
- Overlay #10 : découpage en nœuds-calques · MultiMesh dressing.
- gen_region_names : 3 variantes de langue MORTES (name_elf/dwarf/orc sérialisées jamais
  lues) — nettoyage candidat.
- Calques overlay carte : restent (grands fleuves worldgen : ✓ FAIT cette vague).
- Commandement armée : n'existe pas moteur-side. Prospérité breakdown threading.
- Re-export zip testeurs : sur appel joueur.
