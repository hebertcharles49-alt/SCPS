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
	# RELIEF (lot 2)
	18: ["mountain_range_01", "mountain_range_02", "mountain_single_01", "mountain_single_02", "mountain_pass_01"],  # MONTAGNES
	19: ["mountain_single_01", "mountain_single_02", "mountain_range_01"],   # PIC
	23: ["mountain_single_01", "rocky_outcrop_01"],                          # VOLCAN
	16: ["hill_cluster_01", "hill_mark_01", "mountain_single_02", "lot6_conifer_01", "lot6_conifer_05"],  # HAUTES-TERRES
	17: ["hill_mark_01", "hill_cluster_01", "rocky_outcrop_01", "lot6_ground_05"],  # COLLINES (lot 6 : amas de rochers)
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
	16: 0.85, 17: 0.80, 18: 0.96, 19: 0.92, 23: 0.82, 20: 0.52,
	0: 0.07, 1: 0.18, 2: 0.20,                          # eau : épars (mouvement seul)
}
## PASSES SUPPLÉMENTAIRES par biome (marques EN PLUS par cellule de grille) → CANOPÉE dense. Surtout les
## forêts (le « densifié » demandé) : 1 + N marques jittées par cellule → couvert continu, pas des arbres isolés.
const DRESS_EXTRA := {
	18: 1, 19: 1, 16: 1,        # relief : un peu plus fourni (les forêts ont leur passe CANOPY)
}
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
var _bourg_tex := {}         ## id de vignette → {tex, foot, cw} (cache paresseux ; {} si l'asset manque)
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
var army_selected := false
var _pa_iso := Vector2(-1, -1)
var _pa_r := 0.0
var _dress_tex := {}      ## id de marque de terrain (lot 2) → Texture2D (cache)
var _dressing := []       ## [{pos(monde), id, scale}] — marques de biome semées (display-only)
var _dressing_dirty := true ## la géo a changé (génération/chargement) → re-semer le dressing
var _dress_clear := []    ## [[Vector2, r²]] — la CLAIRIÈRE des bourgs (aucune marque dedans)
var _canopy_batches := [] ## [{mm: MultiMesh, tex}] — la canopée servie en MULTIMESH (un draw/essence),
                          ## rebâtie avec le dressing ; instances en espace MONDE (coût par-frame nul)
var _canopy_mesh: ArrayMesh = null ## quad partagé des arbres (pied à l'origine, y vers le bas)
var nature_mode := false  ## MODE NATURE : on ne montre QUE le terrain + le dressing (pas de frontières/
                          ## villes/routes/armées/noms) — la carte « vierge », touche N. Display-only.
var _country_names := []  ## nom de chaque pays (figé au générate) — pour les étiquettes d'empire
var _borders := {}        ## 0 = TRAME FINE (provinces+régions) → PackedVector2Array jittée
# DÉGRADÉ de frontière : un RUBAN par entité, BLENDÉ (N couches du ton EXTÉRIEUR au ton INTÉRIEUR,
# décalées le long de la normale → vrai dégradé, pas deux traits posés). OUTLINE = CULTURE (héritage,
# 6 familles + variation RGB par pays) ; INLINE = ÉTHOS (axe martial↔ordre, fluide). Cités-états or↔argent.
var _b_segs := {}         ## entité → PackedVector2Array : segments de frontière (jittés)
var _b_norm := {}         ## entité → PackedVector2Array : normale vers l'INTÉRIEUR, 1 par segment
var _cap_segs := {}       ## pays → PackedVector2Array : contour de sa CAPITALE (liseré pourpre)
var _cap_norm := {}       ## pays → PackedVector2Array : normale intérieure du contour capitale
var _war_regions := {}    ## W-GUERRE UI (lot A) : région → {state:1/2, belligerent, poly} — sièges/occupations, recalculé au tick
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
# tiens (owner==player) restent TOUJOURS visibles. Rebâti à la MÊME cadence que le
# lavis politique (_rebuild_borders) : la connaissance ne peut changer que si la
# souveraineté a bougé (_owner_signature couvre déjà ce cas). ──
var _fog_tex: ImageTexture = null
var _fog_mask: PackedByteArray = PackedByteArray()   ## région → 0 voilée / 1 visible (vide = pas encore bâti → fail-open)
## VRAI si la région `r` est visible (fail-open tant que _fog_mask n'a pas encore été bâti).
func _fog_visible_region(r: int) -> bool:
	if _fog_mask.is_empty() or r < 0 or r >= _fog_mask.size():
		return true
	return _fog_mask[r] != 0
# ── SÉLECTION : contour DORÉ de la province choisie (le grain de panneau, charte EU4) ──
var _sel_prov_cache := -2
var _sel_segs := PackedVector2Array()
const SEL_GOLD := Color(0.86, 0.68, 0.26)   ## or de sélection (net, au-dessus du creux d'encre)
var _roads := []          ## [{points, level, nprov, key}] — réseau de routes (façade + méta locale)
var _road_start := {}     ## clé de route → ANNÉE de début de chantier (croissance 1 an/province)
var _roads_dirty := true  ## le réseau commercial a pu bouger → recharger les routes
var _rivers := []         ## [Vector3(x, y, ang)] — nuage de points (façade) gardé pour l'anti-bâti SUR le fil
var _river_hash := {}     ## hash spatial du fil de rivière (Vector2 par cellule) — snap des frontières
var _mv: Node2D = null    ## le MapView parent (porte la projection GLOBE monde→écran)
var _himg_l: Image = null ## couche HEIGHT (cache local) → ombrage cohérent des assets/routes
var _alb_l: Image = null  ## terrain albedo (cache) → couleur/luminosité du SOL sous l'asset

# RIVIÈRES : CARVÉES DANS LE TERRAIN (worldgen), pas un asset par-dessus. Le shader iso_blend lit
# un champ de débit (bâti par iso_ground._build_river_field depuis `river_paths`, fusionné par baie)
# et rend l'eau DANS le relief — cœur propre, berges fondues, continu jusqu'à la mer. L'overlay n'en
# garde QUE le nuage de points (`_rivers`) pour interdire de BÂTIR sur le fil.
const ROAD_ZOOM_MIN := 2.5    ## routes (zoom ISO)
# CHEMIN DE TERRE À 3 TRAITS (le motif cartographique classique — KCD/atlas) : un
# sous-trait sépia sombre (l'ombre du creux), le corps CRÈME pâle (la terre battue),
# un filet clair central. Le pointillé « carte au trésor » s'émiettait au zoom.
## GLACIS : le crème quasi-blanc « brillait » sur le lavis — rabattu vers le ton parchemin,
## alphas baissés (la route est une trace DANS la carte, pas un ruban posé dessus).
const ROAD_EDGE  := Color(0.46, 0.31, 0.17, 0.32)  ## sous-trait : sépia sombre, large
const ROAD_MAIN  := Color(0.84, 0.74, 0.55, 0.58)  ## corps : terre battue (encore rabattu)
const ROAD_LIGHT := Color(0.94, 0.87, 0.70, 0.15)  ## filet central (à peine)
const ROAD_MINOR_EDGE := Color(0.46, 0.31, 0.17, 0.20) ## desserte : plus ténue
const ROAD_MINOR_MAIN := Color(0.84, 0.74, 0.55, 0.40)
const ROAD_FOREST_A := 0.38   ## SOUS LA CANOPÉE : la route se devine — relevé depuis la canopée ×10
                              ## (à 0.22 le massif PLEIN l'avalait tout à fait)
# Traitement FRONT-END du tracé (l'A* moteur reste la vérité ; on en lisse la SORTIE, hors tick) :
#  · SNAP : raccord d'extrémité PROPRE (trim des points qui tanglent près de l'ancre de ville) ;
#  · PATHFINDING (rendu) : ré-échantillonnage à PAS CONSTANT + Chaikin GARDÉ-EAU (courbe nette qui
#    épouse la côte sans la couper) ;
#  · ASSETS : mobilier semé à l'ARC (espacement RÉGULIER, indépendant de la densité de points).
const ROAD_RESAMPLE := 2.0       ## pas d'échantillonnage du tracé (cellules) → points réguliers
const ROAD_SNAP_TRIM := 4.5      ## rayon de nettoyage des points près de l'ancre de ville (cellules)

# ── ROUTES EN TUILES (autotile cardinal, pack « SCPS Full Terrain Tiles ») ──────────────────────
# Chaque cellule-losange traversée par une route reçoit une TUILE plate 256² choisie par le masque
# CARDINAL de ses 4 voisins (n=1,e=2,s=4,w=8). Posée en SPLAT iso ÉLARGI à bord ALPHA-fondu (blend
# TRÈS PROFOND) → les tuiles cardinales-adjacentes se CHEVAUCHENT et FUSIONNENT dans le terrain ; les
# pas diagonaux sont COMBLÉS (cellule intermédiaire) pour qu'aucun lien ne soit seulement diagonal.
const ROADS_IN_SHADER := true    ## les routes sont rendues au niveau TERRAIN (iso_blend) → overlay muet
const DRAW_BRIDGES := true        ## asset pont réparé (alpha OK) → ponts réactivés
const USE_ROAD_TILES := true
const ROUTE_GRID_K := 5          ## DOIT égaler map_view.TILE_K (cellules-monde par losange)
const ROUTE_SURFACE := "road_cobble"
const ROUTE_SPLAT_EXP := 1.6     ## extension des arêtes CONNECTÉES (chevauchement du voisin → continuité)
const ROUTE_CORE_A := 0.96       ## alpha de la route (cœur + arêtes connectées)
var _road_tex := {}              ## masque cardinal 1-15 → Array[Texture2D] (variantes)
var _road_tiles := []            ## [{ctr:Vector2(iso), tex, mask}] — splats précalculés
var _road_tiles_dirty := true    ## le réseau a bougé → recalculer la pose
var _route_meshes := {}          ## masque → ArrayMesh : splat UNIDIRECTIONNEL (plein LE LONG de la route,
                                 ## fondu EN TRAVERS — comme une rivière), bâti/caché par masque

# ── PONTS (kit modulaire RGBA) : là où une route franchit un FLEUVE (le moteur l'y route déjà), on
# pose start→span×N→end en overlay AU-DESSUS de l'eau. Orientation EW (horizontal écran) / NS (vertical)
# selon la direction du franchissement. Sprite 384² centré sur le centre de la tuile-route (losange). ──
var _bridge_tex := {}            ## "ew_start".."ns_end" → Texture2D
var _bridges := []               ## [{tex, tl:Vector2(iso coin haut-gauche), sz:float}]
var _bridges_dirty := true

# ── ROUTE EN COBBLES TRANSPARENTS : la tuile cobble (RGBA épars) est traitée comme une VRAIE TUILE, pas
# un sprite — échantillonnée au niveau TERRAIN (iso_blend) sur le plan du sol (UV losange), donc à l'angle
# iso correct, exactement comme `cliff_atlas`. iso_ground charge la tuile et la passe au shader ; l'overlay
# n'a plus rien à poser pour la route (seuls les PONTS restent en overlay, eux sont au-dessus de l'eau). ──

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

func _on_generated() -> void:
	_set_rivers()
	_himg_l = null              # monde neuf → recharger les caches de lumière (relief + albedo)
	_alb_l = null
	_road_tiles_dirty = true    # monde neuf → reposer les tuiles de route
	_bridges_dirty = true       # … et les ponts
	_borders_dirty = true       # monde neuf → frontières ET routes à refaire
	_dressing_dirty = true      # … et le dressing de terrain (biome semé)
	_roads_dirty = true
	_road_start.clear()         # chantiers remis à zéro (le monde neuf rebâtit ses routes)
	_region_label.clear()       # bannières de lieux : noms recachés (monde neuf)
	_town_cache.clear()         # urbaniste : plans de bourgs recalculés (routes neuves)
	_sea_img = null             # couches eau recachées (quais)
	_rf_img = null
	_bio_img = null             # couche biome recachée (routes sous canopée)
	_river_hash.clear()         # snap de frontières : fil de rivière re-haché (monde neuf)
	_owner_sig = -1
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
func _draw_resources(w, mv: Node2D, is_iso: bool) -> void:
	if _raws_dirty:
		_build_region_raws()         # rebâti à la demande (mode RESSOURCES seulement)
		_raws_dirty = false
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var zoom := maxf(0.01, vt.get_scale().x)
	var sz := 18.0 / zoom if is_iso else 13.0           # MONDE (iso : constant écran) · ÉCRAN (globe)
	for r in range(w.region_count()):
		var raws: Array = _region_raws.get(r, [])
		if raws.is_empty():
			continue
		var ctr: Vector2 = w.region_centroid(r)
		if ctr.x < 0:
			continue
		var sp: Vector2
		if is_iso:
			sp = vt * mv.iso_pos(ctr.x, ctr.y)
		else:
			var pr: Dictionary = mv.globe_to_screen(ctr.x, ctr.y)
			if not pr["vis"]:
				continue
			sp = pr["pos"]
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

func _on_tick(_year: int) -> void:
	_raws_dirty = true         # l'extraction a pu s'établir (an-0 nu) → recache les brutes au prochain dessin RESSOURCES
	_update_top_cap()          # le titre de « plus grande capitale » peut changer → la vignette t7 suit
	_road_tiles_dirty = true   # le réseau a pu croître/changer → reposer les tuiles de route
	_bridges_dirty = true      # … et les ponts (un franchissement neuf)
	var sig := _owner_signature(Sim.world)
	if sig != _owner_sig:      # la souveraineté a changé (conquête/colonisation) →
		_owner_sig = sig       # refaire frontières ET réseau de routes (villes neuves/captées)
		_borders_dirty = true
		_roads_dirty = true
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
		_war_regions[r] = {"state": st, "belligerent": int(ws.get("belligerent", -1)), "polys": polys}

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

## reconstruit les segments de frontière (région + pays) depuis la façade (port bseg).
func _rebuild_borders() -> void:
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
	for entity in ent_flat:
		var r := _smooth_border(ent_flat[entity], ent_nrm[entity])
		_b_segs[entity] = r[0]; _b_norm[entity] = r[1]
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
	# BROUILLARD DE GUERRE (etape 1/2) : meme cadence que le lavis politique - la
	# connaissance ne peut changer que si la souverainete a bouge (fog_update est
	# annuel, pure fonction d'econ->adj + ownership, tous deux couverts par
	# _owner_signature ci-dessus).
	if w.has_method("fog_image"):
		var fimg: Image = w.fog_image()
		if fimg != null:
			if _fog_tex == null or _fog_tex.get_size() != Vector2(fimg.get_size()):
				_fog_tex = ImageTexture.create_from_image(fimg)
			else:
				_fog_tex.update(fimg)
	if w.has_method("fog_region_mask"):
		_fog_mask = w.fog_region_mask()
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

func _draw_band(mv: Node2D, segs: PackedVector2Array, nrms: PackedVector2Array, entity: int, zoom: float) -> void:
	if segs.size() < 2:
		return
	var is_cs: bool = entity >= 0 and Sim.world != null and int(Sim.world.country_role(entity)) == 2
	var out_col: Color = CS_GOLD if is_cs else _ethos_ink(entity)
	var in_col: Color = Color(0.80, 0.68, 0.40) if is_cs else _heritage_wash(entity)
	# l'INLINE d'abord (sous l'outline) : le lavis d'héritage, décalé vers l'INTÉRIEUR
	var have_n := nrms.size() * 2 >= segs.size()
	if have_n:
		var lw := _w(zoom, 0.55, 1.8, 3.4)
		for k in range(3):
			# ⚠ _b_norm porte la normale EXTÉRIEURE (héritée de la façade) → l'intérieur est à -n
			var off := -(0.45 + 0.62 * float(k))
			var a: float = [0.34, 0.20, 0.10][k]
			var proj := PackedVector2Array()
			proj.resize(segs.size())
			for i in range(0, segs.size() - 1, 2):
				var n: Vector2 = nrms[i >> 1] * off
				proj[i] = mv.iso_pos(segs[i].x + n.x, segs[i].y + n.y)
				proj[i + 1] = mv.iso_pos(segs[i + 1].x + n.x, segs[i + 1].y + n.y)
			draw_multiline(proj, Color(in_col.r, in_col.g, in_col.b, a), lw, true)
	# le CREUX gravé (discret) + l'OUTLINE d'éthos NET, sur la ligne
	var proj0 := _project_segs_iso(mv, segs)
	if proj0.size() < 2:
		return
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
func _draw_war_hatch(mv: Node2D, zoom: float, info: Dictionary) -> void:
	var polys: Array = info.get("polys", [])
	if polys.is_empty():
		return
	var belli: int = int(info.get("belligerent", -1))
	var st: int = int(info.get("state", 1))
	var col := _country_color(belli) if belli >= 0 else Color(0.5, 0.1, 0.1)
	var a: float = HATCH_OCC_A if st == 2 else HATCH_SIEGE_A
	var w_line := _w(zoom, 0.5, 0.8, 1.6)
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
					draw_line(mv.iso_pos(pp[0].x, pp[0].y), mv.iso_pos(pp[1].x, pp[1].y),
						Color(col.r, col.g, col.b, a), w_line)
			k += HATCH_STEP


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

## hash scalaire → [0,1) (déterministe, display-only) — varie dressing/orientations.
func _h1(x: float) -> float:
	var v := sin(x * 12.9898) * 43758.5453
	return v - floor(v)

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
	# les PONTS : là où le tracé LISSE traverse l'eau de rivière carvée — milieu + tangente.
	_ink_bridges.clear()
	var rfb: Image = _carved_river_field()
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
	var bundle := {}   # hash spatial (cellule 1.0) des points DÉJÀ tracés — magnétisme de couloir
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
		# 3) ANTI-DÉDOUBLEMENT — le MAGNÉTISME DE COULOIR : un point qui passe à ≤ 0.65 cellule
		#    d'une route DÉJÀ tracée se COLLE dessus → les A* voisins PARTAGENT la chaussée au
		#    lieu de dessiner deux lignes parallèles « far-west ». Les 3 points d'about restent
		#    libres (le raccord au bourg prime). Hash spatial, une passe par route.
		for k in range(3, pts.size() - 3):
			var p5: Vector2 = pts[k]
			var gx := int(floor(p5.x))
			var gy := int(floor(p5.y))
			var bestd := 0.42          # 0.65² : rayon de collage au couloir
			var bestp := p5
			for oy in range(-1, 2):
				for ox in range(-1, 2):
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
		rd["ra"] = ra            # mémorisé : le bâti du bourg s'organise le long des routes de SA ville
		rd["rb"] = rb
		rd["points"] = pts

## range un tronçon de route dans son bucket (artère/desserte × plein/sous-canopée).
func _road_bucket(run: PackedVector2Array, mv, is_main: bool, in_forest: bool,
		pm: Array, pn: Array, pmf: Array, pnf: Array) -> void:
	var ip := _road_iso(run, mv)
	if is_main:
		(pmf if in_forest else pm).append(ip)
	else:
		(pnf if in_forest else pn).append(ip)

## projette une polyligne MONDE en iso (helper du dessin de routes).
func _road_iso(poly: PackedVector2Array, mv) -> PackedVector2Array:
	var out := PackedVector2Array()
	out.resize(poly.size())
	for k in range(poly.size()):
		out[k] = mv.iso_pos(poly[k].x, poly[k].y)
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
func _near_river(rf: Image, x: int, y: int) -> bool:
	if rf == null:
		return false
	var rw := rf.get_width()
	var rh := rf.get_height()
	for dy in range(-1, 2):
		for dx in range(-1, 2):
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
	if _cataclysm:
		queue_redraw()
	# FRONTIÈRES/ASSETS DÉCOUPLÉS DU TICK : jadis seul `_on_tick` (qui ne fire PAS en
	# pause) recalculait la souveraineté → frontières/routes/villes ne se rafraîchissaient
	# qu'au DÉBLOCAGE de la pause. On sonde ~4×/s (même en pause) : la carte reflète l'état
	# COURANT, elle n'attend plus qu'on relève la pause.
	_sig_poll += dt
	if _sig_poll >= 0.25:
		_sig_poll = 0.0
		if Sim.world != null:
			var sig := _owner_signature(Sim.world)
			if sig != _owner_sig:
				_owner_sig = sig
				_borders_dirty = true
				_roads_dirty = true
				queue_redraw()

## Anneau doré autour du pion du joueur SÉLECTIONNÉ (mode marche : cliquez une destination).
func _draw_army_ring(ctr: Vector2, s: float, zoom: float) -> void:
	var r := s * 0.62
	draw_arc(ctr, r + 3.0 / zoom, 0.0, TAU, 40, Color(0.10, 0.08, 0.03, 0.7), 4.0 / zoom, true)
	draw_arc(ctr, r, 0.0, TAU, 40, Color(1.0, 0.86, 0.36, 0.95), 2.4 / zoom, true)

## Le clic (en espace LOCAL de l'overlay = iso) touche-t-il le pion du joueur ?
func point_hits_player_army(local: Vector2) -> bool:
	return _pa_iso.x >= 0.0 and local.distance_to(_pa_iso) <= maxf(_pa_r, 6.0)

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
		_pa_iso = ctr ; _pa_r = s * 0.7        # cible cliquable (sélection d'armée)
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
		_draw_resources(w, mv, true)

## CARTE PARCHEMIN — acteurs tracés en ENCRE vectorielle (zéro sprite) : frontières, routes,
## villes (glyphes), noms d'empire, armées, épicentre §27. La Camera2D met à l'échelle ; les
## tailles d'encre sont en px ÉCRAN (÷ zoom) → lisibles à tous les zooms.
func _draw_iso(w, mv: Node2D) -> void:
	var zoom := get_viewport_transform().get_scale().x
	var vt := get_viewport_transform()
	var vp := get_viewport_rect().size
	var INK := Color(0.20, 0.14, 0.09, 0.95)         # encre brun-sépia (le trait de plume)
	var human_idx := int(w.player())   # BROUILLARD : les tiens (owner==human_idx) restent TOUJOURS visibles

	# ── LAVIS POLITIQUE (aquarelle) : le territoire teinté SOUS tout — la carte DIT qui tient
	#    quoi d'un regard au plan large ; le lavis s'efface vers le zoom profond (le terrain parle). ──
	if not nature_mode and _pol_tex != null:
		if _borders_dirty:
			_rebuild_borders()                        # le lavis se rebâtit avec les frontières
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
		var dress_col := Color(1, 1, 1, DRESS_ALPHA)
		var egg_col := Color(1, 1, 1, EGG_ALPHA)
		for d in _dressing:
			var wp: Vector2 = d["pos"]
			var dip: Vector2 = mv.iso_pos(wp.x, wp.y)
			var dss: Vector2 = vt * dip
			if dss.x < -90 or dss.y < -90 or dss.x > vp.x + 90 or dss.y > vp.y + 90:
				continue
			var did: String = d["id"]
			var dtex := _dress_get(did)
			if dtex == null:
				continue
			var is_egg: bool = d.get("egg", false)
			var dh := _dress_size(did) * float(d["scale"]) / zoom        # hauteur MONDE (taille écran constante)
			var dw := dh
			if did.begins_with("sea_serpent"):
				dw = dh * 2.0                                            # serpent : sprite 2:1 (large)
			draw_texture_rect(dtex, Rect2(dip - Vector2(dw * 0.5, dh * 0.5), Vector2(dw, dh)), false,
				egg_col if is_egg else d.get("tint", dress_col))
	# MODE NATURE : juste le terrain + le dressing — on saute frontières, routes, villes, armées, noms, §27.
	if nature_mode:
		return

	# ── FRONTIÈRES à l'ENCRE (calligraphie) : TRAME FINE 1px (toutes provinces+régions) + BLOCS
	#    d'empire 3px, COULEUR PAR ENTITÉ, en 2 passes (bave d'encre douce + plume nette, jittées). ──
	if _borders_dirty:
		_rebuild_borders()
	# la TRAME FINE fond en survol (sinon mosaïque illisible) et se révèle au plan rapproché — toutes
	# les provinces RESTENT tracées (1px), juste graduées au zoom (LOD ; les blocs d'empire, eux, toujours).
	if _borders.has(0):
		# PROVINCE = administrative, un CHUCHOTEMENT : déjà restreinte à la terre ADMINISTRÉE
		# (rebuild), elle n'émerge qu'au plan rapproché (zoom 2.2+) et plafonne bas (0.24) —
		# le lavis + la frontière d'empire portent la lecture, la trame ne fait que détailler.
		var fine_a := clampf((zoom - 2.2) / 2.6, 0.0, 1.0) * 0.24
		if fine_a > 0.02:
			var fseg := _project_segs_iso(mv, _borders[0])
			if fseg.size() >= 2:
				draw_multiline(fseg, Color(PROV_INK.r, PROV_INK.g, PROV_INK.b, fine_a * 0.45), _w(zoom, 0.6, 0.9, 1.5), true)
				draw_multiline(fseg, Color(PROV_INK.r, PROV_INK.g, PROV_INK.b, fine_a), _w(zoom, 0.34, 0.6, 0.9), true)
	# PAYS : trait GRAVÉ en double passe (halo brun sombre LARGE + pigment politique FIN), pour bien
	# SÉPARER l'administratif (province, cheveu brun) du politique (pays, trait coloré net). Puis le
	# LISERÉ POURPRE FIN de chaque capitale, AU-DESSUS.
	for entity in _b_segs:
		_draw_band(mv, _b_segs[entity], _b_norm.get(entity, PackedVector2Array()), int(entity), zoom)
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
		if _sel_segs.size() >= 2:
			var sseg := _project_segs_iso(mv, _sel_segs)
			draw_multiline(sseg, Color(0.12, 0.08, 0.04, 0.80), _w(zoom, 1.3, 2.6, 4.4), true)
			draw_multiline(sseg, Color(SEL_GOLD.r, SEL_GOLD.g, SEL_GOLD.b, 0.95), _w(zoom, 0.7, 1.5, 2.6), true)
	elif _sel_prov_cache != -2:
		_sel_prov_cache = -2
		_sel_segs = PackedVector2Array()

	# ── ROUTES : CHEMIN DE TERRE À 3 TRAITS (sous-trait sépia + corps crème + filet clair) —
	#    le motif cartographique classique, sur les polylignes DÉJÀ lissées (_augment_roads).
	#    Croissance organique (1 an/province) ; segments cumulés → batchs par hiérarchie. ──
	if zoom >= ROAD_ZOOM_MIN:
		_ensure_roads()
		if not _roads.is_empty():
			var polys_main := []
			var polys_minor := []
			var polys_main_f := []    # tronçons SOUS LA CANOPÉE (forêt) : la route s'efface
			var polys_minor_f := []
			var seen := {}   # dédup : un TRONÇON partagé (couloir commun) ne s'encre qu'UNE fois
			for ri in range(_roads.size()):
				var rd: Dictionary = _roads[ri]
				var pts: PackedVector2Array = rd["points"]
				if pts.size() < 2:
					continue
				# croissance à GRAIN JOUR (l'année entière sautait 0→1 au nouvel an = « instantané ») :
				# une route POUSSE sur ~1 an par province traversée, visible au fil des ticks.
				var st: int = _road_start.get(rd["key"], Sim.day_count)
				var nprov: int = maxi(1, int(rd.get("nprov", 1)))
				var frac := clampf(float(Sim.day_count - st) / (float(nprov) * 365.0), 0.0, 1.0)
				var poly := _road_partial(pts, frac)
				if poly.size() < 2:
					continue
				var is_main: bool = int(rd.get("level", 1)) <= 0
				# découpe en SOUS-POLYLIGNES : segments inédits (dédup) ET homogènes (forêt ou
				# non) — sous la canopée le tronçon bascule dans le bucket « effacé ».
				var run := PackedVector2Array()
				var run_forest := false
				for k in range(poly.size() - 1):
					var a7: Vector2 = poly[k]
					var b7: Vector2 = poly[k + 1]
					var ka := int(a7.x * 4.0) * 8388608 + int(a7.y * 4.0)
					var kb := int(b7.x * 4.0) * 8388608 + int(b7.y * 4.0)
					var kseg := str(mini(ka, kb)) + "_" + str(maxi(ka, kb))
					var mid := (a7 + b7) * 0.5
					var inf := _forest_at(int(mid.x), int(mid.y))
					if seen.has(kseg) or (run.size() >= 2 and inf != run_forest):
						if run.size() >= 2:
							_road_bucket(run, mv, is_main, run_forest, polys_main, polys_minor, polys_main_f, polys_minor_f)
						run = PackedVector2Array()
						if seen.has(kseg):
							continue
					seen[kseg] = true
					if run.is_empty():
						run.append(a7)
						run_forest = inf
					run.append(b7)
				if run.size() >= 2:
					_road_bucket(run, mv, is_main, run_forest, polys_main, polys_minor, polys_main_f, polys_minor_f)
			# PAR POLYLIGNE (joints RONDS aux coudes) ; l'ordre des passes fait le modelé :
			# ombre sépia → terre crème → filet de lumière. SOUS LA CANOPÉE : un seul trait
			# ténu (α×ROAD_FOREST_A) — on DEVINE le chemin entre les masses, il ne coupe plus
			# la forêt en deux.
			for pl2 in polys_minor_f:
				draw_polyline(pl2, Color(ROAD_MINOR_MAIN.r, ROAD_MINOR_MAIN.g, ROAD_MINOR_MAIN.b,
					ROAD_MINOR_MAIN.a * ROAD_FOREST_A), _w(zoom, 0.36, 0.8, 1.5), true)
			for pl2 in polys_main_f:
				draw_polyline(pl2, Color(ROAD_MAIN.r, ROAD_MAIN.g, ROAD_MAIN.b,
					ROAD_MAIN.a * ROAD_FOREST_A), _w(zoom, 0.62, 1.3, 2.4), true)
			for pl2 in polys_minor:
				draw_polyline(pl2, ROAD_MINOR_EDGE, _w(zoom, 0.65, 1.4, 2.6), true)
			for pl2 in polys_minor:
				draw_polyline(pl2, ROAD_MINOR_MAIN, _w(zoom, 0.36, 0.8, 1.5), true)
			for pl2 in polys_main:
				draw_polyline(pl2, ROAD_EDGE, _w(zoom, 1.1, 2.2, 4.0), true)
			for pl2 in polys_main:
				draw_polyline(pl2, ROAD_MAIN, _w(zoom, 0.62, 1.3, 2.4), true)
			for pl2 in polys_main:
				draw_polyline(pl2, ROAD_LIGHT, _w(zoom, 0.26, 0.55, 1.0), true)
			# les PONTS D'ENCRE : deux garde-corps bombés en travers du franchissement de rivière
			for br in _ink_bridges:
				var bp: Vector2 = mv.iso_pos((br["w"] as Vector2).x, (br["w"] as Vector2).y)
				var bt: Vector2 = (mv.iso_pos((br["w"] as Vector2).x + (br["t"] as Vector2).x,
					(br["w"] as Vector2).y + (br["t"] as Vector2).y) - bp).normalized()
				var bperp := Vector2(-bt.y, bt.x)
				var bl := _w(zoom, 0.62, 2.4, 5.2)
				var bw := _w(zoom, 0.26, 1.0, 2.2)
				var biw := _w(zoom, 0.07, 0.5, 1.0)
				for sgn in [-1.0, 1.0]:
					var o3 := bperp * bw * float(sgn)
					draw_polyline(PackedVector2Array([bp - bt * bl + o3,
						bp + o3 + bperp * (bw * 0.5 * float(sgn)),   # le bombé du tablier
						bp + bt * bl + o3]), TOWN_INK, biw, true)

	# ── VILLES : VIGNETTES gravées (pack bourgs/, lot U) — cité t1-t7, cité-état & hameau libre
	# (familles DÉDIÉES). CENTRÉES sur le SIÈGE intérieur de province (≠ jonction ; le centroïde
	# brut tombe pile à l'intersection des provinces). Cité-état (rôle 2) & hameau libre (rôle 4)
	# toujours tracés même sans tier de ville. Les vignettes sont GRANDES → tri fond→avant
	# (peintre, y écran) puis les BANNIÈRES par-dessus tout (jamais sous la vignette voisine).
	if zoom >= CITY_ZOOM_MIN:
		var setts := []
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
			var ss: Vector2 = vt * ip
			if ss.x < -160 or ss.y < -160 or ss.x > vp.x + 160 or ss.y > vp.y + 160:
				continue
			setts.append({"r": r, "role": role, "ctr": ctr, "ip": ip})
		setts.sort_custom(func(a, b): return (a["ip"] as Vector2).y < (b["ip"] as Vector2).y)
		for s in setts:
			_draw_settlement(w, int(s["r"]), int(s["role"]), s["ctr"], s["ip"], zoom, mv)
		# RÉGIME KCD : la BANNIÈRE de lieu éclot au plan rapproché — le relais des
		# noms de pays (régime EU4) qui se sont effacés au même seuil de zoom.
		if zoom >= 4.0:
			for s in setts:
				_draw_banner(w, int(s["r"]), s["ip"], zoom, clampf((zoom - 4.0) / 1.2, 0.0, 1.0))

	# ── ARMÉES : PION DE PLATEAU (planche 32 — la figurine d'étain posée SUR la
	#    table, drapeau teinté au pays, la POSE dit la phase) + ligne de marche.
	#    Ombre de contact = la même pièce en silhouette, décalée SE (front32). ──
	_pa_iso = Vector2(-1, -1)   # cible cliquable du joueur : recalculée ce frame (garnison OU ost)
	for c in range(w.country_count()):
		var a: Dictionary = w.army_info(c)
		if not bool(a.get("active", false)):
			# RÉSERVE MOBILISÉE (régiments recrutés mais pas en campagne) : une garnison à la
			# CAPITALE — sinon une armée levée n'apparaît NULLE PART (retour joueur « les
			# armées mobilisées n'apparaissent pas sur la carte »). Pion plus discret que l'ost.
			_draw_garrison(w, mv, c, zoom, human_idx)
			continue
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
		if c == human_idx:
			_pa_iso = ctr ; _pa_r = _w(zoom, 6.0, 30.0, 64.0) * 0.7   # cible cliquable (ost)
			if army_selected:
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

	# ── NOMS D'EMPIRE : SUIVENT LA FORME du pays (axe principal par ACP des centroïdes projetés →
	#    Chili vertical, Russie en travers de la Sibérie), à l'encre, taille ÉCRAN constante. ──
	for c in range(w.country_count()):
		if c >= _country_names.size():
			break
		var nm: String = _country_names[c]
		if nm == "":
			continue
		# centroïdes PROJETÉS (espace écran : iso_pos comprime Y → angle visuel correct)
		var ps := PackedVector2Array()
		for r in range(w.region_count()):
			if w.region_owner(r) == c:
				# BROUILLARD DE GUERRE (étape 1/2) : une région ENNEMIE voilée n'entre pas dans
				# l'ancre — un pays SANS aucune région visible ne pose plus aucun nom du tout
				# (ps reste vide → le "continue" existant juste dessous s'en charge).
				if c != human_idx and not _fog_visible_region(r):
					continue
				var rc: Vector2 = w.region_centroid(r)
				if rc.x >= 0:
					ps.push_back(mv.iso_pos(rc.x, rc.y))
		if ps.is_empty():
			continue      # (1 centroïde = valide : ancre au point, pas d'orientation — l'empire
			              #  mono-région du DÉPART charte garde son nom ; l'ACP exige ≥2 sinon)
		# moyenne + matrice de covariance → axe principal (ACP 2D)
		var mx := 0.0; var my := 0.0
		for p in ps:
			mx += p.x; my += p.y
		mx /= ps.size(); my /= ps.size()
		var sxx := 0.0; var syy := 0.0; var sxy := 0.0
		for p in ps:
			var dx := p.x - mx; var dy := p.y - my
			sxx += dx * dx; syy += dy * dy; sxy += dx * dy
		var ang := 0.0
		# élongation (rapport des valeurs propres) : on n'oriente QUE les pays nettement allongés
		var tr := sxx + syy
		var det := sxx * syy - sxy * sxy
		var disc := sqrt(maxf(tr * tr * 0.25 - det, 0.0))
		var l1 := tr * 0.5 + disc
		var l2 := tr * 0.5 - disc
		if l2 > 0.001 and l1 / l2 > 1.8:
			ang = 0.5 * atan2(2.0 * sxy, sxx - syy)   # ∈ [-π/2, π/2] : jamais à l'envers
		# ancre = le BARYCENTRE des centroïdes (espace OUVERT, hors des hubs routiers) → lisible.
		var ip := Vector2(mx, my)
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
			var span := clampf(2.8 * sqrt(maxf(l1, 0.0)) + 16.0, 22.0, 220.0)   # étendue-monde du nom
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
	#    Encre estompée (esprit parchemin, jamais noir pur) — cf. fog_image() (scps_sim_node). ──
	if not nature_mode and _fog_tex != null:
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
const TOWN_SHADOW := Color(0.20, 0.15, 0.10, 0.10)   ## ombre portée (souffle)
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
		entry = {"tex": tex, "foot": foot, "cw": cwf}
	_bourg_tex[id] = entry
	return entry

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
			x += DRESS_SPACING
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
	var entry := {"pos": Vector2(px, py), "id": id, "scale": scl}
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
	draw_texture_rect(tex, Rect2(rect.position + Vector2(fw, fw) * 0.040, rect.size), false, TOWN_SHADOW)
	var vj := 0.93 + 0.10 * _h1(float(r) * 5.7)           # GLAZE : valeur jittée par région (vie)
	draw_texture_rect(tex, rect, false, Color(vj, vj, vj * 0.99, BOURG_ALPHA))

## BANNIÈRE DE LIEU (référence KCD) : chip parchemin + liseré d'encre + NOM du siège +
## pastille au pigment du propriétaire — taille ÉCRAN constante, posée AU-DESSUS du tampon.
## Éclot au plan rapproché (a = fondu d'éclosion), le relais des noms de pays effacés.
func _draw_banner(w, r: int, ip: Vector2, zoom: float, a: float) -> void:
	if a <= 0.02:
		return
	if not _region_label.has(r):
		var nmv := ""
		var anc: Vector2 = _region_seat.get(r, Vector2(-1, -1))
		if anc.x >= 0 and w.has_method("province_at"):
			var pid: int = w.province_at(int(anc.x), int(anc.y))
			if pid >= 0:
				nmv = String(w.province_info(pid).get("nom", ""))
		_region_label[r] = nmv
	var nm: String = _region_label[r]
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
