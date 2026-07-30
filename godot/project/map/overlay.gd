extends Node2D
## Overlay — les ACTEURS sur la carte, en espace MONDE (enfant de MapView → suit
## la caméra zoom/pan automatiquement). DISPLAY-ONLY : lit la façade (region_tier,
## army_info, centroïdes), ne calcule rien. Redessine au tick (les données bougent).
##
## Villes : un disque au centroïde, dimensionné au tier (0-5), teinté au pays.
## Armées : un losange au centroïde de leur région + une ligne vers leur but
## (marche), un anneau coloré par phase (marche/siège/bataille).

const UIKit = preload("res://ui/uikit.gd")
const VKit = preload("res://ui/vkit.gd")
const GeoNames = preload("res://map/geo_names.gd")
const HeraldryK = preload("res://ui/heraldry.gd")
const PHASE_MARCH := 1
const PHASE_SIEGE := 2
const PHASE_BATTLE := 3
const LAYER_WATER := 4       ## SCPS_LAYER_WATER : masque mer OU LAC (≥1 = eau) — l'assise des
                             ## bourgs tient l'EAU COMPLÈTE (les lacs intérieurs, ignorés par SEA seul)
const LAYER_SEA := 1         ## SCPS_LAYER_SEA : mer SALÉE seule (≥1) — pour distinguer LAC (eau douce) de MER
# SIÈGE du tampon — les humains habitent près de l'EAU DOUCE (rivière/lac), sinon le RIVAGE (mer), décalé
# vers l'INTÉRIEUR. Rayons de recherche (cellules) depuis le centroïde + décalage inland du siège.
## Seuils de zoom ISO (px/unité-monde de la Camera2D). L'ISO est la surface de JEU : on y montre
## ROUTES & ASSETS (bourg). L'entrée en ISO est à zoom ≈ ISO_FAR (4.0) → assets déjà lisibles.
const CITY_ZOOM_MIN := 3.5   ## villes + bourg
const DECOR_ZOOM_MIN := 3.0  ## forêts/arbres + DRESSING de terrain (lot 2)
# ── DRESSING DE TERRAIN (lot 2, marques peintes) : semé par BIOME, display-only, SOUS frontières/villes,
# basse densité, taille ÉCRAN constante. Biome (couche LAYER_BIOME, enum Biome de scps_types.h) → ids.
const LAYER_BIOME := 2       ## SCPS_LAYER_BIOME : index de biome par cellule
const DRESS_SPACING := 9     ## pas de la grille de semis (cellules) — DENSIFIÉ (trame continue, pas des stickers)
const DRESS_ALPHA := 0.50    ## opacité FADE — translucides : le parchemin transparaît, elles se FONDENT
                             ## (chevauchement des marques denses → trame qui s'auto-construit, pas « posé là »)
const DRESS_BY_BIOME := {
	# RELIEF : CHEVRONS PROCÉDURAUX SEULS (décision joueur 2026-07-28 — plus aucun sprite
	# hill_*/rocky_*/mountain_* sur le relief ; _try_place_dress force l'id par biome)
	18: ["chevron"],   # MONTAGNES
	19: ["chevron"],   # PIC
	23: ["chevron"],   # VOLCAN
	16: ["chevron"],   # HAUTES-TERRES (chevrons ×0.6)
	17: ["chevron"],   # COLLINES (chevrons ×0.6)
	# FORÊTS : AUCUNE entrée ici — la canopée est COMPOSÉE d'arbres INDIVIDUELS lot 6 par la
	# passe dédiée CANOPY (pas fin, ancrage monde, tri de profondeur) — cf. _build_dressing.
	# PLAINES / PRAIRIE (lot 3 herbe + SINGLES lot 6 : l'arbre isolé vit ICI, pas en forêt)
	4:  ["plain_grass_01", "plain_grass_02", "plain_sparse_tufts_01", "plain_wind_strokes_01", "lot6_broadleaf_01", "lot6_ground_01"],  # PLAINES
	5:  ["plain_sparse_tufts_01", "plain_grass_02", "lot6_broadleaf_10"],    # CHAMPS (épars)
	6:  ["plain_grass_01", "plain_grass_02", "plain_sparse_tufts_01", "lot6_broadleaf_07", "lot6_ground_12"],  # PRAIRIE
	# STEPPE / SÈCHE (lot 3 + lot 2)
	7:  ["steppe_grass_01", "steppe_grass_02", "steppe_dry_strokes_01", "steppe_tufts_01"],      # STEPPE
	9:  ["steppe_dry_strokes_01", "steppe_tufts_01", "scrub_brush_01", "rocky_outcrop_01"],      # TERRES SÈCHES
	8:  ["savanna_grass_01", "savanna_grass_02", "savanna_sparse_tree_01", "acacia_mark_01"],    # SAVANE
	# DÉSERTS (lot 2)
	10: ["dune_wind_lines_01"],                                             # DÉSERT
	11: ["dune_wind_lines_01"],                                             # DÉSERT CÔTIER
	# ZONES HUMIDES (lot 3)
	15: ["marsh_reeds_01", "marsh_reeds_02", "marsh_tufts_01", "marsh_ripple_reeds_01"],  # MARAIS
	21: ["marsh_reeds_01", "marsh_reeds_02", "tree_broadleaf_01"],          # MANGROVE
	22: ["marsh_reeds_02", "marsh_tufts_01", "marsh_ripple_reeds_01"],      # TOURBIÈRE
	20: ["lot6_conifer_02", "lot6_conifer_06", "rocky_outcrop_01"],        # GLACIER (sapins épars)
	# EAU — MOUVEMENT seul (lot 3) : rides, houle, courants ; jamais un aplat
	0:  ["ocean_swell_lines_01", "ocean_current_swirl_01"],                # OCÉAN PROFOND (très épars)
	1:  ["sea_ripples_01", "sea_ripples_02", "ocean_swell_lines_01"],      # OCÉAN
	2:  ["sea_ripples_01", "sea_ripples_02"],                              # HAUT-FOND
}
## probabilité de poser une marque par biome (densité) — DENSIFIÉ (trame continue) ; eau reste ÉPARSE.
const DRESS_DENSITY := {
	4: 0.65, 5: 0.38, 6: 0.68,                          # plaines/prairie : herbe DENSE (jadis trop nue)
	7: 0.70, 8: 0.62, 9: 0.68, 10: 0.80, 11: 0.72,
	12: 0.95, 13: 0.90, 14: 0.95, 15: 0.88, 21: 0.72, 22: 0.82,
	16: 0.85, 17: 0.80, 18: 1.0, 19: 1.0, 23: 0.90, 20: 0.52,   # montagnes : COUVERTES (chevrons)
	0: 0.07, 1: 0.18, 2: 0.20,                          # eau : épars (mouvement seul)
}
## PASSES SUPPLÉMENTAIRES par biome (marques EN PLUS par cellule de grille) → CANOPÉE dense. Surtout les
## forêts (le « densifié » demandé) : 1 + N marques jittées par cellule → couvert continu, pas des arbres isolés.
const DRESS_EXTRA := {
	# (vide — retour joueur 2026-07-29 : le relief (18/19/16) empilait 2-3 chevrons/cellule
	# de 9 = « tas de crocs » au centre des massifs. Le mécanisme reste pour d'autres biomes.)
}
## PAS DE SEMIS par biome (cellules) — le relief est GRAND (empreinte ~8-9 cellules, CHEV_H_WORLD
## 5.5 × ~1.2 de large + jitter) : la grille par défaut (DRESS_SPACING=9) sème à la taille de la
## marque, les ∧ se mangent entre rangées. Ceux-ci veulent de l'air — pas de jitter d'amplitude,
## juste un pas d'avancée plus lâche (retour joueur 2026-07-29).
const DRESS_SPACING_BY_BIOME := { 16: 13, 17: 12, 18: 13, 19: 13, 23: 13 }
## ── LA CANOPÉE COMPOSÉE (lot 6) : la forêt est un PEUPLEMENT d'arbres individuels — pas
## fin (5 cellules), ancrés au MONDE (la forêt reste pleine à tous les zooms), ancrage au
## PIED + tri de profondeur (ils s'empilent comme une canopée), essences par biome. ──
const CANOPY_STEP := 2
const CANOPY_BY_BIOME := {
	12: ["lot6_broadleaf_01", "lot6_broadleaf_03", "lot6_broadleaf_07", "lot6_broadleaf_08", "lot6_broadleaf_13", "lot6_broadleaf_15"],  # FORÊT : chênes pleins
	13: ["lot6_broadleaf_02", "lot6_broadleaf_05", "lot6_broadleaf_09", "lot6_broadleaf_10", "lot6_broadleaf_12", "lot6_conifer_03"],    # BOIS : plus clair, mêlé
	14: ["lot6_broadleaf_04", "lot6_broadleaf_06", "lot6_broadleaf_08", "lot6_broadleaf_13", "lot6_broadleaf_14", "lot6_broadleaf_16"],  # JUNGLE : dense, tortueux
}
## LOT 4 — easter eggs RARES (serpents de mer, épaves, récifs, lapins) : placés par une passe à GROS pas.
const EGG_SPACING := 46      ## grille grossière (rare)
const EGG_ALPHA := 0.85      ## moins fadé que le dressing (ce sont des « figures », pas de la trame)
const EGG_WRECKS := ["shipwreck_hull_01", "broken_mast_01", "half_sunk_wreck_01", "floating_debris_01",
	"jagged_reef_01", "low_rocks_01", "sea_stacks_01", "shoal_stones_01"]
const EGG_RABBITS := ["apoc_rabbit_banner_01", "apoc_rabbit_horn_01", "apoc_rabbit_spear_01", "apoc_rabbit_crown_01"]
# ── LOT U — LES BOURGS EN VIGNETTES (pack bourgs/, 144 pièces 256²) : le bourg est UNE gravure —
# T1 ferme → T7 cité impériale, + cité-état à dôme (`bourg_cs`) et hameau sauvage à tour de guet
# (`bourg_wild`), 16 variantes par famille. REMPLACE l'urbaniste composé (maisons/rues/enceinte
# tracées une à une) ; seuls les QUAIS/barque restent composés (le rivage dépend du monde).
const BOURG_DIR := "res://assets/scps/pack/bourgs"
const BOURG_VARIANTS := 16   ## variantes par famille — hash STABLE de la région → _01.._16
const BOURG_ALPHA := 0.76    ## glacis ENCRAGE (retour joueur 2026-07-08 : 0.90 trop opaque — le parchemin doit transparaître)
## largeur MONDE (cellules) du CONTENU de la vignette : T1 discret (~4.6) → T7 dominant (~11) ;
## la cité-état et le hameau sauvage sont calés dans la même gamme. Ancrage au PIED (socle bas).
const BOURG_W_T1 := 3.2
const BOURG_W_T7 := 7.5
const BOURG_W_CS := 6.0      ## cité-état : une fière cité à dôme (entre t5 et t6)
const BOURG_W_WILD := 2.8    ## hameau sauvage : discret (tour de guet)
var _bourg_tex := {}         ## id de vignette → {tex, shadow, foot, cw} (cache paresseux ; {} si asset absent)
var _top_cap_region := -1    ## région-capitale la PLUS PEUPLÉE du monde → la vignette t7 (unique)


var _cataclysm := false   ## un foyer de fin est actif → on anime l'épicentre
var _decor := []          ## FOSSILE (jamais peuplé) — GARDÉ : viewer_audit.gd itère/mesure ces deux
var _structures := []     ## tableaux ; les retirer casserait la probe (à purger AVEC elle, ensemble)
var _bio_img: Image = null ## couche biome (cache) → interdit le PIED d'un asset sur une tuile falaise
var _region_raws := {}    ## région → [{id, name}] : les BRUTES extraites (≤2) — mode carte RESSOURCES (9)
var _raws_dirty := true    ## la production a bougé (an-0 nu → extraction établie) → recache les brutes
var _region_label := {}   ## région → NOM du siège (bannière de lieu KCD, cache paresseux)
var _region_anchor := {}  ## région colonisée → assise de ville CALÉE SUR TERRE (centroïde snappé + rabat côtier)
var _region_seat := {}    ## région colonisée → SIÈGE du tampon : cellule INTÉRIEURE de province (jamais sur une jonction)
## MOUVEMENT D'ARMÉE (clic-armée → clic-destination) : position ISO cliquable du pion du
## JOUEUR (garnison OU ost) + rayon, recalculés à chaque _draw ; army_selected = mode marche.
var army_selected := false                 ## compat panneau historique
var selected_corps: Array[int] = []
var move_preview: Dictionary = {}          ## route survolée avant clic (façade campaign, lecture pure)
## corps_id(int) -> {pos:Vector2, radius:float} POUR LES ARMÉES ; "g<pays>"(String) -> idem POUR
## LES GARNISONS (revue overlay #1 : les deux espaces de clés étaient mélangés — une garnison
## clée par index de PAYS pouvait coïncider avec un id de CORPS réel et se faire sélectionner/
## déplacer comme lui). Les hit-tests ci-dessous ne matchent QUE les clés int (corps réels) —
## une garnison n'est pas un corps, elle ne peut pas être « sélectionnée » pour un ordre de marche.
var _pa_positions := {}
var _dress_tex := {}      ## id de marque de terrain (lot 2) → Texture2D (cache)
var _dressing := []       ## [{pos(monde), id, scale}] — marques de biome semées (display-only)
var _dressing_dirty := true ## la géo a changé (génération/chargement) → re-semer le dressing
# ── DRESSING RAPIDE (revue overlay #6) : le DRAW itérait des MILLIERS de Dictionary/frame (hash
# lookup par champ × entrée, + un mv.iso_pos() par entrée) pour les marques SPRITE. Ici : tableaux
# PARALLÈLES TYPÉS, remplis UNE fois à _build_dressing (position ISO déjà projetée — fixe, comme
# _lane_dash_iso), le draw n'indexe plus que des Packed*Array. Les CHEVRONS restent à part
# (Dictionary, `segs` déjà projetés/clippés par _clip_relief) — trop peu nombreux pour valoir la
# conversion, et leur géométrie n'est pas un simple rect texturé. PAS de MultiMesh ici (chantier
# séparé, cf. TROUVAILLES) : on garde draw_texture_rect, juste sans Dictionary ni re-projection.
var _dress_fast_ip := PackedVector2Array()   ## position ISO pré-projetée (parallèle aux tableaux suivants)
var _dress_fast_tex: Array = []              ## Texture2D déjà résolue (_dress_get, une fois)
var _dress_fast_h := PackedFloat32Array()    ## hauteur MONDE de base (_dress_size(id) × scale), /zoom au draw
var _dress_fast_wide := PackedByteArray()    ## 1 si sprite 2:1 (serpent de mer) — évite un begins_with()/frame
var _dress_fast_col := PackedColorArray()    ## teinte finale déjà résolue (tint lot 6, sinon dress/egg alpha)
var _dress_relief := []                      ## CHEVRONS seuls (Dictionary — segs déjà projetés/clippés)
var _geonames := []       ## GeoNames.build — les ENSEMBLES nommés (forêts/lacs/rivières/massifs)
var _geo_dirty := true    ## re-nommer à la génération (déterministe par graine, display-only)
var _dress_clear := []    ## [[Vector2, r²]] — la CLAIRIÈRE des bourgs (aucune marque dedans)
var _canopy_batches := [] ## [{mm: MultiMesh, tex}] — la canopée servie en MULTIMESH (un draw/essence),
                          ## rebâtie avec le dressing ; instances en espace MONDE (coût par-frame nul)
var _canopy_mesh: ArrayMesh = null ## quad partagé des arbres (pied à l'origine, y vers le bas)
var fog_off := false      ## PROBE SEULEMENT (shot_parch fog=0) : saute le VOILE de guerre — pour
                          ## PHOTOGRAPHIER routes/lanes dans un monde probe où le « joueur » passif
                          ## ne connaît presque rien (display-only, jamais posé par le jeu)
var nature_mode := false  ## MODE NATURE : on ne montre QUE le terrain + le dressing (pas de frontières/
                          ## villes/routes/armées/noms) — la carte « vierge », touche N. Display-only.
var _country_names := []  ## nom de chaque pays (figé au générate) — pour les étiquettes d'empire
## POLISH #5 (revue 2026-07-21) : l'ACP des étiquettes d'empire (boucle régions×pays +
## covariance) tournait À CHAQUE _draw — cachée ici {c → {valid, ip, ang}}, invalidée
## avec les MÊMES signaux que le fog/les frontières (souveraineté ou visibilité qui bouge).
var _name_anchor := {}
var _names_dirty := true
var _borders := {}        ## 0 = TRAME FINE (provinces+régions) → PackedVector2Array jittée
var _fine_proj := PackedVector2Array()  ## TRAME FINE déjà projetée iso (revue #5, cache _rebuild_borders)
# DÉGRADÉ de frontière : un RUBAN par entité, BLENDÉ (N couches du ton EXTÉRIEUR au ton INTÉRIEUR,
# décalées le long de la normale → vrai dégradé, pas deux traits posés). OUTLINE = CULTURE (héritage,
# 6 familles + variation RGB par pays) ; INLINE = ÉTHOS (axe martial↔ordre, fluide). Cités-états or↔argent.
var _b_segs := {}         ## entité → PackedVector2Array : segments de frontière (jittés)
var _b_norm := {}         ## entité → PackedVector2Array : normale vers l'INTÉRIEUR, 1 par segment
# ── PROJECTION EN CACHE (revue overlay #5) : _project_segs_iso (mv.iso_pos, une projection MONDE→
# ISO FIXE, indépendante de la caméra) et les 3 couches de lavis intérieur (offsets FIXES en monde,
# eux aussi indépendants du zoom) étaient recalculés CHAQUE FRAME pour CHAQUE entité. Précalculés
# ici à _rebuild_borders (même cadence que _b_segs/_b_norm) ; le draw n'indexe plus qu'un tableau.
# ⚠ _draw_cap_lisere N'EST PAS traité pareil : son décalage (1.4/zoom) DÉPEND du zoom courant — le
# mettre en cache figerait le liseré à un zoom, visible dès qu'on zoome/dézoome. Laissé tel quel.
var _b_proj := {}         ## entité → PackedVector2Array : OUTLINE déjà projeté (parallèle à _b_segs)
var _b_wash := {}         ## entité → Array[PackedVector2Array] : les 3 couches de lavis, déjà projetées
var _cap_segs := {}       ## pays → PackedVector2Array : contour de sa CAPITALE (liseré pourpre)
var _cap_norm := {}       ## pays → PackedVector2Array : normale intérieure du contour capitale
var _war_regions := {}    ## W-GUERRE UI (lot A) : région → {state:1/2, belligerent, poly} — sièges/occupations, recalculé au tick
# ── VILLES EN CACHE (revue overlay #4, motif _war_regions) : la liste des bourgs à dessiner
# (tier/owner/rôle/pop/fog/siège) refaisait region_count() × (region_tier+region_owner+
# country_role+region_pop+fog+seat) + un sort_custom CHAQUE FRAME — invisible au joueur (mêmes
# résultats image après image la plupart du temps) mais payé en boucle, en appels DLL croisés.
# Reconstruite à _refresh_setts (appelée paresseusement depuis _draw_iso quand dirty — même idiome
# que _borders_dirty/_dressing_dirty dans ce fichier), dirty aux MÊMES signaux que le fog/les
# frontières (souveraineté, année). Projection ISO incluse dans le cache (elle est FIXE — même
# preuve que _lane_dash_iso) : seul le test de visibilité ÉCRAN (zoom/pan courants) reste par-frame.
var _setts := []          ## [{r, role, ctr, ip}] déjà filtré (tier/owner/pop/fog) et trié fond→avant
var _setts_dirty := true
# PALETTE de PIGMENTS LIMITÉE (anti-néon) : des encres NATURELLES choisies à la main (terre de Sienne,
# ocre, ardoise, olive…), pas un échantillonnage de la roue HSV (qui donne des bleus/magentas fluo même
# désaturés). On reste dans une gamme TERREUSE compatible parchemin → fin de l'effet cyberpunk.
const PARCHMENT := Color(0.80, 0.72, 0.54)       ## le ton du papier (toutes les teintes y tendent)
const CAP_INK := Color(0.40, 0.24, 0.36)         ## pourpre SOURD (liseré FIN de capitale, pas une bande)
# FRONTIÈRES en ENCRE GRAVÉE (atlas, pas grille de jeu) : la PROVINCE est un cheveu brun sombre
# (administrative, discrète) ; le PAYS est une DOUBLE PASSE — halo brun très sombre LARGE (le « creux
# gravé ») + pigment politique FIN dessus. Hiérarchie : province émerge, pays domine, sans noir massif.
const PROV_INK := Color(0.165, 0.141, 0.098)     ## #2a2419 brun sombre — frontière de PROVINCE (gravée, ~35 %)
const POL_HALO := Color(0.090, 0.067, 0.043)     ## #17110b brun très sombre — halo LARGE sous le trait de PAYS
# ── ÉPAISSEUR DE TRAIT ADAPTATIVE AU ZOOM (inspirée de CK3) ────────────────────────────────────────
# CK bake ses bordures en géométrie MONDE (elles GROSSISSENT à l'écran quand la caméra descend) puis les
# fond par un shader d'opacité SÉPARÉ (camera.fxh / pdxverticalborder). Un overlay 2D ne peut pas rebaker
# une géométrie par frame → on imite le RENDU : `_w(zoom, base, min, max)` = clamp(base·zoom, min, max)/zoom.
#   - zoom OUT  : plancher min_px → le trait ne DISPARAÎT jamais au plan large.
#   - APPROCHE  : largeur = base (CONSTANTE en monde) → le trait est SOUDÉ au terrain et s'épaissit à l'écran.
#   - zoom IN   : plafond max_px → au zoom profond une bordure n'AVALE jamais une province.
# L'opacité (fondu de la trame fine, gates routes/villes) reste la couche de visibilité INDÉPENDANTE (CK).
# HIÉRARCHIE par asymétrie de rails : la bande d'EMPIRE respire fort (toujours lisible) ; la trame de
# provinces reste un cheveu (≤1.3px) gouvernée par son fondu → l'empire DOMINE, la province ÉMERGE.
# largeurs zoom-adaptatives (_w) du trait de PAYS : halo LARGE + pigment FIN (la province reste un cheveu).
const POL_HALO_BASE := 0.85  ## creux gravé sous l'outline (≈ 2.0→2.4 px écran) — discret
const POL_HALO_MIN  := 2.0   ## plancher px (survit au plan large)
const POL_HALO_MAX  := 2.4   ## plafond px SERRÉ : plus jamais un boudin
const POL_PIG_BASE  := 0.45  ## l'OUTLINE d'éthos net (≈ 1.2→1.5 px écran)
const POL_PIG_MIN   := 1.2
const POL_PIG_MAX   := 1.5
## OUTLINE par HÉRITAGE (6 cultures) : Éso · Métal · Méca · Adapt · Agra · Clan — ENCRES SOMBRES terreuses.
const HERITAGE_PIG := [
	Color(0.31, 0.35, 0.42),   ## Ésotérique  : ardoise (bleu-gris sourd)
	Color(0.46, 0.29, 0.23),   ## Métallurgiste : rouille de fer
	Color(0.48, 0.35, 0.21),   ## Mécaniste   : terre de Sienne brûlée
	Color(0.33, 0.37, 0.25),   ## Adaptatif   : olive
	Color(0.47, 0.39, 0.22),   ## Agraire     : ocre brun
	Color(0.42, 0.28, 0.33),   ## Clanique    : prune sourde
]
# LISSAGE des frontières (escaliers → courbes) : on casse la FRÉQUENCE de l'escalier (ré-échantillonnage
# + passe-bas TAUBIN), PUIS on arrondit (Chaikin). ⚠ Taubin et NON Laplacien pur : le Laplacien RÉTRÉCIT
# les boucles vers leur centre (cumulatif) → les frontières dérivaient de leur vraie ligne et BULGEAIENT
# par-dessus les VILLES (« placement avalé »). Taubin alterne un pas adoucissant (λ) et un pas regonflant
# (μ) → lisse SANS rétrécir : la frontière reste sur la diagonale MOYENNE de l'escalier = sa vraie ligne.
const SMOOTH_RESAMPLE := 1.4  ## pas de ré-échantillonnage (cellules) — RESSERRÉ (2.0 gonflait les petites
                              ## provinces en BLOBS qui débordaient leur territoire)
const SMOOTH_TAUBIN := 4      ## itérations Taubin λ|μ (passe-bas non-rétrécissant) — allégé (anti-blob)
const SMOOTH_CHAIKIN := 2     ## passes de corner-cutting (arrondi final de la courbe)
const TAUBIN_LAMBDA := 0.5    ## pas adoucissant (>0)
const TAUBIN_MU := -0.53      ## pas regonflant (<0, |μ|>λ) → compense le rétrécissement du pas λ
var _borders_dirty := true ## la souveraineté a bougé (conquête/colonisation) → refaire les frontières
var _owner_sig := -1      ## signature de la photo des propriétaires → détecte le changement de souveraineté
# ── LAVIS POLITIQUE (aquarelle de territoire) : owner/cellule teinté au PIGMENT d'entité (une seule
# famille de couleur : lavis = frontière = armée = nom), bâti en C++ (political_image), rebâti avec les
# frontières (même signal de souveraineté). Fort au plan LARGE (la lecture politique du fit), s'efface
# vers le zoom profond (le terrain parle). Transparent hors territoire — le parchemin transparaît. ──
var _pol_tex: ImageTexture = null
## DEUX RÉGIMES (références de DA) : dézoomé = EU4 — le POLITIQUE domine (aplats presque
## pleins, grands noms) ; zoomé = KCD — le TERRAIN domine (politique quasi absent, bannières
## de lieux). La bascule vit ici (lavis) + dans les noms (fondu) + les bannières (éclosion).
const WASH_A_FAR  := 0.72  ## aplat politique au plan large (EU4 : on lit un atlas politique)
const WASH_A_NEAR := 0.06  ## ... au plan rapproché (KCD : le parchemin terrain reprend tout)
const WASH_FADE_LO := 1.8  ## zoom où l'aplat commence à céder
const WASH_FADE_HI := 4.5  ## zoom où le terrain a (presque) tout repris
# ── BROUILLARD DE GUERRE (étape 1/2, VISUEL SEULEMENT — aucune décision de sim n'en
# dépend côté moteur) : un voile d'encre sombre ESTOMPÉ (jamais noir pur, esprit
# parchemin) sur ce que le joueur ne connaît pas encore ; grain RÉGION (_fog_mask,
# 0/1) pour griser/cacher villes, armées et noms ENNEMIS tombant dans le voile — les
# tiens (owner==player) restent TOUJOURS visibles. Le brouillard a son PROPRE cycle :
# la visibilité change aussi au passage d'une année/ère, sans mouvement de frontière. ──
var _fog_tex: ImageTexture = null
var _fog_mask: PackedByteArray = PackedByteArray()   ## région → 0 voilée / 1 visible (vide = pas encore bâti → fail-open)
var _fog_dirty := true
var _fog_year := -1
## VRAI si la région `r` est visible (fail-open tant que _fog_mask n'a pas encore été bâti).
func _fog_visible_region(r: int) -> bool:
	if _fog_mask.is_empty() or r < 0 or r >= _fog_mask.size():
		return true
	return _fog_mask[r] != 0
# ── SÉLECTION : contour DORÉ de la province choisie (le grain de panneau, charte EU4) ──
var _sel_prov_cache := -2
var _sel_segs := PackedVector2Array()
var _sel_proj := PackedVector2Array()   ## _sel_segs déjà projeté iso (revue #5 : même fix que les frontières)
const SEL_GOLD := Color(0.86, 0.68, 0.26)   ## or de sélection (net, au-dessus du creux d'encre)
var _roads := []          ## [{points, level, nprov, key}] — réseau de routes (façade + méta locale)
var _road_clear := {}     ## TROUÉE : cellule (x<<16|y) → densité de canopée [0..1[ près d'une route
var _road_clear_n := -1   ## taille du masque au dernier semis (déclencheur de re-semis par àcoups)
var _chain_geo := {}      ## CHAÎNAGE : paire → hash de géométrie canonique (mesure de stabilité entre rebuilds)
var _road_start := {}     ## clé de route → ANNÉE de début de chantier (croissance 1 an/province)
var _roads_dirty := true  ## le réseau commercial a pu bouger → recharger les routes
var _road_net := {}       ## ANTISPAG cache : polylignes consolidées (dédup + tier d'épaisseur), voir _ensure_road_network
var _road_net_valid := false  ## false ⇒ à reconstruire (réseau changé OU un chantier grandit encore)
var _lanes := []          ## PORTULAN : [{points, open, choke, ra, rb}] — lanes maritimes (sea_paths + méta)
var _lane_dashes := []    ## par lane : PackedVector2Array de PAIRES iso (tirets prêts pour draw_multiline)
var _lanes_dirty := true  ## le commerce maritime a pu bouger → recharger les lanes
var _lanes_day := -999999 ## jour sim du dernier rafraîchissement (re-poll SEA_LANE_POLL_DAYS)
var _rivers := []         ## [Vector3(x, y, ang)] — nuage de points (façade) gardé pour l'anti-bâti SUR le fil
var _river_hash := {}     ## hash spatial du fil de rivière (Vector2 par cellule) — snap des frontières
var _mv: Node2D = null    ## le MapView parent (porte la projection GLOBE monde→écran)
var _himg_l: Image = null ## couche HEIGHT (cache local) → ombrage cohérent des assets/routes
var _alb_l: Image = null  ## terrain albedo (cache) → couleur/luminosité du SOL sous l'asset

# RIVIÈRES : CARVÉES DANS LE TERRAIN (worldgen), pas un asset par-dessus. Le shader iso_blend lit
# un champ de débit (bâti par iso_ground._build_river_field depuis `river_paths`, fusionné par baie)
# et rend l'eau DANS le relief — cœur propre, berges fondues, continu jusqu'à la mer. L'overlay n'en
# garde QUE le nuage de points (`_rivers`) pour interdire de BÂTIR sur le fil.
const ROAD_ZOOM_MIN := 1.6    ## routes (zoom ISO) — dès le lointain, seules les GRANDES bandes (gates fins par couche, cf. ROAD_Z_*)
# CHEMIN DE TERRE À 3 TRAITS (le motif cartographique classique — KCD/atlas) : un
# sous-trait sépia sombre (l'ombre du creux), le corps CRÈME pâle (la terre battue),
# un filet clair central. Le pointillé « carte au trésor » s'émiettait au zoom.
## GLACIS : le crème quasi-blanc « brillait » sur le lavis — rabattu vers le ton parchemin,
## alphas baissés (la route est une trace DANS la carte, pas un ruban posé dessus).
## REFONTE v2 2026-07-31 (spec joueur détaillée) : une route N'EST PAS un trait d'encre
## (ça, c'est la frontière) — c'est du SOL ÉCLAIRCI ET USÉ. Trois lectures :
##   GRANDE ROUTE   = bande claire parchemin + 2 ornières brunes fines discontinues ;
##   ROUTE RÉGIONALE = bande claire plus étroite + 1 ornière discontinue ;
##   SENTIER        = trace brune irrégulière seule, très fine, zoom proche seulement.
## L'usure (tier de multiplicité) fonce/densifie la bande — « plus usé, pas plus épais ».
## Ruptures et longueurs de tiret JITTÉES (hash déterministe) : bords irréguliers, jamais
## de liseré continu. Les 2-3 dernières cellules vers la ville restent CONTINUES et
## finissent sur un PAD de terre battue (la route sort de la porte). En forêt, la bande
## devient le SOL de la TROUÉE (canopée écartée, cf. _build_dressing).
const ROAD_BAND       := Color(0.91, 0.85, 0.70)        ## parchemin poussiéreux (bande)
const ROAD_RUT        := Color(0.38, 0.25, 0.14, 0.50)  ## ornière brune
const ROAD_TRAIL      := Color(0.42, 0.28, 0.16, 0.42)  ## trace de sentier
const ROAD_BAND_A     := 0.30   ## alpha bande hors forêt (l'usure s'y AJOUTE) — −29 % (retour joueur : « autoroute lumineuse »)
const ROAD_BAND_A_F   := 0.40   ## en forêt : le sol de la trouée, PLUS visible (−27 %)
const ROAD_WEAR_A     := 0.09   ## usure : +alpha par palier (la multiplicité FONCE la terre, jamais 2 bandes)
const ROAD_BAND_DASH  := 15.0   ## bande : longs pans avec petites RUPTURES (unités iso)
const ROAD_BAND_GAP   := 0.9
const ROAD_RUT_DASH   := 3.4    ## ornière : discontinue, courte
const ROAD_RUT_GAP    := 1.7
const ROAD_TRAIL_DASH := 1.8    ## sentier : trace hachée
const ROAD_TRAIL_GAP  := 2.1
const ROAD_JIT        := 0.55   ## jitter des longueurs (±55 % hashé — l'irrégulier)
const ROAD_SOLID_END  := 2.6    ## cellules CONTINUES aux abouts (la porte du bourg)
const ROAD_Z_BAND_MAIN  := 1.6  ## grandes routes : bandes visibles dès le zoom lointain
const ROAD_Z_BAND_MINOR := 2.6  ## régionales : zoom moyen
const ROAD_Z_RUT        := 2.8  ## les ornières apparaissent
const ROAD_Z_TRAIL      := 4.2  ## sentiers : zoom proche
const ROAD_Z_BRIDGE     := 3.2  ## tablier/culées
const ROAD_Z_RAIL       := 5.0  ## garde-corps : zoom profond seulement
const ROAD_FOREST_A := 0.38   ## SOUS LA CANOPÉE : la route se devine — relevé depuis la canopée ×10
                              ## (à 0.22 le massif PLEIN l'avalait tout à fait)
# Traitement FRONT-END du tracé (l'A* moteur reste la vérité ; on en lisse la SORTIE, hors tick) :
#  · SNAP : raccord d'extrémité PROPRE (trim des points qui tanglent près de l'ancre de ville) ;
#  · PATHFINDING (rendu) : ré-échantillonnage à PAS CONSTANT + Chaikin GARDÉ-EAU (courbe nette qui
#    épouse la côte sans la couper) ;
#  · ASSETS : mobilier semé à l'ARC (espacement RÉGULIER, indépendant de la densité de points).
const ROAD_RESAMPLE := 2.0       ## pas d'échantillonnage du tracé (cellules) → points réguliers
const ROAD_SNAP_TRIM := 4.5      ## rayon de nettoyage des points près de l'ancre de ville (cellules)
# ── PORTULAN (mission MARITIME N4) : les LANES maritimes en POINTILLÉS d'encre — la
# convention des portulans (actée joueur). Plus FINES que les routes terrestres, ancrées
# aux ports par le snap existant, JAMAIS sur un lac (l'A* moteur les exclut : cellules
# MER seulement), sous le fog RIEN (les deux bouts doivent être connus). Le tracé vient
# du MOTEUR (sea_paths(), cache par signature du commerce maritime — la membrane : des
# coordonnées) ; ici on ne fait que lisser GARDÉ-MER, ancrer, magnétiser et pointiller.
const SEA_LANE_INK := Color(0.24, 0.20, 0.28, 0.62)  ## l'encre froide du trait de mer
const SEA_LANE_DASH := 1.1        ## longueur d'un tiret (cellules monde)
const SEA_LANE_GAP := 0.9         ## le blanc entre tirets
const SEA_LANE_MAGNET_PASSES := 2 ## magnétisme MARIN (mêmes réglages ANTISPAG, bundle lanes seul)
const SEA_LANE_POLL_DAYS := 180   ## re-poll du commerce maritime (il évolue sans conquête)
# ── ANTISPAG (2026-07-17) : consolidation VISUELLE des routes en TRONCS — display-only, aucune
# sémantique. Le magnétisme de couloir existait déjà (nearest-point glouton, rayon 0.65 cellule,
# voisinage ±1, une seule passe) : renforcé (rayon → ROAD_MAGNET_R, ±ROAD_MAGNET_RING, 2 passes).
# ESSAYÉ PUIS ABANDONNÉ (mesuré, cf. TROUVAILLES) : un CONSENSUS DE GRILLE (chaque point vote dans
# une cellule, ≥2 routes dans la cellule ⇒ toutes fusionnent sur le CENTROÏDE) — plus simple sur le
# papier, mais le centroïde n'est ni sur l'un ni l'autre tracé d'origine et n'a AUCUNE conscience de
# la continuité le long de la route : une cellule qui capte incidemment un CROISEMENT (pas un couloir
# partagé) tire un point isolé loin de ses voisins → zigzag artificiel qui a mesurément AUGMENTÉ le
# spaghetti (dump JSON + analyse hors-Godot : avant 1108 paires réelles [écart ≥0.25 cellule],
# grille 1878 — pire). Le glouton nearest-point, lui, ne réassigne un point QUE vers une position qui
# existe déjà sur une autre route (jamais une moyenne inédite) : plus prudent, mesuré meilleur. Rayon
# élargi jusqu'à 2.2 / 4 passes testé aussi (cf. TROUVAILLES) : plafonne vers ~47-48 % quel que soit
# le rayon au-delà de ~1.4 (2 passes suffisent — testé jusqu'à 4, aucun gain de plus) ; retenu R=1.4
# comme meilleur compromis efficacité/prudence (au-delà, rayon > un pas de rééchantillonnage entier
# = risque de coller des routes qui se croisent sans corridor commun, pour un gain marginal +1.6 pt).
const ROAD_MAGNET_R := 1.4               ## cellules — rayon de collage (était 0.65)
const ROAD_MAGNET_R2 := ROAD_MAGNET_R * ROAD_MAGNET_R
const ROAD_MAGNET_RING := 3              ## voisinage de cellules de hash sondé (couvre ROAD_MAGNET_R)
const ROAD_MAGNET_PASSES := 2            ## convergence : 2 passes suffisent (mesuré, cf. TROUVAILLES)
const ROAD_MULT_TIERS := 3               ## paliers d'épaisseur ∝ MULTIPLICITÉ (combien de routes logiques
                                          ## empruntent le même tronçon) — 1 capillaire, 2-3 dessertes, 4+ tronc
const ROAD_TIER_WSCALE := [1.0, 1.28, 1.55]  ## facteur d'épaisseur par palier (indices 0..2 ↔ tier 1..3)
const SPAG_DIST := 1.5                   ## cellules — métrique « spaghetti » : 2 segments à moins de ça…
const SPAG_COS := 0.90                   ## …et quasi-parallèles (cos > .90 ⇒ ~25°) sans être fusionnés…
const SPAG_MIN_OFFSET := 0.25            ## …ET séparés d'un ÉCART PERPENDICULAIRE réel (cf. découverte
## ci-dessous — sans ce filtre la mesure comptait aussi des paires SANS écart visible)
# NOTE (mission ROUTES, audit 2026-07-17) : une PREMIÈRE piste « routes en tuiles autotile cardinal
# + ponts en sprites modulaires » a été esquissée ici (consts ROADS_IN_SHADER/USE_ROAD_TILES/
# ROUTE_GRID_K/…, vars _road_tiles/_route_meshes/_bridge_tex/_bridges) puis ABANDONNÉE au profit du
# rendu vectoriel ci-dessous (chemin de terre à 3 traits + ponts d'ENCRE `_ink_bridges`, plus bas) —
# retirée ici car 100 % morte (0 lecture, seulement des écritures de flag) et trompeuse (le
# commentaire prétendait « overlay muet », faux : c'est CE fichier qui dessine tout). Voir TROUVAILLES.

func _ready() -> void:
	Sim.ticked.connect(_on_tick)
	Sim.generated.connect(_on_generated)
	if Sim.world != null:
		_set_rivers()
		_build_names()
		_build_anchors()
		_update_top_cap()               # la plus grande capitale du monde (vignette t7)
		_ensure_roads(Sim.world.year() > 0)   # monde mûr (save chargée) ⇒ routes déjà bâties
		_build_region_raws()            # brutes extraites par région (mode carte RESSOURCES)
	queue_redraw()

func _build_names() -> void:
	_country_names.clear()
	var w = Sim.world
	if w == null:
		return
	for c in range(w.country_count()):
		var info: Dictionary = w.country_info(c)
		_country_names.append(String(info.get("nom", "")))

## regroupe les ~15 invalidations « monde neuf/save chargée » (revue overlay #12) — un SEUL
## endroit à mettre à jour quand un futur cache apparaît, plutôt qu'un site de plus à chaque fois.
func _invalidate_all() -> void:
	_himg_l = null              # monde neuf → recharger les caches de lumière (relief + albedo)
	_alb_l = null
	_borders_dirty = true       # monde neuf → frontières ET routes à refaire
	_fog_dirty = true; _names_dirty = true           # monde neuf/save chargé → connaissance et rayon à recharger
	_fog_year = -1
	_fog_mask = PackedByteArray()
	_dressing_dirty = true      # … et le dressing de terrain (biome semé)
	_geo_dirty = true           # … et les ensembles nommés (forêts/lacs/rivières/massifs)
	_roads_dirty = true
	_road_start.clear()         # chantiers remis à zéro (le monde neuf rebâtit ses routes)
	_lanes_dirty = true         # PORTULAN : monde neuf → lanes maritimes à recharger
	_lanes = []
	_lane_dashes = []
	_lanes_day = -999999
	_region_label.clear()       # bannières de lieux : noms recachés (monde neuf)
	_town_cache.clear()         # urbaniste : plans de bourgs recalculés (routes neuves)
	_sea_img = null             # couches eau recachées (quais)
	_rf_img = null
	_bio_img = null             # couche biome recachée (routes sous canopée)
	_river_hash.clear()         # snap de frontières : fil de rivière re-haché (monde neuf)
	_owner_sig = -1
	_setts_dirty = true         # revue #4 : la liste de villes en cache aussi à refaire au monde neuf

func _on_generated() -> void:
	_set_rivers()
	_invalidate_all()
	_build_names()
	_build_anchors()
	_update_top_cap()           # la plus grande capitale du monde (vignette t7)
	_ensure_roads(Sim.world.year() > 0)   # an 0 (monde neuf) ⇒ croît ; an N (save/monde mûr) ⇒ déjà bâtie
	_build_region_raws()        # brutes extraites par région (mode carte RESSOURCES)
	queue_redraw()

## lit le nuage de points (anti-bâti) PUIS sélectionne les fleuves MAJEURS (tracé en ruban).
## Calculé 1× au générate, comme le reste du fil.
func _set_rivers() -> void:
	_rivers = Sim.world.river_points()    # gardé : l'anti-bâti (routes/quais) évite le fil de rivière

## pré-calcule la variante de ville TERRAIN de chaque région colonisée (échantillon
## du biome au centroïde ; l'hydro via le groupe de settlement) — pour les petits bourgs.
func _build_region_raws() -> void:
	_region_raws.clear()
	var w = Sim.world
	if w == null:
		return
	for r in range(w.region_count()):
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var pid: int = w.province_at(int(ctr.x), int(ctr.y))
		if pid < 0:
			continue
		var raws := []
		for line in w.province_income(pid):
			if bool(line.get("manufactured", false)):
				continue                                  # on ne veut QUE la brute extraite
			raws.append({"id": int(line.get("res_id", -1)), "name": String(line.get("source", ""))})
			if raws.size() >= 2:                          # règle moteur : 2 brutes/province
				break
		if not raws.is_empty():
			_region_raws[r] = raws

## MODE RESSOURCES (9) : l'icône de chaque brute extraite, à la tuile (centroïde projeté).
## Sprite si dispo, sinon une PASTILLE nommée (3 lettres) → couverture complète. Taille
## ÉCRAN-CONSTANTE (÷zoom) → lisible à tout niveau de zoom.
## ISO est le SEUL mode de rendu (revue overlay #2) : map_view.gd l'affirme lui-même
## (« Il n'y a plus de vue GLOBE 3D... un seul rendu, à tous les zooms ») et son
## globe_to_screen() est un stub COMPAT qui renvoie toujours vis=false — l'ancienne branche
## globe ici était donc déjà morte deux fois (jamais appelée, et n'aurait rien dessiné).
func _draw_resources(w, mv: Node2D) -> void:
	if _raws_dirty:
		_build_region_raws()         # rebâti à la demande (mode RESSOURCES seulement)
		_raws_dirty = false
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var zoom := maxf(0.01, vt.get_scale().x)
	var sz := 18.0 / zoom                               # MONDE → taille ÉCRAN constante
	for r in range(w.region_count()):
		var raws: Array = _region_raws.get(r, [])
		if raws.is_empty():
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var sp: Vector2 = vt * mv.iso_pos(ctr.x, ctr.y)
		if sp.x < -20 or sp.y < -20 or sp.x > vp.x + 20 or sp.y > vp.y + 20:
			continue
		var n := raws.size()
		for i in range(n):
			var rr: Dictionary = raws[i]
			var off := Vector2((float(i) - float(n - 1) * 0.5) * (sz + 2.0), 0.0)
			var tl := sp + off - Vector2(sz * 0.5, sz * 0.5)
			var spr := UIKit.resource_sprite(int(rr.get("id", -1)), String(rr.get("name", "")))
			if spr != null:
				draw_texture_rect(spr, Rect2(tl, Vector2(sz, sz)), false)
			else:                                          # pas de sprite : pastille nommée (couverture complète)
				draw_rect(Rect2(tl, Vector2(sz, sz)), Color(0.13, 0.11, 0.08, 0.92))
				draw_rect(Rect2(tl, Vector2(sz, sz)), VKit.COL_GOLD, false, 1.0)
				if sz >= 11.0:
					VKit.text(self, tl + Vector2(2.0, sz * 0.5 - 5.0), Color(0.92, 0.86, 0.70), String(rr.get("name", "?")).substr(0, 3), VKit.FS_SMALL)

## ANCRE (routes) + SIÈGE (vignette de bourg) de chaque région habitée. L'ancre est poussée
## vers l'intérieur sur les côtes (les ROUTES y aboutissent — le réseau ne change pas) ; le
## siège, lui, est le CENTROÏDE ancré au sec (recentrage 2026-07-08 — le chercheur d'eau déportait la vignette) — c'est là qu'elle pose.
func _build_anchors() -> void:
	_region_anchor.clear()
	_region_seat.clear()
	var w = Sim.world
	if w == null:
		return
	var sea: Image = w.layer_image(LAYER_WATER)   # mer OU lac : 0 = terre, ≥ 1 = eau
	var seaonly: Image = w.layer_image(LAYER_SEA) # mer SALÉE seule → distingue le lac (eau douce)
	var rf: Image = _carved_river_field()         # champ de débit des rivières (eau douce ; peut être null)
	for r in range(w.region_count()):
		var t: int = w.region_tier(r)
		var rl: int = int(w.country_role(w.region_owner(r))) if w.region_owner(r) >= 0 else -1
		if t < 0 and rl != 2 and rl != 4:
			continue                              # wilderness sans ville (mais on garde cité-état/hameau libre)
		# SIÈGE = centroïde de la PROVINCE REPRÉSENTATIVE (retour joueur 2026-07-09 : le
		# barycentre de la RÉGION entière tombait au bord sur les formes concaves — Bois
		# Blanc collé au coin NW de sa région). Repli centroïde région (vieille DLL).
		var ctr: Vector2 = w.region_seat(r) if w.has_method("region_seat") else w.region_centroid(r)
		if ctr.x < 0:
			continue
		var land := _snap_to_land(sea, ctr)
		var want := 10.0 + t * 4.0                         # assise VOULUE (∝ tier — réduite avec le scaling 2026-07-08)
		var best := land
		var best_sz := _max_dry_size(sea, land)
		# si l'assise ne tient pas (côte), POUSSE vers l'intérieur (à l'opposé de la mer)
		# jusqu'à trouver une assise qui la porte au sec — une cité s'asseoit en RETRAIT
		# de son rivage (naturel), plutôt que de rapetisser en pastille.
		var sdir := _nearest_sea_dir(sea, land, 8)
		if sdir != Vector2.ZERO and best_sz < want:
			for push in [2.0, 4.0, 6.0, 9.0, 12.0]:
				var cand := _snap_to_land(sea, land - sdir * push)
				var sz := _max_dry_size(sea, cand)
				if sz > best_sz:
					best_sz = sz
					best = cand
				if best_sz >= want:
					break
		_region_anchor[r] = best
		_region_seat[r] = best  # RECENTRAGE (retour joueur 2026-07-08) : le siège = le CENTROÏDE ancré au sec (le chercheur d'eau douce/rivage déportait la vignette hors du cœur de sa province)

## la CAPITALE LA PLUS PEUPLÉE du monde (unique) → la vignette t7 (cité impériale). Recalculée
## au tick (une boucle pays, bon marché) ; le cache de bourg suit tout seul — la clé `sid`
## d'une région change quand le titre change de mains, et son plan se rebâtit au dessin.
func _update_top_cap() -> void:
	var w = Sim.world
	_top_cap_region = -1
	if w == null:
		return
	var bp := 0
	for c in range(w.country_count()):
		var role := int(w.country_role(c))
		if role == 2 or role == 4:
			continue                              # cité-état/hameau libre : vignettes dédiées, hors course
		var cap: int = w.province_region(w.country_capital_province(c))
		if cap < 0:
			continue
		var p := int(w.region_pop(cap))
		if p > bp:
			bp = p
			_top_cap_region = cap

## rend la cellule de TERRE (sea < 1) la plus proche de `c` (anneaux croissants,
## comme settle_land_spot). Renvoie `c` tel quel si aucune terre à portée.
func _snap_to_land(sea: Image, c: Vector2) -> Vector2:
	if sea == null:
		return c
	var sw := sea.get_width()
	var sh := sea.get_height()
	var cx := int(c.x)
	var cy := int(c.y)
	for R in range(0, 15):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if R > 0 and absi(dx) != R and absi(dy) != R:
					continue                       # bord d'anneau seulement
				var nx := cx + dx
				var ny := cy + dy
				if nx < 0 or ny < 0 or nx >= sw or ny >= sh:
					continue
				if int(sea.get_pixel(nx, ny).r * 255.0 + 0.5) < 1:
					return Vector2(nx, ny)
	return c


## direction NORMALISÉE vers la mer la plus proche (≤ maxrad), Vector2.ZERO si aucune.
func _nearest_sea_dir(sea: Image, c: Vector2, maxrad: int) -> Vector2:
	if sea == null:
		return Vector2.ZERO
	var sw := sea.get_width()
	var sh := sea.get_height()
	var cx := int(c.x)
	var cy := int(c.y)
	for R in range(1, maxrad + 1):
		for dy in range(-R, R + 1):
			for dx in range(-R, R + 1):
				if absi(dx) != R and absi(dy) != R:
					continue
				var nx := cx + dx
				var ny := cy + dy
				if nx < 0 or ny < 0 or nx >= sw or ny >= sh:
					continue
				if int(sea.get_pixel(nx, ny).r * 255.0 + 0.5) >= 1:
					return Vector2(dx, dy).normalized()
	return Vector2.ZERO

## VRAI ssi le RECTANGLE du sprite (ancré au PIED en `base`, large de 2·halfw, montant
## de `up` vers le nord) est ENTIÈREMENT au sec — balayage DENSE de chaque cellule (pas
## un échantillon clairsemé : un seul pixel d'eau sous le sprite suffit à le refuser).
func _sea_clear_rect(sea: Image, base: Vector2, halfw: float, up: float) -> bool:
	if sea == null:
		return true
	var sw := sea.get_width()
	var sh := sea.get_height()
	var x0 := int(floor(base.x - halfw))
	var x1 := int(ceil(base.x + halfw))
	var y0 := int(floor(base.y - up))
	var y1 := int(ceil(base.y))
	var y := y0
	while y <= y1:
		if y >= 0 and y < sh:
			var x := x0
			while x <= x1:
				if x >= 0 and x < sw:
					if int(sea.get_pixel(x, y).r * 255.0 + 0.5) >= 1:
						return false
				x += 1
		y += 1
	return true

## plus grande taille de sprite carré (sz large, sz haut, ancré au pied) TENANT au sec
## à `base` — on essaie des tailles CROISSANTES et on s'arrête au premier débord.
func _max_dry_size(sea: Image, base: Vector2) -> float:
	var best := 0.0
	for sz in [6.0, 8.0, 12.0, 16.0, 22.0, 28.0, 34.0, 40.0, 46.0]:
		if _sea_clear_rect(sea, base, sz * 0.5, sz):
			best = sz
		else:
			break
	return best

## diff de signature de souveraineté PARTAGÉ (revue overlay #11 : ce test + son jeu de flags
## était DUPLIQUÉ mot pour mot entre _on_tick et le poll ~4×/s de _process/_sig_poll — UN
## seul endroit désormais). Renvoie VRAI si la souveraineté a bougé (conquête/colonisation).
func _poll_world_changes() -> bool:
	if Sim.world == null:
		return false
	var sig := _owner_signature(Sim.world)
	if sig == _owner_sig:
		return false
	_owner_sig = sig           # refaire frontières, villes, ET réseau de routes/lanes (villes neuves/captées)
	_borders_dirty = true
	_fog_dirty = true; _names_dirty = true      # les sources de visibilité territoriales ont bougé
	_roads_dirty = true
	_lanes_dirty = true        # PORTULAN : un port conquis/fondé peut recâbler le commerce
	_setts_dirty = true        # revue #4 : villes (owner/rôle) à refaire aussi
	return true

func _on_tick(year: int) -> void:
	_raws_dirty = true         # l'extraction a pu s'établir (an-0 nu) → recache les brutes au prochain dessin RESSOURCES
	_update_top_cap()          # le titre de « plus grande capitale » peut changer → la vignette t7 suit
	_poll_world_changes()
	if year != _fog_year:
		_fog_dirty = true; _names_dirty = true      # rayon d'exploration/ère et connaissance évoluent annuellement
		_setts_dirty = true     # revue #4 : pop/tier peuvent avoir franchi un seuil — même cadence que le fog
	if Sim.day_count - _lanes_day >= SEA_LANE_POLL_DAYS:
		_lanes_dirty = true    # PORTULAN : le commerce maritime évolue SANS conquête (routes
		                       # ordonnées/ouvertes au fil des ans) → re-poll semestriel ; le
		                       # cache moteur (signature) rend le poll quasi gratuit si rien n'a bougé
	_ensure_roads()            # date les chantiers neufs dès maintenant (même non zoomé)
	_refresh_war_regions()     # W-GUERRE UI (lot A) : sièges/occupations bougent AU TICK, pas aux frontières
	queue_redraw()

## W-GUERRE UI (lot A) — recense les régions ASSIÉGÉES(1)/OCCUPÉES(2) (rare : quelques
## régions à la fois) et leur POLYGONE (region_border_segments, comme le liseré capitale)
## pour clipper les hachures. Recalculé CHAQUE TICK (l'état de siège bouge vite — pas
## seulement aux frontières de souveraineté) ; le scan est un aller de int, bon marché.
func _refresh_war_regions() -> void:
	var w = Sim.world
	_war_regions.clear()
	if w == null or not w.has_method("region_war_state"):
		return
	for r in range(w.region_count()):
		var ws: Dictionary = w.region_war_state(r)
		var st := int(ws.get("state", 0))
		if st <= 0:
			continue
		var polys := []
		if w.has_method("region_border_segments"):
			var rc: Dictionary = w.region_border_segments(r)
			var flat: PackedVector2Array = rc.get("pts", PackedVector2Array())
			if flat.size() >= 2:
				polys = _chain_segments(flat)   # segments non ordonnés (bseg) → anneau(x) fermé(s)
		# POLISH #5 (revue 2026-07-21) : le CLIPPING des hachures (Geometry2D.intersect par
		# trait × poly) vivait dans _draw_war_hatch — À CHAQUE FRAME. Les segments monde ne
		# dépendent ni du zoom ni de la caméra : précalculés ICI (au rebuild, cadence tick),
		# le draw ne fait plus que projeter/tracer.
		_war_regions[r] = {"state": st, "belligerent": int(ws.get("belligerent", -1)),
			"polys": polys, "hatch": _hatch_segments(polys)}

## VILLES (lot U), motif _war_regions (revue overlay #4) : la LISTE des bourgs éligibles
## (tier/owner/rôle/pop/fog/siège) tournait CHAQUE FRAME dans _draw_iso — region_count() ×
## (region_tier+region_owner+country_role+region_pop+fog+seat) + un sort_custom, en appels DLL
## croisés, pour un résultat qui ne bouge presque jamais d'une image à l'autre. Reconstruite ici,
## appelée paresseusement par _draw_iso quand `_setts_dirty` (dirtée aux mêmes signaux que le
## fog/les frontières — cf. _poll_world_changes/_on_tick). La projection ISO (`ip`) est incluse :
## FIXE (mv.iso_pos ne dépend pas de la caméra — même preuve que _lane_dash_iso) ; seul le test de
## visibilité ÉCRAN (zoom/pan COURANTS) doit rester par-frame, sur cette liste déjà filtrée/triée.
func _refresh_setts() -> void:
	_setts.clear()
	_setts_dirty = false
	var w = Sim.world
	if w == null:
		return
	var mv := _mv_ref()
	if mv == null:
		return
	var human_idx := int(w.player())
	for r in range(w.region_count()):
		var tier: int = w.region_tier(r)
		var owner: int = w.region_owner(r)
		var role: int = int(w.country_role(owner)) if owner >= 0 else -1
		# un BOURG demande des HABITANTS (≥150 âmes) et un propriétaire — plus de villes
		# fantômes sur la terre vide ; cité-état (2) & hameau libre (4) toujours tracés.
		if (tier < 0 or owner < 0 or int(w.region_pop(r)) < 150) and role != 2 and role != 4:
			continue
		# BROUILLARD DE GUERRE (étape 1/2) : un bourg ENNEMI tombant dans le voile ne se
		# dessine pas — les tiens (owner==human_idx) restent TOUJOURS visibles.
		if owner != human_idx and not _fog_visible_region(r):
			continue
		var ctr: Vector2 = _region_seat.get(r, w.region_centroid(r))
		if ctr.x < 0:
			continue
		var ip: Vector2 = mv.iso_pos(ctr.x, ctr.y)
		_setts.append({"r": r, "role": role, "ctr": ctr, "ip": ip})
	_setts.sort_custom(func(a, b): return (a["ip"] as Vector2).y < (b["ip"] as Vector2).y)

## signature de la photo des propriétaires → détecte conquête/colonisation. Le compte de
## provinces COLONISÉES y entre : une colonisation INTRA-région ne bouge pas l'owner agrégé
## de région — sans lui, le lavis/frontières (grain PROVINCE, charte) ne se rebâtiraient pas.
func _owner_signature(w) -> int:
	if w == null:
		return -1
	var sig := 0
	for r in range(w.region_count()):
		sig = (sig * 1000003 + (w.region_owner(r) + 2)) & 0x3fffffff
	if w.has_method("colonized_total"):
		sig = (sig * 1000003 + int(w.colonized_total())) & 0x3fffffff
	return sig

## Recharge le voile ET son masque régional indépendamment des frontières. Le moteur
## fait évoluer la connaissance/rayon chaque année : l'owner-signature ne suffit pas.
func _refresh_fog() -> void:
	var w = Sim.world
	if w == null:
		return
	if w.has_method("fog_image"):
		var fimg: Image = w.fog_image()
		if fimg != null:
			if _fog_tex == null or _fog_tex.get_size() != Vector2(fimg.get_size()):
				_fog_tex = ImageTexture.create_from_image(fimg)
			else:
				_fog_tex.update(fimg)
	if w.has_method("fog_region_mask"):
		_fog_mask = w.fog_region_mask()
	_fog_year = int(w.year())
	_fog_dirty = false

## reconstruit les segments de frontière (région + pays) depuis la façade (port bseg).
## `mv` (revue overlay #5) : les segments et les couches de lavis sont désormais projetés EN ISO
## ICI (une fois, à la souveraineté) plutôt qu'à chaque frame de _draw_iso — cf. _b_proj/_b_wash.
func _rebuild_borders(mv: Node2D) -> void:
	var w = Sim.world
	if w == null:
		return
	# TRAME FINE (provinces 0 + régions 1) : SEULEMENT là où la civilisation touche — un joint
	# dont les DEUX rives sont vierges (owner<0 et other<0) n'apprend rien au joueur et noyait
	# la carte sous un filet de « boue craquelée » sur toute la terre sauvage. Puis CHAÎNÉE en
	# polylignes ordonnées et lissée → courbes (fin de l'escalier).
	var fine_raw := PackedVector2Array()
	for lvl in [0, 1]:
		var fd: Dictionary = w.border_segments_col(lvl)
		var fp: PackedVector2Array = fd.get("pts", PackedVector2Array())
		var fo: PackedInt32Array = fd.get("owner", PackedInt32Array())
		var ft: PackedInt32Array = fd.get("other", PackedInt32Array())
		for i in range(fo.size()):
			if fo[i] < 0 and (i >= ft.size() or ft[i] < 0):
				continue                                   # terre vierge des deux côtés : muette
			fine_raw.push_back(fp[i * 2]); fine_raw.push_back(fp[i * 2 + 1])
	var fine := PackedVector2Array()
	for ch in _chain_segments(fine_raw):
		var poly: PackedVector2Array = _smooth_poly(ch)
		for i in range(poly.size() - 1):
			fine.push_back(poly[i]); fine.push_back(poly[i + 1])
	_borders[0] = fine
	_fine_proj = _project_segs_iso(mv, fine)   # revue #5 : projection FIXE (mv.iso_pos monde→iso), cachée ici
	# BLOCS (2) en RUBAN int.→ext. : par ENTITÉ, on bâtit l'INLINE (décalé vers l'intérieur, ton clair)
	# et l'OUTLINE (sur l'arête, ton foncé), le long de la normale extérieure. La façade exclut les côtes
	# d'EMPIRE (le rivage suffit) mais GARDE celles des cités-états (leur ruban or-argent doit se voir).
	_b_segs.clear()
	_b_norm.clear()
	var cd: Dictionary = w.border_segments_col(2)
	var pts: PackedVector2Array = cd.get("pts", PackedVector2Array())
	var nrm: PackedVector2Array = cd.get("nrm", PackedVector2Array())
	var own: PackedInt32Array = cd.get("owner", PackedInt32Array())
	var oth: PackedInt32Array = cd.get("other", PackedInt32Array())
	var role_cache := {}
	var ent_flat := {}    # entité → paires BRUTES (non jittées, entières → chaînables par sommet partagé)
	var ent_nrm := {}     # entité → normale INTÉRIEURE par segment (parallèle à ent_flat)
	for i in range(own.size()):
		var o: int = own[i]
		var ot: int = oth[i] if i < oth.size() else -1
		var n: Vector2 = nrm[i] if i < nrm.size() else Vector2.ZERO   # extérieur DEPUIS own
		if not role_cache.has(o):
			role_cache[o] = int(w.country_role(o))
		# ENTITÉ qui colore + DIRECTION de son intérieur : par défaut `own` (intérieur = −normale).
		# Si une CITÉ-ÉTAT est de l'AUTRE côté, c'est ELLE qui colore (intérieur = +normale).
		var entity := o
		var idir := -1.0
		if int(role_cache[o]) != 2 and ot >= 0:
			if not role_cache.has(ot):
				role_cache[ot] = int(w.country_role(ot))
			if int(role_cache[ot]) == 2:
				entity = ot; idir = 1.0
		if not ent_flat.has(entity):
			ent_flat[entity] = PackedVector2Array(); ent_nrm[entity] = PackedVector2Array()
		var ef: PackedVector2Array = ent_flat[entity]
		ef.push_back(pts[i * 2]); ef.push_back(pts[i * 2 + 1]); ent_flat[entity] = ef
		var en: PackedVector2Array = ent_nrm[entity]
		en.push_back(n * idir); ent_nrm[entity] = en        # normale vers l'INTÉRIEUR de l'entité
	# CHAÎNE + Chaikin chaque entité → ruban en COURBES (normale intérieure recalculée le long du tracé).
	_b_proj.clear(); _b_wash.clear()
	for entity in ent_flat:
		var r := _smooth_border(ent_flat[entity], ent_nrm[entity])
		_b_segs[entity] = r[0]; _b_norm[entity] = r[1]
		# revue #5 : OUTLINE + les 3 couches de lavis intérieur, projetés UNE fois ici (offsets
		# FIXES en monde, indépendants du zoom — _draw_band ne fera plus que les indexer).
		_b_proj[entity] = _project_segs_iso(mv, r[0])
		_b_wash[entity] = _build_wash_layers(mv, r[0], r[1])
	# CAPITALES : contour de la PROVINCE-capitale de chaque EMPIRE → liseré POURPRE (au-dessus).
	# Grain PROVINCE (charte) : jadis le contour de toute la RÉGION-siège — incohérent depuis
	# que la carte montre la propriété par province (le liseré entourait de la terre vierge).
	_cap_segs.clear()
	_cap_norm.clear()
	for c in range(w.country_count()):
		var rl := int(w.country_role(c))
		if rl != 0 and rl != 1:                              # empires (joueur/IA) seulement
			continue
		var rc: Dictionary
		if w.has_method("country_capital_province") and w.has_method("province_border_segments"):
			var cpp := int(w.country_capital_province(c))
			if cpp < 0:
				continue
			rc = w.province_border_segments(cpp)
		else:
			var creg := int(w.country_capital_region(c))
			if creg < 0:
				continue
			rc = w.region_border_segments(creg)
		var rp: PackedVector2Array = rc.get("pts", PackedVector2Array())
		var rn: PackedVector2Array = rc.get("nrm", PackedVector2Array())
		if rp.size() < 2:
			continue
		var inrm := PackedVector2Array(); inrm.resize(rn.size())
		for i in range(rn.size()):
			inrm[i] = -rn[i]                                 # normale extérieure → INTÉRIEURE
		var rcap := _smooth_border(rp, inrm)             # chaîné + lissé (liseré en courbe)
		_cap_segs[c] = rcap[0]; _cap_norm[c] = rcap[1]
	# LAVIS POLITIQUE : la palette = le pigment d'entité ÉCLAIRCI (aquarelle, pas une dalle) ;
	# l'image owner→teinte est bâtie en C++ (political_image) — même signal que les frontières.
	if w.has_method("political_image"):
		var pal := PackedColorArray()
		pal.resize(w.country_count())
		for c in range(w.country_count()):
			pal[c] = _entity_wash(c)
		var pimg: Image = w.political_image(pal)
		if pimg != null:
			if _pol_tex == null or _pol_tex.get_size() != Vector2(pimg.get_size()):
				_pol_tex = ImageTexture.create_from_image(pimg)
			else:
				_pol_tex.update(pimg)
	_sel_prov_cache = -2                                 # la géographie/souveraineté a bougé → recache la sélection
	_owner_sig = _owner_signature(w)
	_borders_dirty = false

# ── LISSAGE GÉOMÉTRIQUE DES FRONTIÈRES (escaliers → courbes) ──────────────────────────────────────
# La façade rend les arêtes en SEGMENTS UNITAIRES alignés sur la grille (escalier par construction). Le
# SSAA n'y change rien (c'est de la GÉOMÉTRIE, pas de l'aliasing). On CHAÎNE donc les segments en
# polylignes ordonnées puis on les courbe par Chaikin — comme les routes. Pour le RUBAN, la normale
# INTÉRIEURE est recalculée le long de la courbe (l'intérieur d'une chaîne de frontière reste d'un côté
# constant → décidé au 1er segment via la normale d'origine).

## clé entière d'un point de grille (coords façade = entiers, ≤1024×512) → identité de sommet stable.
func _node_key(p: Vector2) -> int:
	return int(round(p.x)) * 4096 + int(round(p.y))

## CHAÎNE une soupe de segments (paires) en polylignes ordonnées. Jonctions (degré≠2) = fin de chaîne ;
## boucles fermées gérées. Retour : Array[PackedVector2Array].
func _chain_segments(flat: PackedVector2Array) -> Array:
	var ctx := _chain_build(flat)
	var chains := []
	for ch in _chain_walk_all(ctx):
		chains.append(ch["poly"])
	return chains

## CHAÎNE avec NORMALE : comme _chain_segments mais renvoie aussi le côté INTÉRIEUR par chaîne (in_left),
## déduit de la normale d'origine du 1er segment. Retour : Array[{poly, in_left}].
func _chain_segments_n(flat: PackedVector2Array, enrm: PackedVector2Array) -> Array:
	var ctx := _chain_build(flat)
	ctx["nrm"] = enrm
	return _chain_walk_all(ctx)

## construit l'index de chaînage (sommets, adjacence) — partagé par les deux variantes.
func _chain_build(flat: PackedVector2Array) -> Dictionary:
	var nseg := flat.size() >> 1
	var node_pt := []            # id → Vector2
	var key2id := {}             # clé → id
	var sa := PackedInt32Array(); var sb := PackedInt32Array()
	sa.resize(nseg); sb.resize(nseg)
	var adj := {}                # id → Array[seg]
	for i in range(nseg):
		var ka := _node_key(flat[i * 2]); var kb := _node_key(flat[i * 2 + 1])
		var ia: int = key2id.get(ka, -1)
		if ia < 0:
			ia = node_pt.size(); key2id[ka] = ia; node_pt.append(flat[i * 2]); adj[ia] = []
		var ib: int = key2id.get(kb, -1)
		if ib < 0:
			ib = node_pt.size(); key2id[kb] = ib; node_pt.append(flat[i * 2 + 1]); adj[ib] = []
		sa[i] = ia; sb[i] = ib
		adj[ia].append(i); adj[ib].append(i)
	return {"sa": sa, "sb": sb, "adj": adj, "node_pt": node_pt, "nseg": nseg, "flat": flat}

## parcourt toutes les chaînes : départs aux noeuds de degré≠2 (bouts/jonctions VRAIES), puis boucles.
## Le degré 4 N'EST PAS un départ : c'est le COIN DE DAMIER (4 cellules alternées) — un escalier
## diagonal en produit UN PAR MARCHE ; casser là fragmentait la frontière en chaînes de 2 points
## qu'aucun lissage ne peut courber (l'origine des « crénelures incurables »). On le TRAVERSE.
func _chain_walk_all(ctx: Dictionary) -> Array:
	var adj: Dictionary = ctx["adj"]
	var sa: PackedInt32Array = ctx["sa"]
	var nseg: int = ctx["nseg"]
	var node_pt: Array = ctx["node_pt"]
	var used := PackedByteArray(); used.resize(nseg)
	var has_n: bool = ctx.has("nrm")
	var chains := []
	for nid in range(node_pt.size()):
		var inc: Array = adj[nid]
		if inc.size() == 2 or inc.size() == 4:
			continue
		for si in inc:
			if used[si] == 0:
				chains.append(_chain_one(si, nid, ctx, used, has_n))
	for i in range(nseg):
		if used[i] == 0:
			chains.append(_chain_one(i, sa[i], ctx, used, has_n))
	return chains

## suit UNE chaîne depuis (start_seg, start_node) jusqu'à une jonction / un bout / le bouclage.
func _chain_one(start_seg: int, start_node: int, ctx: Dictionary, used: PackedByteArray, has_n: bool) -> Dictionary:
	var sa: PackedInt32Array = ctx["sa"]
	var sb: PackedInt32Array = ctx["sb"]
	var adj: Dictionary = ctx["adj"]
	var node_pt: Array = ctx["node_pt"]
	var poly := PackedVector2Array()
	var cur := start_node
	poly.push_back(node_pt[cur])
	# côté INTÉRIEUR (pour le ruban) : direction de marche vs normale d'origine du 1er segment.
	var in_left := true
	if has_n:
		var nrm: PackedVector2Array = ctx["nrm"]
		var nxt0: int = sb[start_seg] if sa[start_seg] == cur else sa[start_seg]
		var d0: Vector2 = node_pt[nxt0] - node_pt[cur]
		in_left = nrm[start_seg].dot(Vector2(-d0.y, d0.x)) > 0.0
	var seg := start_seg
	while true:
		used[seg] = 1
		var nxt: int = sb[seg] if sa[seg] == cur else sa[seg]
		poly.push_back(node_pt[nxt])
		cur = nxt
		var inc: Array = adj[cur]
		var nseg2 := -1
		if inc.size() == 2:
			for s in inc:
				if used[s] == 0:
					nseg2 = s
		elif inc.size() == 4:
			# COIN DE DAMIER / croisement : on CONTINUE le plus DROIT possible (meilleure
			# continuation directionnelle) — l'escalier diagonal devient UNE chaîne courbable.
			var pdir: Vector2 = (node_pt[cur] - poly[poly.size() - 2]).normalized()
			var bestd := 0.25
			for s in inc:
				if used[s] == 1:
					continue
				var other: int = sb[s] if sa[s] == cur else sa[s]
				var sd: Vector2 = ((node_pt[other] as Vector2) - (node_pt[cur] as Vector2)).normalized()
				var dt := pdir.dot(sd)
				if dt > bestd:
					bestd = dt
					nseg2 = s
		if nseg2 < 0:
			break
		seg = nseg2
	return {"poly": poly, "in_left": in_left}

## Chaikin (corner-cutting) : détecte la BOUCLE (1er≈dernier) → lissée cycliquement ; sinon extrémités fixes.
func _chaikin(poly: PackedVector2Array, passes: int) -> PackedVector2Array:
	var p := poly
	for _it in range(passes):
		var n := p.size()
		if n < 3:
			break
		var closed := p[0].distance_to(p[n - 1]) < 0.001
		var out := PackedVector2Array()
		if closed:
			var src := p.slice(0, n - 1)
			var m := src.size()
			for i in range(m):
				var a: Vector2 = src[i]; var b: Vector2 = src[(i + 1) % m]
				out.push_back(a * 0.75 + b * 0.25); out.push_back(a * 0.25 + b * 0.75)
			out.push_back(out[0])                 # referme la boucle
		else:
			out.push_back(p[0])
			for i in range(n - 1):
				var a: Vector2 = p[i]; var b: Vector2 = p[i + 1]
				out.push_back(a * 0.75 + b * 0.25); out.push_back(a * 0.25 + b * 0.75)
			out.push_back(p[n - 1])
		p = out
	return p

## LISSE une polyligne : ré-échantillonnage grossier (casse la fréquence de l'escalier) → passe-bas
## Laplacien (aplatit les marches vers la diagonale) → Chaikin (arrondi). C'est le pipeline qui transforme
## les marches en COURBE (et non plus en « escalier arrondi »). Détecte la boucle (extrémités préservées).
## ÉPINGLAGE RIVIÈRE : un point de frontière SUR/AU BORD d'une rivière visible est FIXÉ pendant le
## Taubin — le lissage ne tire plus la frontière EN TRAVERS du fleuve, elle en épouse le cours
## (les arêtes de cellules suivent déjà la rivière ; c'est le passe-bas qui les décollait).
func _smooth_poly(poly: PackedVector2Array) -> PackedVector2Array:
	if poly.size() < 3:
		return poly
	var closed := poly[0].distance_to(poly[poly.size() - 1]) < 0.001
	var p := _resample_polyline(poly, SMOOTH_RESAMPLE) if SMOOTH_RESAMPLE > 0.0 else poly
	# ── SNAP RIVIÈRE : la géométrie moteur le long d'un fleuve est une DENT DE SCIE (les
	#    cellules alternent de rive) qu'aucun lissage ne répare — cartographiquement, la
	#    frontière DOIT suivre le fleuve. Un point de frontière à ≤ 1.3 cellule du FIL de
	#    rivière est COLLÉ dessus (plus proche point du nuage river_points, hash spatial),
	#    puis ANCRÉ (lissage réduit + rappel) : la frontière ÉPOUSE le cours d'eau. ──
	var pins := PackedByteArray()
	pins.resize(p.size())
	var any_pin := false
	if not _rivers.is_empty():
		if _river_hash.is_empty():
			for rp in _rivers:                       # le nuage est en Vector3 (x, y, angle)
				var rv2 := Vector2((rp as Vector3).x, (rp as Vector3).y)
				var hk := int(floor(rv2.x)) * 100000 + int(floor(rv2.y))
				if not _river_hash.has(hk):
					_river_hash[hk] = []
				_river_hash[hk].append(rv2)
		for i in range(p.size()):
			var gx := int(floor(p[i].x))
			var gy := int(floor(p[i].y))
			var bestd := 1.69   # (1.3 cellule)²
			var bestp: Vector2 = p[i]
			var found := false
			for oy in range(-1, 2):
				for ox in range(-1, 2):
					var hk2 := (gx + ox) * 100000 + (gy + oy)
					if _river_hash.has(hk2):
						for q in _river_hash[hk2]:
							var dd: float = p[i].distance_squared_to(q)
							if dd < bestd:
								bestd = dd
								bestp = q
								found = true
			if found:
				p[i] = bestp
				pins[i] = 1
				any_pin = true
	p = _taubin_pinned(p, SMOOTH_TAUBIN, closed, pins) if any_pin else _taubin(p, SMOOTH_TAUBIN, closed)
	p = _chaikin(p, SMOOTH_CHAIKIN)   # arrondi local ≤ ¼ de segment : ne saute pas un fleuve
	return _deloop(p)

## CULLING DES MICRO-BOUCLES (retour joueur : « certaines provinces font du hulahoop ») —
## quand la chaîne repasse près d'un point antérieur PROCHE (nœud en 8 né d'une jonction
## mal recousue), on EXCISE la boucle : deux points non voisins à ≤ 1.6 cellule, séparés
## de 3..18 indices → le tronçon entre eux saute. Un seul balayage suffit (les nœuds sont
## rares) ; les vraies formes (péninsules) sont bien plus larges que 18 points resamplés.
func _deloop(p: PackedVector2Array) -> PackedVector2Array:
	var n := p.size()
	if n < 12:
		return p
	var out := PackedVector2Array()
	var i := 0
	while i < n:
		var cut := -1
		var jmax := mini(i + 40, n - 1)          # fenêtre large : les gros nœuds (capture #2)
		for j in range(i + 6, jmax + 1):
			if p[i].distance_to(p[j]) <= 1.5:
				# ⚠ une ligne DROITE resamplée a aussi des points proches à 6 indices — on
				# n'excise que si le tronçon S'ÉLOIGNE vraiment (la boucle sort et revient),
				# et PAS TROP loin (bulge < 7 : une vraie péninsule fine est bien plus longue)
				var bulge := 0.0
				for k in range(i + 1, j):
					bulge = maxf(bulge, p[i].distance_to(p[k]))
				if bulge > 2.4 and bulge < 7.0:
					cut = j
		if cut > 0:
			out.append(p[i])
			i = cut          # la boucle i..cut est excisée (on ressort au point de retour)
		else:
			out.append(p[i])
			i += 1
	return out

## Taubin à ÉPINGLES DOUCES : un point de rivière est LISSÉ à 30 % (l'escalier fond quand
## même) puis RAPPELÉ élastiquement vers sa position d'origine (la frontière reste SUR le
## fleuve sans re-créneler — l'épingle dure ressuscitait l'escalier).
func _taubin_pinned(poly: PackedVector2Array, iters: int, closed: bool, pins: PackedByteArray) -> PackedVector2Array:
	if poly.size() < 3 or iters <= 0:
		return poly
	var orig := poly.duplicate()
	var p := poly
	for _it in range(iters):
		p = _lap_step_pinned(p, TAUBIN_LAMBDA, closed, pins)
		p = _lap_step_pinned(p, TAUBIN_MU, closed, pins)
	for i in range(mini(p.size(), orig.size())):
		if i < pins.size() and pins[i] == 1:
			p[i] = (p[i] as Vector2).lerp(orig[i], 0.35)   # rappel : ancré au cours d'eau
	return p

func _lap_step_pinned(poly: PackedVector2Array, factor: float, closed: bool, pins: PackedByteArray) -> PackedVector2Array:
	var n := poly.size()
	if n < 3:
		return poly
	if closed:
		var src := poly.slice(0, n - 1)
		var m := src.size()
		var out := PackedVector2Array()
		out.resize(m)
		for i in range(m):
			var f := factor * (0.45 if (i < pins.size() and pins[i] == 1) else 1.0)
			var avg: Vector2 = (src[(i - 1 + m) % m] + src[(i + 1) % m]) * 0.5
			out[i] = src[i] + (avg - src[i]) * f
		out.push_back(out[0])
		return out
	var out2 := PackedVector2Array()
	out2.resize(n)
	out2[0] = poly[0]
	out2[n - 1] = poly[n - 1]
	for i in range(1, n - 1):
		var f2 := factor * (0.45 if (i < pins.size() and pins[i] == 1) else 1.0)
		var avg2: Vector2 = (poly[i - 1] + poly[i + 1]) * 0.5
		out2[i] = poly[i] + (avg2 - poly[i]) * f2
	return out2

## un pas de lissage Laplacien : p[i] += factor·(moyenne des 2 voisins − p[i]). factor>0 = adoucit (et
## rétrécit), factor<0 = regonfle. Extrémités FIXES (chaîne ouverte → jonctions intactes) ; cyclique (boucle).
func _lap_step(poly: PackedVector2Array, factor: float, closed: bool) -> PackedVector2Array:
	var n := poly.size()
	if n < 3:
		return poly
	if closed:
		var src := poly.slice(0, n - 1)
		var m := src.size()
		var out := PackedVector2Array(); out.resize(m)
		for i in range(m):
			var avg: Vector2 = (src[(i - 1 + m) % m] + src[(i + 1) % m]) * 0.5
			out[i] = src[i] + (avg - src[i]) * factor
		out.push_back(out[0])                  # referme la boucle
		return out
	var out2 := PackedVector2Array(); out2.resize(n)
	out2[0] = poly[0]; out2[n - 1] = poly[n - 1]   # extrémités fixes (jonctions)
	for i in range(1, n - 1):
		var avg2: Vector2 = (poly[i - 1] + poly[i + 1]) * 0.5
		out2[i] = poly[i] + (avg2 - poly[i]) * factor
	return out2

## TAUBIN λ|μ : alterne un pas adoucissant (λ>0) et un pas regonflant (μ<0) → passe-bas qui lisse l'escalier
## SANS rétrécir la forme (la frontière garde sa vraie position → ne bulge pas sur les villes).
func _taubin(poly: PackedVector2Array, iters: int, closed: bool) -> PackedVector2Array:
	if poly.size() < 3 or iters <= 0:
		return poly
	var p := poly
	for _it in range(iters):
		p = _lap_step(p, TAUBIN_LAMBDA, closed)
		p = _lap_step(p, TAUBIN_MU, closed)
	return p

## chaîne + lisse une soupe de segments à NORMALE → [segs (paires), norms (intérieure/segment)].
## La normale est perpendiculaire à la COURBE locale, orientée selon le côté intérieur de la chaîne.
## GÉNÉRALISATION : les BOUCLES-CONFETTIS (≤ ~7 cellules de périmètre = une cellule isolée en
## damier de possession, fréquent le long des fleuves) ne sont PAS cernées — un atlas ne
## détoure pas les poussières, le lavis politique porte déjà l'information.
func _smooth_border(flat: PackedVector2Array, enrm: PackedVector2Array) -> Array:
	var out_segs := PackedVector2Array(); var out_norm := PackedVector2Array()
	# ── COUTURE : les jonctions et menus trous FRAGMENTAIENT le contour d'une entité en
	#    brins flottants (arcs qui meurent au milieu des terres — « messy »). On RECOUD :
	#    deux bouts de chaînes OUVERTES à ≤ 2.2 cellules se raboutent (le trou devient un
	#    segment, le lissage l'arrondit) — le contour redevient CONTINU. ──
	var loops := []
	var opens := []
	for ch in _chain_segments_n(flat, enrm):
		var raw: PackedVector2Array = ch["poly"]
		var per := 0.0
		for i in range(raw.size() - 1):
			per += raw[i].distance_to(raw[i + 1])
		var isloop := raw.size() >= 3 and raw[0].distance_to(raw[raw.size() - 1]) < 0.001
		if isloop:
			if per >= 7.0:            # boucle-confetti (îlot d'une cellule) : pas de bande
				loops.append(ch)
		else:
			ch["per"] = per
			opens.append(ch)
	var stitched := true
	while stitched:
		stitched = false
		for i in range(opens.size()):
			if stitched:
				break
			var pa: PackedVector2Array = opens[i]["poly"]
			for j in range(i + 1, opens.size()):
				var pb: PackedVector2Array = opens[j]["poly"]
				var d_ee := pa[pa.size() - 1].distance_to(pb[0])                    # fin A → début B
				var d_er := pa[pa.size() - 1].distance_to(pb[pb.size() - 1])        # fin A → fin B
				var d_se := pa[0].distance_to(pb[0])                                # début A → début B
				var d_sr := pa[0].distance_to(pb[pb.size() - 1])                    # début A → fin B
				var dm := minf(minf(d_ee, d_er), minf(d_se, d_sr))
				if dm > 2.2:
					continue
				var joined := PackedVector2Array()
				if dm == d_ee:
					joined = pa.duplicate(); joined.append_array(pb)
				elif dm == d_er:
					joined = pa.duplicate()
					for k in range(pb.size() - 1, -1, -1): joined.push_back(pb[k])
				elif dm == d_se:
					for k in range(pa.size() - 1, -1, -1): joined.push_back(pa[k])
					joined.append_array(pb)
				else:
					joined = pb.duplicate(); joined.append_array(pa)
				opens[i]["poly"] = joined
				opens[i]["per"] = float(opens[i]["per"]) + float(opens[j]["per"]) + dm
				opens.remove_at(j)
				stitched = true
				break
	var kept := loops
	for ch2 in opens:
		if float(ch2["per"]) >= 2.5:  # les orphelins post-couture (slivers côtiers) tombent
			kept.append(ch2)
	for ch in kept:
		var poly: PackedVector2Array = _smooth_poly(ch["poly"])
		if poly.size() < 2:
			continue
		var in_left: bool = ch["in_left"]
		for i in range(poly.size() - 1):
			var a: Vector2 = poly[i]; var b: Vector2 = poly[i + 1]
			var dd: Vector2 = b - a
			if dd.length() < 0.00001:
				continue
			out_segs.push_back(a); out_segs.push_back(b)
			var nn := Vector2(-dd.y, dd.x) if in_left else Vector2(dd.y, -dd.x)
			out_norm.push_back(nn.normalized())
	return [out_segs, out_norm]

## assombrit/éclaircit un pigment d'un cheveu (variation par pays SANS sortir de la gamme : on touche
## la VALEUR seulement, jamais la teinte → pas de dérive néon). `dv` ∈ ~[-0.06, +0.06].
func _shade(c: Color, dv: float) -> Color:
	return Color(clampf(c.r + dv, 0.0, 1.0), clampf(c.g + dv, 0.0, 1.0), clampf(c.b + dv, 0.0, 1.0), c.a)

## or FANÉ des cités-états (encre, pas du métal brillant) — dans la même gamme terreuse.
const CS_GOLD := Color(0.62, 0.50, 0.28)         ## or vieilli

## PIGMENT POLITIQUE d'une entité (le trait fin du pays) : encre d'HÉRITAGE (culture, prune/rouille/
## sienne/olive/ocre/ardoise) + variation par pays sur la VALEUR seule (gamme tenue) ; cité-état = or fané.
## TEINTE (hue) unique d'une entité — LA source partagée de TOUTE sa famille de couleurs
## (frontière · lavis · armée · nom) : golden-ratio par id (voisins bien séparés).
func _entity_hue(e: int) -> float:
	return fmod(float(e) * 0.1607 + 0.04, 1.0)

func _entity_pigment(e: int) -> Color:
	if e < 0:
		return Color(0.30, 0.24, 0.18)
	if int(Sim.world.country_role(e)) == 2:
		return CS_GOLD
	# DISTINCT PAR EMPIRE : jadis la frontière était codée par HÉRITAGE (6 familles) →
	# deux empires du même héritage = MÊME couleur, indistinguables. Désormais une teinte
	# propre à chaque pays (_entity_hue), SATURATION/VALEUR MUETTES (encre terreuse — anti-néon).
	return Color.from_hsv(_entity_hue(e), 0.45, 0.55)

## LAVIS de territoire : MÊME teinte, plus SATURÉE et CLAIRE — l'aquarelle doit TEINTER le
## parchemin (à sat 0.45 le wash lisait GRIS : il assombrissait sans colorer). L'anti-néon
## tient par l'ALPHA bas du wash, pas par la désaturation.
func _entity_wash(e: int) -> Color:
	if e < 0:
		return Color(0.55, 0.50, 0.40)
	if int(Sim.world.country_role(e)) == 2:
		return Color(0.82, 0.68, 0.34)               # cité-état : or clair
	# saturation calée sur les couleurs pays d'EU5 (named_colors : hsv S≈70-90 V≈85-95) —
	# le lavis à 0.60 lisait « pastel délavé » là où l'atlas Paradox assume la couleur.
	return Color.from_hsv(_entity_hue(e), 0.72, 0.88)

## ÉPAISSEUR ADAPTATIVE (CK) : rend une largeur en unités MONDE à passer DIRECTEMENT à draw_* (le /zoom est
## déjà fait). `base·zoom` = px ÉCRAN voulu à taille monde constante, borné aux rails [min,max] de lisibilité.
## min_px == max_px ⇒ se réduit ALGÉBRIQUEMENT à `min_px/zoom` (px écran constant, l'ancien comportement).
## ⚠ NE JAMAIS écrire `_w(...)/zoom` (le /zoom est inclus) ; ne PAS router des longueurs/motifs ici.
func _w(zoom: float, base_world: float, min_px: float, max_px: float) -> float:
	return clampf(base_world * zoom, min_px, max_px) / maxf(zoom, 0.0001)

## frontière de PAYS façon CIV/STELLARIS — fin des « boudins » : (1) un creux gravé discret,
## (2) l'OUTLINE net SUR la ligne = f(ÉTHOS) (l'axe politique se lit à la frontière), (3) un
## LAVIS INTÉRIEUR = f(HÉRITAGE), 3 couches décalées le long de la NORMALE intérieure, alpha
## dégressif → la lueur de territoire, jamais une saucisse opaque. Cité-état = or fané.
const ETHOS_INK := [
	Color(0.47, 0.22, 0.16),   # 0 — braise (le pôle martial/chaos, chaud)
	Color(0.54, 0.34, 0.16),   # 1 — bronze
	Color(0.44, 0.38, 0.20),   # 2 — terre d'ombre
	Color(0.28, 0.38, 0.26),   # 3 — mousse
	Color(0.22, 0.33, 0.42),   # 4 — ardoise d'eau
	Color(0.28, 0.26, 0.46),   # 5 — indigo (le pôle ordre, froid)
]
const HERITAGE_WASH := [
	Color(0.58, 0.42, 0.62),   # Ésotérique — lilas de prune
	Color(0.72, 0.44, 0.30),   # Métallurgiste — rouille
	Color(0.70, 0.56, 0.34),   # Mécaniste — laiton
	Color(0.52, 0.62, 0.38),   # Adaptatif — olive claire
	Color(0.78, 0.64, 0.34),   # Agraire — ocre blé
	Color(0.60, 0.36, 0.32),   # Clanique — sang délavé
]
func _ethos_ink(e: int) -> Color:
	var idx := 0
	if e >= 0 and Sim.world != null:
		idx = clampi(int(Sim.world.country_ethos(e)), 0, 5)
	var c: Color = ETHOS_INK[idx]
	var v := 0.92 + 0.16 * _h1(float(e) * 17.3)     # valeur jittée par pays (jamais la teinte)
	return Color(c.r * v, c.g * v, c.b * v)

func _heritage_wash(e: int) -> Color:
	var idx := 0
	if e >= 0 and Sim.world != null:
		idx = clampi(int(Sim.world.country_heritage(e)), 0, 5)
	return HERITAGE_WASH[idx]

## calque INLINE d'une bande (lavis d'héritage), décalé vers l'INTÉRIEUR le long de la normale
## PUIS projeté — 3 couches, alpha dégressif. ⚠ _b_norm porte la normale EXTÉRIEURE (héritée de
## la façade) → l'intérieur est à -n. Zoom-INDÉPENDANT (offsets FIXES en monde : 0.45/1.07/2.20)
## → précalculable UNE fois à _rebuild_borders (revue overlay #5) au lieu de re-projeter
## segs.size()×3 points CHAQUE frame pour chaque entité.
func _build_wash_layers(mv: Node2D, segs: PackedVector2Array, nrms: PackedVector2Array) -> Array:
	var out: Array = []
	if nrms.size() * 2 < segs.size():
		return out
	for off in [0.45, 1.07, 2.20]:
		var proj := PackedVector2Array()
		proj.resize(segs.size())
		for i in range(0, segs.size() - 1, 2):
			var n: Vector2 = nrms[i >> 1] * (-float(off))
			proj[i] = mv.iso_pos(segs[i].x + n.x, segs[i].y + n.y)
			proj[i + 1] = mv.iso_pos(segs[i + 1].x + n.x, segs[i + 1].y + n.y)
		out.append(proj)
	return out

## `proj0`/`wash` (revue overlay #5) : géométrie déjà PROJETÉE en iso par _rebuild_borders
## (_b_proj/_b_wash) — cette fonction ne fait plus que choisir les couleurs et tracer.
func _draw_band(proj0: PackedVector2Array, wash: Array, entity: int, zoom: float) -> void:
	if proj0.size() < 2:
		return
	var is_cs: bool = entity >= 0 and Sim.world != null and int(Sim.world.country_role(entity)) == 2
	var out_col: Color = CS_GOLD if is_cs else _ethos_ink(entity)
	var in_col: Color = Color(0.80, 0.68, 0.40) if is_cs else _heritage_wash(entity)
	# l'INLINE d'abord (sous l'outline) : le lavis d'héritage, décalé vers l'INTÉRIEUR
	if wash.size() == 3:
		var lw := _w(zoom, 0.55, 1.8, 3.4)
		var wash_alphas := [0.34, 0.20, 0.06]
		for k in range(3):
			draw_multiline(wash[k], Color(in_col.r, in_col.g, in_col.b, float(wash_alphas[k])), lw, true)
	# le CREUX gravé (discret) + l'OUTLINE d'éthos NET, sur la ligne
	draw_multiline(proj0, Color(POL_HALO.r, POL_HALO.g, POL_HALO.b, 0.36), _w(zoom, POL_HALO_BASE, POL_HALO_MIN, POL_HALO_MAX), true)
	draw_multiline(proj0, Color(out_col.r, out_col.g, out_col.b, 0.92), _w(zoom, POL_PIG_BASE, POL_PIG_MIN, POL_PIG_MAX), true)

## LISERÉ de capitale : un SEUL trait FIN pourpre sourd, posé JUSTE à l'intérieur du contour (décalé
## le long de la normale intérieure) — un filet discret, PAS une bande qui prend toute la capitale.
func _draw_cap_lisere(mv: Node2D, segs: PackedVector2Array, norms: PackedVector2Array, zoom: float) -> void:
	var nseg := norms.size()
	if nseg < 1:
		return
	var off := 1.4 / zoom                                   # rentré d'un cheveu (px écran) → le filet borde l'intérieur
	var layer := PackedVector2Array()
	layer.resize(segs.size())
	for i in range(nseg):
		var ni: Vector2 = norms[i]
		layer[i * 2] = segs[i * 2] + ni * off
		layer[i * 2 + 1] = segs[i * 2 + 1] + ni * off
	var proj := _project_segs_iso(mv, layer)
	if proj.size() >= 2:
		draw_multiline(proj, Color(CAP_INK.r, CAP_INK.g, CAP_INK.b, 0.28), 2.2 / zoom, true)  # halo doux
		draw_multiline(proj, Color(CAP_INK.r, CAP_INK.g, CAP_INK.b, 0.85), 1.1 / zoom, true)  # filet net

const HATCH_STEP := 9.0      ## espacement MONDE entre deux traits de hachure (adaptatif zoom via _w)
const HATCH_SIEGE_A := 0.30  ## α d'une région ASSIÉGÉE (siège en cours, propriété inchangée)
const HATCH_OCC_A := 0.42    ## α d'une région OCCUPÉE (le siège a abouti) — un ton plus marqué

## W-GUERRE UI (lot A) — HACHURES à 45° (traits d'encre espacés, teinte du BESIÉGEANT/OCCUPANT,
## densité selon siège(1)/occupé(2)) clippées au(x) polygone(s) de la région (`info.polys`,
## world-space, ANNEAUX déjà CHAÎNÉS depuis region_border_segments — même source que le liseré
## de capitale, chaînage via _chain_segments). Le clip utilise Geometry2D (intersection
## segment×polygone) : chaque ligne de hachure MONDE, tracée en diagonale sur la boîte englobante
## du ring, est coupée aux bords réels — pas un rectangle qui déborde de la région.
## POLISH #5 : le clipping (coûteux) est PRÉCALCULÉ au rebuild de _war_regions —
## renvoie les traits [p0,p1, p0,p1, …] en coordonnées MONDE (indépendants du zoom).
func _hatch_segments(polys: Array) -> PackedVector2Array:
	var out := PackedVector2Array()
	for poly in polys:
		var ring: PackedVector2Array = poly
		if ring.size() < 3:
			continue
		# boîte englobante MONDE (les points sont en coordonnées monde/cellule)
		var minx := ring[0].x
		var maxx := ring[0].x
		var miny := ring[0].y
		var maxy := ring[0].y
		for p in ring:
			minx = minf(minx, p.x); maxx = maxf(maxx, p.x)
			miny = minf(miny, p.y); maxy = maxf(maxy, p.y)
		var diag := (maxx - minx) + (maxy - miny) + HATCH_STEP
		# traits à 45° (monde) : x+y = k, k parcourant la diagonale de la boîte, pas HATCH_STEP.
		var k0 := minx + miny
		var k1 := maxx + maxy
		var k := k0 - fmod(k0, HATCH_STEP)
		while k <= k1:
			var seg := PackedVector2Array([Vector2(minx - HATCH_STEP, k - (minx - HATCH_STEP)),
				Vector2(minx - HATCH_STEP + diag, k - (minx - HATCH_STEP + diag))])
			var clipped: Array = Geometry2D.intersect_polyline_with_polygon(seg, ring)
			for part in clipped:
				var pp: PackedVector2Array = part
				if pp.size() >= 2:
					out.push_back(pp[0]); out.push_back(pp[1])
			k += HATCH_STEP
	return out

func _draw_war_hatch(mv: Node2D, zoom: float, info: Dictionary) -> void:
	var segs: PackedVector2Array = info.get("hatch", PackedVector2Array())
	if segs.is_empty():
		return
	var belli: int = int(info.get("belligerent", -1))
	var st: int = int(info.get("state", 1))
	var col := _country_color(belli) if belli >= 0 else Color(0.5, 0.1, 0.1)
	var a: float = HATCH_OCC_A if st == 2 else HATCH_SIEGE_A
	var w_line := _w(zoom, 0.5, 0.8, 1.6)
	var ink := Color(col.r, col.g, col.b, a)
	for i in range(0, segs.size() - 1, 2):
		draw_line(mv.iso_pos(segs[i].x, segs[i].y), mv.iso_pos(segs[i + 1].x, segs[i + 1].y), ink, w_line)


## TRAIT DE PINCEAU : pile de passes translucides (bave d'encre) du LARGE plumé au cœur dense,
## TOUTES antialiasées → feutre le crénelage des arêtes + bord doux = effet brosse. `core_w`/`feather`
## en px ÉCRAN (÷ zoom). Plus de passes larges = halo plus « mouillé ».
func _ink_brush(segs: PackedVector2Array, col: Color, core_w: float, feather: float, zoom: float) -> void:
	var bands := [
		[feather,        0.07],
		[feather * 0.72, 0.13],
		[feather * 0.46, 0.22],
		[feather * 0.26, 0.38],
	]
	for b in bands:
		var ww: float = core_w + float(b[0])
		draw_multiline(segs, Color(col.r, col.g, col.b, col.a * float(b[1])), ww / zoom, true)
	draw_multiline(segs, col, core_w / zoom, true)            # plume nette (cœur)

## hash scalaire → [0,1) (déterministe, display-only) — varie dressing/orientations. Hash
## ENTIER (revue overlay #8) : l'ancien sin(x·12.9898)·43758.5453 dépend de la précision CPU/libm
## (résultat pas garanti bit-identique entre plateformes) — display-only donc pas un souci de
## déterminisme SIM, mais un souci de REPRODUCTIBILITÉ des probes/captures. ⚠ REDISTRIBUE tout le
## semis (dressing/canopée/chevrons) par rapport à l'ancien hash — display-only, assumé.
func _h1(x: float) -> float:
	var n := int(x * 4096.0) & 0x7fffffff
	n = (n ^ 61) ^ (n >> 16); n += n << 3; n ^= n >> 4
	n *= 0x27d4eb2d; n ^= n >> 15
	return float(n & 0xffffff) / 16777216.0

## (re)charge le réseau de routes + sa méta + l'habillage, et DATE les chantiers neufs.
## Appelé hors zoom (générate/tick) → les routes initiales démarrent dès l'an de fondation,
## même si le joueur n'a pas encore zoomé (sinon elles « repartiraient » au premier zoom).
## `prebuild` (monde MÛR : chargement de save / re-génération à l'an N>0) → les routes initiales
## sont datées DANS LE PASSÉ (déjà bâties) au lieu de re-construire de zéro sous les yeux.
func _ensure_roads(prebuild := false) -> void:
	if not _roads_dirty:
		return
	var w = Sim.world
	if w == null:
		return
	_roads = w.road_paths()
	_augment_roads(w)
	_road_net_valid = false   # ANTISPAG : le réseau a changé → les polylignes consolidées se refont
	# les PONTS : là où le tracé LISSE traverse l'eau de rivière carvée — milieu + tangente.
	# Dédup (`bkey`) : après magnétisme renforcé, 2 routes qui PARTAGENT un franchissement
	# convergent sur des points quasi-identiques — sans ça le même pont s'ajoutait 2× (overdraw
	# inoffensif mais trompeur pour un futur audit qui compterait les ponts).
	_ink_bridges.clear()
	var rfb: Image = _carved_river_field()
	var bseen := {}
	if rfb != null:
		for rd0 in _roads:
			var pts0: PackedVector2Array = rd0["points"]
			var inw0 := -1
			for k in range(pts0.size()):
				var inw := _in_river_water(rfb, int(pts0[k].x), int(pts0[k].y))
				if inw and inw0 < 0:
					inw0 = k
				elif (not inw or k == pts0.size() - 1) and inw0 >= 0:
					var mid: Vector2 = pts0[clampi((inw0 + k) / 2, 0, pts0.size() - 1)]
					var t0: Vector2 = (pts0[mini(k, pts0.size() - 1)] - pts0[maxi(inw0 - 1, 0)]).normalized()
					if t0.length() > 0.5:
						var bkey := str(int(round(mid.x * 2.0))) + "_" + str(int(round(mid.y * 2.0)))
						if not bseen.has(bkey):
							bseen[bkey] = true
							_ink_bridges.append({"w": mid, "t": t0})
					inw0 = -1
	for rd in _roads:
		if not _road_start.has(rd["key"]):
			if prebuild:
				# déjà bâtie (monde mûr) : datée assez loin dans le passé pour paraître finie
				_road_start[rd["key"]] = Sim.day_count - int(rd.get("nprov", 1)) * 365 - 400
			else:
				_road_start[rd["key"]] = Sim.day_count   # route NEUVE → chantier daté à maintenant (croît en JOURS)
	_roads_dirty = false

## marque les cellules occupées par une route (+ marge 1) → le bourg en spirale les ÉVITE
## (le bâti ne pousse pas sur la chaussée ; les ruelles serpentent ENTRE).
func _augment_roads(w) -> void:
	var sea: Image = w.layer_image(LAYER_WATER)
	var rf: Image = _carved_river_field()
	var mv := _mv_ref()
	for rd in _roads:
		var pts: PackedVector2Array = rd["points"]
		# PROVINCES traversées (cadence du chantier 1 an/province) — sur le tracé BRUT (A*).
		var np := 1
		var last := -999
		for p in pts:
			var pv: int = w.province_at(int(p.x), int(p.y))
			if pv >= 0 and pv != last:
				if last != -999:
					np += 1
				last = pv
		rd["nprov"] = np
		var ra: int = w.province_region(w.province_at(int(pts[0].x), int(pts[0].y)))
		var rb: int = w.province_region(w.province_at(int(pts[pts.size() - 1].x), int(pts[pts.size() - 1].y)))
		rd["key"] = (mini(ra, rb) & 0xfff) * 4096 + (maxi(ra, rb) & 0xfff)
		# 1) PATHFINDING (rendu) : on LISSE d'abord le chemin BRUT (resample + Chaikin gardé-eau) — l'A*
		#    moteur reste la vérité ; courbe propre, points réguliers, jamais sur la côte.
		pts = _smooth_resample_road(pts, sea, rf)
		# 2) SNAP : raccord PROPRE au pied des marches de l'asset (l'ancre = sommet bas de tuile, où la
		#    petite route du sprite débouche) ; trim des points qui tanglent → approche radiale nette.
		if mv != null and mv.has_method("tile_anchor_world"):
			if _region_anchor.has(ra):
				var a0: Vector2 = _region_anchor[ra]
				pts = _snap_endpoint(pts, mv.tile_anchor_world(a0.x, a0.y), true)
			if _region_anchor.has(rb):
				var a1: Vector2 = _region_anchor[rb]
				pts = _snap_endpoint(pts, mv.tile_anchor_world(a1.x, a1.y), false)
		rd["ra"] = ra            # mémorisé : le bâti du bourg s'organise le long des routes de SA ville
		rd["rb"] = rb
		rd["points"] = pts
	# 3) ANTI-DÉDOUBLEMENT — le MAGNÉTISME DE COULOIR (ANTISPAG A2, renforcé) : un point qui passe
	#    à ≤ ROAD_MAGNET_R cellules d'une route DÉJÀ tracée se COLLE dessus → les A* voisins
	#    PARTAGENT la chaussée au lieu de dessiner deux lignes quasi-parallèles. Les 3 points
	#    d'about restent libres (le raccord au bourg prime). ITÉRATIF (ROAD_MAGNET_PASSES) : une
	#    route traitée TÔT (peu d'attracteurs encore posés) profite, à la passe suivante, des
	#    attracteurs ajoutés PLUS TARD par des routes voisines — sans ça l'ordre de `_roads`
	#    biaisait quel tracé « gagnait » le couloir commun.
	for _pass in range(ROAD_MAGNET_PASSES):
		var bundle := {}   # hash spatial (cellule 1.0) des points DÉJÀ tracés CETTE passe
		for rd in _roads:
			var pts: PackedVector2Array = rd["points"]
			for k in range(3, pts.size() - 3):
				var p5: Vector2 = pts[k]
				var gx := int(floor(p5.x))
				var gy := int(floor(p5.y))
				var bestd := ROAD_MAGNET_R2
				var bestp := p5
				for oy in range(-ROAD_MAGNET_RING, ROAD_MAGNET_RING + 1):
					for ox in range(-ROAD_MAGNET_RING, ROAD_MAGNET_RING + 1):
						var kk := (gx + ox) * 100000 + (gy + oy)
						if bundle.has(kk):
							for q5 in bundle[kk]:
								var dd: float = p5.distance_squared_to(q5)
								if dd < bestd:
									bestd = dd
									bestp = q5
				pts[k] = bestp
			for k in range(pts.size()):    # cette route ENTRE dans le couloir commun
				var kk2 := int(floor(pts[k].x)) * 100000 + int(floor(pts[k].y))
				if not bundle.has(kk2):
					bundle[kk2] = []
				bundle[kk2].append(pts[k])
			rd["points"] = pts

## PORTULAN — charge les lanes maritimes depuis le moteur (sea_paths, cache par signature
## du commerce) puis les prépare (lissage gardé-MER, ancrage aux ports, magnétisme marin,
## tirets pré-calculés). Garde has_method : une DLL antérieure à la mission MARITIME
## laisse simplement la mer muette.
func _ensure_lanes() -> void:
	if not _lanes_dirty:
		return
	var w = Sim.world
	if w == null or not w.has_method("sea_paths"):
		_lanes_dirty = false   # revue overlay #9 : vieille DLL/monde pas prêt → rien à charger, mais
		return                 # NE PAS rester dirty (sinon ce garde-fou se ré-exécute CHAQUE frame)
	_lanes = w.sea_paths()
	_augment_lanes(w)
	_lanes_day = Sim.day_count
	_lanes_dirty = false

## PORTULAN : le pendant marin d'_augment_roads — resample + Chaikin GARDÉ-MER (l'inverse
## des routes : un coin coupé qui QUITTERAIT l'eau retombe sur le sommet d'origine), snap
## des extrémités à l'ancre du bourg-PORT (le raccord au quai), magnétisme de couloir
## ENTRE LANES seulement (jamais collées aux routes de terre — deux médias), puis les
## TIRETS (paires iso) pré-calculés une fois pour toutes.
func _augment_lanes(w) -> void:
	var sea: Image = w.layer_image(LAYER_WATER)
	var mv := _mv_ref()
	for ln in _lanes:
		var pts: PackedVector2Array = ln["points"]
		pts = _resample_polyline(pts, ROAD_RESAMPLE)
		pts = _chaikin_sea(pts, sea)
		pts = _chaikin_sea(pts, sea)
		if mv != null and mv.has_method("tile_anchor_world"):
			var ra: int = int(ln.get("ra", -1))
			var rb: int = int(ln.get("rb", -1))
			if _region_anchor.has(ra):
				var a0: Vector2 = _region_anchor[ra]
				pts = _snap_endpoint(pts, mv.tile_anchor_world(a0.x, a0.y), true)
			if _region_anchor.has(rb):
				var a1: Vector2 = _region_anchor[rb]
				pts = _snap_endpoint(pts, mv.tile_anchor_world(a1.x, a1.y), false)
		ln["points"] = pts
	# le MAGNÉTISME MARIN (mêmes réglages ANTISPAG que la terre, bundle SÉPARÉ) : les
	# corridors moteur (×0.30) font le gros ; ceci recolle les résidus de déphasage.
	for _pass in range(SEA_LANE_MAGNET_PASSES):
		var bundle := {}
		for ln in _lanes:
			var pts: PackedVector2Array = ln["points"]
			for k in range(3, pts.size() - 3):
				var p5: Vector2 = pts[k]
				var gx := int(floor(p5.x))
				var gy := int(floor(p5.y))
				var bestd := ROAD_MAGNET_R2
				var bestp := p5
				for oy in range(-ROAD_MAGNET_RING, ROAD_MAGNET_RING + 1):
					for ox in range(-ROAD_MAGNET_RING, ROAD_MAGNET_RING + 1):
						var kk := (gx + ox) * 100000 + (gy + oy)
						if bundle.has(kk):
							for q5 in bundle[kk]:
								var dd: float = p5.distance_squared_to(q5)
								if dd < bestd:
									bestd = dd
									bestp = q5
				pts[k] = bestp
			for k in range(pts.size()):
				var kk2 := int(floor(pts[k].x)) * 100000 + int(floor(pts[k].y))
				if not bundle.has(kk2):
					bundle[kk2] = []
				bundle[kk2].append(pts[k])
			ln["points"] = pts
	_lane_dashes.clear()
	if mv == null:
		return
	# DÉDUP DES TIRETS (le « déjà encré ? » des routes, porté à la mer) : plusieurs lanes
	# partagent un corridor CELLULE-IDENTIQUE (tampon exact moteur + magnétisme) mais
	# chacune poserait ses tirets avec SA phase propre — les blancs de l'une comblés par
	# les tirets des autres = un trait SOLIDE (mesuré graine 42, 5 lanes sur la côte sud).
	# Une clé de demi-cellule sur le milieu de tiret : le corridor ne s'encre qu'UNE fois,
	# avec UNE phase — le pointillé du portulan survit à la multiplicité.
	var seen := {}
	for ln in _lanes:
		_lane_dashes.append(_lane_dash_iso(ln["points"], mv, seen))

## Chaikin GARDÉ-MER (portulan) : le miroir exact de _chaikin_safe — un point coupé qui
## SORTIRAIT de l'eau reprend le coin d'origine ; la lane épouse la côte sans jamais
## monter dessus. (Les extrémités, ancrées au quai par _snap_endpoint APRÈS, sont les
## seuls points de terre légitimes du tracé.)
func _chaikin_sea(pts: PackedVector2Array, sea: Image) -> PackedVector2Array:
	if pts.size() < 3:
		return pts
	var out := PackedVector2Array()
	out.append(pts[0])
	for i in range(pts.size() - 1):
		var a: Vector2 = pts[i]
		var b: Vector2 = pts[i + 1]
		var q := a.lerp(b, 0.25)
		var r := a.lerp(b, 0.75)
		out.append(q if _is_sea_cell(sea, int(q.x), int(q.y)) else a)
		out.append(r if _is_sea_cell(sea, int(r.x), int(r.y)) else b)
	out.append(pts[pts.size() - 1])
	return out

## découpe une polyligne MONDE en TIRETS (dash/gap en cellules, phase continue le long
## de l'arc — le pointillé ne « saute » pas aux sommets), projetés iso : des PAIRES de
## points prêtes pour draw_multiline. Pré-calculé dans _augment_lanes (iso_pos est une
## projection fixe monde→iso ; la caméra vit dans le canvas transform).
func _lane_dash_iso(pts: PackedVector2Array, mv, seen: Dictionary) -> PackedVector2Array:
	var out := PackedVector2Array()
	if pts.size() < 2:
		return out
	var period := SEA_LANE_DASH + SEA_LANE_GAP
	var t := 0.0
	for i in range(pts.size() - 1):
		var a: Vector2 = pts[i]
		var b: Vector2 = pts[i + 1]
		var seg := a.distance_to(b)
		if seg < 0.0001:
			continue
		var s := 0.0
		var guard := 0   # FUSIBLE (revue overlay #7) : le plancher ε ci-dessous a déjà corrigé le
		                 # hang documenté (11 min CPU) mais reste un raisonnement float — un garde-fou
		                 # dur en plus, jamais légitimement atteint pour un segment normal.
		while s < seg - 0.0001:
			guard += 1
			if guard > 100000:
				break
			var phase := fmod(t, period)
			var in_dash := phase < SEA_LANE_DASH
			# PIÈGE FLOTTANT (a HANGÉ un run d'audit — 11 min de CPU) : quand phase tend
			# vers la frontière dash/gap, `run` peut devenir ~0 et la boucle ne PROGRESSE
			# plus (fmod re-rend la même phase). Plancher ε : la progression est garantie,
			# l'erreur de phase (≤ 0.005 cellule) est invisible sous le pixel.
			var run := maxf((SEA_LANE_DASH - phase) if in_dash else (period - phase), 0.005)
			run = minf(run, seg - s)
			if run < 0.0049:
				run = minf(0.005, seg - s + 0.001)   # dernier pas : sortir quoi qu'il arrive
			if in_dash:
				var p0 := a.lerp(b, clampf(s / seg, 0.0, 1.0))
				var p1 := a.lerp(b, clampf((s + run) / seg, 0.0, 1.0))
				var mid := (p0 + p1) * 0.5
				# bin de 1 CELLULE (pas 0.5) : le résidu de divergence entre lanes d'un même
				# corridor plafonne à ~0.3-1 cellule (cf. TROUVAILLES) — un bin plus fin
				# laissait les tirets déphasés des lanes voisines COMBLER les blancs (trait
				# solide par morceaux, mesuré graine 42).
				var dk := str(int(floor(mid.x))) + "_" + str(int(floor(mid.y)))
				if not seen.has(dk):        # dédup : un corridor partagé ne s'encre qu'UNE fois
					seen[dk] = true
					out.append(mv.iso_pos(p0.x, p0.y))
					out.append(mv.iso_pos(p1.x, p1.y))
			s += run
			t += run
	return out

## clé quantizée (résolution ROAD_SEGKEY_RES) d'un SEGMENT — identité pour la dédup/multiplicité
## de `_ensure_road_network` (tronc consolidé). Volontairement COARSE (0.5 cellule, pas 0.25) :
## découverte (cf. TROUVAILLES) — même après magnétisme réussi, l'échantillonnage indépendant de
## 2 routes déphase légèrement où tombe le « joint » entre segments (écart perpendiculaire quasi
## nul, ~0.02-0.1 cellule, mais assez pour rater une clé à 0.25). Élargir la clé fait reconnaître
## ces quasi-doublons comme le MÊME tronçon (dédup + comptage de multiplicité plus fidèles), SANS
## toucher aux points RENDUS (seule la décision « déjà encré ? » utilise cette clé, pas le tracé).
const ROAD_SEGKEY_RES := 2.0    ## points / cellule (2.0 ⇒ résolution 0.5 cellule)
func _seg_key(a: Vector2, b: Vector2) -> String:
	var ka := int(a.x * ROAD_SEGKEY_RES) * 8388608 + int(a.y * ROAD_SEGKEY_RES)
	var kb := int(b.x * ROAD_SEGKEY_RES) * 8388608 + int(b.y * ROAD_SEGKEY_RES)
	return str(mini(ka, kb)) + "_" + str(maxi(ka, kb))

## range un tronçon de route dans son bucket (artère/desserte × sous-canopée × TIER d'épaisseur).
## projette une polyligne MONDE en iso (helper du dessin de routes).
func _road_iso(poly: PackedVector2Array, mv) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(poly.size())
	for k in range(poly.size()):
		out[k] = mv.iso_pos(poly[k].x, poly[k].y)
	return out

## indice du sommet-FRONTIÈRE urbaine : premier sommet (en remontant depuis la FIN,
## côté ville) dont l'arc cumulé dépasse `r`. -1 si le tracé est trop court.
func _urban_boundary(poly: PackedVector2Array, r: float) -> int:
	var acc := 0.0
	for k in range(poly.size() - 1, 0, -1):
		acc += poly[k].distance_to(poly[k - 1])
		if acc >= r:
			return k - 1
	return -1

## EXTRAIT l'arc [s0, s1] (abscisses curvilignes) d'une polyligne — `acc` = abscisses
## cumulées par sommet (précalculées par l'appelant). Bouts interpolés exactement.

## Résidu « spaghetti » POST-normalisation : même métrique que _count_spaghetti_segments
## (proche <SPAG_DIST, |cos|>SPAG_COS, écart perpendiculaire ≥SPAG_MIN_OFFSET, étapes
## DIFFÉRENTES) mais sur les étapes canoniques du chaînage — la mesure d'efficacité.
func _spag_steps(steps: Array) -> int:
	var segs := []
	for si in range(steps.size()):
		var pts: PackedVector2Array = steps[si]["poly"]
		for k in range(pts.size() - 1):
			if pts[k].distance_to(pts[k + 1]) < 0.05:
				continue
			segs.append({"a": pts[k], "mid": (pts[k] + pts[k + 1]) * 0.5,
				"dir": (pts[k + 1] - pts[k]).normalized(), "route": si})
	var grid := {}
	for si in range(segs.size()):
		var c: Vector2 = segs[si]["mid"]
		var gk := Vector2i(int(floor(c.x / SPAG_DIST)), int(floor(c.y / SPAG_DIST)))
		if not grid.has(gk):
			grid[gk] = []
		grid[gk].append(si)
	var flagged := {}
	for si in range(segs.size()):
		var sg: Dictionary = segs[si]
		var c: Vector2 = sg["mid"]
		var gk := Vector2i(int(floor(c.x / SPAG_DIST)), int(floor(c.y / SPAG_DIST)))
		for oy in range(-1, 2):
			for ox in range(-1, 2):
				var nk := gk + Vector2i(ox, oy)
				if not grid.has(nk):
					continue
				for sj in grid[nk]:
					if sj <= si:
						continue
					var o: Dictionary = segs[sj]
					if o["route"] == sg["route"]:
						continue
					if (sg["mid"] as Vector2).distance_to(o["mid"]) > SPAG_DIST:
						continue
					if absf((sg["dir"] as Vector2).dot(o["dir"])) < SPAG_COS:
						continue
					if _perp_offset(o["mid"], sg["a"], sg["dir"]) < SPAG_MIN_OFFSET:
						continue
					flagged[si] = true
					flagged[sj] = true
	return flagged.size()

func _sub_poly(pts: PackedVector2Array, acc: PackedFloat32Array, s0: float, s1: float) -> PackedVector2Array:
	var out := PackedVector2Array()
	var n := pts.size()
	for i in range(n - 1):
		var a0 := acc[i]
		var a1 := acc[i + 1]
		if a1 <= s0 or a0 >= s1 or a1 - a0 < 0.0001:
			continue
		var pa: Vector2 = pts[i]
		var pb: Vector2 = pts[i + 1]
		if a0 < s0:
			pa = pts[i].lerp(pts[i + 1], (s0 - a0) / (a1 - a0))
		if out.is_empty():
			out.append(pa)
		if a1 > s1:
			out.append(pts[i].lerp(pts[i + 1], (s1 - a0) / (a1 - a0)))
			break
		out.append(pb)
	return out

## portion BÂTIE d'un tracé (du départ, par longueur) — `frac` ∈ [0,1] → croissance organique.
func _road_partial(pts: PackedVector2Array, frac: float) -> PackedVector2Array:
	if frac >= 1.0:
		return pts
	if frac <= 0.0 or pts.size() < 2:
		return PackedVector2Array()
	var total := 0.0
	for i in range(pts.size() - 1):
		total += pts[i].distance_to(pts[i + 1])
	var target := total * frac
	var out := PackedVector2Array()
	out.append(pts[0])
	var acc := 0.0
	for i in range(pts.size() - 1):
		var seg := pts[i].distance_to(pts[i + 1])
		if acc + seg >= target:
			out.append(pts[i].lerp(pts[i + 1], (target - acc) / maxf(seg, 0.001)))
			break
		acc += seg
		out.append(pts[i + 1])
	return out

## ANTISPAG A2 — reconstruit (ou sert du CACHE) les polylignes ROUTE consolidées : dédup EXACTE des
## tronçons partagés (un couloir commun ne s'encre qu'UNE fois, à la première route qui le porte)
## + TIER d'épaisseur ∝ MULTIPLICITÉ (combien de routes logiques empruntent le tronçon — « troncs
## épais, capillaires fins »). Caché : reconstruit SEULEMENT si `_road_net_valid` est tombé (réseau
## changé, cf. `_ensure_roads`) OU si un chantier grandit encore (frac<1 quelque part — la
## croissance organique change le tracé PARTIEL affiché à chaque frame). Un monde mûr stable ne
## repaie donc plus ce coût par frame (budget mesuré : cf. TROUVAILLES, <1 ms sur un monde mûr).
func _ensure_road_network() -> void:
	var growing := false
	for rd in _roads:
		var st: int = _road_start.get(rd["key"], Sim.day_count)
		var nprov: int = maxi(1, int(rd.get("nprov", 1)))
		if float(Sim.day_count - st) / (float(nprov) * 365.0) < 1.0:
			growing = true
			break
	if _road_net_valid and not growing:
		return
	var mv := _mv_ref()
	# ══ CHAÎNAGE PAR VILLES (normalisation topologique, cadrage joueur 2026-07-31) ══
	# Le moteur fournit déjà UN A* par paire de villes voisines (scps_api.c
	# api_roads_build : 2 plus proches + dédup + attraction de corridors) — le tressage
	# résiduel vient des arêtes qui passent PRÈS d'une ville intermédiaire sans la
	# compter comme étape. Ici : chaque route est DÉCOMPOSÉE en étapes ville→ville
	# (projection curviligne, raccord terrestre, monotone), les étapes sont GROUPÉES
	# par paire non orientée et UNE géométrie canonique est retenue par paire
	# (plus ancien chantier → plus courte → clé — jamais « la première » : l'ordre du
	# tableau ne décide pas de la géographie). Règles de chantier progressif :
	#   visibilité d'étape = couverture MAX parmi ses routes parentes ;
	#   usure d'étape      = nb de parentes l'ayant ATTEINTE (couverture pleine).
	# Classes = level MOTEUR (0 grande · 1 régionale · 2 sentier), l'usure fonce.
	# INSTRUMENTÉ (print au rebuild) : mesurer avant de conclure.
	var st_routes := 0
	var st_wp := 0
	var st_rej_eau := 0
	var st_steps := 0
	var len_logique := 0.0
	var len_canon := 0.0
	var cand := {}                              # clé de paire → [candidats]
	for rd in _roads:
		var pts: PackedVector2Array = rd["points"]
		if pts.size() < 2:
			continue
		st_routes += 1
		var level := clampi(int(rd.get("level", 1)), 0, 2)
		var rstart := int(_road_start.get(rd["key"], Sim.day_count))
		var nprov := maxi(1, int(rd.get("nprov", 1)))
		var frac := clampf(float(Sim.day_count - rstart) / (float(nprov) * 365.0), 0.0, 1.0)
		var acc := PackedFloat32Array()          # abscisses curvilignes cumulées
		acc.resize(pts.size())
		var tot := 0.0
		for i in range(pts.size() - 1):
			acc[i] = tot
			tot += pts[i].distance_to(pts[i + 1])
		acc[pts.size() - 1] = tot
		if tot < 0.5:
			continue
		var built_len := tot * frac
		var ra := int(rd.get("ra", -1))
		var rb := int(rd.get("rb", -1))
		# waypoints : villes à ≤2.5 cellules du tracé, raccord TERRESTRE, loin des abouts
		var wps := []
		for rid in _region_anchor:
			if rid == ra or rid == rb:
				continue
			var A: Vector2 = _region_anchor[rid]
			var bd := 6.25                       # 2.5² — seuil d'acceptation
			var bs := -1.0
			var bp := Vector2.ZERO
			for i in range(pts.size() - 1):
				var ab: Vector2 = pts[i + 1] - pts[i]
				var l2 := ab.length_squared()
				if l2 < 0.0001:
					continue
				var tt := clampf((A - pts[i]).dot(ab) / l2, 0.0, 1.0)
				var proj: Vector2 = pts[i] + ab * tt
				var d2 := A.distance_squared_to(proj)
				if d2 < bd:
					bd = d2
					bs = acc[i] + sqrt(l2) * tt
					bp = proj
			if bs < 2.0 or bs > tot - 2.0:       # inexistant (-1) ou trop près d'un about
				continue
			var wet := false                     # raccord ville→projection entièrement terrestre
			var lr := A.distance_to(bp)
			var nq := maxi(1, int(lr / 0.5))
			for q in range(nq + 1):
				var pq: Vector2 = A.lerp(bp, float(q) / float(nq))
				if _water_at(int(pq.x), int(pq.y)):
					wet = true
					break
			if wet:
				st_rej_eau += 1
				continue
			wps.append([bs, rid])
			st_wp += 1
		wps.sort()                               # progression monotone le long de l'arc
		var nodes := [[0.0, ra]]
		for wp in wps:
			nodes.append(wp)
		nodes.append([tot, rb])
		for i in range(nodes.size() - 1):
			var s0: float = nodes[i][0]
			var s1: float = nodes[i + 1][0]
			if s1 - s0 < 1.0:
				continue                         # étape dégénérée (nœuds quasi confondus)
			var sub := _sub_poly(pts, acc, s0, s1)
			if sub.size() < 2:
				continue
			var na := int(nodes[i][1])
			var nb := int(nodes[i + 1][1])
			# identité de nœud : la ville (région) ; about sans région → cellule quantizée
			var ia := na if na >= 0 else 100000 + (int(sub[0].x) << 10) + int(sub[0].y)
			var ib := nb if nb >= 0 else 100000 + (int(sub[sub.size() - 1].x) << 10) + int(sub[sub.size() - 1].y)
			var pkey := "%d_%d" % [mini(ia, ib), maxi(ia, ib)]
			var cover := clampf((built_len - s0) / (s1 - s0), 0.0, 1.0)
			st_steps += 1
			if not cand.has(pkey):
				cand[pkey] = []
			(cand[pkey] as Array).append({"poly": sub, "len": s1 - s0, "start": rstart,
				"rkey": int(rd["key"]), "level": level, "cover": cover,
				"na": mini(ia, ib), "nb": maxi(ia, ib), "ea": ia, "eb": ib})
	# ── géométrie CANONIQUE par paire + visibilité/usure ──
	var steps := []
	for pkey in cand:
		var lst: Array = cand[pkey]
		lst.sort_custom(func(a, b):
			if int(a["start"]) != int(b["start"]):
				return int(a["start"]) < int(b["start"])
			if absf(float(a["len"]) - float(b["len"])) > 0.01:
				return float(a["len"]) < float(b["len"])
			return int(a["rkey"]) < int(b["rkey"]))
		var can: Dictionary = lst[0]
		var vis := 0.0
		var wear := 0
		var lvl := 2
		for c in lst:
			vis = maxf(vis, float(c["cover"]))
			if float(c["cover"]) >= 0.999:
				wear += 1
			lvl = mini(lvl, int(c["level"]))
			len_logique += float(c["len"]) * float(c["cover"])
		if vis <= 0.001:
			continue
		len_canon += float(can["len"]) * vis
		var geom: PackedVector2Array = can["poly"] if vis >= 0.999 else _road_partial(can["poly"], vis)
		steps.append({"poly": geom, "level": lvl, "wear": clampi(wear, 1, ROAD_MULT_TIERS),
			"na": can["na"], "nb": can["nb"], "ea": int(can["ea"]), "eb": int(can["eb"]),
			"start": int(can["start"]), "done": vis >= 0.999})
	# ── CONSOLIDATION (cadrage joueur v2, 2026-07-31) — ordre : âge → terminal
	# unique par ville → fusion structurelle de corridors. Les VIEUX tracés gagnent. ──
	steps.sort_custom(func(a, b):
		if int(a["start"]) != int(b["start"]):
			return int(a["start"]) < int(b["start"])
		return String("%d_%d" % [int(a["na"]), int(a["nb"])]) < String("%d_%d" % [int(b["na"]), int(b["nb"])]))
	var junctions := []                          # carrefours créés (terminaux + bouts de troncs partagés)
	# ── TERMINAL UNIQUE PAR VILLE : toutes les approches d'un même SECTEUR (~35°)
	# rejoignent LA même courte géométrie terminale (celle du plus ancien) — l'étoile
	# confuse devient un tronc de porte + un carrefour à la frontière urbaine. ──
	var st_term := 0
	var r_term := 4.0
	var by_city := {}
	for si in range(steps.size()):
		var stp: Dictionary = steps[si]
		for endi in range(2):
			var nid := int(stp["ea"]) if endi == 0 else int(stp["eb"])
			if nid >= 100000 or not _region_anchor.has(nid):
				continue
			by_city["%d" % nid] = true
	for ck in by_city:
		var rid := int(ck)
		var A3: Vector2 = _region_anchor[rid]
		var leaders := []                        # [{angle, si, at_start}] par ordre d'âge
		for si in range(steps.size()):
			var stp: Dictionary = steps[si]
			var at_start := int(stp["ea"]) == rid
			var at_end := int(stp["eb"]) == rid
			if not at_start and not at_end:
				continue
			var poly3: PackedVector2Array = (stp["poly"] as PackedVector2Array).duplicate()
			if at_start:
				poly3.reverse()                  # normalise : la ville au BOUT
			var bidx := _urban_boundary(poly3, r_term)
			if bidx < 1:
				continue                         # tracé trop court pour un terminal
			var ang := (poly3[bidx] - A3).angle()
			var merged := false
			for ld in leaders:
				var da := absf(fposmod(ang - float(ld["angle"]) + PI, TAU) - PI)
				if da > 0.6:
					continue
				# SUIVEUR : tronqué à SA frontière, raccordé à la queue du CHEF
				var lstp: Dictionary = steps[int(ld["si"])]
				var lpoly: PackedVector2Array = (lstp["poly"] as PackedVector2Array).duplicate()
				if bool(ld["at_start"]):
					lpoly.reverse()
				var lb := _urban_boundary(lpoly, r_term)
				if lb < 1:
					break
				var np := poly3.slice(0, bidx + 1)
				np.append_array(lpoly.slice(lb))
				if at_start:
					np.reverse()
				stp["poly"] = np
				junctions.append(lpoly[lb])      # le carrefour de porte
				st_term += 1
				merged = true
				break
			if not merged:
				leaders.append({"angle": ang, "si": si, "at_start": at_start})
	# ── FUSION DE CORRIDORS SOUTENUS v2 (seuils joueur : ≤3.5 cellules · ~20° ·
	# ≥8 cellules CONSÉCUTIVES) — STRUCTURELLE : le suiveur est SCINDÉ aux deux
	# JONCTIONS, sa portion médiane DISPARAÎT (une seule bande par corridor, jamais
	# d'empilement « autoroute lumineuse ») et l'USURE du tronc s'incrémente. Un
	# croisement ponctuel ne tient jamais la longueur minimale. ──
	var st_fused := 0
	var steps2 := []
	var fgrid := {}                              # grille (cellule 2.0) → [[a, b, index steps2]]
	for stp in steps:
		var poly: PackedVector2Array = stp["poly"]
		var n := poly.size()
		var pieces := []
		if n >= 3 and not fgrid.is_empty():
			var near := PackedInt32Array()       # cible steps2 par point (-1 = libre)
			var proj := PackedVector2Array()
			near.resize(n)
			proj.resize(n)
			for k in range(n):
				near[k] = -1
				var pk2: Vector2 = poly[k]
				var dloc := (poly[mini(k + 1, n - 1)] - poly[maxi(k - 1, 0)]).normalized()
				var gk := Vector2i(int(floor(pk2.x / 2.0)), int(floor(pk2.y / 2.0)))
				var bd2 := 12.25                 # 3.5²
				for oy in range(-2, 3):
					for ox in range(-2, 3):
						var lst2: Variant = fgrid.get(Vector2i(gk.x + ox, gk.y + oy))
						if lst2 == null:
							continue
						for sg in lst2:
							var sa: Vector2 = sg[0]
							var sb: Vector2 = sg[1]
							var ab2: Vector2 = sb - sa
							var l22 := ab2.length_squared()
							if l22 < 0.0001:
								continue
							if absf(dloc.dot(ab2 / sqrt(l22))) < 0.94:
								continue         # ~20° : un croisement ne fusionne pas
							var tt2 := clampf((pk2 - sa).dot(ab2) / l22, 0.0, 1.0)
							var pr2: Vector2 = sa + ab2 * tt2
							var dd2 := pk2.distance_squared_to(pr2)
							if dd2 < bd2:
								bd2 = dd2
								near[k] = int(sg[2])
								proj[k] = pr2
			var cuts := []                       # [ka, kb, cible] — runs soutenus ≥ 8 cellules
			var k0 := 0
			while k0 < n:
				if near[k0] < 0:
					k0 += 1
					continue
				var k1 := k0
				var arc := 0.0
				while k1 + 1 < n and near[k1 + 1] >= 0:
					arc += poly[k1].distance_to(poly[k1 + 1])
					k1 += 1
				if arc >= 8.0:
					var votes := {}
					for kk in range(k0, k1 + 1):
						votes[near[kk]] = int(votes.get(near[kk], 0)) + 1
					var best_t := -1
					var best_v := 0
					for tsi in votes:
						if int(votes[tsi]) > best_v:
							best_v = int(votes[tsi])
							best_t = int(tsi)
					cuts.append([k0, k1, best_t])
				k0 = k1 + 1
			if cuts.is_empty():
				pieces.append(poly)
			else:
				var prev_idx := 0
				var has_pj := false
				var prev_j := Vector2.ZERO
				for c4 in cuts:
					var ka: int = c4[0]
					var kb: int = c4[1]
					var tsi: int = c4[2]
					var head := PackedVector2Array()
					if has_pj:
						head.append(prev_j)
					head.append_array(poly.slice(prev_idx, ka + 1))
					head.append(proj[ka])        # raccord à la jonction AMONT
					if head.size() >= 3:
						pieces.append(head)
					junctions.append(proj[ka])
					junctions.append(proj[kb])
					if tsi >= 0 and tsi < steps2.size():
						steps2[tsi]["wear"] = int(steps2[tsi]["wear"]) + 1   # l'usure du tronc
					st_fused += kb - ka + 1
					prev_idx = kb + 1
					has_pj = true
					prev_j = proj[kb]
				var tail := PackedVector2Array()
				if has_pj:
					tail.append(prev_j)
				tail.append_array(poly.slice(prev_idx))
				if tail.size() >= 3:
					pieces.append(tail)
		else:
			pieces.append(poly)
		for pc0 in pieces:
			var pc: PackedVector2Array = pc0
			var arcp := 0.0
			for k in range(pc.size() - 1):
				arcp += pc[k].distance_to(pc[k + 1])
			if pc.size() < 2 or arcp < 1.5:
				continue                         # moignon : jamais dessiné
			var ns: Dictionary = stp.duplicate()
			ns["poly"] = pc
			steps2.append(ns)
			var nsi := steps2.size() - 1
			for k in range(pc.size() - 1):       # cette pièce ENTRE dans le réseau retenu
				var mid2: Vector2 = (pc[k] + pc[k + 1]) * 0.5
				var gk2 := Vector2i(int(floor(mid2.x / 2.0)), int(floor(mid2.y / 2.0)))
				if not fgrid.has(gk2):
					fgrid[gk2] = []
				(fgrid[gk2] as Array).append([pc[k], pc[k + 1], nsi])
	steps = steps2
	# ── BUCKETS par classe×usure×forêt (dédup seg anti-overdraw) + masque TROUÉE ──
	var polys := {}
	for t in range(1, ROAD_MULT_TIERS + 1):
		for cls in ["main", "minor", "trail"]:
			polys["%s%d" % [cls, t]] = []
			polys["%s%df" % [cls, t]] = []
	var seen := {}
	var mask := {}                              # TROUÉE : cellule → niveau (2=grande, 1=régionale)
	for stp in steps:
		var poly: PackedVector2Array = stp["poly"]
		var cls: String = ["main", "minor", "trail"][int(stp["level"])]
		var wear: int = clampi(int(stp["wear"]), 1, ROAD_MULT_TIERS)
		var run := PackedVector2Array()
		var run_forest := false
		for k in range(poly.size() - 1):
			var a7: Vector2 = poly[k]
			var b7: Vector2 = poly[k + 1]
			var kseg := _seg_key(a7, b7)
			var mid := (a7 + b7) * 0.5
			var inf := _forest_at(int(mid.x), int(mid.y))
			if int(stp["level"]) < 2:            # la trouée suit grandes + régionales, jamais les sentiers
				var lvl2 := 2 if int(stp["level"]) == 0 else 1
				var seg7 := a7.distance_to(b7)
				var n7 := maxi(1, int(seg7 / 0.7))
				for q in range(n7 + 1):
					var p7 := a7.lerp(b7, float(q) / float(n7))
					var mk := (int(p7.x) << 16) | (int(p7.y) & 0xFFFF)
					mask[mk] = maxi(int(mask.get(mk, 0)), lvl2)
			if seen.has(kseg) or (run.size() >= 2 and inf != run_forest):
				if run.size() >= 2:
					polys["%s%d%s" % [cls, wear, "f" if run_forest else ""]].append(_road_iso(run, mv))
				run = PackedVector2Array()
				if seen.has(kseg):
					continue
			seen[kseg] = true
			if run.is_empty():
				run.append(a7)
				run_forest = inf
			run.append(b7)
		if run.size() >= 2:
			polys["%s%d%s" % [cls, wear, "f" if run_forest else ""]].append(_road_iso(run, mv))
	# ── PASSE 3 « sol usé » : bandes claires + ornières par bucket (paires cachées) ──
	var cell: float = ((mv.iso_pos(1.0, 0.0) - mv.iso_pos(0.0, 0.0)).length()
			   + (mv.iso_pos(0.0, 1.0) - mv.iso_pos(0.0, 0.0)).length()) * 0.5
	var solid: float = ROAD_SOLID_END * cell
	var rut_off := 0.85                          # écart des 2 ornières (unités iso)
	for t in range(1, ROAD_MULT_TIERS + 1):
		for key in ["main%d" % t, "main%df" % t, "minor%d" % t, "minor%df" % t, "trail%d" % t, "trail%df" % t]:
			var is_mn: bool = key.begins_with("main")
			var sentier: bool = key.begins_with("trail")
			var band := PackedVector2Array()
			var ruts := PackedVector2Array()
			var rdash := ROAD_RUT_DASH * (1.0 + 0.18 * float(t - 1))          # l'usure DENSIFIE
			var rgap := ROAD_RUT_GAP * maxf(0.55, 1.0 - 0.22 * float(t - 1))  # (jamais une 2e bande)
			for pl in polys[key]:
				if sentier:
					ruts.append_array(_dash_pairs(pl, ROAD_TRAIL_DASH, ROAD_TRAIL_GAP, ROAD_JIT, solid))
					continue
				band.append_array(_dash_pairs(pl, ROAD_BAND_DASH, ROAD_BAND_GAP, ROAD_JIT * 0.6, solid))
				if is_mn:
					# ornières DÉSAXÉES (+0.85/−0.74) et DÉPHASÉES (périodes ≠) : casse l'effet rails
					ruts.append_array(_dash_pairs(_offset_poly(pl, rut_off), rdash, rgap, ROAD_JIT, solid))
					ruts.append_array(_dash_pairs(_offset_poly(pl, -rut_off * 0.87), rdash * 1.13, rgap * 0.86, ROAD_JIT, solid))
				else:
					ruts.append_array(_dash_pairs(pl, rdash, rgap, ROAD_JIT, solid))
			polys["b_" + key] = band
			polys["r_" + key] = ruts
	# ── PADS de porte : aux NŒUDS-VILLES touchés par ≥1 étape visible ──
	var pad_by_city := {}                        # rid → rayon max (grande route = plus large)
	for stp in steps:
		var r_pad := (1.0 if int(stp["level"]) == 0 else 0.7)
		for nid in [int(stp["na"]), int(stp["nb"])]:
			if nid < 100000 and _region_anchor.has(nid):
				pad_by_city[nid] = maxf(float(pad_by_city.get(nid, 0.0)), r_pad)
	var pads := []
	for rid in pad_by_city:
		var A2: Vector2 = _region_anchor[rid]
		pads.append([mv.iso_pos(A2.x, A2.y), float(pad_by_city[rid]) * cell, rid])
	polys["pads"] = pads
	var jdone := {}
	var juncs := []
	for jp in junctions:
		var jk := Vector2i(int(round(jp.x / 1.5)), int(round(jp.y / 1.5)))
		if jdone.has(jk):
			continue
		jdone[jk] = true
		juncs.append([mv.iso_pos(jp.x, jp.y), 0.55 * cell, jk.x * 131 + jk.y])
	polys["junc"] = juncs
	# ── INSTRUMENTATION (cadrage joueur : mesurer avant de conclure) ──
	if st_routes > 0:
		var reuse := (1.0 - len_canon / len_logique) * 100.0 if len_logique > 0.001 else 0.0
		var gchg := 0                            # étapes dont la géométrie a changé depuis le dernier rebuild
		var gsig := {}
		for stp in steps:
			var pl2: PackedVector2Array = stp["poly"]
			var kk3 := "%d_%d" % [int(stp["na"]), int(stp["nb"])]
			gsig[kk3] = pl2.size() * 100000 + int(pl2[0].x * 7.0) + int(pl2[pl2.size() - 1].y * 13.0)
			if _chain_geo.has(kk3) and int(_chain_geo[kk3]) != int(gsig[kk3]):
				gchg += 1
		_chain_geo = gsig
		print("[CHAINAGE] routes=%d wp=%d (rej eau %d) étapes=%d uniques=%d réutil=%.0f%% | term=%d fusion=%d pts junc=%d | spag routes=%d → étapes=%d | géo-chg=%d" %
			[st_routes, st_wp, st_rej_eau, st_steps, steps.size(), reuse, st_term, st_fused, juncs.size(), _count_spaghetti_segments(), _spag_steps(steps), gchg])
	# TROUÉE FORESTIÈRE : dilate le masque en carte de DENSITÉ de canopée — cœur de
	# grande route : 0 arbre (≤1.5 cellule) puis bords clairsemés ; régionale : plus
	# étroit. Le semis (_build_dressing) lit _road_clear ; quand le réseau a poussé
	# d'assez de cellules, le dressing se RE-SÈME (àcoups annuels — la forêt s'ouvre).
	_road_clear.clear()
	for mk in mask:
		var lvl: int = mask[mk]
		var mx: int = int(mk) >> 16
		var my: int = int(mk) & 0xFFFF
		for dy in range(-3, 4):
			for dx in range(-3, 4):
				var d2 := dx * dx + dy * dy
				var dens := 1.0
				if lvl == 2:
					if d2 <= 2:   dens = 0.0      # cœur : aucune canopée
					elif d2 <= 7: dens = 0.45     # bord : arbres plus rares
				else:
					if d2 <= 1:   dens = 0.0
					elif d2 <= 4: dens = 0.60
				if dens < 1.0:
					var nk: int = ((mx + dx) << 16) | ((my + dy) & 0xFFFF)
					_road_clear[nk] = minf(float(_road_clear.get(nk, 1.0)), dens)
	if absi(_road_clear.size() - _road_clear_n) > 40:
		_road_clear_n = _road_clear.size()
		_dressing_dirty = true                  # la trouée s'ouvre : re-semis (annuel, jamais par frame)
	_road_net = polys
	_road_net_valid = true

## Découpe une polyligne (DÉJÀ projetée iso) en PAIRES tiretées IRRÉGULIÈRES pour
## draw_multiline — l'algo de _lane_dash_iso (phase continue le long de l'arc,
## plancher ε + fusible anti-hang) sans la projection, avec : longueurs de tiret/trou
## JITTÉES par hash déterministe (`jit` = ±fraction), et `solid` unités CONTINUES à
## chaque about (le raccord à la porte du bourg ne finit jamais sur un trou).
func _dash_pairs(pts: PackedVector2Array, dash: float, gap: float, jit := 0.0, solid := 0.0) -> PackedVector2Array:
	var out := PackedVector2Array()
	if pts.size() < 2:
		return out
	var total := 0.0
	for i in range(pts.size() - 1):
		total += pts[i].distance_to(pts[i + 1])
	var t := 0.0
	var di := 0                                  # compteur de tirets → hash du jitter
	var cur_dash := dash
	var cur_gap := gap
	for i in range(pts.size() - 1):
		var a: Vector2 = pts[i]
		var b: Vector2 = pts[i + 1]
		var seg := a.distance_to(b)
		if seg < 0.0001:
			continue
		var dir := (b - a) / seg
		var s := 0.0
		var guard := 0
		while s < seg - 0.0001:
			guard += 1
			if guard > 100000:
				break
			var period := cur_dash + cur_gap
			var phase := fmod(t, period)
			var in_dash := phase < cur_dash
			var lim := (cur_dash - phase) if in_dash else (period - phase)
			var step := maxf(minf(lim, seg - s), 0.01)   # plancher ε : jamais de pas nul (piège float des lanes)
			# ABOUTS CONTINUS : à moins de `solid` du départ OU de l'arrivée, tout est tiret.
			var solid_here := solid > 0.0 and (t < solid or (total - t) < solid + step)
			if in_dash or solid_here:
				out.push_back(a + dir * s)
				out.push_back(a + dir * minf(s + step, seg))
			s += step
			t += step
			if not in_dash and phase + step >= period - 0.0001 and jit > 0.0:
				di += 1                          # nouveau cycle : re-tire les longueurs (hash stable)
				cur_dash = dash * (1.0 + (_h1(float(di) * 17.3) - 0.5) * 2.0 * jit)
				cur_gap = gap * (1.0 + (_h1(float(di) * 31.7) - 0.5) * 2.0 * jit)
	return out

## Décale une polyligne iso de `off` le long de sa NORMALE (les 2 ornières parallèles
## d'une grande route). Normale par sommet = moyenne des normales des segments voisins.
func _offset_poly(pts: PackedVector2Array, off: float) -> PackedVector2Array:
	var n := pts.size()
	var out := PackedVector2Array()
	if n < 2:
		return out
	out.resize(n)
	for i in range(n):
		var d := Vector2.ZERO
		if i > 0:
			d += (pts[i] - pts[i - 1]).normalized()
		if i < n - 1:
			d += (pts[i + 1] - pts[i]).normalized()
		d = d.normalized()
		out[i] = pts[i] + Vector2(-d.y, d.x) * off
	return out

## distance PERPENDICULAIRE du milieu de `s` à la droite portée par le segment `o` (projection —
## retire la composante LE LONG de `o`, ne garde que l'écart LATÉRAL). Décisif pour la métrique
## spaghetti : cf. découverte ci-dessous, deux segments peuvent avoir des milieux proches SANS
## être visuellement décalés (juste déphasés le long de la MÊME droite).
func _perp_offset(smid: Vector2, oa: Vector2, odir: Vector2) -> float:
	var d: Vector2 = smid - oa
	var along: float = d.dot(odir)
	return (d - odir * along).length()

## ANTISPAG A1 — MÉTRIQUE de spaghetti (utilisée par viewer_audit) : compte les SEGMENTS qui ont
## encore un voisin PROCHE (< SPAG_DIST cellules, milieu à milieu), QUASI-PARALLÈLE (cos > SPAG_COS)
## ET RÉELLEMENT DÉCALÉ (écart perpendiculaire ≥ SPAG_MIN_OFFSET), appartenant à une AUTRE route
## logique. Le test d'ÉCART (pas une comparaison de clé — DÉLIBÉRÉ, voir découverte) EST le critère
## de fusion : deux segments à écart quasi nul sont déjà visuellement la MÊME encre, peu importe si
## leurs clés de dédup (`_seg_key`, utilisées ailleurs pour le rendu) coïncident ou non.
## DÉCOUVERTE (analyse hors-Godot, cf. TROUVAILLES) : sans le filtre SPAG_MIN_OFFSET, la mesure
## comptait ~78-82 % de FAUX POSITIFS — des paires de segments dont le tracé A* CONVERGE déjà
## (magnétisme réussi, sommets partagés) mais dont l'ÉCHANTILLONNAGE indépendant par route déphase
## la DÉCOUPE en segments (le « joint » entre 2 segments ne tombe pas au même endroit sur les 2
## tracés) → écart perpendiculaire quasi nul (médiane 0.02-0.1 cellule, donc SOUS le pixel à tout
## zoom de lecture) : c'est de l'encre redondante INVISIBLE, pas du spaghetti VISIBLE. Le vrai
## défaut signalé par le joueur — deux traits visuellement écartés — ne représentait qu'environ
## 15-20 % du total brut. Filtrer par écart perpendiculaire isole le signal réel. Hash spatial
## (cellule SPAG_DIST) pour rester linéaire même sur un monde à plusieurs milliers de points.
## Mesuré en cellules (espace monde), pas en pixels écran : indépendant du zoom.
func _count_spaghetti_segments() -> int:
	var segs := []
	for ri in range(_roads.size()):
		var pts: PackedVector2Array = _roads[ri]["points"]
		for k in range(pts.size() - 1):
			var a: Vector2 = pts[k]
			var b: Vector2 = pts[k + 1]
			if a.distance_to(b) < 0.05:
				continue
			segs.append({"a": a, "mid": (a + b) * 0.5, "dir": (b - a).normalized(), "route": ri, "sea": false})
	# PORTULAN (MARITIME N4) : la métrique COUVRE les lanes — même défaut, même mesure
	# (le magnétisme marin travaille dès l'entrée). Les paires MIXTES terre/mer ne
	# comptent pas (une route côtière et une lane de cabotage qui longent la même côte
	# sont deux MÉDIAS, pas un doublon d'encre) : le test "sea"=="sea" plus bas.
	for li in range(_lanes.size()):
		var lpts: PackedVector2Array = _lanes[li]["points"]
		for k in range(lpts.size() - 1):
			var a2: Vector2 = lpts[k]
			var b2: Vector2 = lpts[k + 1]
			if a2.distance_to(b2) < 0.05:
				continue
			segs.append({"a": a2, "mid": (a2 + b2) * 0.5, "dir": (b2 - a2).normalized(),
				"route": _roads.size() + li, "sea": true})
	var grid := {}
	for si in range(segs.size()):
		var c: Vector2 = segs[si]["mid"]
		var gk := Vector2i(int(floor(c.x / SPAG_DIST)), int(floor(c.y / SPAG_DIST)))
		if not grid.has(gk):
			grid[gk] = []
		grid[gk].append(si)
	var flagged := {}
	for si in range(segs.size()):
		var s: Dictionary = segs[si]
		var c: Vector2 = s["mid"]
		var gk := Vector2i(int(floor(c.x / SPAG_DIST)), int(floor(c.y / SPAG_DIST)))
		for oy in range(-1, 2):
			for ox in range(-1, 2):
				var nk := gk + Vector2i(ox, oy)
				if not grid.has(nk):
					continue
				for sj in grid[nk]:
					if sj <= si:
						continue
					var o: Dictionary = segs[sj]
					if o["route"] == s["route"]:
						continue
					if bool(o["sea"]) != bool(s["sea"]):
						continue          # médias différents (terre vs mer) : jamais un doublon
					if (s["mid"] as Vector2).distance_to(o["mid"]) > SPAG_DIST:
						continue
					if absf((s["dir"] as Vector2).dot(o["dir"])) < SPAG_COS:
						continue
					if _perp_offset(s["mid"], o["a"], o["dir"]) < SPAG_MIN_OFFSET:
						continue
					flagged[si] = true
					flagged[sj] = true
	return flagged.size()

## SNAP d'extrémité : retire les points qui tanglent dans le rayon de l'ancre, puis raccorde l'ancre
## (pied des marches de l'asset) au 1er point survivant → approche RADIALE nette (toujours ≥ 2 points).
func _snap_endpoint(pts: PackedVector2Array, anchor: Vector2, from_start: bool) -> PackedVector2Array:
	if pts.size() < 3:
		var two := PackedVector2Array()
		if from_start:
			two.append(anchor); two.append(pts[pts.size() - 1])
		else:
			two.append(pts[0]); two.append(anchor)
		return two
	var out := PackedVector2Array()
	if from_start:
		var i := 0
		while i < pts.size() - 2 and pts[i].distance_to(anchor) < ROAD_SNAP_TRIM:
			i += 1
		out.append(anchor)
		for k in range(i, pts.size()):
			out.append(pts[k])
	else:
		var j := pts.size() - 1
		while j > 1 and pts[j].distance_to(anchor) < ROAD_SNAP_TRIM:
			j -= 1
		for k in range(0, j + 1):
			out.append(pts[k])
		out.append(anchor)
	return out

## ré-échantillonne un tracé à PAS CONSTANT (cellules) — points réguliers, water-safe (on interpole
## sur des segments DÉJÀ sûrs). Garde exactement la 1re et la dernière position (le snapping tient).
func _resample_polyline(pts: PackedVector2Array, spacing: float) -> PackedVector2Array:
	var out := PackedVector2Array()
	if pts.size() < 2:
		return pts
	out.append(pts[0])
	var cur: Vector2 = pts[0]
	var acc := 0.0
	var i := 1
	while i < pts.size():
		var nxt: Vector2 = pts[i]
		var seg := cur.distance_to(nxt)
		if seg < 0.0001:
			cur = nxt; i += 1; continue
		if acc + seg >= spacing:
			cur = cur.lerp(nxt, (spacing - acc) / seg)
			out.append(cur)
			acc = 0.0
		else:
			acc += seg
			cur = nxt
			i += 1
	var endp: Vector2 = pts[pts.size() - 1]
	if out[out.size() - 1].distance_to(endp) > 0.001:
		out.append(endp)
	return out

## RATTRAPE un point qui tombe en MER/LAC (jamais la rivière : un fleuve se FRANCHIT, au pont —
## ce n'est pas une erreur) — le lissage MOTEUR (`api_road_smooth`, moyenne mobile 3 passes,
## non water-aware) peut tirer un sommet dans l'eau sur une côte concave serrée ; `_chaikin_safe`
## ne garde que ses PROPRES coins coupés (il retombe sur le sommet d'origine comme « sûr » sans le
## vérifier) → le sommet fautif SURVIT jusqu'au rendu (retour joueur/audit : « route dans la mer »,
## viewer_audit ⚠route-mer-path). Recherche en anneaux croissants (rayon ≤ ROAD_WATER_SNAP_R
## cellules) la terre la plus proche — même esprit que `api_snap_land` côté moteur, mais ici
## PUREMENT display : on ne fait QUE corriger le TRACÉ dessiné, aucune sémantique ne bouge.
const ROAD_WATER_SNAP_R := 4
func _snap_water_points(pts: PackedVector2Array, sea: Image) -> PackedVector2Array:
	if sea == null:
		return pts
	var out := pts.duplicate()
	for i in range(1, out.size() - 1):    # jamais les extrémités (déjà ancrées par _snap_endpoint)
		var p: Vector2 = out[i]
		if not _is_sea_cell(sea, int(p.x), int(p.y)):
			continue
		var best := p
		var bestd := INF
		for R in range(1, ROAD_WATER_SNAP_R + 1):
			for dy in range(-R, R + 1):
				for dx in range(-R, R + 1):
					if maxi(absi(dx), absi(dy)) != R:
						continue                       # anneau R seul (pas le disque entier — pas de biais diagonal)
					var q: Vector2 = p + Vector2(dx, dy)
					if _is_sea_cell(sea, int(q.x), int(q.y)):
						continue
					var dd := p.distance_squared_to(q)
					if dd < bestd:
						bestd = dd; best = q
			if bestd < INF:
				break                                  # cet anneau a livré une terre → inutile d'élargir
		out[i] = best
	return out

## Chaikin (corner-cutting) GARDÉ-EAU : un point coupé qui tomberait en eau (mer/lac/rivière) est
## remplacé par le coin d'origine → la route se LISSE partout SAUF à la côte, qu'elle continue d'épouser.
func _chaikin_safe(pts: PackedVector2Array, sea: Image, rf: Image) -> PackedVector2Array:
	if pts.size() < 3:
		return pts
	var out := PackedVector2Array()
	out.append(pts[0])
	for i in range(pts.size() - 1):
		var a: Vector2 = pts[i]
		var b: Vector2 = pts[i + 1]
		var q := a.lerp(b, 0.25)
		var r := a.lerp(b, 0.75)
		out.append(a if (_is_sea_cell(sea, int(q.x), int(q.y)) or _in_river_water(rf, int(q.x), int(q.y))) else q)
		out.append(b if (_is_sea_cell(sea, int(r.x), int(r.y)) or _in_river_water(rf, int(r.x), int(r.y))) else r)
	out.append(pts[pts.size() - 1])
	return out

## tracé RENDU : ré-échantillonné régulier + 2 passes Chaikin gardées-eau (courbe propre, extrémités fixes).
func _smooth_resample_road(pts: PackedVector2Array, sea: Image, rf: Image) -> PackedVector2Array:
	if pts.size() < 3:
		return pts
	var rs := _resample_polyline(pts, ROAD_RESAMPLE)
	rs = _snap_water_points(rs, sea)
	rs = _chaikin_safe(rs, sea, rf)
	rs = _chaikin_safe(rs, sea, rf)
	return rs

func _is_sea_cell(sea: Image, ix: int, iy: int) -> bool:
	if sea == null or ix < 0 or iy < 0 or ix >= sea.get_width() or iy >= sea.get_height():
		return false
	return int(sea.get_pixel(ix, iy).r * 255.0 + 0.5) >= 1

## le CHAMP DÉBIT carvé (L8) que le shader iso_blend rend en EAU — récupéré du nœud IsoGround voisin.
## Sert à interdire la BASE d'un asset dans l'eau de RIVIÈRE (la mer/lac est déjà couverte par la couche WATER).
const RIVER_WATER_MIN := 0.26   ## champ ≥ ça = ZONE INTERDITE aux assets : l'eau rendue (shader 0.40) + une
                                ## VRAIE marge de berge (les arbres/bâtis débordaient sur le fleuve) → plus rien dans/au bord du fleuve
func _carved_river_field() -> Image:
	var p := get_parent()
	if p == null:
		return null
	var g = p.get_node_or_null("IsoGround")
	if g != null and g.has_method("river_field"):
		return g.river_field(Sim.world)
	return null

## VRAI si la BASE (ix,iy) tombe dans l'eau de rivière carvée (≥ river_water) → pas d'asset dessus.
func _in_river_water(rf: Image, ix: int, iy: int) -> bool:
	if rf == null or ix < 0 or iy < 0 or ix >= rf.get_width() or iy >= rf.get_height():
		return false
	return rf.get_pixel(ix, iy).r >= RIVER_WATER_MIN

## VRAI si (x,y) est SUR ou À CÔTÉ d'une rivière VISIBLE (seuil BAS + voisinage 1 cellule) → aucune marque
## de dressing ici (sinon, la rivière étant translucide, la marque transparaît « sous » l'eau = artefact).
const DRESS_RIVER_MIN := 0.08   ## seuil bas (la rivière du shader s'imprime dès ~0.13 ; on l'attrape + marge)
func _near_river(rf: Image, x: int, y: int, rad := 1) -> bool:
	if rf == null:
		return false
	var rw := rf.get_width()
	var rh := rf.get_height()
	for dy in range(-rad, rad + 1):
		for dx in range(-rad, rad + 1):
			var nx := x + dx
			var ny := y + dy
			if nx < 0 or ny < 0 or nx >= rw or ny >= rh:
				continue
			if rf.get_pixel(nx, ny).r >= DRESS_RIVER_MIN:
				return true
	return false

var _sig_poll := 0.0
func _process(dt: float) -> void:
	# pendant un cataclysme, on redessine en continu pour PULSER l'épicentre
	# (horloge MUR, hors déterminisme). Sinon : aucun coût (le tick suffit).
	# (Le suivi caméra-sans-input vit chez le PROPRIÉTAIRE de la caméra : map_view._process
	#  → _nav_redraw — un seul poll pour sol + overlay + carte, pas un par calque.)
	if _cataclysm:
		queue_redraw()
	# FRONTIÈRES/ASSETS DÉCOUPLÉS DU TICK : jadis seul `_on_tick` (qui ne fire PAS en
	# pause) recalculait la souveraineté → frontières/routes/villes ne se rafraîchissaient
	# qu'au DÉBLOCAGE de la pause. On sonde ~4×/s (même en pause) : la carte reflète l'état
	# COURANT, elle n'attend plus qu'on relève la pause.
	_sig_poll += dt
	if _sig_poll >= 0.25:
		_sig_poll = 0.0
		if _poll_world_changes():          # revue #11 : même diff/mêmes flags que _on_tick, factorisé
			queue_redraw()

## Anneau doré autour du pion du joueur SÉLECTIONNÉ (mode marche : cliquez une destination).
func _draw_army_ring(ctr: Vector2, s: float, zoom: float) -> void:
	var r := s * 0.62
	draw_arc(ctr, r + 3.0 / zoom, 0.0, TAU, 40, Color(0.10, 0.08, 0.03, 0.7), 4.0 / zoom, true)
	draw_arc(ctr, r, 0.0, TAU, 40, Color(1.0, 0.86, 0.36, 0.95), 2.4 / zoom, true)

## Le clic (en espace LOCAL de l'overlay = iso) touche-t-il le pion du joueur ? Hit-test TYPÉ
## (revue overlay #1) : seules les clés INT (corps réels) comptent — les garnisons (clé "g<pays>")
## ne sont pas des corps et ne peuvent pas être ordonnées en marche, donc jamais retournées ici.
func point_hits_player_army(local: Vector2) -> int:
	var best := -1
	var best_d := INF
	for id in _pa_positions:
		if typeof(id) != TYPE_INT:
			continue                      # garnison : pas un corps, hors hit-test
		var p: Vector2 = _pa_positions[id]["pos"]
		var d := local.distance_to(p)
		if d <= maxf(float(_pa_positions[id]["radius"]), 6.0) and d < best_d:
			best = int(id); best_d = d
	return best

## idem : seules les clés INT (corps réels) peuvent entrer dans une sélection au rectangle.
func player_corps_in_rect(rect: Rect2) -> Array[int]:
	var out: Array[int] = []
	for id in _pa_positions:
		if typeof(id) != TYPE_INT:
			continue                      # garnison : exclue (pas un corps réel)
		if rect.has_point(_pa_positions[id]["pos"]): out.append(int(id))
	return out

## GARNISON : la réserve MOBILISÉE d'un pays (régiments recrutés, PAS en campagne) — un
## pion à la capitale, pour qu'une armée levée SE VOIE. Plus petit que l'ost de campagne ;
## fog-gaté (les tiennes toujours visibles). Lit country_army/country_capital_region.
func _draw_garrison(w, mv, c: int, zoom: float, human_idx: int) -> void:
	if not w.has_method("country_army") or not w.has_method("country_capital_region"):
		return
	var reg_n := int(w.country_army(c).get("regiments", 0))
	if reg_n <= 0:
		return
	var creg := int(w.country_capital_region(c))
	if creg < 0:
		return
	if c != human_idx and not _fog_visible_region(creg):
		return
	# POSE sur le SIÈGE (la ville dessinée), pas le centroïde géométrique de la région —
	# sinon la garnison flotte loin de la province (retour joueur « apparaît loin »).
	var rc: Vector2 = _region_seat.get(creg, w.region_centroid(creg))
	if rc.x < 0:
		return
	var ctr: Vector2 = mv.iso_pos(rc.x, rc.y)
	var s := _w(zoom, 5.0, 22.0, 48.0)         # plus discret que l'ost de campagne (34..74)
	if c == human_idx:
		# clé PRÉFIXÉE "g<pays>" (revue #1) : distincte de l'espace des id de CORPS — sans ça une
		# garnison keyée par index de pays pouvait coïncider avec un corps_id réel et se faire
		# renvoyer par point_hits_player_army/player_corps_in_rect comme s'il s'agissait de lui.
		_pa_positions["g%d" % c] = {"pos":ctr,"radius":s*0.7}
		# PAS de `if c in selected_corps` ici (ancien bug #1) : une garnison n'a pas de corps_id,
		# la comparer à des ids de corps était vraie PAR COÏNCIDENCE (même espace de petits entiers)
		# — et cumulée avec la ligne suivante, dessinait l'anneau DEUX FOIS. Un seul signal reste :
		# le mode marche global (le panneau n'a pas de sélection plus fine pour une garnison).
		if army_selected:
			_draw_army_ring(ctr, s, zoom)
	var pt: Texture2D = HeraldryK.pion(0, c)   # phase repos, teinté au pays
	if pt != null:
		draw_texture_rect(pt, Rect2(ctr - Vector2(s * 0.5, s * 0.80), Vector2(s, s)), false, Color(1, 1, 1, 0.80))
	else:
		var col := _country_color(c)
		var sv := 4.0 / zoom
		draw_colored_polygon(PackedVector2Array([
			ctr + Vector2(0, -sv), ctr + Vector2(sv, 0), ctr + Vector2(0, sv), ctr + Vector2(-sv, 0)]), col)

func _country_color(c: int) -> Color:
	# UNE SEULE FAMILLE de couleur par entité : la teinte du pigment politique (frontière =
	# lavis = armée = nom), en version FORTE pour un acteur posé SUR la carte (le jeton doit
	# se détacher du lavis muet). Jadis une roue HSV vive INDÉPENDANTE (0.137·c, sat 0.72) :
	# l'armée d'un pays n'avait PAS la couleur de sa frontière.
	if c < 0:
		return Color(0.7, 0.7, 0.72)
	return _shade(_entity_pigment(c), 0.22)

func _phase_color(phase: int) -> Color:
	match phase:
		PHASE_SIEGE:  return Color(0.95, 0.6, 0.2)   # orange : siège
		PHASE_BATTLE: return Color(0.95, 0.25, 0.2)  # rouge : bataille
		PHASE_MARCH:  return Color(0.95, 0.95, 0.95) # blanc : marche
		_:            return Color(0.8, 0.8, 0.85)   # gris : au repos
	return Color.WHITE

func _mv_ref() -> Node2D:
	if _mv == null:
		_mv = get_parent() as Node2D
		if _mv != null and _mv.has_signal("mode_changed") and not _mv.mode_changed.is_connected(_on_mode_changed):
			_mv.mode_changed.connect(_on_mode_changed)   # mode RESSOURCES ↔ autre → redraw immédiat (même en pause)
	return _mv

func _on_mode_changed(_m: int) -> void:
	queue_redraw()

# ── projection GLOBE (segments, deux bouts visibles) ───────────────────────────
func _project_segs_iso(mv: Node2D, segs: PackedVector2Array) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(segs.size())
	for i in range(segs.size()):
		out[i] = mv.iso_pos(segs[i].x, segs[i].y)
	return out

# ════════════════════════ dispatch (parchemin, ISO unique) ════════════════════════
func _draw_move_preview(w, mv: Node2D, zoom: float) -> void:
	if move_preview.is_empty():
		return
	var legal := bool(move_preview.get("valid", false))
	var color := Color(0.92, 0.76, 0.28, 0.92) if legal else Color(0.86, 0.24, 0.20, 0.92)
	var pts := PackedVector2Array()
	for value in move_preview.get("path", []):
		var region := int(value)
		var world_pos: Vector2 = _region_seat.get(region, w.region_centroid(region))
		if world_pos.x >= 0:
			pts.append(mv.iso_pos(world_pos.x, world_pos.y))
	if pts.size() >= 2:
		draw_polyline(pts, color, 2.4 / maxf(zoom, 0.001), true)
		for point in pts:
			draw_circle(point, 2.5 / maxf(zoom, 0.001), color)
	var target := int(move_preview.get("target_region", -1))
	if target >= 0:
		var target_pos: Vector2 = _region_seat.get(target, w.region_centroid(target))
		if target_pos.x >= 0:
			var center: Vector2 = mv.iso_pos(target_pos.x, target_pos.y)
			draw_circle(center, 8.0 / maxf(zoom, 0.001), Color(color, 0.12))
			draw_arc(center, 8.0 / maxf(zoom, 0.001), 0.0, TAU, 24, color,
				1.8 / maxf(zoom, 0.001), true)

func _draw() -> void:
	var w = Sim.world
	if w == null:
		return
	var mv := _mv_ref()
	if mv == null:
		return
	_draw_iso(w, mv)
	# MODE RESSOURCES (9) : les icônes de brutes par tuile, AU-DESSUS de tout (sauf en mode NATURE).
	if not nature_mode and int(mv.get("mode")) == 9:
		_draw_resources(w, mv)

## CARTE PARCHEMIN — acteurs tracés en ENCRE vectorielle (zéro sprite) : frontières, routes,
## villes (glyphes), noms d'empire, armées, épicentre §27. La Camera2D met à l'échelle ; les
## tailles d'encre sont en px ÉCRAN (÷ zoom) → lisibles à tous les zooms.
func _draw_iso(w, mv: Node2D) -> void:
	var zoom := get_viewport_transform().get_scale().x
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var INK := Color(0.20, 0.14, 0.09, 0.95)         # encre brun-sépia (le trait de plume)
	var human_idx := int(w.player())   # BROUILLARD : les tiens (owner==human_idx) restent TOUJOURS visibles
	if _fog_dirty:
		_refresh_fog()                   # cycle indépendant : année/ère OU souveraineté

	# ── LAVIS POLITIQUE (aquarelle) : le territoire teinté SOUS tout — la carte DIT qui tient
	#    quoi d'un regard au plan large ; le lavis s'efface vers le zoom profond (le terrain parle). ──
	if not nature_mode and _pol_tex != null:
		if _borders_dirty:
			_rebuild_borders(mv)                      # le lavis se rebâtit avec les frontières
		var wash_a := lerpf(WASH_A_FAR, WASH_A_NEAR,
			clampf((zoom - WASH_FADE_LO) / (WASH_FADE_HI - WASH_FADE_LO), 0.0, 1.0))
		var p0: Vector2 = mv.iso_pos(0, 0)
		var p1: Vector2 = mv.iso_pos(w.map_w(), w.map_h())
		draw_texture_rect(_pol_tex, Rect2(p0, p1 - p0), false, Color(1, 1, 1, wash_a))

	# ── W-GUERRE UI (lot A) — HACHURES de siège/occupation : AU-DESSUS du lavis politique
	#    (sinon noyées), sous le reste (frontières/villes/armées restent lisibles par-dessus).
	#    α modéré : le lavis reste la lecture dominante, la hachure signale sans l'écraser. ──
	if not nature_mode and not _war_regions.is_empty():
		for r in _war_regions:
			_draw_war_hatch(mv, zoom, _war_regions[r])

	# ── DRESSING DE TERRAIN (lot 2) : marques de biome (relief/végétation/zones), SOUS tout le reste. ──
	if _dressing_dirty:
		_build_dressing()
		_dressing_dirty = false
	# ── LA CANOPÉE (MultiMesh, un draw call par essence) : même seuil que le décor — au
	#    plan large le peuplement dense virerait au BRUIT (35k specks sur le parchemin). ──
	if zoom >= DECOR_ZOOM_MIN:
		for b in _canopy_batches:
			draw_multimesh(b["mm"], b["tex"])
	if zoom >= DECOR_ZOOM_MIN:
		# CHEVRONS (relief) : liste courte, géométrie déjà projetée/clippée par _clip_relief/
		# _finalize_dress_fast (revue #6) — traits pré-clippés STYLÉS, intérieur transparent.
		for d in _dress_relief:
			var dip: Vector2 = d["ip"]
			var dss: Vector2 = vt * dip
			if dss.x < -90 or dss.y < -90 or dss.x > vp.x + 90 or dss.y > vp.y + 90:
				continue
			for seg in d.get("segs", []):
				var sg: Dictionary = seg
				draw_polyline(sg["pts"], sg["col"], float(sg["w"]) / zoom, true)
		# SPRITES (dressing lot 2/3/6) : tableaux PARALLÈLES TYPÉS (revue #6) — ip/texture/teinte
		# déjà résolus à _finalize_dress_fast, le draw n'indexe plus que des Packed*Array.
		for k in range(_dress_fast_ip.size()):
			var dip2: Vector2 = _dress_fast_ip[k]
			var dss2: Vector2 = vt * dip2
			if dss2.x < -90 or dss2.y < -90 or dss2.x > vp.x + 90 or dss2.y > vp.y + 90:
				continue
			var dh2 := _dress_fast_h[k] / zoom            # hauteur MONDE (taille écran constante)
			var dw2 := dh2 * 2.0 if _dress_fast_wide[k] != 0 else dh2   # serpent de mer : sprite 2:1 (large)
			draw_texture_rect(_dress_fast_tex[k], Rect2(dip2 - Vector2(dw2 * 0.5, dh2 * 0.5), Vector2(dw2, dh2)),
				false, _dress_fast_col[k])
	# LES ENSEMBLES NOMMÉS (forêts/lacs/rivières/massifs) se dessinent PLUS BAS (juste avant
	# les noms d'empire) — au-dessus du lavis/routes, sous le brouillard ; mais leur rendu
	# vit AUSSI en mode NATURE : on le fait ici si nature (le return saute la suite).
	if nature_mode:
		_draw_geonames(w, mv, vt, vp, zoom)
		return

	# ── FRONTIÈRES à l'ENCRE (calligraphie) : TRAME FINE 1px (toutes provinces+régions) + BLOCS
	#    d'empire 3px, COULEUR PAR ENTITÉ, en 2 passes (bave d'encre douce + plume nette, jittées). ──
	if _borders_dirty:
		_rebuild_borders(mv)
	# la TRAME FINE fond en survol (sinon mosaïque illisible) et se révèle au plan rapproché — toutes
	# les provinces RESTENT tracées (1px), juste graduées au zoom (LOD ; les blocs d'empire, eux, toujours).
	if _borders.has(0):
		# PROVINCE = administrative, un CHUCHOTEMENT : déjà restreinte à la terre ADMINISTRÉE
		# (rebuild), elle n'émerge qu'au plan rapproché (zoom 2.2+) et plafonne bas (0.24) —
		# le lavis + la frontière d'empire portent la lecture, la trame ne fait que détailler.
		var fine_a := clampf((zoom - 2.2) / 2.6, 0.0, 1.0) * 0.24
		# revue #5 : _fine_proj déjà projeté par _rebuild_borders — plus de _project_segs_iso ici.
		if fine_a > 0.02 and _fine_proj.size() >= 2:
			draw_multiline(_fine_proj, Color(PROV_INK.r, PROV_INK.g, PROV_INK.b, fine_a * 0.45), _w(zoom, 0.6, 0.9, 1.5), true)
			draw_multiline(_fine_proj, Color(PROV_INK.r, PROV_INK.g, PROV_INK.b, fine_a), _w(zoom, 0.34, 0.6, 0.9), true)
	# PAYS : trait GRAVÉ en double passe (halo brun sombre LARGE + pigment politique FIN), pour bien
	# SÉPARER l'administratif (province, cheveu brun) du politique (pays, trait coloré net). Puis le
	# LISERÉ POURPRE FIN de chaque capitale, AU-DESSUS.
	for entity in _b_segs:
		_draw_band(_b_proj.get(entity, PackedVector2Array()), _b_wash.get(entity, []), int(entity), zoom)
	for cc in _cap_segs:
		_draw_cap_lisere(mv, _cap_segs[cc], _cap_norm[cc], zoom)

	# ── SÉLECTION : contour DORÉ de la province choisie (creux d'encre + or net), AU-DESSUS
	#    des frontières — le retour visuel du clic (le panneau dit QUOI, le contour dit OÙ). ──
	var selp := int(mv.get("_selected_prov"))
	if selp >= 0:
		if selp != _sel_prov_cache and w.has_method("province_border_segments"):
			_sel_prov_cache = selp
			_sel_segs = PackedVector2Array()
			var sd: Dictionary = w.province_border_segments(selp)
			var sp: PackedVector2Array = sd.get("pts", PackedVector2Array())
			for ch in _chain_segments(sp):
				var poly: PackedVector2Array = _smooth_poly(ch)
				for i in range(poly.size() - 1):
					_sel_segs.push_back(poly[i]); _sel_segs.push_back(poly[i + 1])
			_sel_proj = _project_segs_iso(mv, _sel_segs)   # revue #5 : projeté UNE fois, au changement de sélection
		if _sel_segs.size() >= 2:
			draw_multiline(_sel_proj, Color(0.12, 0.08, 0.04, 0.80), _w(zoom, 1.3, 2.6, 4.4), true)
			draw_multiline(_sel_proj, Color(SEL_GOLD.r, SEL_GOLD.g, SEL_GOLD.b, 0.95), _w(zoom, 0.7, 1.5, 2.6), true)
	elif _sel_prov_cache != -2:
		_sel_prov_cache = -2
		_sel_segs = PackedVector2Array()
		_sel_proj = PackedVector2Array()

	# ── ROUTES : CHEMIN DE TERRE À 3 TRAITS (sous-trait sépia + corps crème + filet clair) —
	#    le motif cartographique classique, sur les polylignes DÉJÀ lissées (_augment_roads).
	#    Croissance organique (1 an/province) ; segments cumulés → batchs par hiérarchie. ──
	if zoom >= ROAD_ZOOM_MIN:
		_ensure_roads()
		if not _roads.is_empty():
			# ANTISPAG A2 : polylignes déjà DÉDUPLIQUÉES + réparties par TIER de multiplicité
			# (cache — cf. `_ensure_road_network`, recalculé seulement si le réseau bouge ou
			# qu'un chantier grandit encore).
			_ensure_road_network()
			# REFONTE v2 « SOL USÉ » (2026-07-31) : bandes claires d'abord (le sol), pads
			# de porte, puis ornières/traces par-dessus. L'usure du tier = ALPHA (+WEAR_A
			# par palier), jamais l'épaisseur. Gates par zoom : lointain = grandes bandes
			# seules · moyen = + régionales + ornières · proche = + sentiers.
			for t in range(1, ROAD_MULT_TIERS + 1):
				var wear := ROAD_WEAR_A * float(t - 1)
				if zoom >= ROAD_Z_BAND_MINOR:
					var bmf: PackedVector2Array = _road_net.get("b_minor%df" % t, PackedVector2Array())
					if bmf.size() >= 2:
						draw_multiline(bmf, Color(ROAD_BAND.r, ROAD_BAND.g, ROAD_BAND.b,
							ROAD_BAND_A_F + wear), _w(zoom, 0.42, 1.0, 2.2), true)
					var bm: PackedVector2Array = _road_net.get("b_minor%d" % t, PackedVector2Array())
					if bm.size() >= 2:
						draw_multiline(bm, Color(ROAD_BAND.r, ROAD_BAND.g, ROAD_BAND.b,
							ROAD_BAND_A + wear), _w(zoom, 0.42, 1.0, 2.2), true)
				var baf: PackedVector2Array = _road_net.get("b_main%df" % t, PackedVector2Array())
				if baf.size() >= 2:
					draw_multiline(baf, Color(ROAD_BAND.r, ROAD_BAND.g, ROAD_BAND.b,
						ROAD_BAND_A_F + wear), _w(zoom, 0.58, 1.3, 3.0), true)
				var ba: PackedVector2Array = _road_net.get("b_main%d" % t, PackedVector2Array())
				if ba.size() >= 2:
					draw_multiline(ba, Color(ROAD_BAND.r, ROAD_BAND.g, ROAD_BAND.b,
						ROAD_BAND_A + wear), _w(zoom, 0.58, 1.3, 3.0), true)
			if zoom >= ROAD_Z_RUT:
				for pad in _road_net.get("pads", []):
					var pc: Vector2 = pad[0]
					var pr: float = pad[1]
					var pseed := float(pad[2])
					var ppoly := PackedVector2Array()
					for v in range(7):             # aire IRRÉGULIÈRE (7 sommets, rayon hashé)
						var ang := TAU * float(v) / 7.0
						var rr := pr * (0.72 + 0.55 * _h1(pseed * 13.7 + float(v)))
						ppoly.append(pc + Vector2(cos(ang), sin(ang) * 0.5) * rr)   # aplati iso
					draw_colored_polygon(ppoly, Color(ROAD_BAND.r, ROAD_BAND.g, ROAD_BAND.b, ROAD_BAND_A + 0.10))
				for jc in _road_net.get("junc", []):
					var jp2: Vector2 = jc[0]
					var jr: float = jc[1]
					var jseed := float(int(jc[2]))
					var jpoly := PackedVector2Array()
					for v in range(6):             # aire de CARREFOUR : terre usée, sans symbole
						var ang2 := TAU * float(v) / 6.0
						var rr2 := jr * (0.70 + 0.5 * _h1(jseed * 7.9 + float(v)))
						jpoly.append(jp2 + Vector2(cos(ang2), sin(ang2) * 0.5) * rr2)
					draw_colored_polygon(jpoly, Color(ROAD_BAND.r * 0.90, ROAD_BAND.g * 0.87, ROAD_BAND.b * 0.82, ROAD_BAND_A + 0.14))
			if zoom >= ROAD_Z_RUT:
				for t in range(1, ROAD_MULT_TIERS + 1):
					var rw := _w(zoom, 0.16, 0.6, 1.0)
					var rmf: PackedVector2Array = _road_net.get("r_minor%df" % t, PackedVector2Array())
					if rmf.size() >= 2:
						draw_multiline(rmf, Color(ROAD_RUT.r, ROAD_RUT.g, ROAD_RUT.b,
							ROAD_RUT.a * ROAD_FOREST_A), rw, true)
					var rm: PackedVector2Array = _road_net.get("r_minor%d" % t, PackedVector2Array())
					if rm.size() >= 2:
						draw_multiline(rm, ROAD_RUT, rw, true)
					var raf: PackedVector2Array = _road_net.get("r_main%df" % t, PackedVector2Array())
					if raf.size() >= 2:
						draw_multiline(raf, Color(ROAD_RUT.r, ROAD_RUT.g, ROAD_RUT.b,
							ROAD_RUT.a * ROAD_FOREST_A), rw, true)
					var ra: PackedVector2Array = _road_net.get("r_main%d" % t, PackedVector2Array())
					if ra.size() >= 2:
						draw_multiline(ra, ROAD_RUT, rw, true)
					if zoom >= ROAD_Z_TRAIL:     # SENTIERS (level moteur 2) : trace seule, zoom proche
						var rtf: PackedVector2Array = _road_net.get("r_trail%df" % t, PackedVector2Array())
						if rtf.size() >= 2:
							draw_multiline(rtf, Color(ROAD_TRAIL.r, ROAD_TRAIL.g, ROAD_TRAIL.b,
								ROAD_TRAIL.a * ROAD_FOREST_A), rw, true)
						var rt: PackedVector2Array = _road_net.get("r_trail%d" % t, PackedVector2Array())
						if rt.size() >= 2:
							draw_multiline(rt, ROAD_TRAIL, rw, true)
			# les PONTS (refonte v2) : TABLIER clair dans la continuité de la route — il
			# INTERROMPT visuellement l'eau — encadré de deux CULÉES sombres ; les
			# garde-corps bombés (l'ancien pont d'encre) ne sortent qu'au zoom profond.
			if zoom >= ROAD_Z_BRIDGE:
				for br in _ink_bridges:
					var bp: Vector2 = mv.iso_pos((br["w"] as Vector2).x, (br["w"] as Vector2).y)
					var bt: Vector2 = (mv.iso_pos((br["w"] as Vector2).x + (br["t"] as Vector2).x,
						(br["w"] as Vector2).y + (br["t"] as Vector2).y) - bp).normalized()
					var bperp := Vector2(-bt.y, bt.x)
					var bl := _w(zoom, 0.62, 2.4, 5.2)
					var bw := _w(zoom, 0.26, 1.0, 2.2)
					var biw := _w(zoom, 0.07, 0.5, 1.0)
					draw_colored_polygon(PackedVector2Array([                       # tablier
						bp - bt * bl - bperp * bw, bp + bt * bl - bperp * bw,
						bp + bt * bl + bperp * bw, bp - bt * bl + bperp * bw]),
						Color(ROAD_BAND.r, ROAD_BAND.g, ROAD_BAND.b, 0.85))
					for sgn2 in [-1.0, 1.0]:                                        # culées
						var ce := bp + bt * bl * float(sgn2)
						draw_line(ce - bperp * bw * 1.45, ce + bperp * bw * 1.45,
							Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.85), biw * 1.5, true)
					if zoom >= ROAD_Z_RAIL:
						for sgn in [-1.0, 1.0]:                                     # garde-corps
							var o3 := bperp * bw * float(sgn)
							draw_polyline(PackedVector2Array([bp - bt * bl + o3,
								bp + o3 + bperp * (bw * 0.5 * float(sgn)),   # le bombé du tablier
								bp + bt * bl + o3]), TOWN_INK, biw, true)

		# ── PORTULAN (MARITIME N4) : les LANES maritimes en POINTILLÉS d'encre — plus
		#    fines que les routes de terre, ancrées aux ports. Le moteur (sea_paths)
		#    garantit des cellules de MER seulement — jamais un lac, jamais la terre
		#    (hors l'ancrage au quai). Une route encore en FORMATION n'a pas d'encre.
		#    SOUS LE FOG RIEN : le VOILE d'encre (_fog_tex, dessiné APRÈS, en fin de
		#    _draw_iso) couvre la mer inconnue AU PIXEL PRÈS — même mécanisme que les
		#    routes terrestres (non gatées elles non plus). Un gate par-lane « les deux
		#    bouts connus » a été ESSAYÉ puis retiré : il double-cachait ce que le voile
		#    couvre déjà ET aurait caché, depuis TON port, la lane qui part vers
		#    l'horizon inconnu (le geste portulan même). ──
		_ensure_lanes()
		if not _lanes.is_empty():
			var lane_w := _w(zoom, 0.28, 0.8, 1.5)   # un SEUL trait fin (la terre en superpose 3 :
			                                         # le composite terrestre reste plus épais)
			for li in range(_lanes.size()):
				var ln: Dictionary = _lanes[li]
				if int(ln.get("open", 0)) == 0:
					continue
				if li < _lane_dashes.size() and (_lane_dashes[li] as PackedVector2Array).size() >= 2:
					draw_multiline(_lane_dashes[li], SEA_LANE_INK, lane_w, true)

	# ── VILLES : VIGNETTES gravées (pack bourgs/, lot U) — cité t1-t7, cité-état & hameau libre
	# (familles DÉDIÉES). CENTRÉES sur le SIÈGE intérieur de province (≠ jonction ; le centroïde
	# brut tombe pile à l'intersection des provinces). Cité-état (rôle 2) & hameau libre (rôle 4)
	# toujours tracés même sans tier de ville. Les vignettes sont GRANDES → tri fond→avant
	# (peintre, y écran) puis les BANNIÈRES par-dessus tout (jamais sous la vignette voisine).
	if zoom >= CITY_ZOOM_MIN:
		# revue #4 : la LISTE (tier/owner/rôle/pop/fog/siège + tri) est en cache, cf. _refresh_setts
		# — seul le test de visibilité ÉCRAN (dépend du zoom/pan COURANTS) reste par-frame ici.
		if _setts_dirty:
			_refresh_setts()
		var setts := []
		for s in _setts:
			var ss: Vector2 = vt * (s["ip"] as Vector2)
			if ss.x < -160 or ss.y < -160 or ss.x > vp.x + 160 or ss.y > vp.y + 160:
				continue
			setts.append(s)
		for s in setts:
			_draw_settlement(w, int(s["r"]), int(s["role"]), s["ctr"], s["ip"], zoom, mv)
		# RÉGIME KCD : la BANNIÈRE de lieu éclot au plan rapproché — le relais des
		# noms de pays (régime EU4) qui se sont effacés au même seuil de zoom.
		if zoom >= 4.0:
			for s in setts:
				_draw_banner(w, int(s["r"]), s["ip"], zoom, clampf((zoom - 4.0) / 1.2, 0.0, 1.0))

	_draw_move_preview(w, mv, zoom)

	# ── ARMÉES : PION DE PLATEAU (planche 32 — la figurine d'étain posée SUR la
	#    table, drapeau teinté au pays, la POSE dit la phase) + ligne de marche.
	#    Ombre de contact = la même pièce en silhouette, décalée SE (front32). ──
	_pa_positions.clear()
	var actors: Array[Dictionary] = []
	for c in range(w.country_count()):
		var ids: Array = w.corps_ids(c) if w.has_method("corps_ids") else []
		if ids.is_empty():
			# RÉSERVE MOBILISÉE (régiments recrutés mais pas en campagne) : une garnison à la
			# CAPITALE — sinon une armée levée n'apparaît NULLE PART (retour joueur « les
			# armées mobilisées n'apparaissent pas sur la carte »). Pion plus discret que l'ost.
			_draw_garrison(w, mv, c, zoom, human_idx)
			continue
		for n in range(ids.size()):
			var info: Dictionary = w.corps_info(int(ids[n]))
			if bool(info.get("active",false)): actors.append({"country":c,"army":info,"stack":n})
	for actor in actors:
		var c: int = actor["country"]
		var a: Dictionary = actor["army"]
		var corps_id: int = int(a.get("id",-1))
		var reg: int = a.get("region", -1)
		if reg < 0:
			continue
		# BROUILLARD DE GUERRE (étape 1/2) : une armée ENNEMIE tombant dans le voile ne se
		# dessine pas — les tiennes (c==human_idx) restent TOUJOURS visibles.
		if c != human_idx and not _fog_visible_region(reg):
			continue
		# POSE sur le SIÈGE (la ville), pas le centroïde — l'armée reste SUR la province.
		var rctr: Vector2 = _region_seat.get(reg, w.region_centroid(reg))
		if rctr.x < 0:
			continue
		var ctr: Vector2 = mv.iso_pos(rctr.x, rctr.y)
		var stack_i: int = int(actor["stack"])
		ctr += Vector2((stack_i % 4) * 5.0, -float(stack_i / 4) * 4.0) / maxf(zoom,0.0001)
		if c == human_idx:
			_pa_positions[corps_id] = {"pos":ctr,"radius":_w(zoom,6.0,30.0,64.0)*0.7}
			if corps_id in selected_corps:
				_draw_army_ring(ctr, _w(zoom, 6.0, 30.0, 64.0), zoom)
		var phase: int = a.get("phase_id", 0)
		var dest: int = a.get("dest", -1)
		if dest >= 0 and dest != reg:
			var dw: Vector2 = w.region_centroid(dest)
			if dw.x >= 0:
				draw_line(ctr, mv.iso_pos(dw.x, dw.y), Color(_phase_color(phase), 0.7), 1.4 / zoom)
		var pt: Texture2D = HeraldryK.pion(phase, c)
		if pt != null:
			var s := _w(zoom, 7.0, 34.0, 74.0)
			var r := Rect2(ctr - Vector2(s * 0.5, s * 0.80), Vector2(s, s))
			draw_texture_rect(pt, Rect2(r.position + Vector2(s * 0.05, s * 0.04), r.size),
				false, Color(0.05, 0.03, 0.02, 0.32))       # ombre de contact SE
			draw_texture_rect(pt, r, false)
			if phase == PHASE_BATTLE:
				var mk: Texture2D = HeraldryK.marker("battle")
				if mk != null:
					var ms := s * 0.42
					draw_texture_rect(mk, Rect2(ctr + Vector2(s * 0.30, -s * 0.30), Vector2(ms, ms)), false)
		else:
			# repli vectoriel (pièce absente) : l'ancien losange teinté
			var col := _country_color(c)
			var sv := 5.0 / zoom
			draw_circle(ctr, sv + _w(zoom, 0.45, 1.4, 2.6), Color(_phase_color(phase), 0.9))
			var diamond := PackedVector2Array([
				ctr + Vector2(0, -sv), ctr + Vector2(sv, 0), ctr + Vector2(0, sv), ctr + Vector2(-sv, 0)])
			draw_colored_polygon(diamond, col)
		# COMPTEUR D'EFFECTIF (rendu attendu EU4) : strip à la couleur du pays + « N k »,
		# taille ÉCRAN constante, posé SOUS le pion — la force se lit sans cliquer.
		var un := int(a.get("units", 0))
		if not bool(a.get("units_are_humans", false)): un *= 100
		if un > 0:
			var scc := 1.0 / maxf(zoom, 0.0001)
			var ut := str(un)
			if un >= 1000:
				ut = "%.1fk" % (un / 1000.0)
			if un >= 9500:
				ut = "%dk" % int(round(un / 1000.0))
			var utw := VKit.text_map_w(ut, VKit.FS_SMALL) * scc
			var ubh := 13.0 * scc
			var ubw := utw + 10.0 * scc
			var urc := Rect2(Vector2(ctr.x - ubw * 0.5, ctr.y + 6.0 * scc), Vector2(ubw, ubh))
			draw_rect(Rect2(urc.position + Vector2(1.0, 1.2) * scc, urc.size), Color(0.05, 0.03, 0.02, 0.40))
			draw_rect(urc, Color(_country_color(c), 0.92))
			draw_rect(urc, Color(0.10, 0.07, 0.04, 0.90), false, 1.0 * scc)
			draw_set_transform(Vector2(urc.position.x + 5.0 * scc, urc.position.y + 0.5 * scc), 0.0, Vector2(scc, scc))
			VKit.text_map(self, Vector2.ZERO, ut, VKit.FS_SMALL,
				Color(0.97, 0.94, 0.85, 0.98), 2, Color(0.08, 0.05, 0.03, 0.75))
			draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

	# ── LES ENSEMBLES NOMMÉS (forêts/lacs/rivières/massifs) : calligraphie fadée dans le
	#    terrain, au-dessus du lavis/routes, SOUS le brouillard et les noms d'empire. ──
	_draw_geonames(w, mv, vt, vp, zoom)

	# ── NOMS D'EMPIRE : SUIVENT LA FORME du pays (axe principal par ACP des centroïdes projetés →
	#    Chili vertical, Russie en travers de la Sibérie), à l'encre, taille ÉCRAN constante.
	#    POLISH #5 : l'ACP est CACHÉE (ancre+angle stables tant que souveraineté/fog ne
	#    bougent pas — _names_dirty suit les mêmes signaux que frontières/brouillard). ──
	if _names_dirty:
		_name_anchor.clear()
		_names_dirty = false
	for c in range(w.country_count()):
		if c >= _country_names.size():
			break
		var nm: String = _country_names[c]
		if nm == "":
			continue
		if not _name_anchor.has(c):
			# centroïdes PROJETÉS (espace écran : iso_pos comprime Y → angle visuel correct)
			var ps := PackedVector2Array()
			for r in range(w.region_count()):
				if w.region_owner(r) == c:
					# BROUILLARD DE GUERRE (étape 1/2) : une région ENNEMIE voilée n'entre pas dans
					# l'ancre — un pays SANS aucune région visible ne pose plus aucun nom du tout.
					if c != human_idx and not _fog_visible_region(r):
						continue
					var rc: Vector2 = w.region_centroid(r)
					if rc.x >= 0:
						ps.push_back(mv.iso_pos(rc.x, rc.y))
			if ps.is_empty():
				_name_anchor[c] = {"valid": false}   # (1 centroïde = valide : ancre au point —
				                                     #  l'empire mono-région garde son nom)
			else:
				# moyenne + matrice de covariance → axe principal (ACP 2D)
				var mx := 0.0; var my := 0.0
				for p in ps:
					mx += p.x; my += p.y
				mx /= ps.size(); my /= ps.size()
				var sxx := 0.0; var syy := 0.0; var sxy := 0.0
				for p in ps:
					var dx := p.x - mx; var dy := p.y - my
					sxx += dx * dx; syy += dy * dy; sxy += dx * dy
				var cang := 0.0
				# élongation (rapport des valeurs propres) : on n'oriente QUE les pays nettement allongés
				var ctr_ := sxx + syy
				var det := sxx * syy - sxy * sxy
				var disc := sqrt(maxf(ctr_ * ctr_ * 0.25 - det, 0.0))
				var l1 := ctr_ * 0.5 + disc
				var l2 := ctr_ * 0.5 - disc
				if l2 > 0.001 and l1 / l2 > 1.8:
					cang = 0.5 * atan2(2.0 * sxy, sxx - syy)   # ∈ [-π/2, π/2] : jamais à l'envers
				_name_anchor[c] = {"valid": true, "ip": Vector2(mx, my), "ang": cang, "ext": l1}
		var anc: Dictionary = _name_anchor[c]
		if not bool(anc.get("valid", false)):
			continue
		var ang := float(anc.get("ang", 0.0))
		# ancre = le BARYCENTRE des centroïdes (espace OUVERT, hors des hubs routiers) → lisible.
		var ip: Vector2 = anc.get("ip", Vector2.ZERO)
		var lw := VKit.text_w(nm, VKit.FS_SMALL)
		# CALLIGRAPHIE : AUCUNE boîte (fond transparent) — encre directe + halo papier, le nom écrit à
		# la plume LE LONG du pays. AGRANDI (1.35→1.9 : lisible au fit, là où la carte se joue) et
		# TEINTÉ au pigment de l'entité assombri (même famille que frontière/lavis — cohérence).
		# RÉGIME EU4 : les noms de PAYS vivent au plan LARGE — grands, en capitales ESPACÉES
		# le long de l'axe du pays, à l'échelle de sa TAILLE — et s'EFFACENT au zoom (le plan
		# rapproché appartient aux bannières de lieux, régime KCD).
		var name_fade := 1.0 - clampf((zoom - 3.2) / 1.6, 0.0, 1.0)
		if name_fade <= 0.02:
			continue
		var pig := _entity_pigment(c)
		var rl := int(w.country_role(c))
		var is_emp := (rl == 0 or rl == 1)
		var track := 0.45 if is_emp else 0.0            # espacement de capitales (E U 4)
		var name_ink := Color(pig.r * 0.40, pig.g * 0.40, pig.b * 0.40, (0.95 if is_emp else 0.70) * name_fade)
		var halo := Color(0.97, 0.91, 0.74, (0.75 if is_emp else 0.5) * name_fade)
		var disp := nm.to_upper() if is_emp else nm
		# largeur TRACKÉE (par caractère) pour centrer — police de CARTE (IM Fell)
		var tw := 0.0
		for k in range(disp.length()):
			tw += VKit.text_map_w(disp[k], VKit.FS_SMALL) + (track * 6.0 if k < disp.length() - 1 else 0.0)
		# LA RÈGLE EU4 : le nom est ANCRÉ MONDE et DIMENSIONNÉ À SON TERRITOIRE — il s'étire
		# sur l'étendue du pays (≈3σ de l'axe ACP majeur), jamais sur la mer d'à côté. Il
		# grossit donc à l'écran en zoomant, jusqu'au fondu (le relais KCD des bannières).
		var nsc: float
		if is_emp:
			var span := clampf(2.8 * sqrt(maxf(float(anc.get("ext", 0.0)), 0.0)) + 16.0, 22.0, 220.0)   # étendue-monde du nom
			nsc = span / maxf(tw, 1.0)
		else:
			nsc = 1.1 / zoom                              # petites entités : chip écran-constant
		draw_set_transform(ip, ang, Vector2(nsc, nsc))
		var cx0 := -tw * 0.5
		for k in range(disp.length()):
			var ch := disp[k]
			# IM FELL (police de carte) : encre entité (jamais noir pur) sur HALO papier doux
			VKit.text_map(self, Vector2(cx0, -7.0), ch, VKit.FS_SMALL, name_ink, 2, halo)
			cx0 += VKit.text_map_w(ch, VKit.FS_SMALL) + track * 6.0
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

	# ── ÉPICENTRE du cataclysme §27 : anneaux pulsants à l'encre de la fin. ──
	var eg: Dictionary = w.endgame_info()
	var epi: int = eg.get("epicenter_reg", -1)
	var fin: int = eg.get("fin", 0)
	_cataclysm = (fin > 0 and epi >= 0)
	if epi >= 0:
		var ew: Vector2 = w.region_centroid(epi)
		if ew.x >= 0:
			var ec: Vector2 = mv.iso_pos(ew.x, ew.y)
			var col := _fin_color(fin)
			var t := Time.get_ticks_msec() / 1000.0
			# MERVEILLE (fin==4) : le chantier qui GRANDIT — le rayon de base croît avec merv_pct
			# (0..100) au lieu du pulse fixe des apocalypses (le joueur voit l'ascension AVANCER).
			var base_rad := 7.0
			if fin == 4:
				var mp := float(eg.get("merv_pct", 0))
				base_rad = lerpf(7.0, 34.0, clampf(mp / 100.0, 0.0, 1.0))
			for k in range(3):
				var rad := (base_rad + k * 6.0 + fmod(t * 5.0, 6.0)) / zoom
				# §27 : l'anneau d'épicentre DOIT se lire au plan large (drame global) ; trait borné, le rayon de pulse reste /zoom.
				draw_arc(ec, rad, 0.0, TAU, 40, Color(col, 0.7 - k * 0.18), _w(zoom, 0.35, 1.2, 2.4), true)

	# ── BROUILLARD DE GUERRE (étape 1/2) : le VOILE — AU-DESSUS de tout (terrain, routes,
	#    villes, armées, noms, épicentre) pour qu'il obscurcisse vraiment ce qu'il couvre.
	#    Encre estompée (esprit parchemin, jamais noir pur) — cf. fog_image() (scps_sim_node).
	#    fog_off = PROBE seulement (shot_parch fog=0) : photographier sous le voile. ──
	if not nature_mode and not fog_off and _fog_tex != null:
		var fp0: Vector2 = mv.iso_pos(0, 0)
		var fp1: Vector2 = mv.iso_pos(w.map_w(), w.map_h())
		# LA PAGE HORS-MONDE voilée AUSSI : le voile ne couvrait que le rect carte — en zoom
		# serré, la marge de parchemin (au-delà des bords du monde) restait NUE : la « bande
		# beige » pleine largeur en bas d'écran (capture 2026-07-09). Quatre bandes sépia
		# bouchent le tour du monde. Gaté game_on : la vitrine du menu reste nue.
		if Sim.game_on:
			var inv := vt.affine_inverse()
			var s0: Vector2 = inv * Vector2.ZERO
			var s1: Vector2 = inv * vp
			var page := Color(24.0 / 255.0, 19.0 / 255.0, 14.0 / 255.0, 1.0)   # marge hors-monde PLEINEMENT opaque (2026-07-11)
			if fp0.y > s0.y:
				draw_rect(Rect2(s0, Vector2(s1.x - s0.x, fp0.y - s0.y)), page, true)
			if fp1.y < s1.y:
				draw_rect(Rect2(Vector2(s0.x, fp1.y), Vector2(s1.x - s0.x, s1.y - fp1.y)), page, true)
			if fp0.x > s0.x:
				draw_rect(Rect2(Vector2(s0.x, fp0.y), Vector2(fp0.x - s0.x, fp1.y - fp0.y)), page, true)
			if fp1.x < s1.x:
				draw_rect(Rect2(Vector2(fp1.x, fp0.y), Vector2(s1.x - fp1.x, fp1.y - fp0.y)), page, true)
		draw_texture_rect(_fog_tex, Rect2(fp0, fp1 - fp0), false)

## ── LE BOURG (lot U) : la ville est une VIGNETTE (pack bourgs/) — voir le bloc BOURG_* en
## tête de fichier. Il ne reste ici que l'ENCRE partagée (ponts/quais/barque, les seuls
## éléments encore composés au monde : ils dépendent du rivage) et le cache de plan.
const TOWN_INK    := Color(0.23, 0.17, 0.11, 0.60)   ## cerne d'encre (éclairci, jamais noir)
const TOWN_SHADOW := Color(0.20, 0.15, 0.10, 0.16)   ## ombre portée douce, projetée au sol
const TOWN_SHADOW_WILD_MUL := 0.60                    ## hameau libre : empreinte plus discrète
const BOURG_ALPHA_WILD := 0.70                        ## moins présent qu'une ville administrée
const QUAY_WOOD   := Color(0.44, 0.32, 0.22, 0.70)   ## planches de quai (bois patiné, glacis)
const BOAT_WOOD   := Color(0.37, 0.27, 0.19, 0.74)   ## coque (bois sombre, glacis)
var _town_cache := {}       ## region → {sid, quays, boat} (voir _build_quays / _draw_settlement)
var _ink_bridges := []      ## [{w:Vector2, t:Vector2}] — ponts aux franchissements route×rivière
var _sea_img: Image = null  ## couche EAU (cache par monde — quais)
var _rf_img: Image = null   ## champ rivière carvé (cache par monde — quais fluviaux)
## VRAI si la cellule est un biome de FORÊT (12-14) — la route y passe SOUS la canopée.
## (réutilise le cache _bio_img déclaré en tête de fichier.)
func _forest_at(x: int, y: int) -> bool:
	if _bio_img == null and Sim.world != null:
		_bio_img = Sim.world.layer_image(LAYER_BIOME)
	if _bio_img == null or x < 0 or y < 0 or x >= _bio_img.get_width() or y >= _bio_img.get_height():
		return false
	var b := int(_bio_img.get_pixel(x, y).r * 255.0 + 0.5)
	return b >= 12 and b <= 14

## VRAI si la cellule (x,y) est de l'EAU (mer OU rivière carvée) — pour poser les quais.
func _water_at(x: int, y: int) -> bool:
	if _sea_img == null and Sim.world != null:
		_sea_img = Sim.world.layer_image(LAYER_WATER)
	if _rf_img == null:
		_rf_img = _carved_river_field()
	if _sea_img != null and x >= 0 and y >= 0 and x < _sea_img.get_width() and y < _sea_img.get_height():
		if _sea_img.get_pixel(x, y).r > 0.5:
			return true
	return _in_river_water(_rf_img, x, y)

## les QUAIS : si le bourg touche l'EAU (mer ou rivière carvée) à ≤ 3 cellules — une ou deux
## jetées de bois perpendiculaires au rivage + une barque amarrée (t3+/cité-état). Le SEUL
## héritage composé de l'urbaniste (la vignette porte ses murs ; le rivage, lui, dépend du
## monde). Le hameau sauvage n'a pas de quai (une tour de guet, pas un port). L'ancre de
## jetée (`wpt`) est la DERNIÈRE terre avant l'eau — sèche par construction (lot V tenu).
func _build_quays(r: int, ctr: Vector2, tier: int, is_cs: bool, is_wild: bool) -> Dictionary:
	var quays := []
	var boat := {}
	if is_wild or (tier < 2 and not is_cs):
		return {"quays": quays, "boat": boat}
	var bestw := 1e30
	var wdir := Vector2.RIGHT
	var wpt := Vector2.ZERO
	for k in range(16):
		var dirv := Vector2.from_angle(TAU * (float(k) + 0.5 * _h1(float(r) * 6.1)) / 16.0)
		var lastland := ctr
		var t3 := 0.5
		while t3 <= 3.0:
			var pp3: Vector2 = ctr + dirv * t3
			if _water_at(int(pp3.x), int(pp3.y)):
				if t3 < bestw:
					bestw = t3
					wdir = dirv
					wpt = lastland
				break
			lastland = pp3
			t3 += 0.5
	if bestw < 3.0:
		var wside := Vector2(-wdir.y, wdir.x)
		quays.append({"a": wpt, "d": wdir})
		if tier >= 3 or is_cs:
			quays.append({"a": wpt + wside * 0.65, "d": wdir})
			boat = {"c": wpt + wdir * 1.55 + wside * -0.55, "ax": wside}
	return {"quays": quays, "boat": boat}

## charge (paresseux, cache) une VIGNETTE de bourg : texture + ANCRAGE mesurés UNE fois sur
## l'image — `foot` = bas du CONTENU opaque (fraction de hauteur : les pièces sont recentrées
## sur 256², le socle vit au bas du bbox, PAS au bord du cadre) · `cw` = largeur du contenu
## (fraction) → l'échelle vise le CONTENU, pas le cadre (T1 clairsemé ≠ T7 plein). Renvoie {}
## si l'asset manque (le dessin replie sur le glyphe d'encre, jamais un trou).
func _bourg_get(id: String) -> Dictionary:
	if _bourg_tex.has(id):
		return _bourg_tex[id]
	var entry := {}
	var tex := _dress_load("%s/%s.png" % [BOURG_DIR, id])
	if tex != null:
		var foot := 0.84
		var cwf := 0.80
		var img := tex.get_image()
		if img != null:
			var used := img.get_used_rect()
			if used.size.x > 0:
				foot = float(used.position.y + used.size.y) / float(img.get_height())
				cwf = float(used.size.x) / float(img.get_width())
		entry = {"tex": tex, "shadow": _bourg_shadow(img), "foot": foot, "cw": cwf}
	_bourg_tex[id] = entry
	return entry

## Silhouette d'ombre construite UNE fois par vignette depuis son alpha. On travaille
## sur une image 4× plus petite puis on la remonte en bilinéaire : contour doux sans
## convolution coûteuse ni boucle 256² au premier affichage de chaque variante.
func _bourg_shadow(src: Image) -> ImageTexture:
	if src == null or src.is_empty():
		return null
	var full := src.get_size()
	var sw := maxi(8, full.x >> 2)
	var sh := maxi(8, full.y >> 2)
	var small := src.duplicate()
	small.resize(sw, sh, Image.INTERPOLATE_LANCZOS)
	for y in range(sh):
		for x in range(sw):
			var alpha: float = small.get_pixel(x, y).a
			small.set_pixel(x, y, Color(1.0, 1.0, 1.0, alpha))
	small.resize(full.x, full.y, Image.INTERPOLATE_BILINEAR)
	return ImageTexture.create_from_image(small)

## charge (paresseux) une MARQUE DE TERRAIN par id → Texture2D (cache). Cherche dans lot 3 (biomes plats/
## eau) PUIS lot 2 (relief/forêt/désert) — les ids sont uniques entre lots. Fallback Image.load (PNG brut).
func _dress_get(id: String) -> Texture2D:
	if _dress_tex.has(id):
		return _dress_tex[id]
	var tex := _dress_load("res://art/map_stamps/lot3_biomes/assets_alpha/%s.png" % id)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot2_painted/assets_alpha/%s.png" % id)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot4_easter_eggs/assets_alpha/%s.png" % id)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot6_da/%s.png" % id)   # lot 6 : singles individuels (arbres/sol)
	if tex == null:
		tex = _dress_load("res://art/map_stamps/lot6_front32/%s.png" % id)   # front32 : bâtiments (élévations)
	_dress_tex[id] = tex
	return tex

## teinte de DESSIN par famille lot 6 (modulate) : les feuillus livrés KAKI sont ramenés vers
## l'olive de la carte, les conifères vers le sapin — sans retoucher un fichier. Les autres
## familles gardent le blanc-glacis standard (null → dress_col).
func _dress_tint(id: String) -> Variant:
	if id.begins_with("lot6_broadleaf"):
		return Color(0.70, 0.84, 0.51, 0.60)   # un cran plus VERT : la canopée ×10 sature vers sa teinte
	if id.begins_with("lot6_conifer"):
		return Color(0.56, 0.74, 0.52, 0.58)
	if id.begins_with("lot6_ground"):
		return Color(0.95, 0.93, 0.86, 0.52)
	return null

func _dress_load(path: String) -> Texture2D:
	if ResourceLoader.exists(path):
		return load(path)
	if FileAccess.file_exists(path):              # garde : pas d'Image.load sur un fichier absent (≠ spam d'erreurs)
		var img := Image.new()
		if img.load(path) == OK:
			return ImageTexture.create_from_image(img)
	return null

## LES ENSEMBLES NOMMÉS — rendu (données : GeoNames.build, déterministe par graine).
## Calligraphie IM Fell diluée, taille à l'étendue de l'ensemble, rivières inclinées le
## long du cours ; fade au plan large ET au plan profond (« fadée dans le terrain »).
func _draw_geonames(w, mv: Node2D, vt: Transform2D, vp: Vector2, zoom: float) -> void:
	if _geo_dirty:
		_geonames = GeoNames.build(w, Sim.current_seed)
		_geo_dirty = false
	var fade := clampf((zoom - 1.2) / 0.8, 0.0, 1.0) * (1.0 - clampf((zoom - 6.0) / 2.0, 0.0, 0.85))
	if fade <= 0.03:
		return
	for g in _geonames:
		var gp: Vector2 = mv.iso_pos((g["pos"] as Vector2).x, (g["pos"] as Vector2).y)
		var gss: Vector2 = vt * gp
		if gss.x < -220 or gss.y < -80 or gss.x > vp.x + 220 or gss.y > vp.y + 80:
			continue
		var txt: String = g["text"]
		var tw := VKit.text_map_w(txt, VKit.FS_SMALL)
		# UNE SEULE encre (décision joueur) : ardoise/anthracite, halo parchemin discret
		# (lisible sur canopée sombre comme sur sable). Taille ÉCRAN 12-16 px — les grands
		# ensembles à 16, les petits à 12, jamais un nom géant ni illisible.
		var span := clampf(float(g["span"]), 16.0, 90.0)
		var px_t := clampf(10.0 + span * 0.07, 12.0, 16.0)
		var nsc := px_t / (float(VKit.FS_SMALL) * zoom)
		var col := Color(0.17, 0.20, 0.24, 0.62 * fade)
		var halo := Color(0.90, 0.87, 0.78, 0.28 * fade)
		draw_set_transform(gp, float(g["ang"]), Vector2(nsc, nsc))
		VKit.text_map(self, Vector2(-tw * 0.5, -VKit.FS_SMALL * 0.7), txt, VKit.FS_SMALL, col, 1, halo)
		draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

## ∧ D'ENCRE (montagne cartographique) : deux versants jittés (j = hachage déterministe du
## semis) + un trait d'ombre court sur le versant est (lumière du nord-ouest, classique).
const CHEV_INK := Color(0.25, 0.19, 0.12, 0.58)
const CHEV_INK_LIT := Color(0.25, 0.19, 0.12, 0.32)    ## versant NW éclairé : encre diluée
const CHEV_INK_SHADE := Color(0.19, 0.13, 0.08, 0.80)  ## versant SE ombré : encre chargée
const CHEV_H_WORLD := 5.5    ## hauteur MONDE (cellules) — l'empreinte ne varie plus au zoom

## axe principal des cellules de MONTAGNE autour de (x,y) — Vector2.ZERO si massif rond
## (ACP du masque relief, même famille que l'ancre des noms d'empire) : les ∧ s'alignent
## en LIGNE DE CRÊTE au lieu de s'éparpiller en amas.
func _ridge_dir(bio: Image, x: int, y: int, rad := 8) -> Vector2:
	var sw := bio.get_width()
	var sh := bio.get_height()
	var sxx := 0.0; var syy := 0.0; var sxy := 0.0; var n := 0
	for dy in range(-rad, rad + 1, 2):
		for dx in range(-rad, rad + 1, 2):
			var b := int(bio.get_pixel(clampi(x + dx, 0, sw - 1), clampi(y + dy, 0, sh - 1)).r * 255.0 + 0.5)
			if b == 18 or b == 19 or b == 23:
				sxx += float(dx * dx); syy += float(dy * dy); sxy += float(dx * dy); n += 1
	if n < 4:
		return Vector2.ZERO
	var tr := sxx + syy
	var disc := sqrt(maxf(tr * tr * 0.25 - (sxx * syy - sxy * sxy), 0.0))
	var l1 := tr * 0.5 + disc
	var l2 := tr * 0.5 - disc
	if l2 <= 0.001 or l1 / l2 < 1.6:          # pas d'allongement net → grappe, pas chaîne
		return Vector2.ZERO
	var ang := 0.5 * atan2(2.0 * sxy, sxx - syy)
	return Vector2(cos(ang), sin(ang))

## segment BOMBÉ (une pente n'est jamais une droite) : a→b en 3 points
func _bowed(a: Vector2, b: Vector2, sag: float) -> PackedVector2Array:
	var m := (a + b) * 0.5
	var n := Vector2(-(b - a).y, (b - a).x).normalized()
	return PackedVector2Array([a, m + n * sag, b])

## géométrie d'UN glyphe de relief en espace iso : traits STYLÉS [{pts,col,w}] + polygone
## d'occlusion. Convention carte ancienne : lumière du NW — versant gauche dilué, versant
## droit chargé + hachure d'ombre interne à mi-pente.
func _chev_geom(d: Dictionary, mv: Node2D) -> Dictionary:
	var wp: Vector2 = d["pos"]
	var c: Vector2 = mv.iso_pos(wp.x, wp.y)
	var h := CHEV_H_WORLD * float(d["scale"])
	var j: Array = d.get("j", [0.5, 0.5, 0.5])
	var wf := h * (0.85 + 0.50 * float(j[0]))                 # largeur ±30 %
	var apex := c + Vector2((float(j[1]) - 0.5) * 0.30 * wf,  # apex décalé (asymétrie)
		-h * 0.52 + (float(j[2]) - 0.5) * 0.20 * h)
	var lf := c + Vector2(-wf * 0.5, h * 0.34)
	var rf := c + Vector2(wf * 0.5, h * 0.30 - (float(j[2]) - 0.5) * 0.10 * h)
	var iw := clampf(h / CHEV_H_WORLD, 0.6, 1.3)              # poids de plume ∝ taille du pic
	if d.get("rond", false):
		var apx := c + (apex - c) * 0.80                      # colline : dôme tassé
		var dome := PackedVector2Array()
		for k in range(8):
			var t := float(k) / 7.0
			dome.append(lf.lerp(apx, t).lerp(apx.lerp(rf, t), t))   # bézier quadratique
		return {"strokes": [
			{"pts": dome.slice(0, 4), "col": CHEV_INK_LIT,   "w": iw},
			{"pts": dome.slice(3),    "col": CHEV_INK_SHADE, "w": iw * 1.5},
		], "poly": dome}
	# hachure d'ombre : trait court DANS le versant SE, à mi-pente
	var inn := (lf + rf + apex) / 3.0
	var h0 := apex.lerp(rf, 0.30).lerp(inn, 0.22)
	var h1 := apex.lerp(rf, 0.85).lerp(inn, 0.22)
	return {"strokes": [
		{"pts": _bowed(lf, apex, h * 0.05), "col": CHEV_INK_LIT,   "w": iw},
		{"pts": _bowed(apex, rf, h * 0.06), "col": CHEV_INK_SHADE, "w": iw * 1.8},
		{"pts": _bowed(h0, h1, h * 0.03),   "col": CHEV_INK_SHADE, "w": iw * 0.75},
	], "poly": PackedVector2Array([apex, rf, lf])}

## PRIORITÉ DES RELIEFS (décision joueur : intérieurs TRANSPARENTS — le remplissage ne
## collait jamais au fond) : le trait d'un glyphe est COUPÉ par le polygone de chaque
## glyphe DEVANT lui (y plus grand = dessiné après). Précalculé UNE fois au semis
## (panier spatial + Geometry2D) — le draw ne fait que tracer les polylignes restantes.
func _clip_relief() -> void:
	var mv := get_parent() as Node2D
	if mv == null or not mv.has_method("iso_pos"):
		return
	var relief := []
	for d in _dressing:
		if String(d["id"]) == "chevron":
			d["geom"] = _chev_geom(d, mv)
			relief.append(d)
	var buckets := {}
	for idx in range(relief.size()):
		var wp: Vector2 = (relief[idx] as Dictionary)["pos"]
		var key := Vector2i(int(wp.x) / 8, int(wp.y) / 8)
		if not buckets.has(key):
			buckets[key] = []
		(buckets[key] as Array).append(idx)
	for i in range(relief.size()):
		var di: Dictionary = relief[i]
		var strokes: Array = (di["geom"] as Dictionary)["strokes"]
		var wpi: Vector2 = di["pos"]
		var ki := Vector2i(int(wpi.x) / 8, int(wpi.y) / 8)
		for oy in range(-1, 2):
			for ox in range(-1, 2):
				for jdx in buckets.get(Vector2i(ki.x + ox, ki.y + oy), []):
					if int(jdx) == i:
						continue
					var dj: Dictionary = relief[int(jdx)]
					if (dj["pos"] as Vector2).y <= wpi.y:
						continue                     # seul un glyphe DEVANT coupe
					var poly: PackedVector2Array = (dj["geom"] as Dictionary)["poly"]
					var next: Array = []
					for s in strokes:
						var sd: Dictionary = s
						for part in Geometry2D.clip_polyline_with_polygon(sd["pts"], poly):
							if (part as PackedVector2Array).size() >= 2:
								next.append({"pts": part, "col": sd["col"], "w": sd["w"]})
					strokes = next
					if strokes.is_empty():
						break
		di["segs"] = strokes

## taille à l'ÉCRAN (px) d'une marque selon sa famille (montagnes grandes, herbe de plaine petite).
func _dress_size(id: String) -> float:
	if id.begins_with("sea_serpent"): return 84.0          # lot 4 : serpent (largeur ×2 au tracé → 2:1)
	if id.begins_with("lot6_broadleaf") or id.begins_with("lot6_conifer"): return 18.0   # lot 6 : arbre isolé (registre canopée)
	if id.begins_with("lot6_ground"): return 22.0          # lot 6 : détail de sol (buisson/rocher/herbe)
	if id.begins_with("mountain_range"): return 50.0
	if id.begins_with("mountain"): return 42.0
	if id.begins_with("forest"): return 38.0
	if id.begins_with("dune") or id.begins_with("sea_") or id.begins_with("ocean") or id.begins_with("water"): return 34.0
	if id.begins_with("apoc_rabbit"): return 32.0          # lot 4 : lapin marginalia
	if id.begins_with("hill"): return 30.0
	if id.begins_with("shipwreck") or id.begins_with("broken") or id.begins_with("half_sunk") or id.begins_with("floating") \
	   or id.begins_with("jagged") or id.begins_with("low_rocks") or id.begins_with("sea_stacks") or id.begins_with("shoal"): return 30.0  # épaves/récifs
	if id.begins_with("savanna") or id.begins_with("acacia") or id.begins_with("steppe") or id.begins_with("marsh"): return 28.0
	if id.begins_with("tree") or id.begins_with("reeds") or id.begins_with("rocky"): return 26.0
	if id.begins_with("plain"): return 24.0
	return 24.0

## SÈME les marques de terrain par BIOME (grille jittée déterministe), une fois à la génération. Display-only.
func _build_dressing() -> void:
	_dressing.clear()
	var w = Sim.world
	if w == null:
		return
	var bio: Image = w.layer_image(LAYER_BIOME)
	if bio == null:
		return
	var rf: Image = _carved_river_field()      # champ rivière → on N'ENTASSE PAS de marques sur les fleuves
	# la CLAIRIÈRE DES BOURGS : aucune marque de terrain dans le rayon d'un lieu HABITÉ
	# (les arbres ne poussent pas sur les toits ; la VIGNETTE de bourg y respire).
	_dress_clear.clear()
	for r in range(w.region_count()):
		var tier: int = w.region_tier(r)
		var owner: int = w.region_owner(r)
		var role: int = int(w.country_role(owner)) if owner >= 0 else -1
		# même gate que le dessin des bourgs : pas d'habitants ⇒ pas de clairière
		if (tier < 0 or owner < 0 or int(w.region_pop(r)) < 150) and role != 2 and role != 4:
			continue
		var c: Vector2 = _region_seat.get(r, w.region_centroid(r))
		if c.x < 0:
			continue
		var rad := 2.3 + 0.5 * float(maxi(tier, 1)) + (1.4 if role == 2 else 0.0)
		_dress_clear.append([c, rad * rad])
	var sw := bio.get_width()
	var sh := bio.get_height()
	var i := 0
	var y := roundi(DRESS_SPACING * 0.5)
	while y < sh:
		var x := roundi(DRESS_SPACING * 0.5)
		while x < sw:
			# 1 + N passes selon le biome de la cellule (forêts = canopée DENSE → plusieurs marques/cellule).
			var bb := int(bio.get_pixel(clampi(x, 0, sw - 1), clampi(y, 0, sh - 1)).r * 255.0 + 0.5)
			var passes := 1 + int(DRESS_EXTRA.get(bb, 0))
			for p in range(passes):
				i += 1
				_try_place_dress(i, x, y, bio, rf, sw, sh)
			# retour joueur 2026-07-29 : le PAS d'avancée suit le biome (le relief est GRAND,
			# ~8-9 cellules d'empreinte — il veut de l'air, cf. DRESS_SPACING_BY_BIOME) — le PAS
			# seulement, jamais l'amplitude du jitter (_try_place_dress, inchangée).
			x += int(DRESS_SPACING_BY_BIOME.get(bb, DRESS_SPACING))
		y += DRESS_SPACING
	# ── LA CANOPÉE COMPOSÉE : passe dédiée à PAS FIN sur les biomes de forêt — chaque arbre
	#    est un INDIVIDU (lot 6) en espace MONDE, servi en MULTIMESH (un draw call par essence :
	#    des CENTAINES de milliers d'instances pour un coût par-frame NUL — jamais dans _dressing).
	#    Seaux en 3 Array PARALLÈLES (pos/taille/teinte) — ⚠ JAMAIS de packed array dans un
	#    conteneur : type VALEUR (COW), `(bk[0] as Packed…).append()` mute une COPIE et les
	#    seaux restent VIDES (forêt disparue, pris au shot). L'ordre de semis (lignes y
	#    croissantes) EST le tri fond→avant. ──
	var buckets := {}                          # id d'essence → [Array(Vector2), Array(float), Array(Color)]
	var ci := 500000
	var cy := 1
	while cy < sh:
		var cx := 1
		while cx < sw:
			ci += 1
			# jitter FRACTIONNAIRE (jamais tronqué en cellule) : au pas fin, un jitter entier
			# retombe pile sur la grille → colonnes d'arbres visibles. On sème en float.
			var fx := clampf(float(cx) + (_h1(float(ci) * 1.9) - 0.5) * float(CANOPY_STEP) * 1.2, 0.0, float(sw - 1))
			var fy := clampf(float(cy) + (_h1(float(ci) * 3.7) - 0.5) * float(CANOPY_STEP) * 1.2, 0.0, float(sh - 1))
			var px := int(fx)
			var py := int(fy)
			# VOTE DE VOISINAGE (3 échantillons) : la carte de biomes est BRUITÉE à la cellule —
			# un seul point troue le peuplement. 3/3 = cœur PLEIN (+3 individus), 1/3 = lisière plumée.
			var hits := 0
			var bhit := -1
			for off in [[0, 0], [3, 1], [-2, 3]]:
				var sx := clampi(px + int(off[0]), 0, sw - 1)
				var sy := clampi(py + int(off[1]), 0, sh - 1)
				var b3 := int(bio.get_pixel(sx, sy).r * 255.0 + 0.5)
				if CANOPY_BY_BIOME.has(b3):
					hits += 1
					if bhit < 0:
						bhit = b3
			# GARDE-FOU EAU : le VOTE DE VOISINAGE regarde jusqu'à ±3 cellules (il lisse les bords
			# de biome BRUITÉS) — mais un hit qui ne vient QUE d'un échantillon décalé peut retomber
			# sur une ANCRE (px,py) déjà dans l'eau (île étroite/presqu'île). La position RÉELLE
			# de pose (fx,fy, quasi = px,py) doit donc être testée à SON PROPRE compte, pas seulement
			# via le vote — sinon un arbre pousse dans la mer/le lac (archipel, mers intérieures).
			if bhit >= 0 and not _near_river(rf, px, py) and not _water_at(px, py):
				var skip := false
				for cl in _dress_clear:            # la clairière des bourgs vaut aussi en forêt
					if (cl[0] as Vector2).distance_squared_to(Vector2(px, py)) < float(cl[1]):
						skip = true
						break
				# TROUÉE ROUTIÈRE (spec joueur 2026-07-31) : aucune canopée au cœur du grand
				# chemin, arbres raréfiés sur ses bords — la forêt MONTRE le passage. Les
				# sentiers (dessertes tier 1) ne trouent pas : leurs arbres débordent.
				var rc := float(_road_clear.get((px << 16) | (py & 0xFFFF), 1.0))
				if rc <= 0.0 or (rc < 1.0 and _h1(float(ci) * 6.7) > rc):
					skip = true
				var pk: float = [0.0, 0.35, 0.95, 1.0][hits]
				if not skip and _h1(float(ci) * 5.3) < pk:
					var cids: Array = CANOPY_BY_BIOME[bhit]
					var cid: String = cids[int(_h1(float(ci) * 7.1) * float(cids.size())) % cids.size()]
					var tt2: Variant = _dress_tint(cid)
					var tc: Color = tt2 if tt2 != null else Color(1, 1, 1, 0.6)
					var vj := 0.90 + 0.20 * _h1(float(ci) * 9.3)   # variation de VALEUR par arbre (vie)
					if not buckets.has(cid):
						buckets[cid] = [[], [], []]
					var bk: Array = buckets[cid]
					(bk[0] as Array).append(Vector2(fx, fy))
					(bk[1] as Array).append(1.6 * (0.72 + 0.55 * _h1(float(ci) * 11.7)))
					(bk[2] as Array).append(Color(tc.r * vj, tc.g * vj, tc.b * vj, tc.a))
					# cœur & mi-lisière : des individus EN PLUS — l'échelle symbole demande le NOMBRE
					# (lisière ~1, mi-lisière ~20, cœur 40/point ; offsets hashés ±3.5 cellules =
					# les grappes se fondent, jamais de motif de grille)
					var extra: int = [0, 3, 19, 39][hits]
					for e in range(extra):
						var eb := float(ci) * (13.1 + 8.6 * float(e))
						var qfx := clampf(fx + (_h1(eb) - 0.5) * 7.0, 0.0, float(sw - 1))
						var qfy := clampf(fy + (_h1(eb * 1.7) - 0.5) * 7.0, 0.0, float(sh - 1))
						var qskip := false
						for cl2 in _dress_clear:       # l'offset ±3.5 peut retomber DANS la clairière d'un bourg
							if (cl2[0] as Vector2).distance_squared_to(Vector2(qfx, qfy)) < float(cl2[1]):
								qskip = true
								break
						# l'offset ±3.5 cellules est testé à SA POSITION FINALE (qfx,qfy), pas à
						# l'ancrage : sur une côte proche (île/presqu'île), l'extra peut sinon
						# déborder en pleine mer/lac — jamais vérifié jusqu'ici.
						if not qskip:                # trouée routière : les extras aussi (même carte de densité)
							var qrc := float(_road_clear.get((int(qfx) << 16) | (int(qfy) & 0xFFFF), 1.0))
							if qrc <= 0.0 or (qrc < 1.0 and _h1(eb * 3.3) > qrc):
								qskip = true
						if not qskip and not _near_river(rf, int(qfx), int(qfy)) and not _water_at(int(qfx), int(qfy)):
							var cid2: String = cids[int(_h1(eb * 1.9) * float(cids.size())) % cids.size()]
							var vj2 := 0.90 + 0.20 * _h1(eb * 2.3)
							if not buckets.has(cid2):
								buckets[cid2] = [[], [], []]
							var bk2: Array = buckets[cid2]
							(bk2[0] as Array).append(Vector2(qfx, qfy))
							(bk2[1] as Array).append(1.6 * (0.72 + 0.55 * _h1(eb * 2.9)))
							(bk2[2] as Array).append(Color(tc.r * vj2, tc.g * vj2, tc.b * vj2, tc.a))
			cx += CANOPY_STEP
		cy += CANOPY_STEP
	_canopy_flush(buckets)                     # → MultiMesh par essence (tri fond→avant interne)
	_build_easter_eggs(bio, rf, sw, sh)        # lot 4 : serpents/épaves/récifs/lapins (rares)
	# TRI (bande de profondeur, puis id) : le décor s'empile du fond vers l'avant (y croissant)
	# tout en gardant les mêmes textures CONSÉCUTIVES dans une bande (le batcher 2D fusionne).
	_dressing.sort_custom(func(a, b):
		var ba := int((a["pos"] as Vector2).y) >> 2
		var bb3 := int((b["pos"] as Vector2).y) >> 2
		if ba != bb3:
			return ba < bb3
		return String(a["id"]) < String(b["id"]))
	_clip_relief()   # PRIORITÉ des ∧/dômes : le trait de devant coupe celui de derrière
	_finalize_dress_fast()   # revue #6 : tableaux typés pré-projetés pour le draw (sprites)

## SÉPARE `_dressing` (Dictionary, build-only) en deux formes RAPIDES pour le draw (revue overlay
## #6) : les CHEVRONS (peu nombreux, géométrie déjà projetée/clippée par _clip_relief) restent en
## Dictionary dans `_dress_relief` — juste enrichis d'un `ip` pré-projeté pour le test de
## visibilité écran ; les SPRITES (le gros du volume — grass/steppe/désert/lot6…) passent en
## tableaux PARALLÈLES TYPÉS (`_dress_fast_*`), position ISO déjà projetée, texture déjà résolue,
## teinte finale déjà résolue — le draw n'a plus qu'à indexer, zéro hash Dictionary, zéro
## mv.iso_pos()/frame. ⚠ Ordre de dessin : les chevrons se dessinent maintenant en PREMIER (voir
## _draw_iso), les sprites ENSUITE — l'ancien tri combiné (fond→avant, y-bande puis id) les
## interclassait par y ; l'écart est cosmétique (marques semi-transparentes, chevauchement rare
## aux frontières de biome) — vérifié au probe visuel (voir TROUVAILLES), pas de MultiMesh ici
## (chantier séparé).
func _finalize_dress_fast() -> void:
	_dress_fast_ip = PackedVector2Array()
	_dress_fast_tex = []
	_dress_fast_h = PackedFloat32Array()
	_dress_fast_wide = PackedByteArray()
	_dress_fast_col = PackedColorArray()
	_dress_relief.clear()
	var mv := _mv_ref()
	if mv == null:
		return
	var dress_col := Color(1, 1, 1, DRESS_ALPHA)
	var egg_col := Color(1, 1, 1, EGG_ALPHA)
	for d in _dressing:
		var did: String = d["id"]
		var wp: Vector2 = d["pos"]
		if did == "chevron":
			d["ip"] = mv.iso_pos(wp.x, wp.y)   # pré-projeté : le draw ne refait plus l'appel/frame
			_dress_relief.append(d)
			continue
		var dtex := _dress_get(did)
		if dtex == null:
			continue                          # pas de sprite pour cet id : jamais dessiné (comme avant)
		_dress_fast_ip.append(mv.iso_pos(wp.x, wp.y))
		_dress_fast_tex.append(dtex)
		_dress_fast_h.append(_dress_size(did) * float(d["scale"]))
		_dress_fast_wide.append(1 if did.begins_with("sea_serpent") else 0)
		var is_egg: bool = d.get("egg", false)
		var tint: Variant = d.get("tint", null)
		_dress_fast_col.append(tint if tint != null else (egg_col if is_egg else dress_col))

## LA CANOPÉE EN MULTIMESH : un quad partagé (pied à l'origine, y vers le bas), une instance
## par arbre (transform en espace ISO + teinte), UN batch par essence — le coût par frame est
## indépendant du nombre (des dizaines de milliers d'arbres = ~20 draw calls, zéro GDScript).
func _canopy_flush(buckets: Dictionary) -> void:
	_canopy_batches.clear()
	var mv := _mv_ref()
	if mv == null:
		return
	if _canopy_mesh == null:
		# quad 1×1 : pied (bas) à l'origine, sommet à y=-1 ; uv(0,0) = haut de l'image → droit
		var verts := PackedVector2Array([Vector2(-0.5, -1), Vector2(0.5, -1), Vector2(0.5, 0), Vector2(-0.5, 0)])
		var uvs := PackedVector2Array([Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1)])
		var idx := PackedInt32Array([0, 1, 2, 0, 2, 3])
		var arrays := []
		arrays.resize(Mesh.ARRAY_MAX)
		arrays[Mesh.ARRAY_VERTEX] = verts
		arrays[Mesh.ARRAY_TEX_UV] = uvs
		arrays[Mesh.ARRAY_INDEX] = idx
		_canopy_mesh = ArrayMesh.new()
		_canopy_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	var keys := buckets.keys()
	keys.sort()                                # ordre de dessin STABLE entre rebuilds
	for tid in keys:
		var bk: Array = buckets[tid]
		var pos: Array = bk[0]
		var siz: Array = bk[1]
		var col: Array = bk[2]
		var tex := _dress_get(String(tid))
		if tex == null or pos.is_empty():
			continue
		# PAS de tri : l'ordre de semis (lignes y croissantes) est déjà fond→avant à ±3.5
		# cellules près — invisible à l'échelle symbole, et un sort_custom sur des centaines
		# de milliers d'entrées coûterait des dizaines de secondes.
		var tsz: Vector2 = tex.get_size()
		var asp: float = tsz.x / maxf(tsz.y, 1.0)
		var mm := MultiMesh.new()
		mm.transform_format = MultiMesh.TRANSFORM_2D
		mm.use_colors = true
		mm.mesh = _canopy_mesh
		mm.instance_count = pos.size()
		for k in range(pos.size()):
			var wp: Vector2 = pos[k]
			var dh: float = siz[k]
			var ip: Vector2 = mv.iso_pos(wp.x, wp.y)
			# le pied du quad au point ; le tronc vit à ~85 % de la hauteur de l'image
			# → on descend le quad de 15 % (même ancrage que l'ancien draw_texture_rect)
			mm.set_instance_transform_2d(k, Transform2D(Vector2(dh * asp, 0.0), Vector2(0.0, dh),
				ip + Vector2(0.0, dh * 0.15)))
			mm.set_instance_color(k, col[k])
		_canopy_batches.append({"mm": mm, "tex": tex})

## tente UNE marque jittée à partir de (x,y) : hors rivière, biome connu, sous la densité → ajoutée.
func _try_place_dress(i: int, x: int, y: int, bio: Image, rf: Image, sw: int, sh: int) -> void:
	var jx := int((_h1(float(i) * 1.7) - 0.5) * float(DRESS_SPACING))
	var jy := int((_h1(float(i) * 3.3) - 0.5) * float(DRESS_SPACING))
	var px := clampi(x + jx, 0, sw - 1)
	var py := clampi(y + jy, 0, sh - 1)
	if _near_river(rf, px, py):
		return                                 # JAMAIS sur/au bord d'une rivière (sinon elle transparaît sous la marque)
	for cl in _dress_clear:                    # ni dans la CLAIRIÈRE d'un bourg (la vignette y règne)
		if (cl[0] as Vector2).distance_squared_to(Vector2(px, py)) < float(cl[1]):
			return
	var b := int(bio.get_pixel(px, py).r * 255.0 + 0.5)
	if not DRESS_BY_BIOME.has(b):
		return
	var dens: float = DRESS_DENSITY.get(b, 0.6)
	if _h1(float(i) * 5.1) > dens:
		return
	var ids: Array = DRESS_BY_BIOME[b]
	var id: String = ids[int(_h1(float(i) * 7.7) * float(ids.size())) % ids.size()]
	var scl := 0.85 + 0.30 * _h1(float(i) * 9.9)   # 0.85..1.15 : variété d'échelle
	# RELIEF = ∧ D'ENCRE PROCÉDURAUX (décision joueur 2026-07-28, étendue « il reste des
	# anciens assets ») : TOUT biome de relief (hautes-terres/collines/montagnes/pic/volcan)
	# bascule en chevron — plus AUCUN sprite hill_*/rocky_*/mountain_* sur le relief.
	# Collines/hautes-terres : chevrons PETITS (×0.6). Jitter ±30 % déterministe (_h1).
	if b == 16 or b == 17 or b == 18 or b == 19 or b == 23:
		id = "chevron"
		scl = 0.70 + 0.60 * _h1(float(i) * 9.9)    # ±30 % d'échelle
		if b == 16 or b == 17:
			scl *= 0.60                            # collines : chevrons discrets
		# LIGNE DE CRÊTE (passe 2) : la marque COULISSE le long de l'axe local du massif
		# — le jitter de grille devient une chaîne au lieu d'un amas.
		if b == 18 or b == 19 or b == 23:
			var axis := _ridge_dir(bio, px, py)
			if axis != Vector2.ZERO:
				var fp := Vector2(px, py) + axis * (_h1(float(i) * 21.3) - 0.5) * float(DRESS_SPACING) * 1.4
				px = clampi(int(fp.x), 0, sw - 1)
				py = clampi(int(fp.y), 0, sh - 1)
		# BLEND MONTAGNE/RIVIÈRE : un ∧ trop proche d'un cours d'eau saute (rayon élargi —
		# son empreinte dépasse la cellule de semis).
		if _near_river(rf, px, py, 2):
			return
	var entry := {"pos": Vector2(px, py), "id": id, "scale": scl}
	if id == "chevron":
		# LIGNE DE BASE COMMUNE (retour joueur 2026-07-29) : convention de gravure — les SOMMETS
		# varient (jitter/crête ci-dessus), pas les PIEDS. Alignés par rangées de 5 cellules, sur
		# le py FINAL (après le glissement de crête, donc compatible avec lui).
		entry["pos"] = Vector2(px, floor(py / 5.0) * 5.0 + 2.5)
		entry["j"] = [_h1(float(i) * 11.3), _h1(float(i) * 13.7), _h1(float(i) * 17.1)]
		if b == 16 or b == 17:
			entry["rond"] = true                   # colline : V ARRONDI (dôme, décision joueur)
	var tt: Variant = _dress_tint(id)              # teinte lot 6 posée au BUILD (coût nul au draw)
	if tt != null:
		entry["tint"] = tt
	_dressing.append(entry)

## LOT 4 — easter eggs RARES : serpent sur l'océan profond (cap 3), épave/récif sur le haut-fond, lapin
## marginalia sur terre (cap 2). Grille grossière + faibles probas → rares mais présents.
func _build_easter_eggs(bio: Image, rf: Image, sw: int, sh: int) -> void:
	var serp := 0
	var rab := 0
	var i := 100000
	var y := roundi(EGG_SPACING * 0.5)
	while y < sh:
		var x := roundi(EGG_SPACING * 0.5)
		while x < sw:
			i += 1
			var b := int(bio.get_pixel(clampi(x, 0, sw - 1), clampi(y, 0, sh - 1)).r * 255.0 + 0.5)
			var r := _h1(float(i) * 2.13)
			if b == 0 and serp < 3 and r < 0.10:                       # OCÉAN PROFOND → serpent
				var sid := "sea_serpent_01" if _h1(float(i) * 4.4) < 0.5 else "sea_serpent_02"
				_dressing.append({"pos": Vector2(x, y), "id": sid, "scale": 1.0, "egg": true})
				serp += 1
			elif b == 2 and r < 0.05:                                  # HAUT-FOND → épave/récif
				var wid: String = EGG_WRECKS[int(_h1(float(i) * 6.6) * float(EGG_WRECKS.size())) % EGG_WRECKS.size()]
				_dressing.append({"pos": Vector2(x, y), "id": wid, "scale": 1.0, "egg": true})
			elif b >= 4 and b <= 9 and rab < 2 and r > 0.99 and not _near_river(rf, x, y):  # TERRE → lapin (ultra-rare)
				var rid: String = EGG_RABBITS[int(_h1(float(i) * 8.8) * float(EGG_RABBITS.size())) % EGG_RABBITS.size()]
				_dressing.append({"pos": Vector2(x, y), "id": rid, "scale": 1.0, "egg": true})
				rab += 1
			x += EGG_SPACING
		y += EGG_SPACING

## LA VIGNETTE DE BOURG (lot U) : la ville entière est UNE gravure du pack bourgs/ — famille
## par RÔLE (cité-état `bourg_cs` · hameau libre `bourg_wild`) ou par TIER de vignette 1..7 :
##   · tier façade 0-1 → t1 (ferme) · 2 → t2 · 3 → t3 · 4 → t4 · 5 → t5 (grandes cités) ;
##   · la CAPITALE d'un pays monte d'UN cran (la façade la force déjà ≥ 4 ⇒ t5/t6) ;
##   · t7 est UNIQUE : la capitale la plus peuplée du monde (la cité impériale).
## Variante = hash STABLE de la région (_01.._16 — deux voisines diffèrent, stable au redraw).
## Ancrée au PIED sur le siège (sec — centroïde ancré) ; taille MONDE ∝ tier, rails px par _w.
## OMBRE SE = la silhouette du sprite modulée sombre (le motif front32) ; GLAZE = valeur
## jittée par région, jamais la teinte. Quais/barque gardés. Repli : glyphe d'encre.
func _draw_settlement(w, r: int, role: int, ctr: Vector2, ip: Vector2, zoom: float, mv) -> void:
	var is_cs := role == 2
	var is_wild := role == 4
	var t: int = clampi(w.region_tier(r), 0, 5)
	var st := maxi(t, 1)                          # tier de vignette : 0-1→t1 · 2→t2 · … · 5→t5
	if not is_cs and not is_wild:
		var owner0: int = w.region_owner(r)
		var is_cap: bool = owner0 >= 0 and w.province_region(w.country_capital_province(owner0)) == r
		if is_cap:
			st = mini(st + 1, 6)                  # la CAPITALE monte d'un cran (⇒ t5/t6)
			if r == _top_cap_region:
				st = 7                            # LA plus grande capitale du monde : la cité impériale
	var v := 1 + int(_h1(float(r) * 23.7) * float(BOURG_VARIANTS)) % BOURG_VARIANTS
	var sid: String
	if is_wild:
		sid = "bourg_wild_%02d" % v
	elif is_cs:
		sid = "bourg_cs_%02d" % v
	else:
		sid = "bourg_t%d_%02d" % [st, v]
	# le plan (vignette + quais) se REBÂTIT quand la vignette change : le bourg grandit avec
	# son tier, le titre de plus grande capitale peut changer de mains.
	if not _town_cache.has(r) or String(_town_cache[r].get("sid", "")) != sid:
		var plan := _build_quays(r, ctr, t, is_cs, is_wild)
		plan["sid"] = sid
		_town_cache[r] = plan
	var town: Dictionary = _town_cache[r]
	# 1. les QUAIS : jetées de bois dans l'eau + barque amarrée — le bourg regarde le large
	for qd in town.get("quays", []):
		var qa: Vector2 = qd["a"]
		var qv: Vector2 = qd["d"]
		var q0: Vector2 = mv.iso_pos(qa.x, qa.y)
		var q1: Vector2 = mv.iso_pos(qa.x + qv.x * 1.15, qa.y + qv.y * 1.15)
		var qt := (q1 - q0).normalized()
		var qp := Vector2(-qt.y, qt.x)
		var qw := _w(zoom, 0.10, 0.8, 1.8)
		draw_colored_polygon(PackedVector2Array([q0 + qp * qw, q1 + qp * qw, q1 - qp * qw, q0 - qp * qw]), QUAY_WOOD)
		draw_polyline(PackedVector2Array([q0 + qp * qw, q1 + qp * qw, q1 - qp * qw, q0 - qp * qw, q0 + qp * qw]),
			Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.65), _w(zoom, 0.035, 0.3, 0.6), true)
		for ps in [0.3, 0.65, 1.0]:                       # les PIEUX (pointillés d'encre au bord)
			var pp4: Vector2 = q0.lerp(q1, float(ps))
			draw_circle(pp4 + qp * qw, _w(zoom, 0.03, 0.3, 0.55), TOWN_INK)
			draw_circle(pp4 - qp * qw, _w(zoom, 0.03, 0.3, 0.55), TOWN_INK)
	var bt: Dictionary = town.get("boat", {})
	if bt.has("c"):
		var bc: Vector2 = bt["c"]
		var bax: Vector2 = bt["ax"]
		var b0: Vector2 = mv.iso_pos(bc.x, bc.y)
		var bxi: Vector2 = (mv.iso_pos(bc.x + bax.x, bc.y + bax.y) - b0).normalized()
		var bpi := Vector2(-bxi.y, bxi.x)
		var bl := _w(zoom, 0.30, 2.0, 4.4)
		var bw2 := _w(zoom, 0.11, 0.8, 1.7)
		var hull := PackedVector2Array([b0 - bxi * bl * 0.8 + bpi * bw2 * 0.7, b0 + bxi * bl * 0.55 + bpi * bw2,
			b0 + bxi * bl, b0 + bxi * bl * 0.55 - bpi * bw2, b0 - bxi * bl * 0.8 - bpi * bw2 * 0.7])
		draw_colored_polygon(hull, BOAT_WOOD)
		draw_polyline(PackedVector2Array(Array(hull) + [hull[0]]),
			Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.70), _w(zoom, 0.035, 0.3, 0.6), true)
		draw_line(b0, b0 + Vector2(0, -bl * 0.9), Color(TOWN_INK.r, TOWN_INK.g, TOWN_INK.b, 0.75),
			_w(zoom, 0.035, 0.3, 0.6), true)              # le mât nu (barque amarrée)
	# 2. la VIGNETTE : ombre portée SE (la silhouette du sprite, motif front32) puis la
	#    gravure glacée — largeur MONDE du CONTENU ∝ tier, ancrage au PIED (socle du bbox).
	var bg := _bourg_get(sid)
	var rt := 6 if is_cs else (1 if is_wild else st)      # tier de RAIL px (cs ≈ t6, wild ≈ t1)
	if bg.is_empty():
		_draw_town(ip, rt, zoom, Color(0.20, 0.14, 0.09, 0.95))   # repli : glyphe d'encre
		return
	var cwld := lerpf(BOURG_W_T1, BOURG_W_T7, float(st - 1) / 6.0)
	if is_cs:
		cwld = BOURG_W_CS
	elif is_wild:
		cwld = BOURG_W_WILD
	var wpx := _w(zoom, cwld, 10.0 + 1.7 * float(rt), 65.0 + 16.0 * float(rt))
	var fw := wpx / maxf(float(bg["cw"]), 0.4)            # cadre 256² tel que le CONTENU couvre cwld
	var rect := Rect2(ip - Vector2(fw * 0.5, fw * float(bg["foot"])), Vector2(fw, fw))
	var tex: Texture2D = bg["tex"]
	var shadow: Texture2D = bg.get("shadow", null)
	if shadow != null:
		# Le sprite n'est plus simplement décalé : sa silhouette est comprimée contre
		# le pied et étirée vers le SE, comme une ombre couchée sur la carte.
		var sh_h := fw * 0.30
		var sh_w := fw * 1.08
		var sh_foot := float(bg["foot"])
		var sh_pos := Vector2(ip.x - sh_w * 0.50 + fw * 0.075,
			ip.y - sh_h * sh_foot + fw * 0.040)
		var sh_a := TOWN_SHADOW.a * (TOWN_SHADOW_WILD_MUL if is_wild else 1.0)
		draw_texture_rect(shadow, Rect2(sh_pos, Vector2(sh_w, sh_h)), false,
			Color(TOWN_SHADOW.r, TOWN_SHADOW.g, TOWN_SHADOW.b, sh_a))
	var vj := 0.93 + 0.10 * _h1(float(r) * 5.7)           # GLAZE : valeur jittée par région (vie)
	var town_a := BOURG_ALPHA_WILD if is_wild else BOURG_ALPHA
	draw_texture_rect(tex, rect, false, Color(vj, vj, vj * 0.99, town_a))

## BANNIÈRE DE LIEU (référence KCD) : chip parchemin + liseré d'encre + NOM du siège +
## pastille au pigment du propriétaire — taille ÉCRAN constante, posée AU-DESSUS du tampon.
## Éclot au plan rapproché (a = fondu d'éclosion), le relais des noms de pays effacés.
func _draw_banner(w, r: int, ip: Vector2, zoom: float, a: float) -> void:
	if a <= 0.02:
		return
	var nm: String
	if _region_label.has(r):
		nm = _region_label[r]
	else:
		# TOPONYMIE : nom de VILLE (grain région, scps_region_city_name) — assigné UNE fois
		# par le moteur (balayage annuel toponym_world_tick), donc mis en cache DÉFINITIF dès
		# qu'il existe. Tant qu'il n'existe pas encore (ville pas encore nommée cette
		# année-là, ou méthode absente sur un vieux binaire), on NE cache PAS : repli sur le
		# nom de région via la province-siège (motif d'avant cette mission), recalculé
		# chaque frame jusqu'à ce que le vrai nom de ville apparaisse (sinon la bannière
		# resterait figée sur un nom de région périmé).
		var city := String(w.region_city_name(r)) if w.has_method("region_city_name") else ""
		if city != "":
			nm = city
			_region_label[r] = nm
		else:
			nm = ""
			var anc: Vector2 = _region_seat.get(r, Vector2(-1, -1))
			if anc.x >= 0 and w.has_method("province_at"):
				var pid: int = w.province_at(int(anc.x), int(anc.y))
				if pid >= 0:
					nm = String(w.province_info(pid).get("nom", ""))
	if nm == "":
		return
	var sc := 1.0 / maxf(zoom, 0.0001)
	var tw := VKit.text_map_w(nm, VKit.FS_SMALL) * sc   # cartouche : police de CARTE (IM Fell)
	var bh := 14.0 * sc
	var hpad := 5.0 * sc
	var dotw := 7.0 * sc                                   # place de la pastille de propriétaire
	var bw := tw + hpad * 2.0 + dotw
	var top := ip.y - 34.0 * sc - bh                       # au-dessus du tampon (écran constant)
	var rect := Rect2(Vector2(ip.x - bw * 0.5, top), Vector2(bw, bh))
	# ombre portée + CARTOUCHE parchemin (planche 1, pièce 11) — repli : rects plats
	draw_rect(Rect2(rect.position + Vector2(1.2 * sc, 1.4 * sc), rect.size), Color(0.10, 0.07, 0.04, 0.35 * a))
	var chip: Dictionary = UIKit.parch_piece("sheet01_panel_chrome_11")
	if not chip.is_empty():
		draw_texture_rect_region(chip["tex"], rect, chip["rect"], Color(1, 1, 1, a))
	else:
		draw_rect(rect, Color(0.97, 0.93, 0.80, 0.94 * a))                       # le parchemin du chip
		draw_rect(rect, Color(0.25, 0.18, 0.10, 0.95 * a), false, 1.2 * sc)      # liseré d'encre franc
	var own := int(w.region_owner(r))
	var dot: Color = _entity_pigment(own) if own >= 0 else Color(0.52, 0.46, 0.36)
	draw_circle(Vector2(rect.position.x + hpad + 1.5 * sc, rect.position.y + bh * 0.5), 2.6 * sc, Color(dot, a))
	draw_set_transform(Vector2(rect.position.x + hpad + dotw, rect.position.y + 1.0 * sc), 0.0, Vector2(sc, sc))
	VKit.text_map(self, Vector2.ZERO, nm, VKit.FS_SMALL,
		Color(VKit.COL_INK_MAP.r, VKit.COL_INK_MAP.g, VKit.COL_INK_MAP.b, 0.95 * a), 0)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)

## glyphe de ville à l'encre : cercle crème cerné d'encre, taille ∝ tier ; capitale (tier≥4) étoilée.
func _draw_town(ip: Vector2, tier: int, zoom: float, ink: Color) -> void:
	var cream := Color(0.94, 0.89, 0.74, 1.0)
	var r := lerpf(2.2, 6.5, clampf(float(tier) / 5.0, 0.0, 1.0)) / zoom
	draw_circle(ip, r, ink)
	draw_circle(ip, r - 1.2 / zoom, cream)
	draw_circle(ip, r * 0.34, ink)
	if tier >= 4:
		draw_arc(ip, r + 2.0 / zoom, 0.0, TAU, 28, ink, 1.0 / zoom, true)
		for k in range(4):
			var ang := float(k) * (PI / 2.0)
			var d := Vector2(cos(ang), sin(ang))
			draw_line(ip + d * (r + 0.6 / zoom), ip + d * (r + 4.0 / zoom), ink, 1.2 / zoom, true)

## palette recalée DA LAVIS (parchemin) — les bleus/verts vifs "néon" détonnaient sur le
## sépia de la carte ; ces teintes restent dans la même famille de pigments que les
## frontières/bandes politiques. `_` couvre tout fin INCONNU (ex. une 6e fin future côté
## moteur) → couleur neutre, jamais un crash sur un enum non prévu.
func _fin_color(fin: int) -> Color:
	match fin:
		1: return Color(0.35, 0.48, 0.62)   # EAU : ardoise profond
		2: return Color(0.75, 0.80, 0.85)   # FROID : ardoise pâle
		3: return Color(0.45, 0.55, 0.35)   # RONCES : olive sombre
		4: return SEL_GOLD                  # ASCENSION (Merveille) : or vieilli
		5: return Color(0.60, 0.42, 0.38)   # SANG : terre cuite sombre
		_: return Color(0.55, 0.50, 0.45)   # inconnu/indéterminé : neutre (défensif)
