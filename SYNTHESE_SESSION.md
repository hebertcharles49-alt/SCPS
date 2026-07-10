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
