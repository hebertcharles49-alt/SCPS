# SYNTHÈSE SESSION — 2026-09-04 : VAGUES W1/W2 LANDÉES, SWEEP 50×250 EN COURS

- **5 rapports de calibrage** (docs/CALIB_*_2026-09-03.md : influence, économie, tech, armée, population) puis **W1** (5 agents opus, worktrees) : trésor+stock NATIONAUX (908a7cd, save v108 — décision joueur « C'est voulu par toi, pas par moi »), population (invariant sièges, émergence toutes provinces), tech (ruines, exposant 0,65), influence/foi (foi au grain province, Conseil monotone, coûts diplo ×é), armée (corps au front, milice, gates, levée province). Interaction A×F isolée par 3 arbres → frein économique de levée (désertion + 35 % du revenu), jamais un cap.
- **W2** : front (stock retiré, prix diplo réels), chronicle/sweep v2, F2 refermé (âmes = strates, élite 11 %), économie (prix effondrés → PL_SINK_MONTHS ; COURT_MONTHS 60 ; matière maison facturée), décrochage 0,26, banc API réparé (pas de régression), Opus JOUEUR (docs/RAPPORT_JOUEUR_2026-09-04.md). W2-7 façade/UI en vol.
- **EN COURS** : sweep de validation 50 graines × 250 ans (sweep_valid_W1W2_50x250/, 8 jobs) → Opus data analyst → docs/RAPPORT_CORRECTIFS_SWEEP_2026-09-04.md (§1 correctifs écrit, §3 sweep à remplir).
- **DÉCISIONS JOUEUR EN ATTENTE** : Marbrive mort (3 conditions anti-corrélées), abandon de doctrine par l'IA (Divin 0), exposant tech 0,65 vs 0,78, hors-registre ~1700 or/mois/empire, P7/P10/P8 population.
- **Pièges de vague** : Opus 529 en rafale (reprendre par message, worktrees intacts) ; heredoc bash mange les backslashes (Write + python) ; « applied cleanly » ≠ sémantique fusionnée (mesurer 120 ans sur l'arbre fusionné) ; scratchpad partagé entre agents ; bash.exe -l refusé depuis un worktree (PowerShell).

# SYNTHÈSE SESSION — 2026-09-03 : SWEEP 10×200 LU, ASSIETTE SIÈGES, ANOMALIES DE GUERRE

- **Sweep 10 graines × 200 ans dépouillé** (3edb071, docs/SWEEP_DOCT_AI_2026-09-02.md,
  lecture intégrale sans filtre) : arbre neutre sur les agrégats, positif sur la
  répartition ; juges 76/75/59 % valident le score IA. Cadence « saturation an 10 »
  = artefact de l'assiette au plancher 0.25 (re-sweep après correction).
- **Doctrines FLAT** (6c48694, save v107) : zéro entretien, suspension supprimée ;
  seules les synergies paieront (fibonacci, la première gratuite). Dénominateur
  chronicle = jouables seuls (PLAYER/ANTAGONIST).
- **Télémétrie chronicle honnête** (37680a3, golden identique) : PROV vierges ≠
  colonisées sans propriétaire · doctrines actives = instantané + adoptions
  cumulées · plancher V<500 sur les hubs · « X libre » = sécession jouable.
- **ASSIETTE D'INFLUENCE = LES SIÈGES** (ddd1b1c) : le module lisait
  strata[CLASS_ELITE] (richesse, ~1-3 %) au lieu de pop_by_class (sièges, ~13 %).
  3 classes 0.002/0.0011/0.00011 (≈60/20/20 %), courants = boost de leur classe
  (Aristo 0.0025, Bourgeoisie 0.0022, Populaire 0.00022), Divin = + fidèles de la
  religion d'État / 6000 (INFLUENCE_PER_BELIEVER ; INFLUENCE_PER_FAITH purgé).
  Hover « N nobles · M bourgeois · P journaliers × le Conseil ». Kill-switch
  AI_DOCT=0 byte-identique ; golden + golden-deep re-baselinés.
- **ANOMALIES A2/A3 = MOTEUR DE GUERRE, pas les doctrines** (38523b6) : capitale
  orpheline (capital_prov=-1 avec des régions → jamais un régiment ; recalage
  sorti de son propre gate), garde de budget qui ne valait qu'en paix (342 rgt
  pour une limite ~11 → on cesse de grossir au seuil de la paix, sans
  démobiliser), affectation pop_by_class_in_army rendue au bon registre (morts
  et usure). A4/A9 faux positifs. Preuve appariée s512 0-rgt 17/23 → 1/7,
  s777 342 → 43 rgt. Golden + golden-deep re-baselinés (témoin compris).
- **Pièges de vague** : `git stash` sans pathspec dans l'arbre partagé (a avalé le
  travail des autres) ; un agent a supprimé le worktree de contrôle de
  l'orchestrateur — interdire les deux dans les briefs.
- **PROCHAIN PAS : RE-SWEEP (le joueur lance)** — `tools/sweep_doct_ai.sh`
  (SEEDS/REPS/HORIZONS en env) pour relire la cadence de l'arbre avec des
  assiettes justes, puis trancher F1/F4/F5 (marches de coût, plancher 0.5, actes
  multiples/an) et J1/J2 du rapport analyste.
- RESTE : 6 branches de Desseins · 20 idées-verbes · synergies de paires ·
  clés fantômes/promotions · P4 Desseins IA · corps au front absent du pool de
  recrutement (dernier canal de sur-levée) · découvert résiduel côté crédit.

# SYNTHÈSE SESSION — 2026-09-02 (suite) : P2 JOUABLE + P3-IA LANDÉES

- **Assets lots 14-16 intégrés** (bf545f6, 121/121 conformes) : 17 fonds de
  doctrine, 102 icônes d'idées, sceau d'influence ; préfixes idea/doct/
  influence au registre icon2.
- **P2 JOUABLE** (0a600fc, save v106) : moteur doctrines (17×6, slots LIBRES
  — révision joueur —, coûts/entretien linéarisés sur l'assiette de nobles
  SANS le Conseil, suspension, exclusivités, assiettes des courants,
  doctrine_key_mult sur ~50 sites, 40/82 idées câblées — verbes et clés
  fantômes consignés) + façade (cellule influence topbar après Population,
  hover revenus/dépenses, clic → panneau slots/catalogue/détail, hovers
  partout). Probes REGARDÉES + 4 correctifs post-probe (plaque d'entretien,
  icônes non-fantômes, colonne contenue, sceau réchauffé). Golden IDENTIQUE.
- **P3-IA LANDÉE** (d4f96c0) : influence générée pour tous, adoption IA par
  SCORE (14 signaux ancrés), mêmes règles que le joueur, chronicle enrichi
  (distribution + corrélations-juges), kill-switch AI_DOCT prouvé
  byte-identique, golden re-baseliné (documenté §6).
  **tools/sweep_doct_ai.sh PRÊT (36 sims appariées) — le joueur le lance.**
- **DÉCISIONS EN ATTENTE (calibrage/hygiène)** : (1) 6/28 pays IA seulement
  accumulent assez d'influence pour adopter (corrélations 75 % chez les
  adoptants — le score marche, l'économie est serrée : levier
  INFLUENCE_PER_*/DOCT_UPKEEP) ; (2) ~8400 suspensions cumulées (coussin
  AI_DOCT_RESERVE en mois d'entretien candidat) ; (3) golden_deep.txt STALE
  pré-existant (depuis 3e2d568, golden-deep hors full-test) — re-baseline =
  décision joueur.
- RESTE : 6 branches de Desseins · 20 idées-verbes · synergies de paires ·
  clés fantômes/promotions · P4 Desseins IA · sous-onglet Conseil ?

# SYNTHÈSE SESSION — 2026-09-02 : P1 EN MOTEUR (Influence + Desseins/Sol + dépose commission)

- **VAGUE P1 LANDÉE** (a1434e5 + merge abd994c, save v105) :
  - **Influence politique** (scps_influence, v104) : génération mensuelle
    joueur (0.002 × élites × rang moyen du Conseil, plancher ×1 si Conseil
    vide), le coût REMPLACE le cooldown diplo (5 verbes d'émissaire à 12 +
    plancher 30 j, fabrication CB +25, guerre gratuite), refus au drain +
    checklist, section INFL, banc 26/26, golden INTACT.
  - **Desseins** (scps_missions refondu, v105) : commission décennale
    DÉPOSÉE (tunables purgés), Âge des Héros ré-ancré sur le scellage d'un
    parachèvement (dormant IA jusqu'à P4), framework complet (détection
    mensuelle joueur, re-résolution N1, remises datées par latch d'année +
    clamp, claims sur slot fab_state libre, CMD_SEAL_DESSEIN), branche du
    SOL 8 échelons/2 voies, pivot 20 d'influence câblé
    (dessein_pivot_bind), missions_demo réécrit 50/50. Golden RE-BASELINÉ
    (la dépose seule ; framework prouvé neutre au kill-switch).
  - Gates du merge : full-test tout vert · determinism · savetest 2/2 ·
    lang-check 127/127. Trouvaille agent : conq_value inobservable d'une
    clôture (settle→make_peace le solde) — preuve Conquête lue sur rancor.
- **Prompt Codex campagne 3 committé** (docs/PROMPT_CODEX_ASSETS_LOTS_14-16.md,
  121 PNG : 17 fonds de doctrine 512×256 tiers-droit calme · 102 icônes
  d'idées 128 · influence = sceau de cire au ruban ×2) — en attente de
  livraison.
- **RESTE (vagues suivantes)** : P2 façade (page Desseins empire_window,
  influence au Conseil, retrait mission_info/country_panel) · 6 branches de
  Desseins restantes · P3 doctrines (+IA par score, entretien en influence)
  · P4 Desseins IA · intégration des lots 14-16 à livraison.

# SYNTHÈSE SESSION — 2026-09-01 : DESIGN DESSEINS/INFLUENCE/DOCTRINES + fix tribut vassal

- **Design complet du prochain grand chantier** (commits jusqu'à 27dc5ed) :
  docs/DESIGN_MISSIONS_DOCTRINES.md (le système — toutes les décisions joueur
  actées : Desseins générés 3 branches, Influence 0.002/noble × Conseil qui
  REMPLACE le cooldown diplo, 17 doctrines dont 4 courants politiques
  re-siégeant l'assiette, 6 idées séquentielles, complet = rien, synergies
  fibonacciennes 2/3/5/8, abandon libre, commission décennale DÉPOSÉE en P1,
  IA sur l'arbre en P3) + docs/DESIGN_DOCTRINES_ANNEXE.md (6 idées × 17
  doctrines chiffrées sur leviers réels par 17 agents opus, 31 synergies
  fusionnées, harmonisation §H — décisions restantes §H7, vérifs de début de
  vague §H4.4). PAS ENCORE CODÉ.
- **FIX tribut vassal conservatif** (27dc5ed) : les canaux agraire/martial de
  la contribution mûrie créaient grain/armes ex nihilo → miroir M3f porté
  (débit réel borné), banc de conservation dans diplo_demo (96/96). Golden
  INTACT par construction (contribution après l'an 13). + hygiène registre :
  IMPORT_TOLL_FRAC purgé (fantôme double), WILD_DEFECT_YEARS=0 confirmé
  décision joueur (LEVIERS.md réaligné).
- **DESIGN TERMINÉ (« termine le design »)** : §H7/H8 tranchés (clamp composé
  acté, Peuple→TIER1/2, Cénacle des lettrés renommé, repli Bourgeoise idée 5,
  vérifs code levées) + **docs/DESIGN_DESSEINS_ANNEXE.md** écrit (7 agents,
  un par branche : Sol · Mer & Comptoirs · Routes & Caravanes · Foi · Savoir ·
  Creuset · Horde — ~50 échelons, 7 pivots, harmonisation D1-D5 : canal daté
  par latch d'année, gabarits {0}, re-résolution unifiée, l'éthos fixe le
  prix des pivots jamais le mur). Trouvailles moteur à trancher (D3) : la
  porte de l'arcane MURÉE (`has_ruins_access` jamais posé — le bras faustien
  du Savoir inatteignable aujourd'hui), religion comptée à la rep-province
  seule, `CMD_ROUTE` région-grain, double définition du bassin de marché.
- **Prochain pas** : P1 du chantier (dépose commission décennale + ré-ancrage
  Âge des Héros + Influence politique + Desseins moteur joueur-seul) — le
  design est complet, plus rien à valider sauf les décisions D3.

# SYNTHÈSE SESSION — 2026-08-26 : CAMPAGNE D'ASSETS 2 INTÉGRÉE (lots 8-13)

- 200 PNG livrés/audités/intégrés (commits fb985f1 → eef433b) : chrome 9-slice
  (topbar/rail/barre droite/plaque armée — helpers draw_9slice_h/v UIKit),
  ÉCRAN TITRE (menu au tiers gauche, probe title_shot), curseurs (plume/épée-
  pillage), 74 ENCARTS TECH façon Civ 6 (lazy-load 24/74, états
  acquis/or/assombri/pulsé, bandeau tan — le noir translucide était illisible),
  icônes système (créateur de culture, religion aux noms joueur, 26 édifices,
  classes, journal 11/12, diplo, phases, âges, 12 charges héraldiques).
- PURGE FINALE : 46 PNG (icons/ morts, planches édifices 06/07, sheet11
  résiduel) — l'ancienne génération d'icônes n'existe PLUS.
- Registre unique UIKit.icon2 par préfixe (foi_/ethos_/her_/life_/struct_/
  axe_/edi_/cls_/jrn_/dip_/pha_/age_/act_/tech_ + exceptions her_charge_).
- Restes : cursor_loupe non branché, jrn_ 12e genre sans kind, res_talismans
  sans ressource moteur, libellé « Pillage » invisible (bug PRÉEXISTANT,
  tâche séparée), reflow topbar/panneaux à l'épreuve du jeu réel.

# SYNTHÈSE SESSION — 2026-08-19 : CARTE (climat, îles, compacité, routes cuites, lisibilité)

## Arc CARTE du 2026-08-19 (tout committé)
- ROUTES : 6 itérations d'overlay vectoriel (reviews DA opus) → verdict joueur « moche,
  question de stratégie » → STRATÉGIE A+B : réseau CUIT dans le parchemin (road_map
  R8 rasterisée au rebuild, peinte par iso_antique comme les rivières), overlay éteint
  derrière _route_layer_on (futur calque Commerce). Pièges : _w() rend du LOCAL déjà
  clampé px (re-clamper après = boudin ×zoom) ; les premiers rebuilds poussent un champ
  VIDE (garde steps>0) ; ferveur toponymique en tête de priorité = cartes en « Neuv- ».
- Échelle : trait 1 cellule (tampon mono-cellule, kernel 0.55, fenêtres au cœur).
- Grain naturel : main tremblée (±0.35 cellule, basse fréq) + usure par plaques.
- Lisibilité : cartouches centrés/élargis, 1 ville/région jusqu'au zoom moyen,
  toponymes déferveur-isés (fonction avant ferveur 0.8, préfixes 62 %, suffixes 38 %).
- Restes routes consignés : calque Commerce à câbler (_route_layer_on), mipmaps de
  road_map au dézoom extrême, phase des tirets marins aux jointures, rivières cobalt/
  lavis frontière/banding marin (les dominants hors périmètre selon le DA).


## Vague ÎLES (dernier commit)
- 3 étages : PLATEAU (fragment détaché, détroit dérivé de la taille, tangent à la côte,
  ~55 %/noyau, 6 essais anti-avalement ellipse-exacts) · MOYEN (chaînes 1-2 garanties,
  tête ×1.9, rejet vers l'océan) · HAUTURIER (pics fantômes francs : mer fantôme locale
  abaissée une fois le seuil passé).
- Protections structurelles : mers internes creusent AVANT l'union îlots (une Caspienne
  noyait une Sicile) ; force d'émergence PAR îlot (0.55 était codé en dur).
- Mesure 24 graines : 24/24 mondes ≥1 île, spectre écueil→22.8k cellules ; carte regardée
  (graine 219 : « l'Angleterre » provincée + satellite). Restes : poussières 1-2 cellules
  (cull à juger), fusion des noyaux multi-masses (« archipel » = 1 bloc) = chantier
  COMPACITÉ (aussi la clé des déserts continentaux).
- Appellation : trace genèse « N noyau(x) » (demandé) vs ligne « masses » (continents
  RÉELS mesurés + centroïdes pour pointer les probes).
- 2e recalibrage bancs du jour (5 bancs, agents, fixtures seules) — pièges frais dans
  TROUVAILLES.md : régions d'îlots SANS province active (rep=-1), micro-nation à grief nul.


## Ce qui vient de sortir (ce commit)
- **Vague CLIMAT** (critique worldgen du joueur) : itération climat→biomes provisoires→climat
  (recyclage forestier VIVANT via bio_prev), RAZ du fetch entre passes directionnelles,
  bonus fluvial relocalisé post-trace_rivers en retouche RIVERAINE (rayon 3, plancher
  0.58→0.45, boost×0.45), ZCIT ajoutée (gaussienne humide lat 0.06 σ0.09 ×0.22, subtrop
  0.52→0.56), histogramme des biomes instrumenté à chaque genèse.
  Mesures (100 graines) : famille sèche 23.4→30.5 %, marécages 11.6→4.3 %,
  jungle 0.00→1.81 % (96/100), désert 0.64 % (plafond GÉOMÉTRIQUE — archipels).
  Carte REGARDÉE (graines 9/205) : ceintures sèches lisibles, corridors riverains verts.
- **Curseur HUMIDITÉ à la création du monde** (décision joueur) : new_game_panel.gd —
  initialisé au TIRAGE de la graine (non touché = archétype intact), mot vivant
  aride/modéré/humide, T_NG_HUMIDITY (ui.csv). Le tuyau moteur existait déjà.
- **6 bancs recalibrés post-climat** (3 agents, fixtures SEULES, moteur intact) :
  statecraft (la fixture n'appelait jamais demography_tick — élite=0, canal grief mort
  depuis toujours), revolt (région 3→6, rep-province disparue), agency (graine 42→1,
  capitale à 1 hab + érosion M3h DEBASE_K_EROSION_RATE sans plancher), ai (graine 9→2),
  scps_api (graine 9→1 : l'archipel post-climat laminait le joueur ; + faux négatif
  structurel du delta de guerres agrégé → wt_at_war ciblé), endgame (C5 lisait le grain
  au grain RÉGION alors qu'econ_cold_refresh écrit PROVINCE — doctrine).
- Gates : 40/40 bancs · savetest 2/2 · déterminisme 5×12 ans stable · goldens re-baselinés.

## Restes consignés (non tranchés)
- Plafond géométrique du désert : compacité des archétypes « continents » (décision design).
- Démontage du bloc « IA navale frugale » inline de sim.c (routes maritimes dupliquées ai.c).
- Churn de conseil (COUNCIL_CLASS_SAT_W), drain servile (MANUMIT_INTEG), M-vague
  assèchement monétaire, fusion strata↔groups, Découvertes an 3-5, diplomatie/personnages.
- CI cloud GitHub Actions : workflow prêt, Actions à activer côté repo.

## Pièges frais (détail : TROUVAILLES.md)
- scons sous shell -l MSYS2 : exporter PROCESSOR_ARCHITECTURE=AMD64 (intelc.py KeyError).
- python-en-heredoc mange les \n — TOUJOURS Write un .py.
- « GCC » dans les processus = Gigabyte Control Center, pas le compilateur.
