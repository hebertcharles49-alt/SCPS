# SYNTHÈSE DE SESSION — handoff roulant (2026-07-11 soir)

## ÉTAT COURANT — CALIBRAGE FINS + DÉCLENCHEURS D'ÂGES (task #82, ✅ LIVRÉ)
- Branche `claude/vibrant-euler-1tgfp3` (== main). Commit poussé + mergé main : **58874ee**
  (calibrage fins/âges). SAVE_VERSION **78** (inchangé). scps.exe ré-exporté 10:43.
- RÉSULTAT : fins EAU 3·RONCES 4·HIVER 5·RÉCHAUF 5·SANG 3·AUCUNE 0 (était 24/200) ;
  âges Soulèvements 70 %·Tyrans 15 %·ni-ni 15 % (Tyrans était 0/200). Gates verts (39/39
  runnable, determinism STABLE, savetest byte-identique, golden re-baseliné 7/310/411).
- (archive — historique de la mission ci-dessous)

## task #79 UI Lots 3-4 — LIVRÉ (display-only Godot, golden/save intacts)
- Probe shot_ui paramétrée en résolution (res=WxH → shots_ui/<WxH>/) + clear du drapeau de
  crash user:// (shots propres). VÉRIFIÉ à 1280×720, 1600×900, 1920×1080 : les 25 états
  rendent, 0 erreur (§3.4 = le gate, PASSÉ).
- 3.1 TOPBAR : 8 permanents (trésor · +net vert · pop · nourriture+rupture · recherche ·
  Influence · Corruption · date/vitesse), le reste démoté en TOOLTIPS. « Engager : L'Âge de X »
  (nom complet). 3.2 RAIL : 4 états (normal/hover/sélect FORT/indispo) + gating game_on +
  raccourcis. 4.4 : créateur héritage/éthos en CARTES · codex recherche+catégories repliables+
  sommaire · newgame aperçu monde+taille expliquée · annales hauteur adaptée+glyphes.
- ⚠ 1 bug agent pris par la probe : codex.gd `var nmc := nm` (inférence cassée) → codex Nil →
  crash au shot 18. Corrigé (`: String`). Le reste avait prouvé son parse EN RENDANT.
- La « discrepance » du baseline était un shot PÉRIMÉ (la parenthèse/Engager/Pause étaient déjà
  corrigés en source ; l'agent A avait raison). 
- RESTE (honnête) : 4.1 matières / 4.2 fog / 4.3 hiérarchie typo — le P2 le plus SUBJECTIF,
  theme-wide, NON fait (risque de déstabiliser les layouts fraîchement vérifiés). À proposer au
  joueur comme vague de finition séparée. Le chip « Pause » flotte encore un peu (mineur).
- DIRECTIVE JOUEUR : « lisse les déclencheurs et trouve pourquoi certaines fins ne viennent
  jamais. Trouve, itère, corrige. » — les problèmes flaggés au gigasweep : Tyrans 0/200,
  SANG 0/200, EAU 11/200, RÉCHAUF 2/24-aucune.
- ARBRE NON COMMITÉ (travail d'un agent Fable mort à sa limite, repris par moi/Opus) :
  scps_endgame.c (loterie unifiée EAU/RONCES/FROID = plancher climat + part de production →
  EAU n'est plus math. exclue ; findiag gated SCPS_FINDIAG) · scps_tune_list.h (ENDGAME_BLOOD_FRAC
  0.20→0.09 · FUEL_FALLBACK_MIN 4→2 · FIN_BASE_* + FIN_PROD_W_* neufs · AGE_SOULEVEMENTS_MIN_COUNTRIES
  2→8 · AGE_TYRANS_FRACTURE 3.0→0.30 · AGE_TYRANS_SI 5.0→8.5) · scps_events.c (diag SCPS_AGEDIAG)
  · endgame_demo.c + structural_demo.c (recalés, intention préservée — structural §3 « masse
  critique » lit le seuil réel). J'ai corrigé l'invariant FIN_BASE_EAU (call-site 3.0→1.5 = registre).
- VALIDATION FINS (10 graines × 2 sims) : **EAU=3 RONCES=4 HIVER=5 RÉCHAUF=5 SANG=3 AUCUNE=0**
  → toutes les fins tirent, AUCUNE=0 (était 24/200), ratio naturel 1,67:1 ≤3:1. ✓ RÉSOLU.
- EN COURS : les ÂGES. Tyrans 3/20 (RÉSOLU, était 0/200) MAIS Soulèvements 0/20 (MIN=8 a
  déplacé le verrou au lieu de créer un embranchement). Mesure : revolutionnaires pic 3-5
  (mode 3), ≥8 sporadique/tardif → MIN=8 rate la vague de révolte précoce. ITÉRATION EN VOL :
  sweep MIN∈{5,6,7} (script gate_agesplit.sh) pour trouver le split Soulèvements/Tyrans.
- RESTE : choisir MIN → appliquer registre → gates (make test, golden-update, determinism,
  savetest) → TROUVAILLES.md append → commit/push/merge → export scps.exe. Task #79 (UI Lots
  3-4) reste en attente après.

---
# (archive) SYNTHÈSE 2026-07-10 — VAGUE multi-agents

## MISE À JOUR fin d'après-midi — VAGUE CONSEIL + UI-2/3/4 EN VOL
- COMMITÉ/MERGÉ/EXPORTÉ à 3c7e8b0 : toute la vague du matin (traditions↔registre J,
  équilibrage culture/foi, éditorial tech+events, kit, UI-1/5, fixes probes). scps.exe 15:34.
- SPEC NEUVE : docs/CONSEIL_ORIENTATIONS_2026-07-10.md (Conseil 3 sièges × 6 factions,
  efficacité K/loyauté/Corruption, coûts % revenu×IPM, mission décennale au siège, hooks
  d'événements dynamiques, 9 orientations légères, Affranchissement + Audit). ⚠ Elle TRANCHE :
  identités de conseiller = NARRATIVES PURES (effet 0) — corrigé dans le brief de l'agent façade.
- AGENTS EN VOL (4) : façade (action_preview + country_shortages + identités narratives —
  repris post-limite avec correction) · UI-2 topbar 4 blocs (repris) · UI-3/4 zone contextuelle
  + hiérarchie d'actions (repris, autorisé à réparer la sélection foe de shot_ui) ·
  CONSEIL-MOTEUR C1 (P0+P1+P3 : 6 factions × 3 sièges, personne+maison, efficacité, coûts
  % revenu, renvoi=rancœur congédiée, mission décennale au siège — statecraft/factions/
  missions/tune_list, INTERDIT api.c/events.c/godot).
- CHAÎNE APRÈS C1 : task #74 P2 hooks événements (events.c) · #75 P4 orientations légères
  (decrees + lecteurs par-pays « tune_f × mult du pays », JAMAIS tune_set global) · #76 cartes
  UI Conseil (après façade + UI-2 qui tient sidebar_drawer + C1). Puis gates complets,
  re-baseline golden (le conseil IA mord), probe FENÊTRÉE, commit, export.
- ⚠ Session limit API frappe par vagues (2 fois aujourd'hui, reset horaire) : les agents morts
  se RESSUSCITENT par SendMessage (transcript conservé), le travail partiel reste dans l'arbre.


> Branche `claude/vibrant-euler-1tgfp3` (== main, sync au commit 4798331/a7a2c21) ·
> **SAVE_VERSION 73** · Thread du jour : DEUX GROS DOCUMENTS JOUEUR (audit UI +
> passe éditoriale tech/events + équilibrage culture/foi) exécutés en hiérarchie
> multi-agents. ⚠ ARBRE NON COMMITÉ, très chargé — voir « État à la reprise ».

## ÉTAT À LA REPRISE (si compaction) — 2026-07-10 après-midi
- **Specs persistées** : docs/RETOURS_2026-07-10.md (verdict UI 5 points + éditorial
  tech/events) · docs/EQUILIBRAGE_CULTURE_FOI_2026-07-10.md (éthos/héritages/traditions/foi).
- **AGENTS LIVRÉS (diffs NON commités dans l'arbre)** :
  1. KIT DE DÉPART (scps_econ.c + scps_tune_list.h : SPAWN_KIT_* ×9, dépôt capitale
     de chaque empire à la genèse, jurisprudence CS_TRADE_POOL).
  2. CRÉATEUR 4 onglets (culture_creator.gd : sections plates par rang + grisage d'axe,
     onglet Identité blason+nom, en-têtes renommés) + shot_ui.gd (capture 01e).
  3. TRADITIONS↔REGISTRE J (le gros) : 9 leviers HeritageLeviers câblés sur des formules
     rang-registre — 7 tunables TRAD_*_W (REND 1.0 · INFL 10 · CAP 0.30 · PERM 3.0 ·
     ARCANE 0.25 · DERIVE 1.0 · FRACT 0.06). Fichiers : econ/statecraft/demography/
     revolt/ai(+h)/sim/api/heritage/tune_list. Découvertes : `langue` = horloge morte ;
     `rendement` ne touchait pas la prod. Mesuré : pop +15-25 %, révoltes 18→13,
     hégémon 3/3. ARCANE désormais = coût branche faustienne ±25 %/pt (plus une
     pénalité pure — réconciliation faite dans le brief équilibrage).
  4. ÉDITORIAL TECH (scps_tech.c seul) : eff% retirés du trio Scriptorium/Académie/
     Université, charges civiles cachées→0 (Outillage/Wards/Fortifs/Armurerie),
     +NODE_PROD_PCT (Taille de précision 5 · Automates 8 · Alchimie 5), Scrying dK+0.5,
     7 renommages, hovers honnêtes (« production globale »), 74/74 flavors (champ
     existait déjà).
- **AGENTS EN VOL** : ÉVÉNEMENTS (ouvertures+flavors par option, effets INCHANGÉS,
  scps_events.c) · FOI (canaux morts des 16 pôles/3 crédos remappés K/P/H/L/démo,
  scps_religion.c) · TRADITIONS-ÉQUILIBRAGE (36 deltas cibles unité commune,
  scps_heritage.c, Arcanique/Sourd GARDENT arcane±1) · ÉTHOS+HÉRITAGES (tolérances
  fiscales, Bureaucrate dégonflé, Pacifiste relevé, esclavage re-gaté TECH = revert
  partiel brassage ASSUMÉ joueur, signatures normalisées 0.60).
- **FAIT DE MA MAIN (non commité)** : tooltips cascade FIXÉE (grâce de régression 0.3 s
  — le niveau 2 sautait à la frame de naissance) · politique hover (défs SEULEMENT via
  mots turquoise, plus de leçon dans le corps, footer méta retiré) · double hover natif
  TUÉ (project.godot [gui] tooltip_delay 100000) · F1-F8 dans les hovers du rail ·
  redraw overlay/sol au pan/zoom (map_view._nav_redraw) · ledger droit DÉCOUPÉ au
  contenu + 288px (Frame.LEDGER_W partagé alerts/tech_panel/topbar) · chips alertes
  280px/42 chars · tiroir 380px + Conseil aéré · UI-1 : échelle 100/125/150 %
  (options_panel + stretch canvas_items project.godot + T_OPT_SCALE i18n) + FS 16/14 ·
  UI-5 : focus clavier RESTAURÉ (Espace intercepté par main._input AMONT du focus,
  focus_mode=NONE retiré de ui_theme).
- **PIPELINE DE CLÔTURE (à dérouler quand les 4 agents restants ont atterri)** :
  (1) compilation complète + make smoke + bancs sensibles (heritage/statecraft/revolt/
  demography/religion/tech/events/ai) ; (2) re-baseline golden UNE FOIS (kit+câblage+
  équilibrages mordent an-0) ; (3) sweep santé ./chronicle 9 5 250 6 12 (satisfaction,
  hégémon mortel 5/5, §27 gaté an-180) ; (4) probe shot_ui headless (⚠ tuer les
  instances Godot zombies AVANT — 2 process bloqués sur le verrou d'import la dernière
  fois) ; (5) commit vague + push + ff-merge main ; (6) export build_godot.sh (Git Bash
  → bash MSYS2 : APPDATA scrubé, le script détecte le profil par PRÉSENCE des templates).
- **RESTE EN FILE (tasks)** : UI-2 topbar 4 blocs+matières en tiroir+alerte rupture ·
  UI-3 zone contextuelle unique · UI-4 hiérarchie d'actions+conséquences · UI-5 suite
  (symboles doublant la couleur) · #70 portraits conseillers=identité=modificateur
  (agents équilibrage à lancer après la vague) · rendu du champ flavor tech dans le
  panneau Medusa (le champ est rempli, l'UI ne l'affiche peut-être pas encore).
- **PIÈGES chauds** : bash -lc mange le cd (scripts dans packaging/windows/) · worktrees
  KO sur cette machine (git introuvable pour le harnais) · un seul make à la fois
  (agents en fsyntax-only) · limite de session API ~12:50 Paris (l'agent traditions
  est mort dessus une fois, SendMessage l'a ressuscité).


---

# ARCHIVE (threads précédents)

# SYNTHÈSE DE SESSION — handoff roulant (2026-07-09)

> Branche `claude/vibrant-euler-1tgfp3` · **SAVE_VERSION 73** · moteur ~figé ce thread.
> Thread du jour : **CORRECTIFS UI + Godot jouable** (musique menu, feedback joueur,
> scps.exe, vagues de retours écran). Travail SOLO, prudent — consigne joueur : « tu prends
> pas d'initiatives à la con, demande avant de t'enfoncer, le code est propre ».

## Fait ce thread (2026-07-09)
- Musique menu (main_menu.mid → OGG, soundfont SGM-V2.01) · feedback joueur (bouton +
  crash + export user://feedback) · scps.exe (build_godot.sh, export via **Git Bash** car
  APPDATA non résolu en MSYS2 login) · bug assets export CORRIGÉ (`UIKit.load_img`/`has` :
  load().get_image()+ResourceLoader, PAS Image.load_from_file/FileAccess qui lisent le
  disque réel hors PCK) · ticks MENSUELS (`Sim.month_ticked`) · boutons StyleBoxFlat
  (sheet02 fleurons au milieu = pas 9-sliceable) · curseur rotate_180+hotspot(2,2) ·
  sweep 10×5×250 sain (levier luxe laissé ~19.5 %, mesuré ≠ 4 %).

## Vague correctifs écran (retours screenshot joueur)
**FAIT** :
- **FOG = VRAI voile** (`scps_api.c` `scps_fog_visible`) : cellules SANS région (mer/lac/
  basse-terre) partaient VISIBLES (fail-open) → **fail-closed** + **dilatation bornée**
  `FOG_SEA_HALO 8` (révèle lacs enclavés + frange côtière du joueur ; océan/terres
  étrangères restent voilés) ; garde `human_player<0` = carte nue. **DLL debug+release
  rebâties** (Jul-09-19:48). ⚠ le rebuild manuel EXIGE `PROCESSOR_ARCHITECTURE=AMD64` +
  `TMP=/tmp` (sinon scons KeyError) ; NE PAS masquer l'exit code par `| tail`.
- **Topbar ressources raw** (`topbar.gd`) : bois·argile·pierre·fer·armes via `country_stocks`
  (ordinaux RES_ miroir scps_types.h : 9·24·25·13·36) + `resource_sprite`.
- **Diplo raisons grisées** (`country_actions.gd`) : `_why_disabled` nomme au survol pourquoi
  chaque verbe est grisé, dérivé de `opinion_summary` (langue-indépendant).

## VAGUE UI GLOBALE (2026-07-09 soir) — « revérifie l'intégralité de l'UI, rends-la agréable »
Directive joueur. Outil : **probe `shot_ui.gd`** (boote Main.tscn ENTIER, lance seed 9 + 25 ans,
ouvre chaque panneau, 19 PNG dans `godot/project/shots_ui/`). L'audit visuel a produit la vague 1 :
- **Impôts ÷12 CONFIRMÉ juste** en capture (~1 950 or/an sur 5 532 hab) ; libellé aligné.
- **Fog VÉRIFIÉ superbe** (voile + halo doux + territoire lisible) ; **bande beige bas d'écran
  ÉLUCIDÉE** = la MARGE DE PAGE hors-monde (le voile ne couvrait que le rect carte) → 4 bandes
  sépia autour du monde (overlay.gd, gaté game_on).
- **province_panel** : prospérité SORTIE du header (chevauchait paysage + ✕) → ligne habitants ;
  camemberts cerclés + « Culture · X » / « Idéologie · Y » dominants ; **hauteur AU CONTENU**
  (_ph latché — fin de la colonne pleine hauteur vide).
- **topbar** : « An N » décalé x=48 (sous l'ornement de capsule) ; TOUTES icônes 16→20 px.
- **controls** : zoom bas-droite coupé = `_zb.size.x` nul au 1er resize → largeur explicite.
- **country_actions** : grille 3×2 à largeur égale, « Fabriquer une revendication » sur SA ligne.
- **sidebar_drawer diplo** : hint unique en tête (était répété sous CHAQUE pays).
- **tech_panel** : layout radial → **ELLIPSE** pleine largeur (le cercle min(w,h) entassait le
  centre) ; anneaux de tier en ellipse miroir ; bande métabolisation 66→92 px (labels ne se
  chevauchent plus), troncature noms 8→14 chars.
- **main** : panneau construction décalé x+320 (recouvrait le panneau province).
- **icon_button** : lift des icônes sur chrome sombre (rail gauche illisible).
⚠ Itération 2 VÉRIFIÉE en captures : panneau province au contenu (set_deferred — poser size
pendant _draw est IGNORÉ par Godot, piège payé) · arbre tech en ellipse pleine largeur + bande
propre · grille diplo égale · marge de page voilée. ⚠ PIÈGE probe : les PNG s'écrivent AU FIL
de la tournée — purger le dossier avant de relire (jugé des captures STALE une fois) ; et un
parse error dans UN panneau (`:=` sur expression non typée → tech_panel) fait échouer le
`new()` de main.gd → tournée bloquée.

## « INSPIRE-TOI » CK3/EU5 (2026-07-09 soir) — les .gui MINÉS (2 éclaireurs, installs locales)
Rapports complets dans les transcripts d'agents ; l'ESSENTIEL transposé :
- **Topbar CK3/EU5** (FAIT) : cellule = icône 22-24 + VALEUR (empilée) sur DELTA signé
  vert/rouge + séparateur 12px · TOPBAR_H 38→48 (propagation via Frame) · stocks avec
  net/jour (ScpsStock.net_day). EU5 : delta zéro = « = » (non repris, vide chez nous).
- **Ledger/outliner EU5** (FAIT, 1re passe) : rubans de catégorie colorés (39px chez eux,
  4px chez nous) + compte à droite dans la bande (`_lsection`, empire_sidebar). ⚠ BUG
  LATENT PRIS : `visible = Sim.game_on` DANS _draw → un Control caché ne redessine jamais
  ⇒ l'empire_sidebar n'apparaissait JAMAIS en jeu (aucune capture ne l'avait) — visibilité
  déplacée en _process.
- **Lavis politique** : WASH_A_FAR 0.72 était DÉJÀ aligné flatmap EU5 (0.75) ; saturation
  `_entity_wash` 0.60→0.72, val 0.82→0.88 (named_colors EU5 : S 70-90 · V 85-95).
- **En réserve** (patterns notés, non posés) : lignes outliner 31px constantes + texte droit
  13px · jauges VERTICALES fines 5×22 (stock/moral) · tooltip padding 10/titre 420/lignes
  alternées · marges fenêtre CK3 40/18/20 · gradient de zoom EU5 (inside/outside alpha +
  edge_width + color_mult par palier — pour pousser le « paper map » plus loin) · compteur
  d'unité EU5 = bannière papier + icône type 57 + jauges exp/moral verticales.

## LOT 3 RETOURS JOUEUR (2026-07-09, nuit) — arbre en COLONNES · satisfaction · CK3-slots
- **Arbre tech : MEDUSA ABANDONNÉ** (« peut-être que médusa n'était pas la solution ») →
  colonnes par TIER gauche→droite (Civ), quinconce si colonne pleine, séparateurs + T0-T5 ;
  hovers/médaillons/arêtes conservés. « le creuset digéré » → « Peuples intégrés : +X% de
  recherche ».
- **HUMEUR → SATISFACTION** : barres par CLASSE (façade neuve `scps_province_class_sat`,
  0-100, −1=classe vide, lit strata[].satisfaction) + strate SERVILE toujours listée (même
  à 0) ; l'ex-humeur (légitimité locale) reste en ligne « Loyauté » compacte (concept ≠).
- **BÂTIMENTS façon CK3** : carrés du bâti (manuf_sprite + hover nom·niveau·ouvriers) +
  case « + » = construire (remplace le gros bouton) — `province_buildings` existait déjà.
  ⚠ PAS de compte « slots possibles » affiché : aucun cap de slots n'existe moteur (le
  palier est gaté tier/tech) — en inventer un serait un mensonge d'affichage.
- **Ressources par ICÔNE** partout (res_id → resource_sprite ; RESSOURCES + PRODUCTION).
- **FOG durci 236→252** (« ce n'est pas un brouillard de guerre » : lavis/bandes
  transparaissaient) + bandes de page alignées.
- **HULAHOOP** : `_deloop` en sortie de `_smooth_poly` — excise les nœuds en 8 (points
  à ≤1.6 cellule séparés de 6-22 indices, SEULEMENT si le tronçon s'éloigne >3.2 = vraie
  boucle ; sans ce garde, une droite resamplée se ferait décimer).
- ⚠ Piège GDScript payé 2× : `var bh` redéclaré dans le même scope = parse error qui
  tue TOUT le script (et le `load().new()` de main en cascade).

## ARBRE DE TECH EN COULOIRS + INSPIRATION RIMWORLD (2026-07-10)
- **Arbre raffiné** (retour joueur : « profite des icônes, plus organique ») : 3 COULOIRS
  thématiques (quarter/3 → Savoir·Forge·Société, ordre THM_*), fond alterné + ruban coloré +
  étiquette par couloir ; chaque branche se lit en LIGNE gauche→droite ; panneau agrandi
  (jusqu'à 1460×1000) ; médaillons ~2× (rayon 14-24 adaptatif, art ×1.62 du disque).
  VÉRIFIÉ en capture : lisible, arêtes qui coulent.
- **RimWorld « inspire-toi »** : ⚠ le Source/ shippé = échantillon modding, l'UI est dans
  Assembly-CSharp.dll (éclaireur honnête : rien à miner sans décompiler) → patterns ÉTABLIS
  appliqués. FAIT : **TimeControls** — 4 boutons de vitesse DISCRETS (❙❙·▶·▶▶·▶▶▶, l'actif
  suréclairé, tips, clic ciblé — remplace le chip cyclique). Date déplacée AVANT le chip
  d'âge (ils se chevauchaient) — le chip se cale à gauche de la date.
- **LETTERS (FAIT)** : alerts.gd — chaque notification porte son LABEL VISIBLE (cartouche
  sombre + liseré au domaine, texte dérivé du tip via `_short` : coupe au tiret/parenthèse,
  tronque à 26) contre le chip-icône ; **clic droit = BALAYER** un transient (les CONDITIONS
  persistantes restent — elles disent un état). Colonne élargie LABELW 184.
- **VAGUE STELLARIS (2026-07-10, en cours)** — retour joueur « ça fait encore très patate,
  dimensionne les fenêtres, laisse les icônes bottom bar flotter, inspire toi de Stellaris ».
  Minage D:\Steam\...\Stellaris\interface (rapport en TROUVAILLES §Minage UI Stellaris) :
  fenêtre standard ≈ 2/3 largeur × 60 % hauteur (jamais plein écran) · header 36 px, titre
  grand corps, close HAUT-DROITE · marge 12 · rangées 38-40 · bottom bar = boutons AUTONOMES
  sans plaque continue. Appliqué :
  · **Bottom bar FLOTTANTE** (controls.gd) : la bande cuir pleine largeur + liseré or SUPPRIMÉE,
    parent en MOUSE_FILTER_IGNORE (la carte reste cliquable entre les boutons, chaque IconButton
    capte les siens) ; border_art descend au bord bas (offset_bottom 0).
  · **VKit.header(ci, w, title) → Rect2** : le header standard (bande 36 px + titre FS_BIG +
    filet or + ✕ haut-droite, renvoie le close-rect) — consommé par construction_panel et
    economy_panel ; à propager au fil de l'eau.
  · **Hauteur AU CONTENU** : construction_panel (_ph latché — fini le vide sous les grilles) ;
    sidebar_drawer (les 8 _draw_* renvoient leur y final, flashes dans le FLUX au lieu de
    l'ancrage bas, clips sur _hmax viewport ; latch clampé [120, _hmax]).
  · **economy_panel** : le déroulant TRONQUAIT le titre (x=210) → ancré à droite du header.
  ⚠ Godot exe = `Godot_v4.6.3-stable_mono_win64/` à la RACINE DU REPO (pas de Program Files —
  rc=127 payé une fois de plus).
- **VAGUE SETUP + PANNEAUX PLATS (2026-07-10, retours joueur sur captures)** :
  · **Créateur d'empire EN ONGLETS** (culture_creator.gd réécrit) : TabContainer
    Héritage / Éthos / Traditions. Héritage explique CE QUE ÇA OUVRE (noms + accès natif
    tier 3 à sa branche d'arbre — vrai depuis Temps 2a) ; Éthos répond « ça m'apporte
    quoi » (épithète + hint moteur + ce que l'éthos pilote : armée, factions, évents) ;
    Traditions = BOUTONS-toggle visibles avec le CHIFFRE dans le libellé (+2/+1/−1),
    plus de déroulants (« expliquer combien »). La GRAINE est SORTIE du créateur
    (mode autonome → Sim.current_seed). API conservée (open/open_for_slot/composed).
  · **Nouvelle partie** : le SEUL réglage monde = TAILLE en slider Tiny→Huge ; les
    autres sliders (âge/terres/relief/érosion/climat/continents) RETIRÉS — l'ARCHÉTYPE
    de la graine décide. ⚠ PIÈGE ÉVITÉ : worldgen_set remplit les clés absentes par des
    constantes fixes → on fusionne worldparams_default(graine) + la taille (sinon les
    cartes redeviennent toutes pareilles, tuant le lot W). Graine : LineEdit + bouton 🎲
    (plus d'up/down).
  · **PANNEAUX PLATS** (« les joueurs pardonnent le cheap », RimWorld) : VKit.panel_bg
    perd le 9-slice parchemin → ombre + cuir plat + liseré fin 1 px (styleboxes cachées) ;
    plaque de titre du tiroir → bande plate ; capsule de date + bande parchemin du chip
    d'âge (topbar) → plat. GARDÉS (contenu, pas cadre) : icônes ressources, sceaux
    d'évents, bateaux, poignée de rabat du ledger.
  · **Marché** : le « (+X % édifices) » inline quitte le label → il vit dans le HOVER
    de « Puissance comm. » (« Dont +X % apportés par vos édifices de commerce »).
  · shot_ui gagne 3 captures de setup (01b Nouvelle partie · 01c créateur · 01d onglet
    Traditions). Probes : boot 0 erreur · MENU AUDIT OK · CULTURE AUDIT OK.
- **ARBRE DE TECH — réalignement + réécriture (2026-07-10)** :
  · Panneau : tri BARYCENTRE par colonne (l'ancien tri alphabétique croisait les arêtes),
    médaillons rr 0.46·rowh (16-30) ×1.9, bande du bas : cellules d'héritage CENTRÉES +
    BARRE de % de digestion (or=natif) + pips de tier ; rangée Ascension centrée.
  · TEXTES moteur (agent sonnet, coupé par la limite de session à 80 %, fini main) :
    plus de « capacité narrative »/« fédéralisme »/« béton→marbre » — chaque utility dit
    les leviers VIVANTS (dK=prospérité · dL=stabilité · doctrine · prod/eff %), les champs
    morts (dF/.eco/.mil) ne sont plus jamais promis ; RÈGLE 3 : nœuds sans levier vivant
    → petit levier réel (ex. Fortifications dL 1.5) ; scps_readout.c TECH_UTILITY synchronisée
    (béton, « métal » supprimé) ; commentaires scps_tech.h réécrits (K/L vivants, F MORT).
  · GATES : tech_demo 23/23 · lang-check 0 · **golden IDENTIQUE** (les leviers neufs ne
    mordent pas <12 ans) · determinism stable · smoke 7/7 APRÈS fix d'un LIEN LATENT :
    scps_ai.o référençait country_knows (fog) sans scps_fog.o dans AI_DEMO_OBJS — vert
    sur binaire périmé depuis le lot diplo-fog, exposé par le re-lien (Makefile fixé).
- **POLISH GÉNÉRAL (2026-07-10)** : police +1 cran partout (FS 15 · SMALL 12 · BIG 20 ·
  thème 16 · tooltips 14) ; topbar recollé à x=16 (trou de l'ex-capsule) ; ruban Pause
  ancré sous les contrôles de vitesse ; VKit.gauge = piste + remplissage teinté (fini
  l'arc-en-ciel-palette) ; sous-titre du menu lisible (encre claire + liséré).
- **FACTIONS EN TOPBAR + LEDGER ADAPTÉ (2026-07-10)** — retour joueur « Adapte le menu
  de droite et les factions doivent être en top bar » : le bloc COUR & FACTIONS quitte
  empire_sidebar → topbar (cellule Bonheur % — détail Laboureurs/Artisans/Noblesse au
  survol — puis un BLASON par faction avec mini-barre d'ADHÉSION dessous, ★ dominante,
  liseré rouge si rancœur ≥25, ⚑ tension de coup/corruption au survol ; l'influence y
  était déjà). Ledger aéré : rangées 16→18 px, villes 8→10, journal LOG_MAX 14→18,
  lignes de log 14→16 px. Vérifié en capture (1920 : la rangée tient avant le chip d'âge).
- **À FAIRE (RimWorld, suite)** : learning helper contextuel (concepts dismissibles,
  raccordé au codex F1/pages tuto) ; inspect-pane-isation éventuelle du panneau province
  (terseur RimWorld) — à goût joueur.

## LOT 5 RETOURS JOUEUR (2026-07-09, très tard) — EN COURS (tâche #55)
**FAIT** : diplo sous FOG — façade `scps_country_known` (country_knows du joueur ; sans
joueur = tout connu) + binding + FILTRE de la liste diplo (un pays jamais découvert
n'existe pas) + garde `open_country`. Missions : la récompense or+matière est RÉELLE
depuis lot B 2026-07-07 (« récompense matière de mission » routée econ_region_stock_add,
mesurée) — la capture montrait la PROMESSE (an 0 = émission), versée à l'accomplissement.
**FAIT (2e passe « go »)** :
- (B) CONSTRUCTION : section MANUFACTURES (grille, manuf_legal/manuf_cost/player_build_manuf,
  ciblée sur `target_pid` posé par main aux 2 sites build_requested) ; les UNITÉS en SORTENT.
  ⚠ pas de « bâtiment de boost » posé : AUCUN verbe joueur raw_boost n'existe (l'exploitation
  est agency/IA) — à créer moteur-side si voulu.
- (C) tiroir ARMÉE : sous-section « Composer l'armée » (grille unit_roster, tuile grisée ✦ =
  verrouillée, clic = player_recruit + flash) + `_tips`/`_get_tooltip` GÉNÉRIQUES ajoutés au
  drawer ; « Recompléter » AUSSI au ledger droite (chip → player_refill).
- (D) tiroir CONSEIL scindé : chips « Gouvernement » (sièges) / « Politiques » (décrets +
  peuple servile), `_conseil_tab`.
- (B-bis, FIX MOTEUR golden-prouvé) : les RAW-WORKS (four à brique·carrière·scierie,
  in1=RES_NONE hors-sol N1) étaient VERROUILLÉES hors du jeu joueur — les 2 miroirs
  (scps_manuf_legal + drain CMD_BUILD_MANUF) rejetaient in1==NONE comme « dégénéré ».
  Ouvert (seul out==NONE rejette ; feed saute si in1==NONE) ; `make golden` IDENTIQUE.
  ⚠ `raw_boost` (paliers d'exploitation) reste IA-seul (aucun verbe joueur — à créer si voulu).
- (E, FAIT) OPTIONS D'ÉVÉNEMENT CHIFFRÉES : `ScpsPendingEvent.effets[4]` composé façade
  (trésor ±N % du revenu mensuel / ±N or · population ±N % · légitimité/agitation/
  fertilité/influence ↑↓ · cicatrice durable · pari (N %)) → binding "effets" →
  event_dialog : l'effet EN TÊTE du tooltip, le flavor dessous.
- (F, FAIT) HOVERS DE TECH CHIFFRÉS : accesseurs `tech_node_prod_pct/_eff_pct` (scps_tech,
  additifs) + composition façade dans scps_tech_nodes (buffers statiques TECH_COUNT×176) :
  hover = mot mécanique + « prospérité +2 · stabilité +1 · production +5 % · charge
  faustienne +0.5 » — SEULS les leviers VIVANTS (dEco/dMil/dF morts, jamais affichés).
- EN SUSPENS (demande joueur) : bâtiments de raw (`raw_boost` joueur).
⚠ discipline : chaque déplacement d'UI se REVÉRIFIE par la probe shot_ui avant export.

## LOT 4 RETOURS JOUEUR (2026-07-09, fin de nuit)
- **Date complète à côté de la vitesse** : « Jour X · mois Y · an Z » (façade neuve
  `scps_day_of_year` = sim.day%365 ; mois=doy/30+1 borné 12) — la capsule gauche ne garde
  que la rose.
- **Boutons rail/modes SANS fond de chrome** (icône nue + soulignement or sélection/hover,
  icon_button) — le fond cuir noyait les glyphes.
- **Panneau pays étranger AVEUGLE** : plus de jauges internes/trésor (« pourquoi je vois
  les métriques des autres ? ») — reste éthos·régions·âmes·influence (le PUBLIC). PH 296→240.
- **« J B N » → en toutes lettres** (ledger bonheur par classe).
- **Feedback colonisation** : clic → bord clair + « Ordre émis — la colonne part sous
  peu » (3 s, horloge mur) + son.
- **BÂTIMENTS : les ÉDIFICES DE BASE affichés** — façade neuve `scps_province_edifices`
  (masque edi_built → nom + TYPE porté par le champ niveau, vignette building_sprite).
- **« Stock national de X »** : noms en DUR (RAW_NAMES) — country_stocks omet les biens à
  stock 0, le nom manquait.
- **Hulahoop v2** : fenêtre 22→40 (gros nœuds) + bulge borné <7 (épargne les vraies
  péninsules fines — un out-and-back légitime est LONG, un nœud est compact).
- **Pause → UI au clic** : notify_action branché au hook universel de clic + chips custom.
- **Hovers dégonflés** (« les gens savent lire ») : le hover NOMME ; seuls les CONCEPTS
  (stabilité, influence, productivité…) gardent une explication.

## DOCTRINE DE LAYOUT (2026-07-09, retour joueur annoté) — LOI DURABLE
**National = TOPBAR · Local = GAUCHE · Raccourcis monde/notifications = DROITE.**
Vague posée : jauges nationales (stabilité·prospérité·légitimité·cohésion·influence) au topbar
avec HOVERS (TIPS de country_panel réutilisés) ; country_panel = étranger SEUL (le sien vit en
haut) + décalé à gauche du ledger (il était caché dessous) ; bande de BIOME 96px nue, titre
DESSOUS ; panneau province RÉTRACTABLE (chevron « – ») ; `panel_bg` = fond PLAT + cadre 9-slice
sans centre (le centre étiré était « très moche ») ; rail 48→64 / BTN 38→52 / bottom 50→66 ;
tooltips sur CHAQUE cellule topbar + lignes CAPITALE/Impôts/RESSOURCES/PRODUCTION (« un
explicatif sur chaque display » — passe à CONTINUER sur tiroirs/détail/construction).
RESTE de ce retour : « boutons mal découpés » (re-cut d'art des feuilles d'icônes — même
chantier que recut_parch.ps1, à cadrer) ; hovers systématiques sur les autres panneaux.

## « RENDU ATTENDU » (2026-07-09, 4 captures EU4 du joueur) — la CIBLE UI
EU4 = référence de densité/structure. Convergences POSÉES : tooltips sombres liseré or
(ui_theme TooltipPanel/TooltipLabel) · DELTAS MENSUELS or/pop verts-rouges au topbar (photo
month_ticked, display-only) · RUBAN PAUSE centre-haut. ROADMAP restante (par impact) :
1. **LEDGER droite** (empire_sidebar → colonne EU4 : Armées effectifs+noms · Flottes ·
   Diplomate/CD · Colonisation · Factions/agitation top-N · Constructions en cours — les
   readers façade existent : army_info/colony_status/factions/…).
2. **Compteurs d'armée sur carte** (strip coloré au pays + « N k » sous le pion — army_info.units).
3. **Headers de section en bannières** (VKit.section → bande sombre + or, façon ruban EU4).
4. **Minimap** bas-droite (scps_map_rgba réduit) + grille de modes.
5. ⚠ QUESTION posée au joueur : la CARTE elle-même — garder le PARCHEMIN (investi) ou pousser
   le lavis politique vers la saturation EU4 (wash fort + frontières noires épaisses) ?

**RESTE** (besoin des yeux/direction du joueur) :
- **Villes non centrées** : SCREENSHOT requis (déjà retouché 2026-07-08 : siège = centroïde
  ancré sec ; encore off → itération visuelle, pas re-guess).
- **Barre province massive + désolidariser biome + herbe médicinale (RESSOURCES cap 2 masque
  la 3e brute affichée en PRODUCTION)** : redesign, cadrage requis.
- **loyauté des ordres** (topbar) : AMBIGU — conseil (statecraft `council_loyalty`) vs ordres
  sociaux (classes) vs cohésion ? à clarifier avant câblage (pas de reader empire-level net).
- **impôts 6300** : PROBABLEMENT PAS un bug — miroir du ~42 % moteur sur la richesse de classe
  d'une province riche ; à expliquer.
- **Layer MARCHÉ** : reader `intertrade_region_hub` EXISTE → prêt (façade `scps_market_owner`
  + binding `market_image` + wash overlay + TOGGLE). Besoin d'un NOUVEAU mode carte
  (`nature_mode` = debug seul) → confirmer l'UX (touche/bouton, teinte cité-état).
- **Arbre tech dézoom + icônes de barres** : besoin du « combien » (tech_panel n'a pas de var
  zoom : layout radial à ajuster).

---

# SYNTHÈSE DE SESSION — handoff roulant (2026-07-07, soir)

> Branche `claude/vibrant-euler-1tgfp3` · **SAVE_VERSION 73** (<v73 refusé) · HEAD `2a6530b`+.
> Golden re-baseliné par le lot W (archétypes de graine), tenu vert post-merge A+E+M+W.
> Vague du jour : audit adversarial → 4 lots de fixes en hiérarchie multi-agents
> (orchestrateur Fable + implémenteurs sonnet en worktrees, merges séquencés).

## En vol
- **Lot B « écritures fantômes »** (agent sonnet, arbre principal) : 4 sites — armes IA
  `scps_ai.c:1307` · rares Merveille `scps_endgame.c:568` · récompense mission
  `scps_missions.c:103` · dîme raid `scps_navy.c:497` — + commentaires menteurs « stock au
  grain RÉGION ». RE-BASELINE golden attendue. → revue diff + commit orchestrateur au retour.
- **Worktree SCPS-wt-W** encore monté (captures shot_w*.png) — retirer après le bilan.
  M/E retirés (mergés). DLL GDExtension rebâtie (bindings E+M+W).

## Livré aujourd'hui (2026-07-07)
1. **Assets** : écrans de fin + barre entropie + 13 bordures enluminées câblés (a400f80) ·
   re-découpe recentrée 431 cellules (58514fd, tools/recut_parch.ps1) · triage du trove
   Codex (docs/CODEX_TROVE_TRIAGE.md).
2. **Sons réels** (3 lots) : un clic = un son (modèle Paradox), ui_click universel, cor de
   guerre, tech_notif, month_tick seul ; TOUT le synthé purgé (14 fichiers), sound.gd minimal.
3. **AUDIT complet** 5 voies + vérif adversariale → **docs/AUDIT_2026-07-06.md** : 38/38
   verbes câblés · 2 HIGH + 9 MED confirmés · 9 faux positifs écartés · ADDENDUM balayage
   region[] (4 écritures fantômes).
4. **Lot A** (v72→73) : 3 grâces de révolte sérialisées (⚠ borne basse **-31** — le repos
   post-expiration est SOUS zéro) + g_hub_of borné (refus net, pas de dirty-rebuild) + cas
   fuzztest prouvé par test négatif.
5. **Lot E English** : switch FR/EN moteur (passe-plat `scps_lang_set`) + Options (langue +
   plein écran, persistés) + shell migré tr() (55 clés) + `tools/extract_gd_literals.py` →
   backlog **629 littéraux / 28 .gd**. ⚠ `strings_en.h` encore ~46 % copie FR.
6. **Lot M membrane honnête** : FIN_SANG visible + épilogue · Construction grisée
   (`build_legal_ex` or+matière — ⚠ MIROIR des gates du drain, à re-synchroniser si
   agency/intertrade changent) · prix manufacture/servile affichés · noms de ministre
   (trahisons) · codex corrigé · lettré religieux jouable · probe membrane_audit.
7. **Lot W worldgen** : **8 archétypes de graine** (7 graines → 7 mondes distincts ; overrides
   sliders/argv priment) + **falaises maritimes émergentes** (lithologie ; le tueur était la
   gamma vallées lf^1.6 ; piège cell.lake) ; encre musclée post-merge (lisible fit + zoom).
   5 bancs recalibrés intention préservée.

## Reste / prochains pas
- **Retour lot B** → revue, commit ; re-mesurer raid/Merveille (coûts redevenus réels).
- **Chantier 3 audit** (outillage) : lang-check ré-armé + 25 littéraux readout → STR_* ·
  6 tune_f au registre J · recalibrage E3/interception/péage.
- **Publiable** (analyse du 07) : packaging Windows (export preset, DLL, saves → user://),
  soak test front 200 ans, onboarding guidé, musique (externe — lasonotheque pour les SFX
  déjà fait), autosave, research_target/cmd_n dans la save (prochain bump), page Steam.
- **i18n** : traduire ~165 entrées strings_en.h + migrer le backlog CSV par lots
  (⚠ uikit.gd = clés de correspondance sprites, ne pas traduire naïvement).
- **Env (pièges payés 2×)** : make = login shell MSYS2 + `TMP=/tmp` + script .sh (le cd
  inline se fait MANGER) ; Godot exe dans `Godot_v4.6.3-stable_mono_win64/` ; shot_parch
  FENÊTRÉ (--headless = noir) ; PS 5.1 sans ternaire ; scripts .ps1 en ASCII pur.
