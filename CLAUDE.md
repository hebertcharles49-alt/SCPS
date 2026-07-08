- **VAGUE CALIBRAGE POST-SWEEP — lots G/F/I (2026-07-08, le gel levé par directives joueur)** : trois
  lots sur les verdicts du giga sweep. **(G) FLUX HUMAINS** : pacte migratoire à DEUX taux —
  `MIG_PACT_FRAC` (canal pacte COMMERCIAL, gardé au taux d'origine : un pacte commercial naît AVANT
  l'an-12, le bumper casse le golden — mesuré) et `MIG_PACT_FRAC_ALLY` ×3 (golden-safe par
  construction) ; réfugiés `REFUGEE_FLEE_SCAR` 0.50→0.40 + `FLEE_FRAC` 0.03→0.04 (A/B apparié : flux ↑,
  satisfaction STABLE — le bassin de bistabilité n'a pas mordu ; 0.04 = borne TESTÉE, pas plancher) ;
  **l'IA ACHÈTE au pool servile** (can_enslave + pénurie de bras → achat borné budget/cadence — le pool
  tournait à vide, rachats méd 0). **(F) FINS & EXODE** : le biais GRAND HIVER 4:1 était un HASH FAIBLE
  sur entrées corrélées (épicentre = capitale du fauteur) → mélange avalanche (`fin_mix32`) + poids
  diégétiques ±35 % (monde froid pénalise FROID, aride gonfle RONCES) — ≈33/33/33 prouvé sur 3600
  paires ; catastrophes des MONDES CALMES (pas de fin en vue + an>200 ⇒ fréquence EVENTS[] ×
  `CALM_DISASTER_MULT` — le motif existant, aucun système neuf) ; **L'EXODE AVANT LA MORT** :
  EAU/FROID/RONCES/SANG routent une part de leur pression par la machinerie de réfugiés (API publique
  demography — le fichier appartient à G) ⇒ 6.7k-147k âmes évacuées mesurées. ⚠ DEUX BUGS LATENTS pris :
  `biome_habitability` n'avait AUCUN cas `BIO_THORNS` (les ronces plus vivables que la steppe !) et
  `endgame_region_intensity(FROID)` était PLATE (la modulation locale du commentaire jamais codée) —
  c'est ce qui bloquait tout exode. **(I) MOTEURS D'INNOVATION** : deux bugs structurels (ethos-gating —
  seuls 2/6 éthos atteignaient le sélecteur savoir ; grain — le gate lisait la capitale figée au lieu du
  site) + LA découverte : les bâtiments étaient une FAUSSE PISTE (×10 sur leur plafond ⇒ arbre
  IDENTIQUE — le bonus bâti plafonne +33 %, jamais approché) ; le vrai levier = la chaîne
  Scriptorium→Académie→Université (yield +0.5/nœud, COMPOSE sur 250 ans) jamais priorisée par le
  glouton → **SOIF DE SAVOIR** (même motif que SOIF DE PALIER/S1/S3/S4). Arbre 26.0→28.3 % sur
  échantillon réduit — la cible 35-45 % non atteinte, le sweep de re-validation juge à l'échelle.
  Gates de vague : golden RE-BASELINÉ (une fois, post-merge) · determinism STABLE · savetest
  byte-identique · test 39/40 (KO setenv pré-existant) · 0 warning · SAVE non bumpé (G/F/I dérivés).
## Disciplines non négociables

- **La membrane** : `viewer.c` n'inclut jamais `scps_core.h` et ne lit aucun flottant SCPS — des MOTS (readout) et des nombres tangibles seulement.
- **On lit des coordonnées, on n'assigne jamais de modificateur** : un effet passe par les entrées du moteur (K, P, H…), jamais par un bonus plat.
- **TROUVAILLES.md — la mémoire des agents** : tout agent (éclaireur, implémenteur,
  réparateur) APPEND en fin de mission une entrée structurée (Découvertes avec
  fichier:symbole · Pièges · Restes) dans `TROUVAILLES.md` — ce qui a coûté cher à
  TROUVER, pas ce qui a été écrit (le code et le commit racontent le reste). Le
  successeur LIT ce fichier avant de fouiller. L'orchestrateur inclut cette consigne
  dans chaque brief d'agent. (`eco_fable.md` reste le carnet de raisonnement long ;
  TROUVAILLES.md est l'index structuré.)
- **RÉSUMÉ DE HANDOFF AVANT COMPACTION (demande joueur)** : le contexte se comprime
  quand la session s'allonge. Pour NE PAS PERDRE LE FIL, tenir `SYNTHESE_SESSION.md`
  à jour comme un handoff ROULANT : l'état courant (branche, SAVE_VERSION, ce qui
  tourne en agents), ce qui vient d'être livré, ce qui RESTE, et le prochain pas
  attendu. Le rafraîchir aux jalons (fin de vague, avant de lancer une grosse
  orchestration) et **dès qu'on sent la session longue** — c'est le filet qui permet
  au « moi » d'après-compaction de reprendre sans re-fouiller. Une session = un
  fichier vivant, pas un rapport de fin.

## Langue (brief table de chaînes)

- **Aucune chaîne littérale face-joueur hors des tables.** Tout panneau, journal, preview, bande, tooltip à venir naît directement en `STR_*` :
  - la liste maîtresse vit dans `scps/strings_ids.h` (X-macro, texte FR de référence inline) ;
  - l'anglais est la liste jumelle `scps/strings_en.h` (même ordre — la complétude est vérifiée à la **compilation**, retirer une ligne casse le build) ;
  - appels : `tr(STR_X)`, plages `tr_band(STR_X_0, idx, n)`, paramètres **positionnels** `tr_fmt(buf, n, STR_X, a0, a1)` — `{0}..{9}`, l'ordre des mots n'est pas universel ;
  - pluriels : deux clés (`STR_X_UN` / `STR_X_PLUSIEURS`) là où c'est nécessaire, et seulement là.
- **Clôture** : `chronicle.c`, `econ_scan.c`, `batch.c`, `dump.c`, tout `printf` de télémétrie/journal console et les commentaires restent en **français, définitivement** (l'outillage de l'ingénieur, pas le jeu).
- **Surcharge runtime éditable** (rupture ASSUMÉE de « zéro asset ») : `scps_lang.txt` à côté du binaire **remplace** n'importe quel `STR_*` par son ID. Les défauts compilés restent (le binaire tourne sans le fichier) ; c'est **display-only** (le moteur/déterminisme n'y touchent pas) et c'est le mécanisme de **traduction** (un fichier = une langue surchargée). `scps_viewer --dump-lang` écrit le fichier éditable complet ; **F4** le recharge à chaud. Tout texte face-joueur naît donc en `STR_*` ⇒ devient éditable sans recompiler.
- `make lang-check` : le **cliquet** — échoue si le nombre de littéraux face-joueur dépasse la base (`scps/lang_baseline.txt`). Le reflux est attendu à mesure que la migration avance (abaisser la base à chaque extraction).
- État de migration : lexique readout (bandes, labels, hovers) + shell (menu, pause, slots, tutoriel) **migrés** ; le reste de `viewer.c` (panneaux, chips, zone_add) part de la base 64 et descend.

## Sauvegarde

- Format versionné (`SAVE_VERSION`), sections taguées, ChaCha20 (clé = obfuscation assumée, pas un secret) + empreinte FNV du clair.
- Toute valeur désérialisée qui borne une boucle ou indexe un tableau **se revalide** au chargement (`save_sane`) — refus net.
- Changer la taille d'une struct sérialisée ⇒ bump `SAVE_VERSION` (« ère antérieure »).
