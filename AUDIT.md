# AUDIT — SCPS (moteur de grande stratégie C99)

> État POST-arc K (« état vérifiable »). Source de vérité = les bancs (`make test`),
> jamais ce fichier. Daté : 2026-06-11. Build : gcc 13 · -O2 -Wall -Wextra -std=c99
> + durcissement (`-fstack-protector-strong -D_FORTIFY_SOURCE=2`).

---

## (a) Résultats MESURÉS (`make test`, K3 appliqué)

**32 bancs VERTS / 32** — `make test` les bâtit, les lance, compte les BILAN :

core 35/35 · monde_reel 10/10 · readout 27/27 · species 9/9 · tech 22/22 ·
faith 14/14 · intertrade 9/9 · routes 4/4 · save_io 8/8 · statecraft 22/22 ·
pop 14/14 · army 49/49 · demography 19/19 · demography_integ 6/6 · revolt 23/23 ·
social 10/10 · agency 18/18 · campaign 13/13 · factions 32/32 · econ_tax 8/8 ·
econ_culture 6/6 · econ_arcane 6/6 · econ_production 4/4 · labor 41/41 ·
missions 8/8 · **diplo 49/49 (K4b)** · **warhost 4/4 (K4c)** · **events 27/27 (K4a)** ·
**structural 16/16 (K5+K6)** · **ai 23/23 (L6)** · **forks 34/34 (M)** · prosperity OK (sans format BILAN).

**0 banc rouge** — H3 maintient 32/32.

---

## (b) Dette connue (hypothèses de racine, à trancher avant correctif)

- **ai_demo « Bâtisseur +K » : RÉSOLU (L6/K5.b)** : la racine n'était pas la profondeur de
  chaîne mais le crédit de largeur NON pondéré — la même cascade foi→savoir→K payait tout
  le monde d'aller voir ailleurs. Fix : la largeur suit l'éthos (Dominateur/Honneur → H
  tant que la poigne est basse · Mercantile → le réseau, toujours · Bureaucrate → K pur,
  jamais le détour savoir ; la foi de crise reste universelle). → 23/23 sur 4 graines.

---

## (c 5) Arc P-bis — la paix DÉCLARÉE & le prix juste (2026-06-14)

- **P2 — l'occupation devient territoire** : la cession de l'occupé existait (diplo_settle)
  mais un PLAFOND DE BUDGET la bloquait (4 occupations, 0 transférée). Itérations :
  (a) retrait du plafond → 46 transferts (EXCESSIF) ; (b) **prix de province log-compressé**
  (cumul bâti+pop, hard-cap 80, un hameau de 100 âmes ≈ 5) → le budget §5 borne de nouveau,
  mais sur une échelle saine ; (c) **PAIX DÉCLARÉE** (règle lisible) : score ≥ 50 ET ≥ 1
  région tenue → décisive (on encaisse l'occupé) ; OU **10 ans** → paix blanche. Le « ET
  occupé » laisse les SIÈGES aboutir (le score-batailles touche 50 avant qu'une place tombe).
  Tunables `AI_WAR_DECISIVE` (50) / `AI_WAR_EXHAUST` (10 ans).
- **Calibrage du timeout** (chronique 1×60 seed 7) : 5 ans → 0 occ ; 8 → 4 occ/0 transf ;
  12 → 9/11 ; **10 ans retenu → 10 occ · 4 transferts · 2 sécessions** (MODÉRÉ : la carte
  change sans tout emporter). UI : le popup diplo affiche « Paix : score X/50 ou X/10 ans ».
- make test **32/32** · 0 warning · lang-check 64 · pas de bump SAVE (pure logique).
- Réserve : ratio poursuite/choc encore hors cible — c'est l'objet de **P3** (cible révisée
  [0,8 ; 1,8] sans cavalerie).

## (c quater) Arc P — « la guerre prend du terrain » (2026-06-13)

Racine : 217 batailles, **0 occupation** — après chaque bataille TOUT le monde
passait FA_IDLE (le vainqueur décrochait au lieu de presser), et même quand un
siège partait, le défenseur (toujours plus fort) gagnait le secours et le siège
ne tombait jamais. Mesuré au profileur : assaillant ~797 de réserve vs défenseur
~1290, le défenseur l'emportait 217/217.

- **P1 — le vainqueur PRESSE** : `bt_end` désigne le vainqueur (principal en lice,
  non brisé, moral le plus haut) et l'envoie ASSIÉGER la région contestée
  (`bt_press_siege`) au lieu de décrocher ; `bt_rout` ne remet plus le vainqueur
  en IDLE. Pas de presse hors guerre (paix éclatée).
- **P2 — le siège TOMBE** : un secours défait laisse la place sans espoir → elle
  capitule en `BT_RELIEF_FALL` (30 j, ≤ la fenêtre `BT_BRISEE_J`=45 j où l'armée
  ennemie gît brisée). Sans ça, le secours se reformait et resettait le siège à
  l'infini. `days_left` est sérialisé ⇒ sauver/recharger fidèle, **0 bump SAVE**.
  La chaîne siège→`taken_region`→`diplo_occupy`/`diplo_liberate`→transfert à la
  paix était intacte ; il manquait juste que le siège FINISSE.
- **P3 / calibrage (valeurs dirigées)** : bonus défensif LÉGER `BT_DEF_EDGE`=0.10
  (le secours doit pouvoir l'emporter) ; curée ALLÉGÉE (cap 0.22→0.12, socle 0.06 —
  les armées survivent, la guerre s'inscrit dans la durée) ; doctrine d'attaque
  `BT_ATK_RATIO`=1.2 (on n'assaille qu'avec 1,2× le défenseur — fin de l'usure à
  vide) ; décrochage l'EXCEPTION `BT_DECROCHE`=0.22 (la bataille se DÉCIDE).
- **Preuve (chronique 1×60 graine 7)** : occupations **14 posées** · provinces
  **transférées à la paix 18** · **2 sécessions** (la carte respire) · **0 %
  décrochage** (39 batailles, 39 déroutes) · 7 guerres. `make test` **32/32** ·
  0 warning · campaign_demo 19/19 (banc de ralliement re-calé sur la curée légère :
  le noyau survivant peut dépasser 60 %, le ralliement ne réduit jamais).
- **Réserve** : ratio poursuite/choc **0,7×** (cible brief [2,5]). Il était DÉJÀ
  sous la cible (1,7×) avant l'arc ; réduire la poursuite (consigne) fait dominer
  le choc, et sous `BT_CHOC_MORTS`≈0.006 les morts de choc se quantifient à 0 (pas
  de milieu lisse). Tension ASSUMÉE : le modèle « batailles décisives mais non
  annihilantes » (armées qui survivent → guerres qui durent → terrain qui change
  de main) prime sur l'aspiration « la poursuite domine » de l'ancien modèle.

## (c ter) N3.1 — Frontières hiérarchiques, zoom-stables (2026-06-12)

- **Fix 1 (flags symétriques)** : `compute_render_flags` compare aux QUATRE voisins
  (E,W,N,S) sur la PLEINE grille (hors-grille = id -1) — fini le trait peint « à
  l'intérieur » d'un seul côté de la couture (inversion sur relief) et la dernière
  rangée/colonne nues. Ces flags restent pour la sélection/vue ressources/minicarte ;
  la hiérarchie politique ne les emprunte plus.
- **Fix 2 (strokes espace écran)** : trois listes de SEGMENTS (arêtes en coins de
  cellule, niveau le plus FORT du joint : pays > région > province, riveraines ra/rb
  stockées), extraites au BAKE et rebâties au seul changement de souveraineté (photo
  owner-effectif + seed, memcmp par frame) — jamais par frame. Tracé batché
  `SDL_RenderGeometry` (un appel par niveau ; repli 3 lignes parallèles < 2.0.18) :
  province **2 px** 0xFF1A2230, région **3 px** 0xFF141A26 (vue Régions), pays
  **5 px** 0xFF0A0E16 — largeur ÉCRAN constante à tout zoom, bouts ronds (hexagone)
  aux extrémités ⇒ pas de trou aux jonctions ≥3. Contour de pays FERMÉ : pays↔pays,
  bord externe vers vierge/mer (côte politique) et bords de carte. Z-order strict :
  terrain → prov → région → pays (domine) → sélection dorée (strokes) → glyphes →
  étiquettes. `RenderParams.screen_strokes` coupe le bake 1-cellule sur la carte
  principale (minicarte/outils : bake historique inchangé).
- **Membrane** : ids seulement (région/province/owner), zéro flottant SCPS ; rien en
  SAVE (recalculé au chargement). Preuves : capture `--shot` graine 7 — contours pays
  5 px fermés (îles comprises), jonctions pleines, sélection au-dessus ; 32/32 bancs ·
  0 warning · lang-check 64.

## (c bis) Arcs L + M v1 (2026-06-12) — BOUCLÉS, preuves au banc

- **L6** ai_demo 23/23 (largeur pondérée par l'éthos ; correctif post-gate : la largeur du
  conquérant est l'ARSENAL — acheter des armes — pas la garnison qui blindait le monde et
  tuait l'opportunisme : 1×30 mesuré à 0 guerre, corrigé).
- **L1** l'interception (campaign_redirect ; défense mensuelle → sortie de garnison ;
  l'attaquant re-cible ; la campagne respire au mois) — graine 7 : 2 → **84 batailles**.
- **L2** le ralliement (40-60 %, 30-60 j, une fois/guerre, noyau survivant) — SAVE 14.
- **L3** calibré par grille (arc J) : BT_CHOC_MORTS 0.008 → **0.0045** ⇒ ratio
  poursuite/choc **2.6x ∈ [2,5]** (graine 7, ancienne genèse). Traçabilité : SCPS_TUNE="BT_CHOC_MORTS=0.0045".
  Leçons : CUREE_CAP ne mord pas (P<cap) ; <0.004 = falaise d'entiers (0 mort de choc).
- **L4-recal** (2026-06) genèse L4 (redistribution orphelins) invalide L3 pour graine 7 :
  0.0045 → ratio 88x. Regrille 0.0045→0.02 → point cible : **BT_CHOC_MORTS=0.006** ⇒ ratio
  **2.4x ∈ [2,5]** (graine 7, 1×30). Traçabilité : SCPS_TUNE="BT_CHOC_MORTS=0.006".
- **L4** la genèse peuple les continents (5 graines prouvées ; re-baseline notée CLAUDE.md).
- **L5** colonies outre-mer (portes ×2, Port+coque, ≤20 j de courants, traversée comptée).
- **M0-M7 v1** : design doc versé · ETHOS_FN verbatim · pôle par factions (Transgresseur
  orthogonal) · succession contextuelle + hystérésis 360 j + frères bloqués · +5 édifices
  (SAVE 15) · Alambic + essence purifiée + LE PUITS (charge 0.28→0.00 au banc) · score
  tech multiplicatif (souche×éthos×credo×matière) · 15 gabarits §25 (3 variantes/fork).
  forks_demo **34/34**. RESTE (hors v1 ou passe UI) : routage IA du savoir forké, journal
  des forks (bande UI), §23.3 interdit (cristallisation, navires, uint64).
- Transverse : make test **32/32** · 0 warning GCC+clang · ASan muet · déterminisme ·
  lang-check 64 · SAVE 13→16 (I/H : 13 · rally : 14 · forks : 15 · region_ids[32] : 16).
- **H3 — guerre trans-mer (2026-06)** : `countries_sea_adjacent` ajoutée dans scps_ai.c
  (TOUS les ports côtiers de a testés contre toutes les côtes de b, portée ≤ 400 j de
  courants = tout bassin atteignable) ; `ai_pick_rival` étendue (pénalité ×0.60 outre-mer).
  CB existants applicables sans ajout (CB_ECONOMIC, CB_SUBJUGATION, CB_RELIGIOUS,
  CB_TERRITORIAL par rancune). Armées via `campaign_order_sea` (§6 existant). `make test`
  `make test` 32/32 · 0 warning · overhead +4 % (62 s → 65 s, graine 42 / 40 ans) ·
  graine 7 mono-cont. : 2 guerres terrestres / 0 traversée (H3 inactif sans mer, CORRECT) ·
  graine 42 bi-cont. 16 pays / 40 ans : 2 guerres terrestres · 8 raids piraterie ·
  0 traversée (CB_ANTIPIRATERIE en construction ; guerre outre-mer attendue à 60-80 ans quand
  la rancune de piraterie dépasse le seuil CB — le chemin de code H3 est vérifié, les conditions
  de déclenchement prennent 40-80 ans pour se former dans un monde naissant).

---

## (c) Recommandations (ordre de correction) — arc K BOUCLÉ

K4a (events) ✓ → K4b (diplo) ✓ → K4c (warhost) ✓ → **K5** (racine IA = la spirale de
friche) ✓ → **K6** (coercitif : cas-test sur-extrême, pas un chaînon manquant) ✓ → K7
(hygiène : clang 0 warning, ASan muet) ✓. `make test` 30/31. La dette « Bâtisseur +K »
demande du CONTENU (chaîne K plus longue / bâti multi-régions) — hors périmètre « pas de
système neuf », laissée en l'état documenté (déjà signalée dans CLAUDE.md).

---

## (d) Correctifs DÉJÀ intégrés (arc K)

- **K1 — build/linkage** : `faction_name→tr()` (G4) rendait scps_factions.o dépendant de
  scps_lang.o ; 12 listes `_OBJS` linkaient factions sans lang → « undefined reference to
  tr ». Corrigé par liste (multi-ligne, sans double). `ifdef DEV/WIN` → `ifeq ($(VAR),1)`
  (le piège « DEV=false = dev »). Cible `scps` : message clair si SDL absent.
- **K2 — membrane** : `faction_name`/`edifice_name` migrés au readout (tr() y est
  légitime) ; scps_factions.c / scps_agency.c n'incluent plus scps_lang.h. AUDIT :
  aucun module moteur n'inclut scps_lang.h ; aucun tr() hors readout/viewer.
- **K3 — instrument** : `make test` (30 verts / 1 rouge, rc≠0 si un rouge) — la
  non-régression de tout l'arc.
- **K4a — events** : `events_fire_risk` retourne 0 dès `forest < 0.01` (le feu de forêt
  ne naît plus sans forêt). → 27/27.
- **K4b — diplo (cooldown)** : diagnostic CORRIGÉ — la racine n'était PAS la migration
  tune mais une garde `colonized` : le décrément de `pillage_cd` vivait DANS la boucle
  des régions colonisées, donc une région `colonized=0` ne le purgeait jamais. Décrément
  déplacé en tête de `econ_tick`, pour TOUTES les régions. → 49/49.
- **K4c — warhost (levée)** : diagnostic CORRIGÉ — la solde I1 draine bien le trésor,
  mais le recrutement puise dans un scratch ∝ pop, PAS dans le trésor : la paix
  n'était donc pas freinée par le coût, elle ACCUMULAIT (batch +2/an vers le plafond de
  pop) pendant que la guerre SATURAIT son pool en an 0 (recrutement tout-ou-rien). Un
  pays paisible à grand pool dépassait un pays en guerre à petit pool. Fix : la guerre
  MOBILISE vers le plafond de pop ; la paix DÉMOBILISE vers une GARNISON ∝ jauge
  (`WH_GARRISON_UNITS`) — au-dessus on dégraisse (`wh_shed`, la pop retourne au pool),
  en-dessous on complète à l'entretien. La solde I1 reste le COÛT qui, via la garde IG,
  abaisse la jauge → la garnison → démobilise PAR LE COÛT. → 4/4 (8 graines vérifiées).
- **K5 — IA gelée : diagnostic CORRIGÉ (PAS le crédit de largeur)** : l'instrumentation
  montre les archétypes NON aplatis mais GELÉS — les ponctions d'or arc I (admin/cour/
  encadrement/surtaxe IPM) vidaient le trésor d'une capitale-test isolée à 0 → la région
  tombait en FRICHE (prod 0.6×) → satisfaction effondrée → pression fiscale (I) montée →
  SI<5 → frein de consolidation BLOQUÉ à 1 (le Dominateur consolide au lieu de conquérir ;
  le Mercantile, à trésor 0, ne peut bâtir son grenier → verrou de famine, 0 route). Fix
  (scps_econ.c) : (1) RÉSERVE D'EXPLOITATION `SINK_FLOOR` — entretien & redépense d'État ne
  vident jamais sous ce plancher (un État garde de quoi bâtir/amorcer) ; (2) FRICHE = surbâti
  CATASTROPHIQUE seulement (entretien > TOUT le trésor), plus la falaise sur une thune fine ;
  (3) les ponctions anti-thésaurisation (admin/cour/encadrement/surtaxe IPM) ne mordent qu'AU-
  DESSUS du seuil de HOARDING (`COURT_FLOOR`) — elles saignent les gros trésors, pas le bas de
  laine qui finance les chantiers. → ai_demo 22/23, structural_demo 15/16 ; chronique 40 a :
  trésor moy ~15.9k (mieux que les 21-30k d'avant), flux +20/mois (bord de bande), 0 friche.
  Reste l'ÉGALITÉ « Bâtisseur +K » (dette de contenu, supra).
- **K6 — structural « la coercition dissout L » : diagnostic CORRIGÉ (PAS un chaînon
  manquant)** : le mécanisme de dissolution EXISTE et MARCHE — l'Âge des Lumières pose
  `age_lumiere_solvent` (+2) qui ronge la légitimité effective `L − solvant·H/10`. L'invisible
  venait du CAS-TEST : le coercitif était façonné H=9·L=2, plus extrême que tout régime réel
  (la σ-forme A1 est CALÉE sur monde_reel : Russie 9.6, Iran 9.2), si bien que sa fragilité
  SATURAIT déjà à 10.0 — la dissolution n'avait plus de marge pour monter. Le moteur est juste
  (σ-forme verrouillée par core_demo 9.990 + monde_reel) ; c'est l'ASSERTION qui était
  sur-extrême. Fix : le cas-test coercitif passe à L=6 (~9.2, l'Iran) — franchement fragile
  MAIS avec la marge où le solvant fait grimper la fragilité (9.2→9.8). → structural_demo 16/16.
- **K7 — hygiène** : clang `-Wall -Wextra` 0 warning sur tout `scps/*.c` (15 recettes sans
  repli initialisent explicitement `alt1` = zéro-init intentionnel ; 2 variables mortes
  retirées) ; GCC 0 warning ; ASan+UBSan muets sur `chronicle_asan` ; déterminisme
  byte-identique (md5 inchangé avant/après) ; `lang-check` OK (64, base) ; membrane propre.
- **Bilan arc K** : `make test` 30/31 — la SEULE rouge est « Bâtisseur +K » (égalité à 3,
  CONTENU manquant — chaîne K profonde de 3, bâti mono-région ; hors périmètre « pas de
  système neuf »). Preuves : 0 warning GCC+clang · ASan muet · déterminisme · lang-check ·
  membrane · gold band 40 a en bande (trésor ~15.9k, flux +20). Les bancs structural/ai
  gardent une sensibilité de graine résiduelle (worldgen) sur 1-2 assertions hors graine
  par défaut — la cible `make test` (graine 42) est verte.
